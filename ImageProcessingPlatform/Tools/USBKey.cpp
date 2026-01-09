#include "USBKey.h"

#include <vector>

#include "Vikey.h"

#pragma comment(lib, "legacy_stdio_definitions.lib")

CUSBKeyData::CUSBKeyData()
{
	m_cDeviceRunMode = 3;
}

CUSBKey::CUSBKey()
{
	m_dwHID = 0;
	m_wIndex = 0;
}

void CUSBKey::LoopCheckCheckUSBKeyExist()
{
	DWORD dwHID = 0;
	DWORD dwCount = 0;
	while (true)
	{
		Sleep(1000);
		auto dwRetCode = VikeyFind(&dwCount);
		if (dwRetCode != 0)
		{
			break;
		}
		if (dwCount <= 0)
		{
			break;
		}

		dwRetCode = VikeyGetHID(m_wIndex, &dwHID);
		if (dwRetCode != 0)
		{
			break;
		}
		if (dwHID != m_dwHID)
		{
			break;
		}
	}
	_Exit(0);
}

bool CUSBKey::ReadUSBKey(CUSBKeyData* pData)
{
	DWORD dwCount = 0;
	auto dwRetCode = VikeyFind(&dwCount);
	if (dwRetCode != 0)
	{
		return false;
	}

	DWORD dwHID = 0;
	dwRetCode = VikeyGetHID(m_wIndex, &dwHID);
	if (dwRetCode != 0)
	{
		return false;
	}
	//这个key很重要
	char apiKey[] =
		"2065FB974A180643D011C8B76EFD7B1E8CA18FBEC54942CDECE2CADC032E6D0D371249D658D1415E88EC94AFA21AD464E921AA9C995ECFB79B01DFCEEB17B9A8";

	dwRetCode = VikeySetApiKey(m_wIndex, apiKey);

	if (dwRetCode != 0)
	{
		return false;
	}

	char passwd[] = "BiuBiu66";
	dwRetCode = VikeyUserLogin(m_wIndex, passwd);
	if (dwRetCode != 0)
	{
		return false;
	}

	dwRetCode = VikeyAdminLogin(m_wIndex, passwd);
	if (dwRetCode != 0)
	{
		return false;
	}
	std::vector<uint8_t> buffer(32);
	dwRetCode = VikeyReadData(m_wIndex, 0, 32, buffer.data());
	if (dwRetCode != 0)
	{
		return false;
	}
	pData->m_cDeviceRunMode = buffer[0];
	VikeyLogoff(0);
	VikeyUninitialization(); //程序退出之前一定要调用此函数

	m_dwHID = dwHID;
	return true;
}