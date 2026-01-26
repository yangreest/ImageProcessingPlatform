#include "MainForm.h"
#include <algorithm>
#include <sstream>
#include <qdatetime.h>
#include <QTimer>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QInputDialog>
#include <QPainter>
#include <QImageWriter>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QFileDialog>
#include <QSettings>
#include <iostream>

#include "AlertForm.h"
#include "CloseEventFilter.h"
#include "SingleInputForm.h"
#include "Tools/HttpClient.h"
#include "Tools/Tools.h"

#define MY_WARNING(t) 	{AlertForm f("警告", t);f.showModal();}//QMessageBox::warning(this, "警告", t)
#define MY_INFO(t) {AlertForm f("提示", t);f.showModal();}//QMessageBox::information(this, "提示", t)

#define HeartbeatFaultTolerance 5


MainForm::MainForm(const std::string& guid, int model, QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);
	m_strWorkGuid = guid;
	m_nWorkMode = model;
	InitParam();
	InitUI(model);
	BindAction();
}

MainForm::~MainForm()
{
	WHSD_Tools::SafeRelease(m_pKeyEventFilter);
	WHSD_Tools::SafeRelease(m_pConfig);
	for (auto sine_table : m_apWHSDControlBoardProtocol)
	{
		WHSD_Tools::SafeRelease(sine_table);
	}
	WHSD_Tools::SafeRelease(m_pMouseEventFilter);
	WHSD_Tools::SafeRelease(m_pTimer);
	//SafeReleaseWithEndWork(m_pDeviceCom);
	WHSD_Tools::SafeReleaseWithEndWork(m_pDeviceLog);
}

void MainForm::ConnectDevice()
{
	if (m_strWorkGuid.empty() && m_pConfig->m_memTimsConfig.m_nForceGuid > 0)
	{
		SingleInputForm f("请输入检测任务ID");
		f.showModal();
		int number = SingleInputForm::result;
		if (number == 0)
		{
			MY_WARNING("输入错误!");
			return;
		}
		m_strWorkGuid = std::to_string(number);
		ReadSavedFiles(0);
	}
	WHSD_Tools::SafeRelease(m_pSampleBoardBase);
	m_pSampleBoardBase = ISampleBoardBase::GetSampleBoard(m_pConfig->m_memCSampleBoardConfig.m_nManufacturer);
	if (m_pSampleBoardBase != nullptr && m_pSampleBoardBase->Init(m_pConfig->m_memCSampleBoardConfig))
	{
		m_pSampleBoardBase->RegisterImgCallback(std::bind(&MainForm::Callback_SampleBoardNewImg, this,
		                                                  std::placeholders::_1, std::placeholders::_2,
		                                                  std::placeholders::_3, std::placeholders::_4));
		m_pSampleBoardBase->RegisterDeviceConnectCallback(
			std::bind(&MainForm::Callback_SampleBoardDeviceConnectStatus, this, std::placeholders::_1));
		m_pSampleBoardBase->RegisterDeviceRunCallback(
			std::bind(&MainForm::Callback_SampleBoardDeviceRunStatus, this, std::placeholders::_1));
		m_pSampleBoardBase->RegisterDeviceValueCallback(
			std::bind(&MainForm::Callback_GetSampleBoardValue, this, std::placeholders::_1, std::placeholders::_2));
		if (m_pSampleBoardBase->BeginWork())
		{
			ui.pushButton_2->setEnabled(false);
			ui.pushButton_2->setText("已启动");
		}
		else
		{
			QMessageBox::critical(this, "错误", "启动失败！", QMessageBox::Ok);
		}
	}
	else
	{
		QMessageBox::critical(this, "错误", "初始化失败！", QMessageBox::Ok);
	}
}

void MainForm::On_timer_timeout()
{
	//刷新线程间数据
	//m_pDeviceLog->Write("666777");
	if (m_nShowImgTips >= 0 && m_nShowImgTips < 10)
	{
		QPixmap pixmap;
		if (pixmap.load("1.jpg"))
		{
			ui.label->setPixmap(pixmap.scaled(ui.label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
		m_nShowImgTips++;
	}
	if (m_nShowImgTips == 10)
	{
		Callback_ShowImgOnLabel_2();
		m_nShowImgTips = -1;
	}
	m_mutexDeviceInfoLock.lock();
	auto memHeartBeat = m_amemDeviceHeartBeat[0];
	auto enumDeviceConnectStatus = m_enumDeviceConnectStatus;
	auto nTemperature = m_nTemperature;
	auto nBattery = m_nBattery;
	auto nWifi = m_nWifi;
	m_mutexDeviceInfoLock.unlock();

	{
		std::string info;
		switch (enumDeviceConnectStatus)
		{
		case DeviceConnectStatus::UnKnown:
			{
				info = "未知/未连接";
				break;
			}
		case DeviceConnectStatus::Closed:
			{
				info = "连接断开";
				break;
			}
		case DeviceConnectStatus::OpendListen:
			{
				info = "连接中";
				break;
			}
		case DeviceConnectStatus::Connected:
			{
				info = "已连接";
				break;
			}
		}

		info.append("_");
		switch (m_enumDeviceRunStatus)
		{
		case DeviceRunStatus::Unknow:
			{
				info.append("未知");
				break;
			}
		case DeviceRunStatus::Awake:
			{
				info.append("已唤醒");
				break;
			}
		case DeviceRunStatus::Busy:
			{
				info.append("忙");
				break;
			}
		case DeviceRunStatus::Ready:
			{
				info.append("就绪");
				break;
			}
		default:
			{
				info.append("未知");
				break;
			}
		}
		ui.label_4->setText(QString::fromStdString(info));
	}
	ui.label_10->setText(QString::number(nTemperature));
	ui.label_11->setText(QString::number(nBattery));
	ui.label_12->setText(QString::number(nWifi));
	UpdateMotorRunStatus(memHeartBeat);
	UpdateOTAStatus();
	{
		//处理主控制板的心跳逻辑
		if (m_abControlBroadConnected[0])
		{
			auto timeDiff = m_atime_LastHeartBeatTime[0].GetTimeSpan_ms(false);
			if (timeDiff > m_pConfig->m_memControlBoardConfig.m_wDeviceHeartBeat * HeartbeatFaultTolerance)
			{
				ui.label_9->setText("已连接_心跳异常");
			}
			else
			{
				ui.label_9->setText("已连接");
			}
		}
		else
		{
			ui.label_9->setText("未连接");
		}
	}
	{
		std::string strXRayDeviceStatus("未知");
		switch (memHeartBeat.m_cXRayDeviceStatus)
		{
		case 0:
			{
				strXRayDeviceStatus = "空闲";
				break;
			}
		case 1:
			{
				strXRayDeviceStatus = "延时开启中";
				break;
			}
		case 2:
			{
				strXRayDeviceStatus = "工作完成";
				break;
			}
		default:
			{
				break;
			}
		}
		if (m_pConfig->m_memControlBoardConfig.m_bEnableIp2)
		{
			//处理第二控制板的心跳逻辑
			if (m_abControlBroadConnected[1])
			{
				auto timeDiff = m_atime_LastHeartBeatTime[1].GetTimeSpan_ms(false);
				if (timeDiff > m_pConfig->m_memControlBoardConfig.m_wDeviceHeartBeat * HeartbeatFaultTolerance)
				{
					strXRayDeviceStatus = "已连接_心跳异常";
				}
			}
			else
			{
				strXRayDeviceStatus = "未连接";
			}
		}
		ui.label_24->setText(QString::fromStdString(strXRayDeviceStatus));
	}

	ui.label_33->setText(QString::number(m_nPicCount));
	ui.label_46->setText(QString::number(m_anHeartBeatCount[0]));
	ui.label_40->setText(QString::number(m_anHeartBeatCount[1]));

	{
		switch (m_memImageProcessParam1.m_nDenoise)
		{
		case 0:
			{
				ui.pushButton_33->setStyleSheet(QString::fromStdString(ChangeUIImg("putongjiangzao", false)));
				ui.pushButton_38->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojijiangzao", false)));
				ui.pushButton_31->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojijiangzao", false)));
				break;
			}
		case 3:
			{
				ui.pushButton_33->setStyleSheet(QString::fromStdString(ChangeUIImg("putongjiangzao", true)));
				ui.pushButton_38->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojijiangzao", false)));
				ui.pushButton_31->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojijiangzao", false)));
				break;
			}
		case 7:
			{
				ui.pushButton_33->setStyleSheet(QString::fromStdString(ChangeUIImg("putongjiangzao", false)));
				ui.pushButton_38->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojijiangzao", true)));
				ui.pushButton_31->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojijiangzao", false)));
				break;
			}
		case 10:
			{
				ui.pushButton_33->setStyleSheet(QString::fromStdString(ChangeUIImg("putongjiangzao", false)));
				ui.pushButton_38->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojijiangzao", false)));
				ui.pushButton_31->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojijiangzao", true)));
				break;
			}
		}

		switch (m_memImageProcessParam1.m_nEnhance)
		{
		case 0:
			{
				ui.pushButton_42->setStyleSheet(QString::fromStdString(ChangeUIImg("putongzengqiang", false)));
				ui.pushButton_46->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojizengqiang", false)));
				ui.pushButton_45->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojizengqiang", false)));
				break;
			}
		case 2:
			{
				ui.pushButton_42->setStyleSheet(QString::fromStdString(ChangeUIImg("putongzengqiang", true)));
				ui.pushButton_46->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojizengqiang", false)));
				ui.pushButton_45->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojizengqiang", false)));
				break;
			}
		case 4:
			{
				ui.pushButton_42->setStyleSheet(QString::fromStdString(ChangeUIImg("putongzengqiang", false)));
				ui.pushButton_46->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojizengqiang", true)));
				ui.pushButton_45->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojizengqiang", false)));
				break;
			}
		case 6:
			{
				ui.pushButton_42->setStyleSheet(QString::fromStdString(ChangeUIImg("putongzengqiang", false)));
				ui.pushButton_46->setStyleSheet(QString::fromStdString(ChangeUIImg("gaojizengqiang", false)));
				ui.pushButton_45->setStyleSheet(QString::fromStdString(ChangeUIImg("chaojizengqiang", true)));
				break;
			}
		}
	}
	//if ()
	{
		ui.pushButton_48->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("huaxian", m_nMouseMode == 1)));
		ui.pushButton_34->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("juxing", m_nMouseMode == 2)));
		ui.pushButton_49->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("tuoyuan", m_nMouseMode == 3)));
		ui.pushButton_50->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("dazi", m_nMouseMode == 5)));
		ui.pushButton_54->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("wanqudu", m_nMouseMode == 6)));
		ui.pushButton_47->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("jiaodu", m_nMouseMode == 4)));
		ui.pushButton_55->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("jietu", m_nMouseMode == 7)));
		ui.pushButton_11->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("fanzhuan", m_bImgNeedReverse)));
		ui.pushButton_57->
		   setStyleSheet(QString::fromStdString(ChangeUIImg("shangchubiaozhu", m_nMouseMode == 8)));
	}
	ui.label_42->setText(QString::number(m_nXRaySendCout));
}

void MainForm::On_Up_PressDown()
{
	int speed = ui.comboBox_3->currentIndex();
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x02, 0b11, 0x01, speed);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Up_Release()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceStop(0x02);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Down_PressDown()
{
	int speed = ui.comboBox_3->currentIndex();
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x02, 0b11, 0x02, speed);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Down_Release()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceStop(0x02);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Left_PressDown()
{
	int speed = ui.comboBox_3->currentIndex();
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x01, 0b11, 0x02, speed);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Left_Release()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceStop(0x01);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Right_PressDown()
{
	int speed = ui.comboBox_3->currentIndex();
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x01, 0b11, 0x01, speed);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Right_Release()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceStop(0x01);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_ImgLabelMouseMove(Qt::MouseButton button, const QPoint& pos)
{
	if (m_bNeedChangePicInfo) // 鼠标右键控制图片的灰度
	{
		auto bv = 1;
		auto dx = (pos.x() - m_nLastMouseX) * bv;
		auto dy = (pos.y() - m_nLastMouseY) * bv;
		ui.horizontalSlider->setValue(ui.horizontalSlider->value() + dx);
		ui.horizontalSlider_2->setValue(ui.horizontalSlider_2->value() - dy);
		On_SliderValueChanged2(0);
	}
	if ((m_nMouseMode == 1 || m_nMouseMode == 2 || m_nMouseMode == 3 || (m_nMouseMode == 6 && m_nMouseClickCount == 0))
		&& !m_vector_ImgTag.empty())
	{
		auto& p = m_vector_ImgTag.back();
		auto labelSize = ui.label->size();


		double newX = 0, newY = 0;
		WHSD_Tools::CalculateOriginalCoordinates(
			pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
				* (m_dImgScale -
					1) / 2), pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
			m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
			m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);


		p.m_nEndX = newX;
		p.m_nEndY = newY;

		PaintImg();
	}

	// 处理四边形标记模式下的鼠标移动（模式4），根据点击次数更新不同的端点坐标
	if (m_nMouseMode == 4 && !m_vector_ImgTag.empty())
	{
		if (m_nMouseClickCount == 0)
		{
			auto& p = m_vector_ImgTag.back();
			auto labelSize = ui.label->size();


			double newX = 0, newY = 0;
			WHSD_Tools::CalculateOriginalCoordinates(
				pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
					* (m_dImgScale -
						1) / 2), pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
				m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
				m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

			p.m_nEndX = newX;
			p.m_nEndY = newY;
		}
		if (m_nMouseClickCount == 1)
		{
			auto& p = m_vector_ImgTag.back();
			auto labelSize = ui.label->size();


			double newX = 0, newY = 0;
			WHSD_Tools::CalculateOriginalCoordinates(
				pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
					* (m_dImgScale -
						1) / 2), pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
				m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
				m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

			p.m_nEndX2 = newX;
			p.m_nEndY2 = newY;
		}
		PaintImg();
	}
	// 处理截图模式下的鼠标移动（模式7），更新截图区域的尺寸和位置
	if (m_nMouseMode == 7)
	{
		auto labelSize = ui.label->size();


		double newX = 0, newY = 0;
		WHSD_Tools::CalculateOriginalCoordinates(
			pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
				* (m_dImgScale -
					1) / 2), pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
			m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
			m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

		auto p2x = newX;
		auto p2y = newY;
		m_memCImageCapture.m_nPicW = std::abs(m_memCImageCapture.m_nPosX - p2x);
		m_memCImageCapture.m_nPicH = std::abs(m_memCImageCapture.m_nPosY - p2y);
		m_memCImageCapture.m_nPosX = std::min(m_memCImageCapture.m_nPosX, (int)p2x);
		m_memCImageCapture.m_nPosY = std::min(m_memCImageCapture.m_nPosY, (int)p2y);
		PaintImg();
	}
}

