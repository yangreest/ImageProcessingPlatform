#include <windows.h>
#include "PZMedical.h"
#include "NetCom.h"

#ifdef __DEBUG__
#include <iostream>
#include <sstream>
#endif

#include <cstring>
#include <vector>
#include <thread>

#define FPDTOTALNUM 3   //total number of FPD

std::function<void(uint8_t*, int, int, int)> CPZMedical::m_function_ImgCallBack = nullptr;

std::function<void(DeviceConnectStatus)> CPZMedical::m_function_DeviceConnectStatusCallBack = nullptr;

std::function<void(DeviceRunStatus)> CPZMedical::m_function_DeviceRunStatusCallBack = nullptr;

std::function<void(DeviceValue, double)> CPZMedical::m_function_DeviceValueCallBack = nullptr;

std::vector<uint8_t> CPZMedical::m_vector_ImgBuffer = {};

bool CPZMedical::m_bIsInited = false;
bool CPZMedical::m_bLinked = false;
bool CPZMedical::m_bIsNeedExit = false;
bool CPZMedical::m_bSoftwareTrigger = false;
int8_t CPZMedical::m_cFpCurStat = 0;
int CPZMedical::m_nInitStatus = 0;

CPZMedical::CPZMedical()
{
	m_bIsNeedExit = false;
}

bool CPZMedical::Init(const CSampleBoardConfig& pConfig)
{
	m_bSoftwareTrigger = pConfig.m_nExposureType > 0;
	if (m_bIsInited)
	{
		return false;
	}
	COM_RegisterEvCallBack(EVENT_LINKUP, FuncLinkCallBack);
	COM_RegisterEvCallBack(EVENT_LINKDOWN, FuncBreakCallBack);
	COM_RegisterEvCallBack(EVENT_IMAGEVALID, FuncImageCallBack);
	COM_RegisterEvCallBack(EVENT_HEARTBEAT, FuncHeartBeatCallBack);
	COM_RegisterEvCallBack(EVENT_READY, FuncReadyCallBack);
	COM_RegisterEvCallBack(EVENT_TrigErr, FuncErrorCallBack);

	COM_RegisterEvCallBack(EVENT_READY, FuncReadyCallBack);
	COM_RegisterEvCallBack(EVENT_EXPOSE, FuncExposeCallBack);
	if (COM_Init())
	{
		if (COM_SetCalibMode(6))
		{

			m_bIsInited =  true;
		}
	}
	
	return m_bIsInited;
}

bool CPZMedical::BeginWork()
{
	if (m_function_DeviceConnectStatusCallBack != nullptr)
	{
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::UnKnown);
	}
	if (COM_SetCalibMode(IMG_CALIB_GAIN | IMG_CALIB_DEFECT))
	{
		m_nInitStatus = 0;
		std::thread td(LoopAutoConnect);
		td.detach();
		return true;
	}
	return false;
}

void CPZMedical::LoopAutoConnect()
{
	while (!m_bIsNeedExit)
	{
		Sleep(500);
		switch (m_nInitStatus)
		{
		case 0:
		{
			if (COM_Open(NULL))
			{
				m_nInitStatus = 1;
			}
			break;
		}
		case 1:
		{
			if (COM_Stop())
			{
				m_nInitStatus = 2;
			}
			break;
		}
		case 2:
		{
			if (m_bSoftwareTrigger ? COM_HstAcq() : COM_AedAcq())
			{
				m_nInitStatus = 3;
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

bool CPZMedical::EndWork()
{
	return COM_Close();
}

void CPZMedical::RegisterImgCallback(
	const std::function<void(uint8_t*, int, int, int)>& f)
{
	m_function_ImgCallBack = f;
}

void CPZMedical::RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus)>& f)
{
	m_function_DeviceConnectStatusCallBack = f;
}

void CPZMedical::RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus)>& f)
{
	m_function_DeviceRunStatusCallBack = f;
}

void CPZMedical::RegisterDeviceValueCallback(const std::function<void(DeviceValue, double)>& f)
{
	m_function_DeviceValueCallBack = f;
}

bool CPZMedical::ManuallyTrigger()
{
	return COM_Trigger();
}

BOOL CPZMedical::FuncExposeCallBack(char nEvent)
{
	return 0;
}

int CPZMedical::FuncLinkCallBack(char nEvent)
{
	m_bLinked = true;
#ifdef __DEBUG__
	std::cout << "PZ FuncLinkCallBack" << std::endl;
#endif
	CHAR FPSnOpened[32] = { 0 };
	COM_GetFPsn(FPSnOpened);

	TComFpList tComFpList = { 0 };
	COM_List(&tComFpList);

	for (CHAR i = 0; i < FPDTOTALNUM; i++)
	{
		if (0 == strncmp(FPSnOpened, tComFpList.tFpNode[i].FPPsn, 32))
		{
			if (m_function_DeviceConnectStatusCallBack != nullptr)
			{
				m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::OpendListen);
			}
		}
		else
		{
			if (m_function_DeviceConnectStatusCallBack != nullptr)
			{
				m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::Connected);
			}
		}
	}
	return 0;
}

