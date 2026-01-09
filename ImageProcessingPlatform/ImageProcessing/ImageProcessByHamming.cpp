#include <windows.h>
#include "ImageProcessByHamming.h"
#include "HmImgProApi.h"
#include <array>

CImageProcessByHamming::CImageProcessByHamming()
{
#ifdef _DEBUG
	auto ko = InitiImgPro(true);
	int tm = 10;
#else
	InitiImgPro(false);
#endif

	m_bImgNeedReverse = false;
}

bool CImageProcessByHamming::ImageProcess1(int wid, int hei, uint8_t* picData,
	CImageProcessParam1* imageProcessParam, std::vector<uint8_t>* v)
{
	if (wid == 0 || hei == 0)
	{
		return false;
	}
	//auto dataLen = wid * hei * sizeof(uint16_t);
	setRawData(picData, wid, hei);
	setParam(imageProcessParam->m_nBalanced, imageProcessParam->m_nDetail, imageProcessParam->m_nEnhance,
		imageProcessParam->m_nDenoise);

	//setParam(imageProcessParam->m_nBalanced, 6, 8, 3);
	process();
	//setBrightnessContrast(6452, 1977);
	//saveImage("E:\\temp\\test.png");
	int len;
	auto dt = getProcessedData(len);
	if (dt == nullptr)
	{
		return false;
	}
	if (v->size() != len)
	{
		v->resize(len);
	}
	memcpy(v->data(), dt, len);
	return true;
}

bool CImageProcessByHamming::BrightAndContrastProcess(int wid, int hei, uint16_t* picData,
	CImageProcessParam2* imageProcessParam,
	std::vector<uint8_t>* result)
{
	auto length = wid * hei;
	auto windowWidth = imageProcessParam->m_wContrast;
	auto windowLevel = imageProcessParam->m_wBright;
	if (result->size() != length)
	{
		result->resize(length);
	}

	// 计算窗宽窗位的上下限
	unsigned short lower = windowLevel - windowWidth / 2;
	unsigned short upper = windowLevel + windowWidth / 2;

	// 确保下限不小于0 (对于unsigned short)
	if (windowWidth / 2 > windowLevel) lower = 0;
	auto r = result->data();
	// 遍历所有像素进行转换
	std::array<uint8_t, 65536> lut;
	// 假设 picData 是 uint16_t 类型
	for (int i = 0; i < 65536; ++i)
	{
		unsigned short val = static_cast<unsigned short>(i);
		if (val <= lower)
		{
			lut[i] = 0;
		}
		else if (val >= upper)
		{
			lut[i] = 255;
		}
		else
		{
			lut[i] = static_cast<uint8_t>((val - lower) * 255.0f / (upper - lower));
		}
	}

	// 使用查表法替换循环
	for (size_t i = 0; i < length; ++i)
	{
		if (m_bImgNeedReverse)
		{
			r[i] = 255 - lut[picData[i]];
		}
		else
		{
			r[i] = lut[picData[i]];
		}
	}

	return result;
}

std::string CImageProcessByHamming::GetVersion()
{
	SDKVersionInfo verInfo;
	GetSDKVersion("ImgProcCplusplus.dll", &verInfo);
	return std::to_string(HIWORD(verInfo.dwFileVersionMS)) + "." + std::to_string(LOWORD(verInfo.dwFileVersionMS)) + "."
		+ std::to_string(HIWORD(verInfo.dwFileVersionLS)) + "." + std::to_string(LOWORD(verInfo.dwFileVersionLS));
}

void CImageProcessByHamming::SetReverse(bool bImgNeedReverse)
{
	m_bImgNeedReverse = bImgNeedReverse;
}