#include "ImageProcessBase.h"
#include "ImageProcessByHamming.h"

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
	default:
	{
		return new CImageProcessByHamming();
	}
	}
}