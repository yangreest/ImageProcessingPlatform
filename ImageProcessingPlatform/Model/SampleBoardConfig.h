#pragma once

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
