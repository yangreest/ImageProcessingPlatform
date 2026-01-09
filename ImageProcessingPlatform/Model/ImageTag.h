#pragma once
#include <string>

class CImageTag
{
public:
	CImageTag();

	uint64_t m_nUID;

	/// <summary>
	/// 0、线 1、矩形 2、椭圆 3、角度、 4、文字 5、弯曲度
	/// </summary>
	int m_nTagType;
	int m_nStartX;
	int m_nStartY;
	int m_nEndX;
	int m_nEndY;
	int m_nEndX2;
	int m_nEndY2;
	std::string m_strContent;

	bool m_bSelected;

	static uint64_t g_nUID;
};