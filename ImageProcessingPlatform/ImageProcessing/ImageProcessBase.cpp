#include "ImageProcessBase.h"
#include "ImageProcessByHamming.h"
#include "ImageProcessByOpenCV.h"

CImageProcessParam1::CImageProcessParam1()
{
	m_nBalanced = 0;
	m_nDetail = 7;
	m_nEnhance = 0;
	m_nDenoise = 0;
}

CImageProcessParam2::CImageProcessParam2()
{
	m_wMaxBright = 0;
	m_wMinBright = 0;
}

CImageProcessBase* CImageProcessBase::GetCImageProcessObj(int type)
{
	switch (type)
	{
	case 1:
	{
		// 新优化算法：开关中值去椒盐噪点 + 平坦区平滑 + 边缘非锐化增强 + 百分位自动窗宽窗位
		return new CImageProcessByOpenCV();
	}
	default:
	{
		return new CImageProcessByHamming();
	}
	}
}