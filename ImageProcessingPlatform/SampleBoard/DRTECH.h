#pragma once
#include "SampleBoardBase.h"

class CDRTECH : public ISampleBoardBase
{
public:
	CDRTECH();
	bool Init(const CSampleBoardConfig& pConfig) override;
	bool BeginWork() override;
	bool EndWork() override;
	void RegisterImgCallback(const std::function<void(uint8_t*, int, int, int)>& f) override;
	void RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus)>& f) override;
	void RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus)>& f) override;
	void RegisterDeviceValueCallback(const std::function<void(DeviceValue, double)>& f) override;
	bool ManuallyTrigger() override;

private:
	void DeviceConnectThread();
	std::function<void(uint8_t*, int, int, int)> m_function_ImgCallBack;
	std::function<void(DeviceConnectStatus)> m_function_DeviceConnectStatusCallBack;
	std::function<void(DeviceRunStatus)> m_function_DeviceRunStatusCallBack;
	std::function<void(DeviceValue, double)> m_function_DeviceValueCallBack;
	std::vector<uint8_t> m_vector_ImgBuffer;
	int m_nModelType;
	int m_nMapType;
	bool m_bSoftwareTrigger;
	int m_nExposureTime;
	int m_nImgWidth;
	int m_nImgHeight;
	static void CallbackMessageProcess(int _nMsgType, char* _pMsg, void* _pData);
	void CallbackMessageProcess_Inner(int _nMsgType, char* _pMsg);
	bool m_bIsNeedExit;

	DeviceConnectStatus m_enumDeviceConnectStatus;

	int m_nInitStatus;
	std::string m_strMapPath;
};

enum DetectorExpTypeApp
{
	EXP_TYPE_APP_USB = 1,
	EXP_TYPE_APP_AED = 2,
	EXP_TYPE_APP_SYSTEM_TRIGGER = 3,
	EXP_TYPE_APP_SOFTWARE_TRIGGER = 6,
};
