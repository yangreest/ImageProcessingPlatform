#include <windows.h>
#include "ImageProcessByHamming.h"
#include "HmImgProApi.h"
#include <array>
#include <iostream>

CImageProcessByHamming::CImageProcessByHamming()
{
#ifdef __DEBUG__
	InitiImgPro(true);
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

bool CImageProcessByHamming::calculateMeanAndStdDev(const std::vector<uint8_t>& data, int& nMean, int& nStdDev)
{
	// 1. 合法性检查：空数组直接返回失败
	if (data.empty())
	{
		std::cerr << "错误：输入数组为空，无法计算均值和标准差！" << std::endl;
		nMean = 0.0;
		nStdDev = 0.0;
		return false;
	}

	std::vector<uint16_t> data_16bit;
	data_16bit.reserve(data.size() / 2);
	for (size_t i = 0; i < data.size(); i += 2)
	{
		// 小端模式：低字节在前，高字节在后
		uint16_t pixel_value = static_cast<uint16_t>(data[i]) |
			(static_cast<uint16_t>(data[i + 1]) << 8);
		data_16bit.push_back(pixel_value);
	}


	// 2. 计算总和（使用uint64_t避免溢出，因为uint16_t最大65535，若数组有100万元素，总和会超32位）
	uint64_t sum = 0;
	for (uint16_t val : data_16bit)
	{
		sum += val;
	}
	// 也可以用std::accumulate（需要包含<numeric>），效果等价
	// uint64_t sum = std::accumulate(data.begin(), data.end(), 0ULL);

	// 3. 计算均值（转换为double，保证精度）
	size_t n = data_16bit.size();
	nMean = static_cast<double>(sum) / n;

	// 4. 计算平方差之和（每个元素与均值的差的平方和）
	double sumOfSquares = 0.0;
	for (uint16_t val : data_16bit)
	{
		double diff = static_cast<double>(val) - nMean;
		sumOfSquares += diff * diff; // 等价于pow(diff, 2)，效率更高
	}

	// 5. 计算标准差（总体标准差，除以n；若要样本标准差则除以n-1）
	nStdDev = std::sqrt(sumOfSquares / n);

	return true;
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