void MainForm::On_ImgLabelMousePress(Qt::MouseButton button, const QPoint& pos)
{
	if (button == Qt::RightButton)
	{
		m_bNeedChangePicInfo = true;
	}
	if (button == Qt::LeftButton && m_bControlPressed)
    { 
		m_bNeedRoteImg = true;
    }

	m_nLastMouseX = pos.x();

	m_nLastMouseY = pos.y();

	if (m_nMouseMode == 1 || m_nMouseMode == 2 || m_nMouseMode == 3 || (m_nMouseMode == 4 && m_nMouseClickCount == 0) ||
		(m_nMouseMode == 6 && m_nMouseClickCount == 0))
	{
		auto labelSize = ui.label->size();
		double newX = 0, newY = 0;
		WHSD_Tools::CalculateOriginalCoordinates(
			m_nLastMouseX - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
				* (m_dImgScale -
					1) / 2), m_nLastMouseY - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
			m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
			m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

		CImageTag t;
		t.m_nTagType = m_nMouseMode - 1;
		t.m_nStartX = newX;
		t.m_nStartY = newY;
		AddOneImgTag(t);
	}
	if (m_nMouseMode == 7)
	{
		auto labelSize = ui.label->size();

		double newX = 0, newY = 0;
		WHSD_Tools::CalculateOriginalCoordinates(
			m_nLastMouseX - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
				* (m_dImgScale -
					1) / 2), m_nLastMouseY - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
			m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
			m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);
		m_memCImageCapture.m_nPosX = newX;
		m_memCImageCapture.m_nPosY = newY;
	}
	if (m_nMouseMode == 8)
	{
		auto labelSize = ui.label->size();


		double newX = 0, newY = 0;
		WHSD_Tools::CalculateOriginalCoordinates(
			m_nLastMouseX - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.width()
				* (m_dImgScale -
					1) / 2), m_nLastMouseY - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
			m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
			m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);
		auto x = newX;
		auto y = newY;
		int index = -1, index2 = -1;

		auto isPointInExpandedRectangle = [](double xa, double ya, double xb, double yb,
		                                     double xc, double yc, double m)
		{
			// 计算原始矩形的边界
			double originalMinX = std::min(xa, xb);
			double originalMaxX = std::max(xa, xb);
			double originalMinY = std::min(ya, yb);
			double originalMaxY = std::max(ya, yb);

			// 计算扩展后的矩形边界（向四个方向各扩展m单位）
			double expandedMinX = originalMinX - m;
			double expandedMaxX = originalMaxX + m;
			double expandedMinY = originalMinY - m;
			double expandedMaxY = originalMaxY + m;

			// 判断点C是否在扩展后的矩形范围内（包括边界）
			return (xc >= expandedMinX && xc <= expandedMaxX &&
				yc >= expandedMinY && yc <= expandedMaxY);
		};

		auto isPointInBoundingRectangle = [](double xa, double ya,
		                                     double xb, double yb,
		                                     double xc, double yc,
		                                     double xd, double yd)
		{
			// 计算三个点的x坐标的最小值和最大值，确定矩形的左右边界
			double minX = std::min(std::min(xa, xb), xc);
			double maxX = std::max(std::max(xa, xb), xc);

			// 计算三个点的y坐标的最小值和最大值，确定矩形的上下边界
			double minY = std::min(std::min(ya, yb), yc);
			double maxY = std::max(std::max(ya, yb), yc);

			// 判断点d是否在这个矩形范围内（包括边界）
			return (xd >= minX && xd <= maxX && yd >= minY && yd <= maxY);
		};
		auto isDistanceLessOrEqual = [](double xa, double ya, double xb, double yb, double m)
		{
			// 计算两点在x轴和y轴上的距离差
			double dx = xb - xa;
			double dy = yb - ya;

			// 计算欧氏距离的平方（避免开方运算，提高效率）
			double distanceSquared = dx * dx + dy * dy;

			// 计算m的平方
			double mSquared = m * m;

			// 比较距离的平方（结果与比较距离本身一致，但更高效）
			return distanceSquared <= mSquared;
		};
		for (const auto& t : m_vector_ImgTag)
		{
			index2++;
			//0、线 1、矩形 2、椭圆 3、角度、 4、文字 5、弯曲度
			if (t.m_nTagType == 0 || t.m_nTagType == 1 || t.m_nTagType == 2)
			{
				if (isPointInExpandedRectangle(t.m_nStartX, t.m_nStartY, t.m_nEndX, t.m_nEndY, x, y, 10))
				{
					index = index2;
					break;
				}
			}
			if (t.m_nTagType == 3 || t.m_nTagType == 5)
			{
				if (isPointInBoundingRectangle(t.m_nStartX, t.m_nStartY, t.m_nEndX, t.m_nEndY, t.m_nEndX2, t.m_nEndY2,
				                               x, y))
				{
					index = index2;
					break;
				}
			}
			if (t.m_nTagType == 4)
			{
				if (isDistanceLessOrEqual(t.m_nStartX, t.m_nStartY, x, y, 50))
				{
					index = index2;
					break;
				}
			}
		}
		if (index >= 0)
		{
			const auto& pt = m_vector_ImgTag[index];
			m_vector_ImgTag.erase(m_vector_ImgTag.begin() + index);
			m_vector_ImgTag_bak.emplace_back(pt);
			PaintImg();
		}
		m_nMouseMode = 0;
	}
}

void MainForm::On_ImgLabelMouseRelease(Qt::MouseButton button, const QPoint& pos)
{
	if (button == Qt::LeftButton)
	{
		switch (m_nMouseMode)
		{
		case 0:
			{
				m_nImgXOffset = m_nImgXOffset + (pos.x() - m_nLastMouseX);
				m_nImgYOffset = m_nImgYOffset + (pos.y() - m_nLastMouseY);
				PaintImg();
				break;
			}

		case 1:
		case 2:
		case 3:
			{
				if (!m_vector_ImgTag.empty())
				{
					auto& p = m_vector_ImgTag.back();
					auto labelSize = ui.label->size();

					double newX = 0, newY = 0;
					WHSD_Tools::CalculateOriginalCoordinates(
						pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.
							width()
							* (m_dImgScale -
								1) / 2), pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
						m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
						m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

					p.m_nEndX = newX;
					p.m_nEndY = newY;
				}

				PaintImg();
				//这里就算绘制好一条线了
				m_nMouseMode = 0;
				break;
			}
		case 5:
			{
				CImageTag p;
				p.m_nTagType = 4;
				auto labelSize = ui.label->size();

				double newX = 0, newY = 0;
				WHSD_Tools::CalculateOriginalCoordinates(
					pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset - m_memMainQImage.
						width()
						* (m_dImgScale -
							1) / 2), pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
					m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
					m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);


				p.m_nStartX = newX;
				p.m_nStartY = newY;


				// 创建输入对话框，指定当前窗口为主窗口
				QInputDialog dialog(this);

				// 为对话框也设置总在最前属性
				dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);

				// 设置对话框其他属性
				dialog.setWindowTitle("输入");
				dialog.setLabelText("请输入内容:");
				dialog.setInputMode(QInputDialog::TextInput);

				// 显示对话框
				if (dialog.exec() == QInputDialog::Accepted)
				{
					QString text = dialog.textValue();
					// 处理输入内容
					if (!text.isEmpty())
					{
						QByteArray utf8Bytes = text.toUtf8();
						std::string stdStr(utf8Bytes.constData(), utf8Bytes.size());
						p.m_strContent = stdStr;
						AddOneImgTag(p);
					}
				}

				PaintImg();
				m_nMouseMode = 0;
				break;
			}
		case 4:
		case 6:
			{
				if (m_nMouseClickCount == 0)
				{
					if (!m_vector_ImgTag.empty())
					{
						auto labelSize = ui.label->size();

						double newX = 0, newY = 0;
						WHSD_Tools::CalculateOriginalCoordinates(
							pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset -
								m_memMainQImage.
								width()
								* (m_dImgScale -
									1) / 2),
							pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
							m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
							m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

						auto& t = m_vector_ImgTag.back();

						t.m_nEndX = newX;
						t.m_nEndY = newY;
					}
					m_nMouseClickCount++;
				}
				else if (m_nMouseClickCount == 1)
				{
					if (!m_vector_ImgTag.empty())
					{
						auto labelSize = ui.label->size();

						double newX = 0, newY = 0;
						WHSD_Tools::CalculateOriginalCoordinates(
							pos.x() - ((labelSize.width() - m_memMainQImage.width()) / 2 + m_nImgXOffset -
								m_memMainQImage.
								width()
								* (m_dImgScale -
									1) / 2),
							pos.y() - (m_nImgYOffset - m_memMainQImage.height() * (m_dImgScale - 1) / 2),
							m_nRotate * 90, m_dImgScale, m_dImgScale, 0, 0, m_bLeftRightMirror, m_bUpDownMirror,
							m_memMainQImage.width(), m_memMainQImage.height(), &newX, &newY);

						auto& t = m_vector_ImgTag.back();

						t.m_nEndX2 = newX;
						t.m_nEndY2 = newY;
					}
					m_nMouseClickCount = 0;
					m_nMouseMode = 0;
				}
				PaintImg();
				break;
			}
		case 7:
			{
				m_memCImageCapture.m_bSaved = true;
				m_nMouseMode = 0;
				PaintImg();
				break;
			}
		default:
			{
				break;
			}
		}
	}
	m_bNeedChangePicInfo = false;
}

void MainForm::On_ImgLabelWheelUp()
{
	m_dImgScale = m_dImgScale * 1.1;
	m_dImgScale = std::max(m_dImgScale, 0.1);
	m_dImgScale = std::min(m_dImgScale, 10.0);
	PaintImg();
}

void MainForm::On_ImgLabelWheelDown()
{
	m_dImgScale = m_dImgScale / 1.1;
	m_dImgScale = std::max(m_dImgScale, 0.1);
	m_dImgScale = std::min(m_dImgScale, 10.0);
	PaintImg();
}

void MainForm::On_ImgRightDoubleClick()
{
	if (m_nMouseMode == 0)
	{
		m_dImgScale = 1;
		m_nImgXOffset = 0;
		m_nImgYOffset = 0;
		m_nRotate = 0;
		PaintImg();
	}
}

void MainForm::On_ImgLeftDoubleClick()
{
	if (m_nMouseMode == 0)
	{
		m_nRotate++;
		PaintImg();
	}
}

