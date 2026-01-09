#pragma once

#include <cstdint>
#include <windows.h>

class CUSBKeyData
{
public:
	CUSBKeyData();
	uint8_t m_cDeviceRunMode;
};

class CUSBKey
{
public:
	CUSBKey();
	void LoopCheckCheckUSBKeyExist();
	bool ReadUSBKey(CUSBKeyData* pData);

private:
	DWORD m_dwHID;
	WORD m_wIndex;
};
