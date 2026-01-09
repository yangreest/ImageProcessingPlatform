#include "ControlBoardConfigForm.h"

#include <QMessageBox>

#include "../Protocol/WHSDControlBoradProtocol.h"

ControlBoardConfigForm::ControlBoardConfigForm(IDeviceCom* p, QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	setWindowTitle("控制板配置");

	QIcon appIcon("./1.png");
	if (!appIcon.isNull())
	{
		setWindowIcon(appIcon);
	}
	m_pControlBoardCom = p;
	connect(ui.pushButton, &QPushButton::clicked, this, &ControlBoardConfigForm::On_SendCmd_Click);
}

ControlBoardConfigForm::~ControlBoardConfigForm()
{
}

void ControlBoardConfigForm::On_SendCmd_Click()
{
	//这里是写进去了
	CControlBoardProtocolConfig config;
	config.m_wLidarMinDis = ui.lineEdit_2->text().toUShort();
	config.m_wLidarMaxDis = ui.lineEdit_3->text().toUShort();
	config.m_fSafeAngle = ui.lineEdit->text().toFloat();
	config.m_fSBAngle = ui.lineEdit_4->text().toFloat();
	config.m_fWalkingV = ui.lineEdit_5->text().toFloat();
	config.m_fSafeV = ui.lineEdit_6->text().toFloat();
	config.m_fSBV = ui.lineEdit_7->text().toFloat();
	config.m_fJYV = ui.lineEdit_8->text().toFloat();
	config.m_fJY2V = ui.lineEdit_9->text().toFloat();
	config.m_nSBRun = ui.lineEdit_10->text().toInt();
	auto tp = CWHSDControlBoardProtocol::SetControlBoardConfig(&config);
	m_pControlBoardCom->Write(tp.data(), tp.size());
	QMessageBox::critical(this, "提示", "指令已发送！", QMessageBox::Ok);
}