void MainForm::showEvent(QShowEvent* event)
{
	QMainWindow::showEvent(event);
	switch (m_nWorkMode)
	{
	case 1:
		{
			ReadSavedFiles(0);
			break;
		}
	case 2:
		{
			if (!m_bDownloadedPic)
			{
				QPixmap pixmap;
#ifdef _DEBUG

				auto path = "jjz.png";

#else
				auto path = WHSD_Tools::GetExeDirectory() + "\\" + "jjz.png";
#endif

				if (pixmap.load(QString::fromStdString(path)))
				{
					ui.label->setPixmap(pixmap.scaled(ui.label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
				}
				std::thread td(&MainForm::DownloadPic, this);
				td.detach();//线程分离
				break;
			}
		}
	}
}

void MainForm::InitUI(int model)
{
#ifdef _DEBUG
#else
	//this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
#endif
	this->setFocus();
	SetAllVis(true);
	std::string title("设备");
	switch (m_cWorkModel)
	{
	case 0:
		{
			title = "单双分裂一体式";
			ui.groupBox_5->setVisible(false);
			break;
		}
	case 1:
		{
			title = "单双分裂分体式";
			ui.groupBox_5->setVisible(false);
			break;
		}
	case 2:
		{
			title = "四分裂分体式";
			//四分裂需要隐藏的
			//ui.groupBox_6->setVisible(false);
			//Set4SplitNotVisible(false);
			break;
		}
	case 3:
		{
			title = "六八分裂";
			//六八分裂一体式的功能是最全的
			break;
		}
	default:
		{
			break;
		}
	}
	ui.groupBox->setTitle(QString::fromStdString(title));

	this->move(0, 0);
	if (m_strWorkGuid.empty())
	{
		setWindowTitle("河南四达");
	}
	else
	{
		std::string str("河南四达  检测任务ID：");
		str.append(m_strWorkGuid);
		setWindowTitle(str.c_str());
	}

	QIcon appIcon("./1.png");
	if (!appIcon.isNull())
	{
		setWindowIcon(appIcon);
	}
	if (m_pConfig->m_memControlBoardConfig.m_bFactoryMode)
	{
		ui.groupBox_5->setTitle("机器人控制_工程模式");
	}
	else
	{
		ui.pushButton_29->setVisible(false);
		ui.pushButton_30->setVisible(false);
		ui.label_34->setVisible(false);
		ui.label_37->setVisible(false);
	}

	auto primaryScreen = QGuiApplication::primaryScreen();
	if (primaryScreen)
	{
		QRect screenGeometry = primaryScreen->geometry();
		int screenWidth = screenGeometry.width();
		int screenHeight = screenGeometry.height();

		// 设置窗口大小
		setFixedSize(screenWidth, screenHeight - 80);
	}

	ui.groupBox_10->setVisible(false);
	ui.groupBox_8->setVisible(false);
	if (m_strWorkGuid.empty())
	{
		if (m_pConfig->m_memTimsConfig.m_nForceGuid == 1)
		{
			ui.groupBox_10->setVisible(true);
		}
	}
	else
	{
		switch (model)
		{
		case -1:
			{
				ui.groupBox_10->setVisible(true);
				break;
			}
		case 1:
			{
				ui.groupBox_10->setVisible(true);
				break;
			}
		case 2:
			{
				ui.groupBox->setVisible(false);
				ui.groupBox_8->setVisible(true);
				break;
			}
		default:
			{
				break;
			}
		}
	}
	ui.comboBox_5->setCurrentIndex(1);

	QSettings settings("河南四达", "Star_HMI"); // 公司名和应用名
	auto ck = settings.value("CK", QDir::currentPath()).toUInt();
	auto cw = settings.value("CW", QDir::currentPath()).toUInt();

	if (ck == 0)
	{
		ck = 20000;
	}
	if (cw == 0)
	{
		cw = 10000;
	}
	ui.horizontalSlider->setValue(ck);
	ui.lineEdit_4->setText(QString::number(ck));
	ui.horizontalSlider_2->setValue(cw);
	ui.lineEdit_3->setText(QString::number(cw));
	ui.label_40->setVisible(m_pConfig->m_memControlBoardConfig.m_bEnableIp2);
	//ui.groupBox_8->setTitle(QString::fromStdString("图像处理_" + m_pImageProcess->GetVersion() + "："));
}

void MainForm::SetAllVis(const bool v)
{
	ui.groupBox_3->setVisible(v);
	ui.groupBox_6->setVisible(v);
	ui.groupBox_5->setVisible(v);
	Set4SplitNotVisible(v);
}

void MainForm::Set4SplitNotVisible(const bool v)
{
	ui.label_15->setVisible(v);
	ui.pushButton_18->setVisible(v);
	ui.pushButton_19->setVisible(v);
}

void MainForm::BindAction()
{
	// 设置表格属性
	m_pTimer = new QTimer(this);
	m_pTimer->setInterval(500);
	connect(m_pTimer, &QTimer::timeout, this, &MainForm::On_timer_timeout);
	m_pTimer->start();
	connect(ui.pushButton_2, &QPushButton::clicked, this, &MainForm::ConnectDevice);

	m_pKeyEventFilter = new KeyEventFilter(this);
	qApp->installEventFilter(m_pKeyEventFilter);
	connect(m_pKeyEventFilter, &KeyEventFilter::upKeyPressed, this, &MainForm::On_Up_PressDown);
	connect(m_pKeyEventFilter, &KeyEventFilter::upKeyReleased, this, &MainForm::On_Up_Release);
	connect(m_pKeyEventFilter, &KeyEventFilter::downKeyPressed, this, &MainForm::On_Down_PressDown);
	connect(m_pKeyEventFilter, &KeyEventFilter::downKeyReleased, this, &MainForm::On_Down_Release);
	connect(m_pKeyEventFilter, &KeyEventFilter::leftKeyPressed, this, &MainForm::On_Left_PressDown);
	connect(m_pKeyEventFilter, &KeyEventFilter::leftKeyReleased, this, &MainForm::On_Left_Release);
	connect(m_pKeyEventFilter, &KeyEventFilter::rightKeyPressed, this, &MainForm::On_Right_PressDown);
	connect(m_pKeyEventFilter, &KeyEventFilter::rightKeyReleased, this, &MainForm::On_Right_Release);
	connect(m_pKeyEventFilter, &KeyEventFilter::ctrlAEvent, this, &MainForm::On_LoadPic_Clicked);
	connect(m_pKeyEventFilter, &KeyEventFilter::controlOnlyPressed, this, &MainForm::On_ControlOnly_Pressed);
    connect(m_pKeyEventFilter, &KeyEventFilter::controlOnlyReleased, this, &MainForm::On_ControlOnly_Released);
	m_pMouseEventFilter = new MouseEventFilter(this);
	ui.label->installEventFilter(m_pMouseEventFilter);
	connect(m_pMouseEventFilter, &MouseEventFilter::mousePositionChanged, this, &MainForm::On_ImgLabelMouseMove);
	connect(m_pMouseEventFilter, &MouseEventFilter::mousePressed, this, &MainForm::On_ImgLabelMousePress);
	connect(m_pMouseEventFilter, &MouseEventFilter::mouseReleased, this, &MainForm::On_ImgLabelMouseRelease);
	connect(m_pMouseEventFilter, &MouseEventFilter::wheelUp, this, &MainForm::On_ImgLabelWheelUp);
	connect(m_pMouseEventFilter, &MouseEventFilter::wheelDown, this, &MainForm::On_ImgLabelWheelDown);
	connect(m_pMouseEventFilter, &MouseEventFilter::rightDoubleClicked, this, &MainForm::On_ImgRightDoubleClick);
	connect(m_pMouseEventFilter, &MouseEventFilter::leftDoubleClicked, this, &MainForm::On_ImgLeftDoubleClick);

	connect(ui.pushButton_32, &QPushButton::clicked, this, &MainForm::On_ManuallyTrigger_Clicked);
	connect(this, &MainForm::On_Pic_Receive, this, &MainForm::Callback_ShowImgOnLabel_2);
	connect(this, &MainForm::On_OnLineLoadSuccess, this, &MainForm::On_Pic_Receive_Slots);
	connect(this, &MainForm::On_OnLineLoadFailed, this, &MainForm::On_OnLineLoadFailed_Slots);
	connect(ui.pushButton_36, &QPushButton::clicked, this, &MainForm::On_LoadPic_Clicked);

	connect(ui.pushButton_3, &QPushButton::clicked, this, &MainForm::On_NumberOfPulses_clicked);
	connect(ui.pushButton, &QPushButton::clicked, this, &MainForm::On_DelayTime_clicked);

	connect(ui.pushButton_41, &QPushButton::clicked, this, &MainForm::On_LeftRightMirror_clicked);
	connect(ui.pushButton_10, &QPushButton::clicked, this, &MainForm::On_ResetPic_Click);
	connect(ui.pushButton_40, &QPushButton::clicked, this, &MainForm::On_UpDownMirror_clicked);
	connect(ui.pushButton_4, &QPushButton::clicked, this, &MainForm::On_XRayStart_clicked);
	connect(ui.pushButton_5, &QPushButton::clicked, this, &MainForm::On_XRayStop_clicked);

	connect(ui.pushButton_19, &QPushButton::pressed, this, &MainForm::On_SampleBoard_Change_Clicked1);
	connect(ui.pushButton_18, &QPushButton::pressed, this, &MainForm::On_SampleBoard_Change_Clicked2);
	connect(ui.pushButton_7, &QPushButton::pressed, this, &MainForm::On_SafeChanged_On);
	connect(ui.pushButton_6, &QPushButton::pressed, this, &MainForm::On_SafeChanged_Off);
	connect(ui.pushButton_6, &QPushButton::released, this, &MainForm::On_SafeChanged_Stop);
	connect(ui.pushButton_7, &QPushButton::released, this, &MainForm::On_SafeChanged_Stop);
	connect(ui.pushButton_19, &QPushButton::released, this, &MainForm::On_SampleBoard_Change_Stop);
	connect(ui.pushButton_18, &QPushButton::released, this, &MainForm::On_SampleBoard_Change_Stop);

	connect(ui.pushButton_15, &QPushButton::clicked, this, &MainForm::On_TurnOnAll_Click);
	connect(ui.pushButton_12, &QPushButton::clicked, this, &MainForm::On_TurnOffAll_Click);
	auto filter = new CloseEventFilter(this);
	installEventFilter(filter);
	connect(filter, &CloseEventFilter::windowAboutToClose, []
	{
		// 窗口即将关闭，执行自定义操作
		_Exit(0);
	});

	connect(ui.pushButton_16, &QPushButton::clicked, this, &MainForm::On_Upgrade_DoubleClick);
	connect(ui.pushButton_25, &QPushButton::clicked, this, &MainForm::On_TurnOnCamera_Click);
	connect(ui.pushButton_17, &QPushButton::clicked, this, &MainForm::On_TurnOffCamera_Click);
	connect(ui.pushButton_27, &QPushButton::clicked, this, &MainForm::On_ShowConfigFormClick);

	connect(ui.horizontalSlider_2, &QSlider::valueChanged, this, &MainForm::On_SliderValueChanged2);
	connect(ui.horizontalSlider, &QSlider::valueChanged, this, &MainForm::On_SliderValueChanged2);

	connect(ui.pushButton_13, &QPushButton::clicked, this, &MainForm::On_SavePicByGuid_Click);
	connect(ui.pushButton_22, &QPushButton::clicked, this, &MainForm::On_DeletePic_Click);
	connect(ui.pushButton_35, &QPushButton::clicked, this, &MainForm::On_PreviousPic_Click);
	connect(ui.pushButton_37, &QPushButton::clicked, this, &MainForm::On_NextPic_Click);
	connect(ui.pushButton_21, &QPushButton::clicked, this, &MainForm::On_UploadAllPic_Click);

	connect(ui.pushButton_39, &QPushButton::clicked, this, &MainForm::On_UploadAllPic_Click2);

	connect(ui.pushButton_43, &QPushButton::clicked, this, &MainForm::On_PreviousPic_Click2);
	connect(ui.pushButton_44, &QPushButton::clicked, this, &MainForm::On_NextPic_Click2);
	connect(ui.pushButton_9, &QPushButton::clicked, this, &MainForm::On_SavePic_Click);

	//connect(ui.pushButton_29, &QPushButton::clicked, this, &MainForm::On_SaveRaw_Clicked);
	connect(ui.pushButton_28, &QPushButton::clicked, this, &MainForm::On_SavePNG_Clicked);
	connect(ui.pushButton_30, &QPushButton::clicked, this, &MainForm::On_SetFMode);
	connect(ui.pushButton_29, &QPushButton::clicked, this, &MainForm::On_NoSetFMode);

	//connect(ui.pushButton_52, &QPushButton::clicked, this, &MainForm::On_Denoise0);
	connect(ui.pushButton_33, &QPushButton::clicked, this, &MainForm::On_Denoise1);
	connect(ui.pushButton_38, &QPushButton::clicked, this, &MainForm::On_Denoise2);
	connect(ui.pushButton_31, &QPushButton::clicked, this, &MainForm::On_Denoise3);

	//connect(ui.pushButton_53, &QPushButton::clicked, this, &MainForm::On_Enhance0);
	connect(ui.pushButton_42, &QPushButton::clicked, this, &MainForm::On_Enhance1);
	connect(ui.pushButton_46, &QPushButton::clicked, this, &MainForm::On_Enhance2);
	connect(ui.pushButton_45, &QPushButton::clicked, this, &MainForm::On_Enhance3);

	connect(ui.pushButton_48, &QPushButton::clicked, this, &MainForm::On_DrawLine_Click);
	connect(ui.pushButton_34, &QPushButton::clicked, this, &MainForm::On_Rect_Click);
	connect(ui.pushButton_49, &QPushButton::clicked, this, &MainForm::On_Ellipse_Click);
	connect(ui.pushButton_51, &QPushButton::clicked, this, &MainForm::On_MoveLast_Click);
	connect(ui.pushButton_50, &QPushButton::clicked, this, &MainForm::On_InputText_Click);
	connect(ui.pushButton_54, &QPushButton::clicked, this, &MainForm::On_Curvature_Click);
	connect(ui.pushButton_47, &QPushButton::clicked, this, &MainForm::On_Angle_Click);

	connect(ui.pushButton_56, &QPushButton::clicked, this, &MainForm::On_SaveRaw_Clicked);
	connect(ui.pushButton_55, &QPushButton::clicked, this, &MainForm::On_Capture_Click);
	connect(ui.pushButton_8, &QPushButton::clicked, this, &MainForm::On_NoCapture_Click);
	connect(ui.pushButton_8, &QPushButton::clicked, this, &MainForm::On_NoCapture_Click);

	connect(ui.pushButton_58, &QPushButton::clicked, this, &MainForm::On_DenoiseAndEnhance0);
	connect(ui.pushButton_11, &QPushButton::clicked, this, &MainForm::On_ImgReverse_Click);

	connect(ui.pushButton_20, &QPushButton::clicked, this, &MainForm::On_RotateLeft_Click);
	connect(ui.pushButton_14, &QPushButton::clicked, this, &MainForm::On_RotateRight_Click);
	connect(ui.pushButton_26, &QPushButton::clicked, this, &MainForm::On_RecoverImgTag_Click);
	connect(ui.pushButton_57, &QPushButton::clicked, this, &MainForm::On_deleteTag_Click);
	connect(ui.pushButton_59, &QPushButton::clicked, this, &MainForm::On_SaveDealedPic);
}

void MainForm::InitParam()
{
	//第一件事儿就是USB加密狗鉴权
#ifdef _DEBUG

#else

	m_pUSBKey = new CUSBKey();
	if (!m_pUSBKey->ReadUSBKey(&m_memUSBKeyData))
	{
		MY_WARNING("加密狗验证失败！");
		_Exit(0);
	}
	m_cWorkModel = m_memUSBKeyData.m_cDeviceRunMode;
	std::thread td(&CUSBKey::LoopCheckCheckUSBKeyExist, m_pUSBKey);
	td.detach();

#endif

#ifdef _DEBUG
	m_cWorkModel = 0;
#endif
	m_bDownloadedPic = false;
	m_bImgNeedReverse = false;
	for (int i = 0; i < MaxControlBoardCount; ++i)
	{
		m_abControlBroadConnected[i] = false;
		m_apWHSDControlBoardProtocol[i] = nullptr;
		m_apDeviceCom[i] = nullptr;
		m_atime_LastHeartBeatTime[i] = CCFRD_Time();
		m_anHeartBeatCount[i] = 0;
	}
	m_nMouseClickCount = 0;
	m_nMouseMode = 0;
	m_nXRaySendCout = 0;
	m_nPicIndex = 0;
	m_bNeedChangePicInfo = false;
	m_cOTAStatus = 0;
	m_nOTAAllPackNumber = 0;
	m_nOTAPackIndex = 0;
	m_nShowImgTips = -1;
	m_nPicCount = 0;
	m_bLeftRightMirror = false;
	m_bUpDownMirror = false;
	m_bControlPressed = false;
	m_nRotate = 0;
	m_nImgXOffset = 0;
	m_nImgYOffset = 0;
	m_dImgScale = 1;
	m_pCameraFrom = nullptr;
	m_nWifi = 0;
	m_nBattery = 0;
	m_nTemperature = 0;
	m_enumDeviceConnectStatus = DeviceConnectStatus::UnKnown;
	m_enumDeviceRunStatus = DeviceRunStatus::Unknow;
	m_pSampleBoardBase = nullptr;
	m_pDeviceLog = new CWriteLog(WHSD_Tools::GetAbsolutePath("Log\\DeviceLog.txt"), 10000, 250);
	m_pDeviceLog->BeginWork();
	m_pConfig = new CConfigManager();
	m_pConfig->Read(WHSD_Tools::GetAbsolutePath("Config.xml"));

	auto xRayResult = std::bind(&MainForm::Callback_XRaySendResult, this, std::placeholders::_1);

	{
		//先初始化主控制板
		m_apDeviceCom[0] = IDeviceCom::GetIDeviceCom(1);
		m_apWHSDControlBoardProtocol[0] = new CWHSDControlBoardProtocol(
			m_pConfig->m_memControlBoardConfig.m_wDeviceHeartBeat);
		auto pWHSDControlBoardProtocol = m_apWHSDControlBoardProtocol[0];
		auto pDeviceCom = m_apDeviceCom[0];
		pDeviceCom->SetParam(m_pConfig->m_memControlBoardConfig.m_strIp.c_str(),
		                     m_pConfig->m_memControlBoardConfig.m_wPort);
		pDeviceCom->RegisterReadDataCallBack(std::bind(&CWHSDControlBoardProtocol::ReceiveNewData,
		                                               pWHSDControlBoardProtocol, std::placeholders::_1,
		                                               std::placeholders::_2));
		pDeviceCom->RegisterConnectStatusCallBack(std::bind(&MainForm::Callback_ComDeviceConnectionChanged, this,
		                                                    std::placeholders::_1, std::placeholders::_2, 0));
		pWHSDControlBoardProtocol->RegisterAnswerFunction(
			std::bind(&IDeviceCom::Write, pDeviceCom, std::placeholders::_1, std::placeholders::_2));
		pWHSDControlBoardProtocol->RegisterDeviceLog(std::bind(&CWriteLog::Write, m_pDeviceLog, std::placeholders::_1));
		pWHSDControlBoardProtocol->RegisterDeviceHeartBeat(
			std::bind(&MainForm::Callback_DeviceHeartBeat, this, std::placeholders::_1, 0));
		pWHSDControlBoardProtocol->RegisterOTAStatus(std::bind(&MainForm::Callback_OTAStatus, this,
		                                                       std::placeholders::_1,
		                                                       std::placeholders::_2, std::placeholders::_3));
		pWHSDControlBoardProtocol->RegisterXRaySendResult(xRayResult);
		pDeviceCom->BeginWork();
		pWHSDControlBoardProtocol->BeginWork();
	}

	if (m_pConfig->m_memControlBoardConfig.m_bEnableIp2)
	{
		//再初始化次控制板
		m_apDeviceCom[1] = IDeviceCom::GetIDeviceCom(1);
		m_apWHSDControlBoardProtocol[1] = new CWHSDControlBoardProtocol(
			m_pConfig->m_memControlBoardConfig.m_wDeviceHeartBeat);
		auto pXRayControlPanelCom = m_apDeviceCom[1];
		pXRayControlPanelCom->SetParam(m_pConfig->m_memControlBoardConfig.m_strIp2.c_str(),
		                               m_pConfig->m_memControlBoardConfig.m_wPort2);

		auto pWHSDControlBoardProtocol2 = m_apWHSDControlBoardProtocol[1];
		pXRayControlPanelCom->RegisterReadDataCallBack(std::bind(&CWHSDControlBoardProtocol::ReceiveNewData,
		                                                         pWHSDControlBoardProtocol2, std::placeholders::_1,
		                                                         std::placeholders::_2));
		pXRayControlPanelCom->RegisterConnectStatusCallBack(std::bind(&MainForm::Callback_ComDeviceConnectionChanged,
		                                                              this,
		                                                              std::placeholders::_1, std::placeholders::_2, 1));
		pWHSDControlBoardProtocol2->RegisterDeviceHeartBeat(
			std::bind(&MainForm::Callback_DeviceHeartBeat, this, std::placeholders::_1, 1));
		pWHSDControlBoardProtocol2->RegisterAnswerFunction(
			std::bind(&IDeviceCom::Write, pXRayControlPanelCom, std::placeholders::_1, std::placeholders::_2));

		pWHSDControlBoardProtocol2->RegisterXRaySendResult(xRayResult);
		pXRayControlPanelCom->BeginWork();
		pWHSDControlBoardProtocol2->BeginWork();
	}

	m_pImageProcess = CImageProcessBase::GetCImageProcessObj(m_pConfig->m_memCImageProcessConfig.m_nType);
	ui.pushButton_32->setEnabled(m_pConfig->m_memCSampleBoardConfig.m_nExposureType == 1);
}

void MainForm::Callback_ComDeviceConnectionChanged(const bool connected, int guid, int index)
{
	m_abControlBroadConnected[index] = connected;
}

void MainForm::SendData2ComDevice(uint8_t* p, const int len)
{
	m_apDeviceCom[0]->Write(p, len);
}

void MainForm::Callback_GetSampleBoardValue(const DeviceValue v, const double d)
{
	m_mutexDeviceInfoLock.lock();
	switch (v)
	{
	case Temperature:
		{
			m_nTemperature = d;
			break;
		}
	case Battery:
		{
			m_nBattery = d;
			break;
		}
	case Wifi:
		{
			m_nWifi = d;
			break;
		}
	default:
		{
			break;
		}
	}
	m_mutexDeviceInfoLock.unlock();
}

void MainForm::Callback_OTAStatus(const uint8_t t1, const uint32_t t2, const uint32_t t3)
{
	m_cOTAStatus = t1;
	m_nOTAAllPackNumber = t2;
	m_nOTAPackIndex = t3;
}

void MainForm::Callback_XRaySendResult(uint8_t re)
{
	if (re > 0)
	{
		m_nXRaySendCout++;
		if (m_pConfig->m_memCSampleBoardConfig.m_nExposureType == 1)
		{
			On_ManuallyTrigger_Clicked();
		}
	}
}

bool MainForm::CheckPassword()
{
	// 创建输入对话框，指定当前窗口为主窗口
	QInputDialog dialog(this);

	// 为对话框也设置总在最前属性
	dialog.setWindowFlags(dialog.windowFlags() | Qt::WindowStaysOnTopHint);

	// 设置对话框其他属性
	dialog.setWindowTitle("身份验证");
	dialog.setLabelText("请输入密码:");
	dialog.setInputMode(QInputDialog::TextInput);

	// 显示对话框
	if (dialog.exec() == QInputDialog::Accepted)
	{
		QString password = dialog.textValue();
		// 处理输入内容
		if (!password.isEmpty() && password == "biubiubiu666")
		{
			return true;
		}
	}

	MY_WARNING("密码错误");
	return false;
}

void MainForm::Callback_ShowImgOnLabel_2()
{
	auto m_vector_LastImgBuffer = m_memSDRaw.GetOriginalRawData();
	QImage image(m_vector_LastImgBuffer.data(), m_memSDRaw.m_wPicWidth, m_memSDRaw.m_wPicHeight,
	             m_memSDRaw.m_wPicWidth * sizeof(uint16_t),
	             QImage::Format_Grayscale16);
	m_memMainQImage = image.convertToFormat(QImage::Format_Grayscale16).scaled(ui.label->size(), Qt::KeepAspectRatio,
	                                                                           Qt::SmoothTransformation);
	On_SliderValueChanged();
	//PaintImg();
}

void MainForm::ShowImgOnLabel()
{
	QImage image(m_vecNeedShowBuffer.data(), m_memSDRaw.m_wPicWidth, m_memSDRaw.m_wPicHeight,
	             m_memSDRaw.m_wPicWidth * sizeof(uint8_t),
	             QImage::Format_Grayscale8);
	m_memMainQImage = image.convertToFormat(QImage::Format_Grayscale16).scaled(ui.label->size(), Qt::KeepAspectRatio,
	                                                                           Qt::SmoothTransformation);
	PaintImg();
}

//void MainForm::ShowImgOnLabel(const std::vector<uint8_t>& v, const int hei, const int wid)
//{
//	QImage image(v.data(), wid, hei, wid * sizeof(uint16_t), QImage::Format_Grayscale16);
//	PaintImg(image.convertToFormat(QImage::Format_Grayscale16).scaled(ui.label->size(), Qt::KeepAspectRatio,Qt::SmoothTransformation));
//}
//
//void MainForm::ShowImgOnLabel(const std::vector<uint16_t>& v, const int hei, const int wid)
//{
//	//直接用QImage::Format_Grayscale16进行显示，qt直接支持，如果转换为图片，需要为tiff
//	QImage image((uint8_t*)v.data(), wid, hei, wid * sizeof(uint16_t), QImage::Format_Grayscale16);
//	QImage displayImage = image.convertToFormat(QImage::Format_Grayscale16);
//	QSize labelSize = ui.label->size();
//	QImage scaledImage = displayImage.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
//	PaintImg(scaledImage);
//}

void MainForm::On_LeftRightMirror_clicked()
{
	m_bLeftRightMirror = !m_bLeftRightMirror;
	PaintImg();
}

void MainForm::On_UpDownMirror_clicked()
{
	m_bUpDownMirror = !m_bUpDownMirror;
	PaintImg();
}

void MainForm::On_NoCapture_Click()
{
	m_memCImageCapture.Clear();
	PaintImg();
}

void MainForm::On_ImgReverse_Click()
{
	m_bImgNeedReverse = !m_bImgNeedReverse;
	m_pImageProcess->SetReverse(m_bImgNeedReverse);
	On_SliderValueChanged();
}

void MainForm::On_RotateLeft_Click()
{
	m_nRotate = m_nRotate + 3;
	PaintImg();
}

void MainForm::On_RotateRight_Click()
{
	m_nRotate++;
	PaintImg();
}

void MainForm::On_RecoverImgTag_Click()
{
	if (!m_vector_ImgTag_bak.empty())
	{
		const auto& p = m_vector_ImgTag_bak.back();
		m_vector_ImgTag.push_back(p);
		m_vector_ImgTag_bak.pop_back();
		PaintImg();
	}
}

void MainForm::On_deleteTag_Click()
{
	m_nMouseMode = 8;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_SaveDealedPic()
{
	QString dirPath = QFileDialog::getExistingDirectory(
		this,
		tr("选择目录"),
		QDir::homePath(),
		QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
	);

	if (!dirPath.isEmpty())
	{
		std::string path = dirPath.toLocal8Bit().constData();
		path.append("/");
		path.append(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss").toStdString());
		// 获取当前日期时间


		for (const auto & p: m_mapDealedPic)
		{
			std::string fileName = path +"_"+ std::to_string(p.first) + ".png";
			WHSD_Tools::SaveDataToFile((uint8_t*)p.second.data(), p.second.size(), fileName.c_str());
		}
		if (!m_mapDealedPic.empty())
		{
			MY_INFO("保存成功");
		}
		// 处理选择的目录路径
	}
}

void MainForm::On_TurnOnAll_Click()
{
	auto cmds = CWHSDControlBoardProtocol::TurnOnAll();
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_TurnOffAll_Click()
{
	auto cmds = CWHSDControlBoardProtocol::TurnOffAll();
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_Upgrade_DoubleClick()
{
	if (CheckPassword())
	{
		// 用户点击了"确定"且输入不为空
		// 处理密码...
		QString qFilePath = QFileDialog::getOpenFileName(
			this, // 父窗口指针
			"选择文件", // 对话框标题
			".", // 默认路径（用户主目录）
			"bin文件 (*.bin);" // 文件过滤器
		);

		if (!qFilePath.isEmpty())
		{
			auto filePath = qFilePath.toStdString();
			std::vector<uint8_t> buf;
			if (WHSD_Tools::ReadFileToVector(filePath, &buf))
			{
				m_apWHSDControlBoardProtocol[0]->BeginOTA(buf);
			}
			else
			{
				MY_WARNING("文件读取失败，请放置在英文路径并且程序有权限读取");
			}
		}
		else
		{
			MY_WARNING("未选择文件");
		}
	}
}

void MainForm::On_TurnOnCamera_Click()
{
	WHSD_Tools::SafeRelease(m_pCameraFrom);
	m_pCameraFrom = new CameraFrom(m_pConfig);
	m_pCameraFrom->show();
}

void MainForm::On_TurnOffCamera_Click()
{
	if (m_pCameraFrom != nullptr)
	{
		m_pCameraFrom->close();
	}
	WHSD_Tools::SafeRelease(m_pCameraFrom);
	//m_pCameraFrom = nullptr;
}

void MainForm::On_ShowConfigFormClick()
{
	if (CheckPassword())
	{
		WHSD_Tools::SafeRelease(m_pControlBoard);
		m_pControlBoard = new ControlBoardConfigForm(m_apDeviceCom[0]);
		m_pControlBoard->show();
	}
}

void MainForm::On_SliderValueChanged()
{
	auto ck = ui.horizontalSlider->value();
	auto cw = ui.horizontalSlider_2->value();
	m_memImageProcessParam2.m_wBright = cw;
	m_memImageProcessParam2.m_wContrast = ck;

	{
		std::lock_guard<std::mutex> g(m_mutexLastImgMutex);
		auto m_vector_LastImgBuffer = m_memSDRaw.GetOriginalRawData();
		auto m_vector_ChangedImgBuffer2 = m_memSDRaw.GetTempRawData();
		if (m_pImageProcess->ImageProcess1(m_memSDRaw.m_wPicWidth, m_memSDRaw.m_wPicHeight,
		                                   m_vector_LastImgBuffer.data(),
		                                   &m_memImageProcessParam1, &m_vector_ChangedImgBuffer2))
		{
			m_memSDRaw.SetTempRawData(&m_vector_ChangedImgBuffer2);
			if (m_pImageProcess->BrightAndContrastProcess(m_memSDRaw.m_wPicWidth, m_memSDRaw.m_wPicHeight,
			                                              reinterpret_cast<uint16_t*>(m_vector_ChangedImgBuffer2.
				                                              data()),
			                                              &m_memImageProcessParam2, &m_vecNeedShowBuffer))
			{
			}
		}
	}


	ui.lineEdit_3->setText(QString::number(cw));
	ui.lineEdit_4->setText(QString::number(ck));
	ShowImgOnLabel();
}

void MainForm::On_SliderValueChanged2(int value)
{
	auto ck = ui.horizontalSlider->value();
	auto cw = ui.horizontalSlider_2->value();
	m_memImageProcessParam2.m_wBright = cw;
	m_memImageProcessParam2.m_wContrast = ck;
	{
		std::lock_guard<std::mutex> g(m_mutexLastImgMutex);
		auto m_vector_ChangedImgBuffer2 = m_memSDRaw.GetTempRawData();
		m_pImageProcess->BrightAndContrastProcess(m_memSDRaw.m_wPicWidth, m_memSDRaw.m_wPicHeight,
		                                          (uint16_t*)m_vector_ChangedImgBuffer2.data(),
		                                          &m_memImageProcessParam2, &m_vecNeedShowBuffer);
	}
	ShowImgOnLabel();
	ui.lineEdit_3->setText(QString::number(cw));
	ui.lineEdit_4->setText(QString::number(ck));

	QSettings settings("河南四达", "Star_HMI"); // 公司名和应用名
	//auto cl = settings.value("CK", QDir::currentPath()).toUInt();
	//auto cw = settings.value("CW", QDir::currentPath()).toUInt();

	settings.setValue("CK", ck);
	settings.setValue("CW", cw);
	/*

		QString filter = "RAW文件 (*.raw);;SDRAW文件 (*.sdraw);;所有文件 (*)";
	QSettings settings("河南四达", "Star_HMI"); // 公司名和应用名
	QString lastPath = settings.value("LastFileDialogPath", QDir::currentPath()).toString();
	QString pa = QDir::homePath();
	if (!lastPath.isEmpty())
	{
		pa = lastPath;
	}
	// 打开文件选择对话框
	QString qFilePath = QFileDialog::getOpenFileName(
		this, // 父窗口指针
		tr("选择文件"), // 对话框标题
		pa, // 初始目录（这里使用用户主目录）
		filter // 文件过滤器
	);

	// 判断用户是否选择了文件
	if (qFilePath.isEmpty())
	{
		return;
	}
	QFileInfo fileInfo(qFilePath);
	QString currentPath = fileInfo.path();
	settings.setValue("LastFileDialogPath", currentPath);
	// 获取文件后缀并判断类型
	QString fileExt = QFileInfo(qFilePath).suffix().toLower();

	*/
}

void MainForm::On_SavePicByGuid_Click()
{
	if (m_strWorkGuid.empty())
	{
		MY_WARNING("未获取到工作GUID，请在网页端唤起本程序!");
	}
	auto dt = m_memSDRaw.GetSaveOriginalData();
	if (dt.empty())
	{
		MY_WARNING("未读取到图片");
	}
	if (WHSD_Tools::SaveFileByGuid(dt.data(), dt.size(), m_strWorkGuid, 1))
	{
		ReadSavedFiles(-1);
		MY_INFO("保存成功");
	}
	else
	{
		MY_WARNING("保存失败");
	}
}

void MainForm::On_DeletePic_Click()
{
	if (!m_vectorReadedPicPath.empty() && m_nPicIndex >= 0 && m_nPicIndex < m_vectorReadedPicPath.size())
	{
		int result = remove(m_vectorReadedPicPath[m_nPicIndex].c_str());
		if (result == 0)
		{
			ReadSavedFiles(m_nPicIndex - 1);
			MY_INFO("删除成功");
			return;
		}
	}
	MY_WARNING("删除失败");
}

void MainForm::On_PreviousPic_Click()
{
	ReadSavedFiles(m_nPicIndex - 1);
}

void MainForm::On_NextPic_Click()
{
	ReadSavedFiles(m_nPicIndex + 1);
}

void MainForm::On_UploadAllPic_Click2()
{
	bool dealAll = true;
	for (const auto& mp : m_mapLoadedMap)
	{
		auto p = m_mapDealedPic.find(mp.first);
		if (p == m_mapDealedPic.end())
		{
			dealAll = false;
			break;
		}
		if (p->second.empty())
		{
			dealAll = false;
			break;
		}
	}
	if (!dealAll)
	{
		MY_WARNING("请处理完所有图片");
		return;
	}
	auto [t1, t2, t3] = HttpClient::Get(m_pConfig->m_memTimsConfig.m_strGetGuidInfo,
	                                    {{"sampleId", m_strWorkGuid}});
	QString clampName;
	if (t1)
	{
		auto jsonStr = QString::fromStdString(t2);
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
		// 检查解析是否成功
		if (parseError.error == QJsonParseError::NoError)
		{
			// 2. 提取 JSON 对象（假设根节点是对象）
			QJsonObject rootObj = doc.object();
			auto code = rootObj["code"].toInt();
			if (code == 200)
			{
				auto result = rootObj["data"].toObject();
				clampName = result["ClampName"].toString();
			}
		}
	}
	if (clampName.isEmpty())
	{
		MY_WARNING("操作失败");
		return;
	}
	QJsonObject jsonObj;
	QJsonArray picArray;
	int index = 0;
	for (const auto& v666 : m_mapDealedPic)
	{
		auto v = v666.second;
		index++;
		QJsonObject node;
		auto imgBts = WHSD_Tools::Base64Encode(v);
		node.insert("Index", index);
		node.insert("FileName", clampName + "~" + QString::number(index));
		//node.insert("FileName", "haha");
		node.insert("PicData", QString::fromStdString(WHSD_Tools::Base64Encode(v)));
		//node.insert("PicData", "123321");
		picArray.append(node);
	}
	jsonObj.insert("Guid", QString::fromStdString(m_strWorkGuid));
	jsonObj.insert("ImgType", 1);
	jsonObj.insert("Imgs", picArray);
	QJsonDocument doc(jsonObj);
	bool success = false;
	auto [t4, t5, t6] = HttpClient::Post(m_pConfig->m_memTimsConfig.m_strUploadPic, doc.toJson(),
	                                     "application/json", {}, 20000);
	if (t4)
	{
		auto jsonStr = QString::fromStdString(t2);
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
		// 检查解析是否成功
		if (parseError.error == QJsonParseError::NoError)
		{
			// 2. 提取 JSON 对象（假设根节点是对象）
			QJsonObject rootObj = doc.object();
			auto code = rootObj["code"].toInt();
			if (code == 200)
			{
				success = true;
			}
		}
	}
	if (success)
	{
		MY_INFO("操作成功");
	}
	else
	{
		MY_WARNING("操作失败");
	}
}

void MainForm::On_UploadAllPic_Click()
{
	auto [t1, t2, t3] = HttpClient::Get(m_pConfig->m_memTimsConfig.m_strGetGuidInfo,
	                                    {{"sampleId", m_strWorkGuid}});
	QString clampName;
	if (t1)
	{
		auto jsonStr = QString::fromStdString(t2);
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
		// 检查解析是否成功
		if (parseError.error == QJsonParseError::NoError)
		{
			// 2. 提取 JSON 对象（假设根节点是对象）
			QJsonObject rootObj = doc.object();
			auto code = rootObj["code"].toInt();
			if (code == 200)
			{
				auto result = rootObj["data"].toObject();
				clampName = result["ClampName"].toString();
			}
		}
	}
	if (clampName.isEmpty())
	{
		MY_WARNING("操作失败");
		return;
	}
	QJsonObject jsonObj;
	QJsonArray picArray;
	int index = 0;
	for (const auto& value : m_vectorReadedPicPath)
	{
		std::vector<uint8_t> v;
		if (WHSD_Tools::ReadFileToVector(value, &v))
		{
			index++;
			QJsonObject node;
			auto imgBts = WHSD_Tools::Base64Encode(v);
			node.insert("Index", index);
			node.insert("FileName", clampName + "~" + QString::number(index));
			//node.insert("FileName", "haha");
			node.insert("PicData", QString::fromStdString(WHSD_Tools::Base64Encode(v)));
			//node.insert("PicData", "123321");
			picArray.append(node);
		}
	}
	jsonObj.insert("Guid", QString::fromStdString(m_strWorkGuid));
	jsonObj.insert("ImgType", 0);
	jsonObj.insert("Imgs", picArray);
	QJsonDocument doc(jsonObj);
	bool success = false;
	auto [t4, t5, t6] = HttpClient::Post(m_pConfig->m_memTimsConfig.m_strUploadPic, doc.toJson(),
	                                     "application/json", {}, 20000);
	if (t4)
	{
		auto jsonStr = QString::fromStdString(t2);
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
		// 检查解析是否成功
		if (parseError.error == QJsonParseError::NoError)
		{
			// 2. 提取 JSON 对象（假设根节点是对象）
			QJsonObject rootObj = doc.object();
			auto code = rootObj["code"].toInt();
			if (code == 200)
			{
				success = true;
			}
		}
	}
	if (success)
	{
		MY_INFO("操作成功");
	}
	else
	{
		MY_WARNING("操作失败");
	}
}

void MainForm::On_ResetPic_Click()
{
	//直接清空即可
	auto dt = m_mapDealedPic.find(m_nPicIndex);
	if (dt != m_mapDealedPic.end())
	{
		dt->second.clear();
	}
	m_vector_ImgTag.clear();
	m_vector_ImgTag_bak.clear();

	ClearPicOpt();

	ReadPicFromMem(m_nPicIndex);
}

void MainForm::On_SavePic_Click()
{
	// 将 QImage 直接转换为 PNG 格式的 std::vector<uint8_t>（无中间文件）
	auto imageToPngVector = [](const QImage& image)-> std::vector<uint8_t>
	{
		// 1. 创建内存缓冲区（QBuffer）用于存储 PNG 数据
		QByteArray byteArray;
		QBuffer buffer(&byteArray);
		if (!buffer.open(QIODevice::WriteOnly))
		{
			return {};
		}

		// 2. 使用 QImageWriter 编码为 PNG 格式
		QImageWriter writer(&buffer, "PNG"); // 第二个参数指定格式为 PNG
		if (!writer.write(image))
		{
			buffer.close();
			return {};
		}

		// 3. 关闭缓冲区并获取数据
		buffer.close();

		return std::vector<uint8_t>(byteArray.begin(), byteArray.end());
	};
	m_mapDealedPic[m_nPicIndex] = imageToPngVector(m_memShowedImg);
	ui.label_35->setStyleSheet("color:lightgreen");
	//ReadPicFromMem(m_nPicIndex);
	MY_INFO("保存成功");
}

void MainForm::On_DeletePic_Click2()
{
	m_mapDealedPic.erase(m_nPicIndex);
	ReadPicFromMem(m_nPicIndex);
}

void MainForm::On_Pic_Receive_Slots()
{
	ReadPicFromMem(0);
}

void MainForm::On_PreviousPic_Click2()
{
	m_vector_ImgTag.clear();
	ReadPicFromMem(m_nPicIndex - 1);
}

void MainForm::On_NextPic_Click2()
{
	m_vector_ImgTag.clear();
	ReadPicFromMem(m_nPicIndex + 1);
}

QImage MainForm::PaintTag()
{
	QImage q(m_memMainQImage.width(), m_memMainQImage.height(), QImage::Format_ARGB32);
	QPainter painter2(&q);
	painter2.setRenderHint(QPainter::TextAntialiasing); // 抗锯齿
	painter2.drawImage(0, 0, m_memMainQImage);
	QPen pen;
	pen.setWidth(1);
	pen.setColor(Qt::red);
	painter2.setPen(pen);
	for (const auto& s : m_vector_ImgTag)
	{
		//pen.setColor(s.m_bSelected ? Qt::green : Qt::red);
		painter2.save();

		switch (s.m_nTagType)
		{
		case 0:
			{
				painter2.drawLine(s.m_nStartX, s.m_nStartY, s.m_nEndX, s.m_nEndY);
				break;
			}
		case 1:
			{
				painter2.drawRect(std::min(s.m_nStartX, s.m_nEndX), std::min(s.m_nStartY, s.m_nEndY),
				                  std::abs(s.m_nStartX - s.m_nEndX), std::abs(s.m_nStartY - s.m_nEndY));
				break;
			}
		case 2:
			{
				painter2.drawEllipse(std::min(s.m_nStartX, s.m_nEndX), std::min(s.m_nStartY, s.m_nEndY),
				                     std::abs(s.m_nStartX - s.m_nEndX), std::abs(s.m_nStartY - s.m_nEndY));
				break;
			}
		case 3:
			{
				painter2.drawLine(s.m_nStartX, s.m_nStartY, s.m_nEndX, s.m_nEndY);
				if (s.m_nEndX2 != 0 && s.m_nEndY2 != 0)
				{
					painter2.drawLine(s.m_nEndX2, s.m_nEndY2, s.m_nEndX, s.m_nEndY);
					auto calculateAngleABC = [](double ax, double ay, double bx, double by, double cx, double cy)
					{
						// 计算向量BA和向量BC
						const double ba_x = ax - bx; // 向量BA = A - B
						const double ba_y = ay - by;
						const double bc_x = cx - bx; // 向量BC = C - B
						const double bc_y = cy - by;

						// 计算向量点积
						const double dot_product = ba_x * bc_x + ba_y * bc_y;

						// 计算向量模长
						const double len_ba = std::sqrt(ba_x * ba_x + ba_y * ba_y);
						const double len_bc = std::sqrt(bc_x * bc_x + bc_y * bc_y);

						// 处理特殊情况：如果有向量长度为0，角度为0°
						if (len_ba < 1e-12 || len_bc < 1e-12)
						{
							return 0.0;
						}

						// 计算余弦值，并用clamp确保在[-1, 1]范围内（处理浮点误差）
						double cos_theta = dot_product / (len_ba * len_bc);
						cos_theta = std::clamp(cos_theta, -1.0, 1.0);
						// 确保不会因精度问题超出范围
						// 计算角度（acos返回0到π弧度，转换为0到180度）
						const double angle_rad = std::acos(cos_theta);
						const double angle_deg = angle_rad * 180.0 / M_PI;

						// 由于acos的特性，角度自然在0°到180°之间，这里再次确认
						return std::clamp(angle_deg, 0.0, 180.0);
					};
					auto jd = calculateAngleABC(s.m_nStartX, s.m_nStartY, s.m_nEndX, s.m_nEndY, s.m_nEndX2, s.m_nEndY2);

					std::stringstream ss;
					ss << "angel:" << jd << "°";

					painter2.translate(s.m_nStartX, s.m_nStartX);
					painter2.rotate(0 - (m_nRotate % 4) * 90); // 平移到绘制起点（旋转中心）
					painter2.drawText(0, 0, ss.str().c_str());
					painter2.restore();
				}
				break;
			}
		case 4:
			{
				painter2.translate(s.m_nStartX, s.m_nStartX);
				painter2.rotate(0 - (m_nRotate % 4) * 90); // 平移到绘制起点（旋转中心）
				painter2.drawText(0, 0, s.m_strContent.c_str());
				painter2.restore();
				break;
			}
		case 5:
			{
				painter2.drawLine(s.m_nStartX, s.m_nStartY, s.m_nEndX, s.m_nEndY);
				if (s.m_nEndX2 != 0 && s.m_nEndY2 != 0)
				{
					auto findPerpendicularFoot = [](double ax, double ay, double bx, double by,
					                                double cx, double cy, double* dx, double* dy)
					{
						// 计算向量AB和AC
						double ab_x = bx - ax;
						double ab_y = by - ay;
						double ac_x = cx - ax;
						double ac_y = cy - ay;

						// 计算AB·AC（点积）
						double dot_product = ab_x * ac_x + ab_y * ac_y;

						// 计算AB·AB（AB的长度平方）
						double ab_squared = ab_x * ab_x + ab_y * ab_y;

						// 如果AB长度为0（A和B是同一点），直接返回A
						if (fabs(ab_squared) < 1e-9)
						{
							*dx = ax;
							*dy = ay;
							return;
						}

						// 计算参数t，即投影比例
						double t = dot_product / ab_squared;

						// 计算垂足D
						*dx = ax + t * ab_x;
						*dy = ay + t * ab_y;
					};

					auto calculateDistance = [](double ax, double ay, double bx, double by)
					{
						// 计算x坐标差的平方
						double dx_squared = (bx - ax) * (bx - ax);
						// 计算y坐标差的平方
						double dy_squared = (by - ay) * (by - ay);
						// 返回平方根，即两点间的距离
						return std::sqrt(dx_squared + dy_squared);
					};

					double dx, dy;
					findPerpendicularFoot(s.m_nStartX, s.m_nStartY, s.m_nEndX, s.m_nEndY, s.m_nEndX2, s.m_nEndY2, &dx,
					                      &dy);

					painter2.drawLine(dx, dy, s.m_nEndX2, s.m_nEndY2);

					auto len = calculateDistance(s.m_nStartX, s.m_nStartY, s.m_nEndX, s.m_nEndY);
					auto hei = calculateDistance(dx, dy, s.m_nEndX2, s.m_nEndY2);

					std::stringstream ss;
					ss << "L:" << len << ",H:" << hei;

					painter2.translate(s.m_nEndX2, s.m_nEndY2);
					painter2.rotate(0 - (m_nRotate % 4) * 90); // 平移到绘制起点（旋转中心）
					painter2.drawText(5, 5, ss.str().c_str());
					painter2.restore();
				}
				break;
			}
		}
	}
	if (!m_memCImageCapture.m_bSaved)
	{
		painter2.drawRect(m_memCImageCapture.m_nPosX, m_memCImageCapture.m_nPosY, m_memCImageCapture.m_nPicW,
		                  m_memCImageCapture.m_nPicH);
	}
	painter2.end();
	return q;
}

void MainForm::PaintImg()
{
	//创建用于绘制在label上的img
	QSize labelSize = ui.label->size();
	QImage q(labelSize, QImage::Format_ARGB32);
	//绘制标记
	auto memMainQImage = PaintTag();
	// 缩放
	memMainQImage = memMainQImage.scaled(
		memMainQImage.width() * m_dImgScale, // 目标宽度（仅作参考，会根据比例调整）
		memMainQImage.height() * m_dImgScale, // 目标高度（仅作参考，会根据比例调整）
		Qt::KeepAspectRatio, // 保持原始长宽比
		Qt::SmoothTransformation // 使用高质量双线性插值
	);
	{
		//旋转
		auto rt = m_nRotate % 4;
		if (rt != 0)
		{
			memMainQImage = memMainQImage.transformed(QTransform().rotate(rt * 90));
		}
		//镜像
		if (m_bLeftRightMirror)
		{
			memMainQImage = memMainQImage.transformed(QTransform().scale(-1, 1), Qt::SmoothTransformation);
		}
		if (m_bUpDownMirror)
		{
			memMainQImage = memMainQImage.transformed(QTransform().scale(1, -1), Qt::SmoothTransformation);
		}
	}
	{
		//绘图
		QPainter painter(&q);
		if (m_memCImageCapture.m_nPosX != 0 && m_memCImageCapture.m_nPosY != 0 && m_memCImageCapture.m_nPicW != 0 &&
			m_memCImageCapture.m_nPicH != 0 && m_memCImageCapture.m_bSaved)
		{
			//截图
			auto stretchImageKeepRatio = [](QImage& a, int starX, int starY, int width, int height)
			{
				// 确保源区域有效
				if (starX < 0 || starY < 0 || width <= 0 || height <= 0 ||
					starX + width > a.width() || starY + height > a.height())
				{
					return; // 无效区域，直接返回
				}

				// 提取指定区域
				QImage sourceRegion = a.copy(starX, starY, width, height);

				// 计算源区域的宽高比
				qreal aspectRatio = static_cast<qreal>(width) / height;

				// 获取目标图像a的尺寸
				int targetWidth = a.width();
				int targetHeight = a.height();

				// 计算按比例拉伸后的尺寸（完全填充目标区域，保持比例）
				int drawWidth, drawHeight;

				if (static_cast<qreal>(targetWidth) / targetHeight > aspectRatio)
				{
					// 目标区域更宽，按高度缩放
					drawHeight = targetHeight;
					drawWidth = static_cast<int>(drawHeight * aspectRatio);
				}
				else
				{
					// 目标区域更高，按宽度缩放
					drawWidth = targetWidth;
					drawHeight = static_cast<int>(drawWidth / aspectRatio);
				}

				// 计算居中绘制的位置（可选，也可根据需要调整位置）
				int x = (targetWidth - drawWidth) / 2;
				int y = (targetHeight - drawHeight) / 2;

				a.fill(Qt::transparent);
				// 创建QPainter绘制
				QPainter painter23(&a);
				painter23.setRenderHint(QPainter::SmoothPixmapTransform, true); // 平滑缩放

				// 清空目标图像（可选，根据需求决定是否保留背景）
				painter23.fillRect(a.rect(), Qt::transparent); // 透明背景，也可改为其他颜色

				// 拉伸绘制（保持比例）
				painter23.drawImage(QRect(x, y, drawWidth, drawHeight), sourceRegion);

				painter23.end();
			};
			stretchImageKeepRatio(memMainQImage, m_memCImageCapture.m_nPosX, m_memCImageCapture.m_nPosY,
			                      m_memCImageCapture.m_nPicW, m_memCImageCapture.m_nPicH);
		}
		auto lbw = labelSize.width();
		auto lbh = labelSize.height();
		auto wd = memMainQImage.width();
		auto ht = memMainQImage.height();
		auto xb =
			(labelSize.width() - memMainQImage.width()) / 2 + m_nImgXOffset - memMainQImage.width() * (m_dImgScale -
				1) / 2;
		auto yb =
			(labelSize.height() - memMainQImage.height()) / 2 + m_nImgYOffset - memMainQImage.height() * (m_dImgScale -
				1) / 2;
		painter.drawImage(xb, yb, memMainQImage);

		//painter.drawImage
		//	(0, 0, scaledB);
		painter.end();
	}
	//ui.label->clear();
	ui.label->setPixmap(QPixmap::fromImage(q));
	m_memDealedImg = memMainQImage;
	m_memShowedImg = q;
}

void MainForm::UpdateMotorRunStatus(const CDeviceHeartBeat& c)
{
	std::string strWalkingMotorStatus = "未知";

	switch (c.m_vectorWalkingMotorStatus[0].m_cDeviceStatus)
	{
	case 0:
	case 3:
		{
			strWalkingMotorStatus = "停止";
			break;
		}
	case 1:
		{
			strWalkingMotorStatus = "右运行";
			break;
		}
	case 2:
		{
			strWalkingMotorStatus = "左运行";
			break;
		}
	case 4:
		{
			strWalkingMotorStatus = "故障";
			break;
		}
	default:
		{
			break;
		}
	}
	ui.label_19->setText(QString::fromStdString(strWalkingMotorStatus));

	strWalkingMotorStatus = "未知";

	switch (c.m_vectorWindmillMotorStatus[0].m_cDeviceStatus)
	{
	case 0:
	case 3:
		{
			strWalkingMotorStatus = "停止";
			break;
		}
	case 1:
		{
			strWalkingMotorStatus = "上运行";
			break;
		}
	case 2:
		{
			strWalkingMotorStatus = "下运行";
			break;
		}
	case 4:
		{
			strWalkingMotorStatus = "故障";
			break;
		}
	default:
		{
			break;
		}
	}

	ui.label_20->setText(QString::fromStdString(strWalkingMotorStatus));

	strWalkingMotorStatus = "未知";

	switch (c.m_vectorSafetyMotorStatus[0].m_cDeviceStatus)
	{
	case 0:
	case 3:
		{
			strWalkingMotorStatus = "停止";
			break;
		}
	case 1:
		{
			strWalkingMotorStatus = "关闭中";
			break;
		}
	case 2:
		{
			strWalkingMotorStatus = "开启中";
			break;
		}
	case 4:
		{
			strWalkingMotorStatus = "故障";
			break;
		}
	default:
		{
			break;
		}
	}

	ui.label_22->setText(QString::fromStdString(strWalkingMotorStatus));

	strWalkingMotorStatus = "未知";

	switch (c.m_vectorSBMotorStatus[0].m_cDeviceStatus)
	{
	case 0:
	case 3:
		{
			strWalkingMotorStatus = "停止";
			break;
		}
	case 1:
		{
			strWalkingMotorStatus = "水平打开中";
			break;
		}
	case 2:
		{
			strWalkingMotorStatus = "垂直收拢中";
			break;
		}
	case 4:
		{
			strWalkingMotorStatus = "故障";
			break;
		}
	default:
		{
			break;
		}
	}

	ui.label_23->setText(QString::fromStdString(strWalkingMotorStatus));
	if (c.m_cBattery > 100)
	{
		ui.label_30->setText("未知");
	}
	else
	{
		ui.label_30->setText(QString::number(c.m_cBattery));
	}
	QString cl = c.m_cBattery <= 30 ? "color: red" : "color: white";
	ui.label_13->setStyleSheet(cl);
	ui.label_30->setStyleSheet(cl);
	if (c.m_cHardwareYear >= 25)
	{
		std::stringstream ss;
		ss << (2000 + c.m_cHardwareYear) << "." << (int)c.m_cHardwareMonth << "." << (int)c.m_cHardwareDay << "." << (c.
			m_cHardwareVersionOfDay >> 4) << (c.m_cHardwareVersionOfDay & 0x0f);

		ui.label_31->setText(QString::fromStdString(ss.str()));
	}
	else
	{
		ui.label_31->setText("-");
	}
	ui.label_54->setText(c.m_cMainPowerSupply > 0 ? "开" : "关");
	uint8_t cFmode = c.m_bFactoryMode ? 1 : 0;
	ui.label_37->setText(QString::number(cFmode));
}

void MainForm::UpdateOTAStatus()
{
	auto lb = ui.label_56;
	switch (m_cOTAStatus)
	{
	case 0:
		{
			lb->setText("");
			break;
		}
	case 1:
		{
			lb->setText("开始升级固件");
			break;
		}
	case 2:
		{
			std::stringstream ss;
			ss << "升级中：" << m_nOTAPackIndex << "/" << m_nOTAAllPackNumber;
			lb->setText(QString::fromStdString(ss.str()));
			break;
		}
	case 3:
		{
			lb->setText("升级完成");
			break;
		}
	case 255:
		{
			lb->setText("升级失败");
			break;
		}
	default:
		{
			lb->setText("");
			break;
		}
	}
}

void MainForm::ReadSavedFiles(int index)
{
	if (!m_strWorkGuid.empty())
	{
		auto path = WHSD_Tools::GetGuidPath(m_strWorkGuid, "pic");
		if (!path.empty())
		{
			m_vectorReadedPicPath.clear();
			auto files = WHSD_Tools::GetAllFilesInDirectory(path, ".sdraw");
			if (files.empty())
			{
				ui.label_35->setText("0/0");
			}
			else
			{
				for (const auto& file : files)
				{
					auto allpath = path + "\\" + file;
					m_vectorReadedPicPath.emplace_back(allpath);
				}
				if (index >= 0 && index < files.size())
				{
					m_nPicIndex = index;
				}
				else
				{
					m_nPicIndex = files.size() - 1;
				}

				std::stringstream ss;
				ss << m_nPicIndex + 1 << "/" << files.size();
				ui.label_35->setText(QString::fromStdString(ss.str()));
				std::vector<uint8_t> v;
				if (WHSD_Tools::ReadFileToVector(m_vectorReadedPicPath[m_nPicIndex], &v))
				{
					m_memSDRaw.LoadSDRaw(&v);
					Callback_ShowImgOnLabel_2();
				}
			}
		}
	}
}


void MainForm::LoadOnlinePicByThread()
{
	auto si = m_vector_NeedDownLoadPic.size();
	for (int i = 1; i < si; ++i)
	{
		const auto& downloadUrl = m_vector_NeedDownLoadPic[i];
		m_mapLoadedMap[i] = HttpClient::DownloadFileToVector(downloadUrl);
	}
}

// 软件通过 启动界面调用加载图像方法
void MainForm::DownloadPic()
{
	//return;
	auto [t1, t2, t3] = HttpClient::Get(m_pConfig->m_memTimsConfig.m_strDownloadPic,
	                                    {{"sampleId", m_strWorkGuid}}, {},
	                                    m_pConfig->m_memTimsConfig.m_nDownloadTimeOut);
	bool loadSuccess = false;
	if (t1)
	{
		auto jsonStr = QString::fromStdString(t2);
		QJsonParseError parseError;
		auto doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &parseError);
		// 检查解析是否成功
		if (parseError.error == QJsonParseError::NoError)
		{
			// 2. 提取 JSON 对象（假设根节点是对象）
			auto rootObj = doc.object();
			auto code = rootObj["code"].toInt();
			if (code == 200)
			{
				auto result = rootObj["data"].toObject();
				auto guid = result["Guid"].toString();
				if (guid.toStdString() == m_strWorkGuid)
				{
					m_nLoadedMapType = result["ImgType"].toInt();
					auto imgsArray = result["Imgs"].toArray();
					auto imgCount = result["ImgCount"].toInt();
					if (imgCount>0)
					{
						auto path = WHSD_Tools::GetGuidPath(m_strWorkGuid, "temp");
						WHSD_Tools::DeleteDirectoryContents(path);
						auto downloadUrl = imgsArray.first().toString().toStdString();
						m_mapLoadedMap[0] = HttpClient::DownloadFileToVector(downloadUrl);
						for (int i = 1; i < imgCount; ++i)
						{
							m_mapLoadedMap[i] = std::vector<uint8_t>(0);
						}
						for (const QJsonValue& imgVal : imgsArray)
						{
							m_vector_NeedDownLoadPic.emplace_back(imgVal.toString().toStdString());
						}
						std::thread td2(&MainForm::LoadOnlinePicByThread, this);
						td2.detach();
						//for (const QJsonValue& imgVal : imgsArray)
						//{
						//	// 检查数组元素是否为对象
						//	if (!imgVal.isObject())
						//	{
						//		//qDebug() << "数组元素不是有效的对象！";
						//		continue;
						//	}
						//	auto imgObj = imgVal.toObject();

						//	// 提取PicData（检查是否存在且为字符串）
						//	if (imgObj.contains("PicData") && imgObj["PicData"].isString())
						//	{
						//		auto picData = imgObj["PicData"].toString();
						//		m_mapLoadedMap[index++] = WHSD_Tools::Base64Decode(picData.toStdString());
						//	}
						//}
						loadSuccess = true;
					}
				}
			}
		}
	}

	if (loadSuccess)
	{
		this->On_OnLineLoadSuccess();
		m_bDownloadedPic = true;
	}
	else
	{
		this->On_OnLineLoadFailed();
	}
}

void MainForm::On_OnLineLoadFailed_Slots()
{
	ui.label_36->setText("加载失败！");
	//MY_WARNING("加载失败！");

	QPixmap pixmap;

	if (pixmap.load(QString::fromStdString(WHSD_Tools::GetExeDirectory() + "\\" + "jjsb.png")))
	{
		ui.label->setPixmap(pixmap.scaled(ui.label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
}

void MainForm::On_SaveRaw_Clicked()
{
	auto tp = m_memSDRaw.GetOriginalRawData();
	if (tp.empty())
	{
		MY_WARNING("无图片！");
		return;
	}
	QFileDialog dialog(this);

	// 设置对话框标题
	dialog.setWindowTitle("保存文件");

	// 设置默认保存格式为xxx
	// 设置文件过滤器，只显示.xxx文件和所有文件
	dialog.setNameFilter("raw Files (*.raw);");

	// 设置默认后缀，如果用户没有输入扩展名，会自动添加.xxx
	dialog.setDefaultSuffix("raw");
	if (dialog.exec() == QFileDialog::Accepted)
	{
		std::string apth = dialog.selectedFiles().first().toLocal8Bit().constData();
		if (WHSD_Tools::SaveDataToFile2_Raw(tp.data(), tp.size(), apth))
		{
			MY_INFO("保存成功");
			return;
		}
	}
	MY_WARNING("保存失败！");
}

void MainForm::On_SavePNG_Clicked()
{
	auto isImageValid = [](const QImage& image)
	{
		// 检查图像是否为空
		if (image.isNull())
		{
			return false;
		}

		// 检查图像尺寸是否有效
		if (image.width() <= 0 || image.height() <= 0)
		{
			return false;
		}

		// 检查图像格式是否有效
		if (image.format() == QImage::Format_Invalid)
		{
			return false;
		}

		return true;
	};
	if (!isImageValid(m_memDealedImg))
	{
		MY_WARNING("无图片！");
		return;
	}
	QFileDialog dialog(this);

	// 设置对话框标题
	dialog.setWindowTitle("保存文件");

	// 设置默认保存格式为xxx
	// 设置文件过滤器，只显示.xxx文件和所有文件
	dialog.setNameFilter("png Files (*.png);");

	// 设置默认后缀，如果用户没有输入扩展名，会自动添加.xxx
	dialog.setDefaultSuffix("png");
	if (dialog.exec() == QFileDialog::Accepted)
	{
		if (m_memDealedImg.save(dialog.selectedFiles().first()))
		{
			MY_INFO("保存成功");
			return;
		}
	}

	MY_WARNING("保存失败！");
}

void MainForm::On_SetFMode()
{
	auto cmds = CWHSDControlBoardProtocol::SetFactoryMode(true);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_NoSetFMode()
{
	auto cmds = CWHSDControlBoardProtocol::SetFactoryMode(false);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_DenoiseAndEnhance0()
{
	m_memImageProcessParam1.m_nDenoise = 0;
	m_memImageProcessParam1.m_nEnhance = 0;
	On_SliderValueChanged();
}

void MainForm::On_Denoise1()
{
	m_memImageProcessParam1.m_nDenoise = 3;
	On_SliderValueChanged();
}

void MainForm::On_Denoise2()
{
	m_memImageProcessParam1.m_nDenoise = 7;
	On_SliderValueChanged();
}

void MainForm::On_Denoise3()
{
	m_memImageProcessParam1.m_nDenoise = 10;
	On_SliderValueChanged();
}

void MainForm::On_Enhance1()
{
	m_memImageProcessParam1.m_nEnhance = 2;
	On_SliderValueChanged();
}

void MainForm::On_Enhance2()
{
	m_memImageProcessParam1.m_nEnhance = 4;
	On_SliderValueChanged();
}

void MainForm::On_Enhance3()
{
	m_memImageProcessParam1.m_nEnhance = 6;
	On_SliderValueChanged();
}

void MainForm::On_DrawLine_Click()
{
	m_nMouseMode = 1;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_Rect_Click()
{
	m_nMouseMode = 2;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_Ellipse_Click()
{
	m_nMouseMode = 3;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_MoveLast_Click()
{
	if (!m_vector_ImgTag.empty())
	{
		const auto& p = m_vector_ImgTag.back();
		m_vector_ImgTag.pop_back();
		m_vector_ImgTag_bak.emplace_back(p);
		PaintImg();
	}
}

void MainForm::On_InputText_Click()
{
	m_nMouseMode = 5;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_Curvature_Click()
{
	m_nMouseMode = 6;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_Angle_Click()
{
	m_nMouseMode = 4;
	ClearPicOpt();
	PaintImg();
}

void MainForm::On_Capture_Click()
{
	m_nMouseMode = 7;
	ClearPicOpt();
	PaintImg();
}

void MainForm::ClearPicOpt()
{
	//m_bUpDownMirror = false;
	//m_bLeftRightMirror = false;
	m_nMouseClickCount = 0;
	m_dImgScale = 1;
	//m_nRotate = 0;
	m_nImgXOffset = 0;
	m_nImgYOffset = 0;
	m_memCImageCapture.Clear();
}

void MainForm::AddOneImgTag(const CImageTag& t)
{
	m_vector_ImgTag.push_back(t);
	m_vector_ImgTag_bak.clear();
}

std::string MainForm::ChangeUIImg(const std::string& s, const bool ac)
{
	std::string s2 = s;
	if (ac)
	{
		s2.append("_2");
	}
	else
	{
		s2.append("_1");
	}
	std::string result = "QPushButton{background-image:url(:/Image/icon/" + s2 +
		".png);background-repeat:no-repeat;background-position:center;border:none;min-width:70px;max-width:70px;min-height:70px;max-height:70px;}QPushButton:hover{background-image:url(:/Image/icon/"
		+ s + "_2.png);}";
	return result;
}

void MainForm::ReadPicFromMem(int index)
{
	bool needReadOrgPic = true;
	if (!m_mapDealedPic.empty())
	{
		auto tp = m_mapDealedPic.find(index);
		if (tp != m_mapDealedPic.end())
		{
			if (!tp->second.empty())
			{
				auto pngVectorToImage = [](const std::vector<uint8_t>& pngData) -> QImage
				{
					// 1. 将 vector 转换为 QByteArray（Qt 的字节容器）
					QImage outImage;
					QByteArray dataArray(
						reinterpret_cast<const char*>(pngData.data()), // 转换为 const char*
						pngData.size() // 数据长度
					);

					// 2. 从字节数组加载 PNG 图像
					bool loaded = outImage.loadFromData(
						reinterpret_cast<const uchar*>(dataArray.constData()), // 转换为 uchar*（QImage 要求）
						dataArray.size() // 数据长度
					);

					if (!loaded)
					{
						//qDebug() << "加载 PNG 失败！数据可能不完整或格式错误。";
						return QImage();
					}

					//qDebug() << "PNG 数据成功转换为 QImage，尺寸：" << outImage.size();
					return outImage;
				};
				m_memMainQImage = pngVectorToImage(tp->second);
				PaintImg();
				ui.label_36->setStyleSheet("color:lightgreen");
				m_nPicIndex = index;
				needReadOrgPic = false;
			}
		}
	}
	if (!m_mapLoadedMap.empty())
	{
		//读取原始图片
		auto tp = m_mapLoadedMap.find(index);
		if (tp == m_mapLoadedMap.end())
		{
			//输入错误，直接显示第一张
			tp = m_mapLoadedMap.begin();
			index = 0;
		}
		auto dat = tp->second;
		if (needReadOrgPic)
		{
			bool readSuccess = false;
			if (m_nLoadedMapType == 3)
			{
				m_memSDRaw.LoadSDRaw(&dat);
				readSuccess = true;
			}
			if (m_nLoadedMapType == 0)
			{
				//raw文件的读取方式
				auto si = dat.size();
				for (const auto& value : m_pConfig->m_memCImageProcessConfig.m_vec_RawFileSize)
				{
					if (value.m_nFileSize == si)
					{
						m_memSDRaw.m_wPicHeight = value.m_nRawFileHeight;
						m_memSDRaw.m_wPicWidth = value.m_nRawFileWidth;
						m_memSDRaw.SetOriginalRawData(&dat);
						m_memSDRaw.SetTempRawData(&dat);
						readSuccess = true;
						break;
					}
				}
			}
			if (readSuccess)
			{
				Callback_ShowImgOnLabel_2();
			}
			else
			{
				MY_WARNING("文件读取失败！");
			}
			ui.label_36->setStyleSheet("color:white");
		}

		m_nPicIndex = index;
		auto si = m_mapLoadedMap.size();
		if (si <= 0)
		{
			ui.label_36->setText("0/0");
		}
		else
		{
			std::stringstream ss;
			ss << m_nPicIndex + 1 << "/" << si;
			ui.label_36->setText(QString::fromStdString(ss.str()));
		}
	}
}

void MainForm::Callback_SampleBoardNewImg(uint8_t* data, int width, int height, int pixLen)
{
	m_nPicCount++;
	std::vector<uint8_t> v;
	std::string fileType;
	auto dataLen = width * height * pixLen;
	{
		std::lock_guard<std::mutex> g(m_mutexLastImgMutex);
		m_memSDRaw.SetOriginalRawData(data, dataLen);
		m_memSDRaw.SetTempRawData(data, dataLen);
		m_memSDRaw.m_wPicWidth = width;
		m_memSDRaw.m_wPicHeight = height;
		if (m_pConfig->m_memCImageProcessConfig.m_nRawFile == 0)
		{
			v = m_memSDRaw.GetSaveOriginalData();
			fileType = ".sdraw";
		}
		else
		{
			v = m_memSDRaw.GetOriginalRawData();
			fileType = ".raw";
		}
	}
	WHSD_Tools::SaveDataToFile_Raw(v.data(), v.size(), fileType);
	//WHSD_Tools::SaveDataToFile(data, dataLen, ".raw");
	m_nShowImgTips = 0;
	//this->On_Pic_Receive();
	//ShowImgOnLabel();
}

void MainForm::Callback_SampleBoardDeviceConnectStatus(const DeviceConnectStatus s)
{
	m_mutexDeviceInfoLock.lock();
	m_enumDeviceConnectStatus = s;
	m_mutexDeviceInfoLock.unlock();
}

void MainForm::Callback_DeviceHeartBeat(const CDeviceHeartBeat& b, int nComdeviceIndex)
{
	m_mutexDeviceInfoLock.lock();
	m_amemDeviceHeartBeat[nComdeviceIndex] = b;
	m_atime_LastHeartBeatTime[nComdeviceIndex].GetCurTime();
	m_anHeartBeatCount[nComdeviceIndex]++;
	m_mutexDeviceInfoLock.unlock();
}

void MainForm::Callback_SampleBoardDeviceRunStatus(const DeviceRunStatus s)
{
	m_mutexDeviceInfoLock.lock();
	m_enumDeviceRunStatus = s;
	m_mutexDeviceInfoLock.unlock();
}

void MainForm::On_SafeChanged_On()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x03, 0b11, 0x02, 0x00);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_SafeChanged_Stop()
{
	if (m_pConfig->m_memControlBoardConfig.m_bFactoryMode)
	{
		auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x03, 0b11, 0x00, 0x00);
		m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
	}
}

void MainForm::On_SafeChanged_Off()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x03, 0b11, 0x01, 0x00);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_ManuallyTrigger_Clicked()
{
	if (m_pSampleBoardBase != nullptr)
	{
		m_pSampleBoardBase->ManuallyTrigger();
	}
}

void MainForm::On_LoadPic_Clicked()
{
	QString filter = "原始Raw和四达Sdraw (*.raw *.sdraw)";
	QSettings settings("河南四达", "Star_HMI"); // 公司名和应用名
	QString lastPath = settings.value("LastFileDialogPath", QDir::currentPath()).toString();
	QString pa = QDir::homePath();
	if (!lastPath.isEmpty())
	{
		pa = lastPath;
	}
	// 打开文件选择对话框
	QString qFilePath = QFileDialog::getOpenFileName(
		this, // 父窗口指针
		tr("选择文件"), // 对话框标题
		pa, // 初始目录（这里使用用户主目录）
		filter // 文件过滤器
	);

	// 判断用户是否选择了文件
	if (qFilePath.isEmpty())
	{
		return;
	}
	QFileInfo fileInfo(qFilePath);
	QString currentPath = fileInfo.path();
	settings.setValue("LastFileDialogPath", currentPath);
	// 获取文件后缀并判断类型
	QString fileExt = QFileInfo(qFilePath).suffix().toLower();

	bool readSuccess = false;
	if (fileExt == "raw")
	{
		if (!qFilePath.isEmpty())
		{
			std::vector<uint8_t> buf;
			std::string utf8_str = qFilePath.toLocal8Bit().constData();
			if (WHSD_Tools::ReadFileToVector(utf8_str, &buf))
			{
				auto si = buf.size();
				for (const auto& value : m_pConfig->m_memCImageProcessConfig.m_vec_RawFileSize)
				{
					if (value.m_nFileSize == si)
					{
						m_memSDRaw.m_wPicHeight = value.m_nRawFileHeight;
						m_memSDRaw.m_wPicWidth = value.m_nRawFileWidth;
						m_memSDRaw.SetOriginalRawData(&buf);
						m_memSDRaw.SetTempRawData(&buf);
						this->On_Pic_Receive();
						readSuccess = true;
						break;
					}
				}
			}
		}
		if (!readSuccess)
		{
			QMessageBox::critical(this, "错误", "raw读取失败！", QMessageBox::Ok);
		}
	}
	else if (fileExt == "sdraw")
	{
		if (!qFilePath.isEmpty())
		{
			std::vector<uint8_t> buf;
			std::string utf8_str = qFilePath.toLocal8Bit().constData();
			if (WHSD_Tools::ReadFileToVector(utf8_str, &buf))
			{
				readSuccess = m_memSDRaw.LoadSDRaw(&buf);
				if (readSuccess)
				{
					this->On_Pic_Receive();
				}
				readSuccess = true;
			}
		}
		if (!readSuccess)
		{
			QMessageBox::critical(this, "错误", "sdraw读取失败！", QMessageBox::Ok);
		}
	}
	else
	{
		MY_WARNING("程序错误！");
	}
}

void MainForm::On_ControlOnly_Pressed()
{
	m_bControlPressed = true;
	std::cout<< "ControlOnly_Pressed" << std::endl;
}

void MainForm::On_ControlOnly_Released()
{
    m_bControlPressed = false;
	std::cout<< "ControlOnly_Released" << std::endl;
}

void MainForm::On_SampleBoard_Change_Clicked1()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x04, 0b11, 0x02, 0x00);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_SampleBoard_Change_Clicked2()
{
	auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x04, 0b11, 0x01, 0x00);
	m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
}

void MainForm::On_SampleBoard_Change_Stop()
{
	if (m_pConfig->m_memControlBoardConfig.m_bFactoryMode)
	{
		auto cmds = CWHSDControlBoardProtocol::DeviceRun(0x04, 0b11, 0x00, 0x00);
		m_apDeviceCom[0]->Write(cmds.data(), cmds.size());
	}
}

void MainForm::On_NumberOfPulses_clicked()
{
	bool success = false;
	auto nb = ui.lineEdit->text().toInt(&success);
	if (success)
	{
		auto cmds = CWHSDControlBoardProtocol::SendNumberOfPulses(nb);
		auto p = m_pConfig->m_memControlBoardConfig.m_bEnableIp2 ? m_apDeviceCom[1] : m_apDeviceCom[0];
		if (p != nullptr)
		{
			p->Write(cmds.data(), cmds.size());
		}
	}
}

void MainForm::On_DelayTime_clicked()
{
	auto lineEdit = ui.lineEdit_2;
	QString text = lineEdit->text();
	if (text.isEmpty())
	{
		// 可以选择给用户提示
		MY_INFO("请输入大于15的整数");
		return;
	}

	bool ok;
	int value = text.toInt(&ok);
	if (ok && value >= 15)
	{
		auto cmds = CWHSDControlBoardProtocol::SendDelayTime(value);
		auto p = m_pConfig->m_memControlBoardConfig.m_bEnableIp2 ? m_apDeviceCom[1] : m_apDeviceCom[0];
		if (p != nullptr)
		{
			p->Write(cmds.data(), cmds.size());
		}
	}
	else
	{
		MY_WARNING("请输入大于15的整数");
		lineEdit->clear();
	}
}

void MainForm::On_XRayStart_clicked()
{
	auto dp = ui.comboBox_4->currentIndex();
	auto gdp = ui.comboBox_5->currentIndex();
	auto djs = ui.comboBox_2->currentIndex();
	auto cmds = CWHSDControlBoardProtocol::StartXRay(dp, gdp, djs);

	auto p = m_pConfig->m_memControlBoardConfig.m_bEnableIp2 ? m_apDeviceCom[1] : m_apDeviceCom[0];
	if (p != nullptr)
	{
		p->Write(cmds.data(), cmds.size());
	}
}

void MainForm::On_XRayStop_clicked()
{
	auto cmds = CWHSDControlBoardProtocol::StopXRay();
	auto p = m_pConfig->m_memControlBoardConfig.m_bEnableIp2 ? m_apDeviceCom[1] : m_apDeviceCom[0];
	if (p != nullptr)
	{
		p->Write(cmds.data(), cmds.size());
	}
}
