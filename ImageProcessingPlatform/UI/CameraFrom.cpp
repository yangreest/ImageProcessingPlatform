#include "CameraFrom.h"

#include <thread>

#include "Tools/Tools.h"

CameraFrom::CameraFrom(const CConfigManager* nConfig, QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);

	setWindowTitle("摄像头预览");

	QIcon appIcon("./1.png");
	if (!appIcon.isNull())
	{
		setWindowIcon(appIcon);
	}
	m_strLeftIp = nConfig->m_memCCameraConfig.m_strLeftIp;
	m_strMidIp = nConfig->m_memCCameraConfig.m_strMidIp;
	m_strRightIp = nConfig->m_memCCameraConfig.m_strRightIp;

	m_pC1 = ICameraBase::GetCameraObj(0);
	m_pC2 = ICameraBase::GetCameraObj(0);
	m_pC3 = ICameraBase::GetCameraObj(0);
	auto hwnd = reinterpret_cast<HWND>(ui.label->winId());
	auto hwnd2 = reinterpret_cast<HWND>(ui.label_2->winId());
	auto hwnd3 = reinterpret_cast<HWND>(ui.label_3->winId());

	m_pC1->RegisterVideoViewHandle(hwnd);
	m_pC2->RegisterVideoViewHandle(hwnd2);
	m_pC3->RegisterVideoViewHandle(hwnd3);
	std::thread td(&CameraFrom::Connect, this);
	td.detach();
}

CameraFrom::~CameraFrom()
{
	WHSD_Tools::SafeRelease(m_pC1);
	WHSD_Tools::SafeRelease(m_pC2);
	WHSD_Tools::SafeRelease(m_pC3);
}

void CameraFrom::Connect()
{
	int initStatus = 0;
	bool needBreak = false;
	while (true)
	{
		if (needBreak)
		{
			break;
		}
		switch (initStatus)
		{
		case 0:
		{
			if (m_pC1->Init({}))
			{
				initStatus = 1;
			}
			break;
		}
		case 1:
		{
			if (m_pC1->Connect(m_strLeftIp, 0, "", ""))
			{
				initStatus = 2;
			}
			break;
		}
		case 2:
		{
			if (m_pC2->Connect(m_strMidIp, 0, "", ""))
			{
				initStatus = 3;
			}
			break;
		}
		case 3:
		{
			if (m_pC3->Connect(m_strRightIp, 0, "", ""))
			{
				initStatus = 4;
			}
			break;
		}
		default:
		{
			break;
		}
		}
	}
}