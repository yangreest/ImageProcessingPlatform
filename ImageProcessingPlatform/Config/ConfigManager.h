#pragma once
#include <string>
#include <vector>

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

class CSampleBoardConfig
{
public:
	CSampleBoardConfig();

	/// <summary>
	/// 厂家
	/// </summary>
	int m_nManufacturer;

	/// <summary>
	/// 型号
	/// </summary>
	int m_nModel;

	/// <summary>
	/// 图片类型
	/// </summary>
	int m_nMapType;

	/// <summary>
	/// 曝光模式 0-AED触发 1-手动
	/// </summary>
	int m_nExposureType;

	/// <summary>
	/// 曝光时间
	/// </summary>
	int m_nExposureTime;
};

class CTimsConfig
{
public:
	int m_nForceGuid;
	std::string m_strUploadPic;
	std::string m_strDownloadPic;
	std::string m_strGetGuidInfo;
	int m_nDownloadTimeOut;
	int nLanguage; //0 中文 1 英文
};

class CTcpClientParam
{
public:
	std::string m_strIp;
	int m_nPort;
};

class CConfigManager
{
public:
	CControlBoardConfig m_memControlBoardConfig;
	CImageProcessConfig m_memCImageProcessConfig;
	CCameraConfig m_memCCameraConfig;
	CSampleBoardConfig m_memCSampleBoardConfig;
	CTimsConfig m_memTimsConfig;
	CTcpClientParam m_mCTcpClientParam;
	void Read(const std::string& filePath);
	void Write(const std::string& filePath);
};
