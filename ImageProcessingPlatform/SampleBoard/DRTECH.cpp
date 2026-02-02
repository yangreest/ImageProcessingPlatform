#include "DRTECH.h"

#include <thread>
#include <windows.h>

#ifdef __DEBUG__
#include <iostream>
#include <sstream>
#endif

#include "DRTECH_DLLs.h"

enum MapType
{
	OLD_MAP = 0,
	AMAP = 1,
	DMAP = 2,
};

CDRTECH::CDRTECH()
{
	m_nInitStatus = 0;
	m_enumDeviceConnectStatus = DeviceConnectStatus::UnKnown;
	m_function_DeviceValueCallBack = nullptr;
	m_function_ImgCallBack = nullptr;
	m_function_DeviceConnectStatusCallBack = nullptr;
	m_function_DeviceRunStatusCallBack = nullptr;
	m_bSoftwareTrigger = false;
}

bool CDRTECH::Init(const CSampleBoardConfig& pConfig)
{
	m_nModelType = pConfig.m_nModel;
	m_nMapType = pConfig.m_nMapType;
	m_nExposureTime = pConfig.m_nExposureTime;
	m_bSoftwareTrigger = pConfig.m_nExposureType > 0;
	switch (m_nModelType)
	{
	case 504:
	{
		m_strMapPath = "EXT_2143S_MAP";
		break;
	}
	case 4007:
	{
		m_strMapPath = "EXT_2143U_MAP";
		break;
	}
	default:
	{
		m_strMapPath = "";
		break;
	}
	}
	return true;
}

bool CDRTECH::BeginWork()
{
	SelectPanel(0);
	std::string szTempMapFile;

	auto nRet = InitializeDetector(m_nModelType);
	if (nRet == 1)
	{
		SetCallback(CallbackMessageProcess, this);
	}
	else
	{
		return false;
	}
	// Load Panel Map
	std::string strStartPah("./");
	if (m_nMapType != DMAP) // DMAP doesn't load Panel Map.
	{
		szTempMapFile = strStartPah + m_strMapPath + "/DetectorA.PMP";
		if (LoadPanelMap((char*)szTempMapFile.c_str()) != 1)
		{
			return false;
		}
	}

	szTempMapFile = strStartPah + m_strMapPath + "/DetectorA.MAP";
	if (LoadPixelMap((char*)szTempMapFile.c_str()) != 1)
	{
		return false;
	}
	szTempMapFile = strStartPah + m_strMapPath + "/DetectorA.GMP";

	if (LoadGainMap((char*)szTempMapFile.c_str()) != 1)
	{
		return false;
	}
	szTempMapFile = strStartPah + "DetectorA.FIL";

	// Load Grid Filter
	if (LoadFilter((char*)szTempMapFile.c_str()) != 1)
	{
		return false;
	}
	SelectPanel(0);
	GetImageResolutionByPanelIndex(0, m_nImgWidth, m_nImgHeight); //Get Image Resolution
	m_bIsNeedExit = false;
	std::thread td(&CDRTECH::DeviceConnectThread, this);
	td.detach();
	return true;
}

bool CDRTECH::EndWork()
{
	m_bIsNeedExit = true;
	return false;
}

void CDRTECH::RegisterImgCallback(const std::function<void(uint8_t*, int, int, int)>& f)
{
	m_function_ImgCallBack = f;
}

void CDRTECH::RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus)>& f)
{
	m_function_DeviceConnectStatusCallBack = f;
}

void CDRTECH::RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus)>& f)
{
	m_function_DeviceRunStatusCallBack = f;
}

void CDRTECH::RegisterDeviceValueCallback(const std::function<void(DeviceValue, double)>& f)
{
	m_function_DeviceValueCallBack = f;
}

bool CDRTECH::ManuallyTrigger()
{
	int nRet = DRTECH_StartSoftwareTrigger(1);
	return 1 == nRet;
}

void CDRTECH::DeviceConnectThread()
{
	while (!m_bIsNeedExit)
	{
		if (m_enumDeviceConnectStatus == DeviceConnectStatus::Connected)
		{
			if (m_nInitStatus == 0)
			{
				//连接上之后，直接设置AED模式

				auto nExpType = m_bSoftwareTrigger ? EXP_TYPE_APP_SOFTWARE_TRIGGER : EXP_TYPE_APP_AED;
				int nRet = SetExposureType(nExpType, 0);
				if (nRet == nExpType)
				{
					m_nInitStatus = 1;
				}
			}
			if (m_nInitStatus == 1)
			{
				if (StartAcquisition() == 1)
				{
					m_nInitStatus = 2;
				}
			}
			if (m_nInitStatus == 2)
			{
				if (m_bSoftwareTrigger)
				{
					if (SetWindowOpenTime(m_nExposureTime) == 1)
					{
						m_nInitStatus = 3;
					}
				}
				else
				{
					m_nInitStatus = 3;
				}
			}
		}
		Sleep(800);
	}
}

