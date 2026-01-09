#pragma once
#include <string>
#include <vector>

#include "Model/SampleBoardConfig.h"

class CControlBoardConfig
{
public:
	CControlBoardConfig();
	std::string m_strIp;
	uint16_t m_wPort;
	std::string m_strIp2;
	uint16_t m_wPort2;
	uint16_t m_wDeviceHeartBeat;
	bool m_bFactoryMode;
	bool m_bEnableIp2;
};

class CRawFileSize
{
public:
	int m_nFileSize;
	int m_nRawFileWidth;
	int m_nRawFileHeight;
};

class CImageProcessConfig
{
public:
	int m_nType;
	int m_nRawFile;
	std::vector<CRawFileSize> m_vec_RawFileSize;
};

class CCameraConfig
{
public:
	std::string m_strLeftIp;
	std::string m_strMidIp;
	std::string m_strRightIp;
};

class CTimsConfig
{
public:
	int m_nForceGuid;
	std::string m_strUploadPic;
	std::string m_strDownloadPic;
	std::string m_strGetGuidInfo;
	int m_nDownloadTimeOut;
};

class CConfigManager
{
public:
	CControlBoardConfig m_memControlBoardConfig;
	CImageProcessConfig m_memCImageProcessConfig;
	CSampleBoardConfig m_memCSampleBoardConfig;
	CCameraConfig m_memCCameraConfig;
	CTimsConfig m_memTimsConfig;
	void Read(const std::string& filePath);
};