int CPZMedical::FuncBreakCallBack(char nEvent)
{
	m_bLinked = false;
	m_nInitStatus = 0;
#ifdef __DEBUG__
	std::cout << "PZ FuncBreakCallBack" << std::endl;
#endif

	CHAR acSn[32] = { '\0' };
	COM_GetFPsn(acSn);
	CHAR cFpCurStat = COM_GetFPCurStatus();
	TFPStat tFPStat = { 0 };
	COM_GetFPStatus(&tFPStat);

	if (m_function_DeviceConnectStatusCallBack != nullptr)
	{
		m_function_DeviceConnectStatusCallBack(DeviceConnectStatus::Closed);
	}
	return 0;
}

int CPZMedical::FuncImageCallBack(char nEvent)
{
#ifdef __DEBUG__
	std::cout << "PZ FuncImageCallBack" << std::endl;
#endif
	TImageMode tImageMode;
	COM_GetImageMode(&tImageMode);
	auto u16ImgRow = tImageMode.usRow;
	auto u16ImgCol = tImageMode.usCol;
	if (m_vector_ImgBuffer.size() != u16ImgRow * u16ImgCol * sizeof(UINT16))
	{
		m_vector_ImgBuffer.resize(u16ImgRow * u16ImgCol * sizeof(UINT16));
	}
	COM_GetImage((CHAR*)m_vector_ImgBuffer.data());

	if (m_function_ImgCallBack != nullptr)
	{
		m_function_ImgCallBack(m_vector_ImgBuffer.data(), u16ImgCol, u16ImgRow, sizeof(UINT16));
	}
	return 0;
}

int CPZMedical::FuncHeartBeatCallBack(char nEvent)
{
#ifdef __DEBUG__
	//COM_HstAcq();
	std::stringstream ss;
	std::cout << "PZ FuncHeartBeatCallBack" << std::endl;
	//心跳，暂时还不知道怎么处理
#endif
	m_cFpCurStat = COM_GetFPCurStatus();
	TFPStat tFPStat = { 0 };
	CHAR acSn[32] = { '\0' };
	COM_GetFPsn(acSn);
	COM_GetFPStatus(&tFPStat);

	std::string auctmp = "Sn:";
	auctmp += acSn;

	if (m_function_DeviceRunStatusCallBack != nullptr)
	{
		switch (m_cFpCurStat)
		{
		case STATUS_IDLE:
		{
			m_function_DeviceRunStatusCallBack(DeviceRunStatus::Awake);
			if (!m_bSoftwareTrigger)
			{
				m_nInitStatus = 2;
			}
			break;
		}
		case STATUS_HST:
		case STATUS_AED1:
		case STATUS_AED2:
		{
			m_function_DeviceRunStatusCallBack(DeviceRunStatus::Ready);
			break;
		}
		default:
		{
			m_function_DeviceRunStatusCallBack(DeviceRunStatus::Unknow);
			break;
		}
		}
	}
	if (m_function_DeviceValueCallBack != nullptr)
	{
		if (0 != (tFPStat.tBatInfo1.full + tFPStat.tBatInfo2.full))
		{
			double value = 100 * (tFPStat.tBatInfo1.Remain + tFPStat.tBatInfo2.Remain) / (tFPStat.tBatInfo1.full +
				tFPStat.tBatInfo2.full);
			m_function_DeviceValueCallBack(DeviceValue::Battery, value);
		}
		//double humValue = double(tFPStat.tFpTempHum.Hum) / 10;
		m_function_DeviceValueCallBack(DeviceValue::Temperature, tFPStat.tFpTempHum.Temp / 10);
		m_function_DeviceValueCallBack(DeviceValue::Wifi, tFPStat.tWifiStatus.ucSignal_level);
	}

	return 0;
}

int CPZMedical::FuncReadyCallBack(char nEvent)
{
#ifdef __DEBUG__
	std::cout << "PZ FuncReadyCallBack" << std::endl;
#endif
	//设备已就绪
	if (m_function_DeviceRunStatusCallBack != nullptr)
	{
		m_function_DeviceRunStatusCallBack(DeviceRunStatus::Ready);
	}
	return 0;
}

int CPZMedical::FuncErrorCallBack(char nEvent)
{
	return 0;
}