void CDRTECH::CallbackMessageProcess(int _nMsgType, char* _pMsg, void* _pData)
{
	((CDRTECH*)_pData)->CallbackMessageProcess_Inner(_nMsgType, _pMsg);
}

void CDRTECH::CallbackMessageProcess_Inner(int _nMsgType, char* _pMsg)
{
	int size = m_nImgWidth * m_nImgHeight * 2; //get image capacity
	switch (_nMsgType)
	{
	case IDC_READY_STATUS:
	{
		if (_pMsg != NULL)
		{
			// Get status from _pMsg parameter.
			int ReadyState = *(int*)_pMsg; // convert to int.

#ifdef __DEBUG__
			std::cout << "IDC_READY_STATUS:" << ReadyState << std::endl;
#endif
			switch (ReadyState)
			{
			case READY_STATUS_READY:
			{
				if (m_function_DeviceRunStatusCallBack != nullptr)
				{
					m_function_DeviceRunStatusCallBack(DeviceRunStatus::Ready);
				}
				break;
			}
			case READY_STATUS_AWAKE:
			{
				m_function_DeviceRunStatusCallBack(DeviceRunStatus::Awake);
				break;
			}
			case READY_STATUS_BUSY:
			default:
			{
				m_function_DeviceRunStatusCallBack(DeviceRunStatus::Busy);
				break;
			}
			}
		}
		break;
	}
	case IDC_DEVICE_READY:
	{
#ifdef __DEBUG__
		std::cout << "IDC_DEVICE_READY" << std::endl;
#endif
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::Connected);
		m_enumDeviceConnectStatus = DeviceConnectStatus::Connected;
		break;
	}
	case IDC_DISCONNECTED:
	{
		// Detector is disconnected
		break;
	}
	case IDC_SIGNAL_QUALITY:
	{
		// Wi-Fi sense when the detector runs wireless mode
		if (_pMsg != NULL)
		{
			int nSignalQuality = *((int*)_pMsg);
			if (m_function_DeviceValueCallBack != nullptr)
			{
				m_function_DeviceValueCallBack(DeviceValue::Wifi, nSignalQuality);
			}
		}
		break;
	}

	case IDC_BATTERY_INFO:
	{
		if (_pMsg != NULL)
		{
			CB_IDC_BATTERY_INFO* pBattery = (CB_IDC_BATTERY_INFO*)_pMsg; // convert to int.
			int nBatteryRemainRate = pBattery->nBatteryValue;
			if (nBatteryRemainRate < 0)
			{
				m_function_DeviceValueCallBack(DeviceValue::Battery, -1);
			}
			else if (nBatteryRemainRate >= 0 && nBatteryRemainRate <= 100)
			{
				m_function_DeviceValueCallBack(DeviceValue::Battery, nBatteryRemainRate);
			}
			else if (nBatteryRemainRate == 500)
			{
				m_function_DeviceValueCallBack(DeviceValue::Battery, 100);
			}
			else if (nBatteryRemainRate >= 1000 && nBatteryRemainRate <= 1100)
			{
				m_function_DeviceValueCallBack(DeviceValue::Battery, nBatteryRemainRate - 1000);
			}
			else if (nBatteryRemainRate >= 2000 && nBatteryRemainRate <= 2100)
			{
				m_function_DeviceValueCallBack(DeviceValue::Battery, nBatteryRemainRate - 2000);
			}
			else
			{
				// Error; Unknown Battery Status
			}
			m_function_DeviceValueCallBack(DeviceValue::Temperature, pBattery->nBatteryTemperature);
		}
		break;
	}

	case IDC_XCF_RECEIVED:
	{
		auto pInfo = (CallbackDataImageNew*)_pMsg;
		if (pInfo != nullptr && m_function_ImgCallBack != nullptr)
		{
			m_function_ImgCallBack((uint8_t*)GetImageBuffer(), pInfo->ImageWidth, pInfo->ImageHeight,
				sizeof(unsigned short));
		}
		break;
	}
	}
}