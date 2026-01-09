#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "SampleBoardBase.h"

class CPZMedical : public ISampleBoardBase
{
public:
	CPZMedical();
	bool Init(const CSampleBoardConfig& pConfig) override;
	bool BeginWork() override;
	bool EndWork() override;
	void RegisterImgCallback(const std::function<void(uint8_t*, int, int, int)>& f) override;
	void RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus)>& f) override;
	void RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus)>& f) override;
	void RegisterDeviceValueCallback(const std::function<void(DeviceValue, double)>& f) override;
	bool ManuallyTrigger() override;

private:
	static void LoopAutoConnect();
	static bool m_bLinked;

	static int FuncLinkCallBack(char nEvent);
	static int FuncExposeCallBack(char nEvent);
	static int FuncBreakCallBack(char nEvent);
	static int FuncImageCallBack(char nEvent);
	static int FuncHeartBeatCallBack(char nEvent);
	static int FuncReadyCallBack(char nEvent);
	static int FuncErrorCallBack(char nEvent);

	static std::function<void(uint8_t*, int, int, int)> m_function_ImgCallBack;
	static std::function<void(DeviceConnectStatus)> m_function_DeviceConnectStatusCallBack;
	static std::function<void(DeviceRunStatus)> m_function_DeviceRunStatusCallBack;
	static std::function<void(DeviceValue, double)> m_function_DeviceValueCallBack;
	static std::vector<uint8_t> m_vector_ImgBuffer;
	static bool m_bIsInited;
	static bool m_bIsNeedExit;
	static bool m_bSoftwareTrigger;
	static int m_nInitStatus;
	static int8_t m_cFpCurStat;
};
