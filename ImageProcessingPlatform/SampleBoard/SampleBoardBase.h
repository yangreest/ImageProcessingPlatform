#pragma once
#include <cstdint>
#include <functional>
#include <string>

#include "Model/SampleBoardConfig.h"

enum DeviceConnectStatus
{
	UnKnown = -1,
	Closed = 0,
	OpendListen = 1,
	Connected = 2,
};

enum DeviceRunStatus
{
	Unknow = -1,
	Awake = 0,
	Ready = 1,
	Busy = 2,
};

enum DeviceValue
{
	Temperature = 0,
	Battery = 1,
	Wifi = 2,
};

class ISampleBoardBase
{
public:
	virtual ~ISampleBoardBase() = default;
	virtual bool Init(const CSampleBoardConfig& pConfig) = 0;
	virtual bool BeginWork() = 0;
	virtual bool EndWork() = 0;
	virtual void RegisterImgCallback(const std::function<void(uint8_t*, int, int, int)>& f) = 0;
	virtual void RegisterDeviceConnectCallback(const std::function<void(DeviceConnectStatus)>& f) = 0;
	virtual void RegisterDeviceRunCallback(const std::function<void(DeviceRunStatus)>& f) = 0;
	virtual void RegisterDeviceValueCallback(const std::function<void(DeviceValue, double)>& f) = 0;
	virtual bool ManuallyTrigger() = 0;
	static ISampleBoardBase* GetSampleBoard(int SampleBoardType);
};
