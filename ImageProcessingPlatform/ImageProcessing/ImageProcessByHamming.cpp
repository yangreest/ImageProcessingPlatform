#include <windows.h>
#include "ImageProcessByHamming.h"
#include "HmImgProApi.h"
#include <array>
#include <iostream>

#include <opencv2/opencv.hpp>

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
	SetConsoleOutputCP(CP_UTF8);  // 或者使用 CP_GBK
	cv::Mat src(hei, wid, CV_16UC1, picData);

	uint16_t targetMean = imageProcessParam->m_wBright;
	uint16_t targetStd = imageProcessParam->m_wContrast;

	// 校验输入图像格式：必须是16位单通道
	if (src.type() != CV_16UC1) {
		std::cerr << "错误：输入图像必须是16位单通道（CV_16UC1）！" << std::endl;
		return false;
	}
	if (src.empty())
	{
		std::cerr << "错误：输入图像为空！" << std::endl;
		return false;
	}


	// 校验传入的目标参数是否在合理范围
	if (targetMean < 0 || targetMean >  65535) {
		std::cerr << "警告：目标亮度均值超出0~65535范围，已自动修正为32768！" << std::endl;
		targetMean = 32768;
	}
	if (targetStd < 0 || targetStd >65535) {
		std::cerr << "警告：目标对比度标准差超出0~65535范围，已自动修正为16384！" << std::endl;
		targetStd = 16384;
	}

	// 计算图像的原始均值和标准差
	cv::Scalar meanVal, stdDevVal;
	meanStdDev(src, meanVal, stdDevVal);
	double mean = meanVal[0];    // 原始亮度均值
	double stdDev = stdDevVal[0];// 原始对比度标准差

	// 16位图像的数值范围：0~65535
	const int MAX_16BIT = 65535;

	// 计算对比度系数alpha（避免除以0）
	double alpha = (stdDev > 1e-6) ? targetStd / stdDev : 1.0;
	// 计算亮度偏移beta（将均值调整到传入的目标均值）
	double beta = targetMean - alpha * mean;


	// 调整亮度和对比度，限制像素值范围
	cv::Mat dst;
	src.convertTo(dst, CV_16UC1, alpha, beta); // 核心调整公式
	// 裁剪超出0~65535的像素值，避免溢出
	threshold(dst, dst, MAX_16BIT, MAX_16BIT, cv::THRESH_TRUNC);
	threshold(dst, dst, 0, 0, cv::THRESH_TOZERO);

	// 使用CV_16UC1 dst 赋值给 result <uint8_t>
	result->resize(wid * hei);
	cv::Mat dst8bit;
	dst8bit = dst / 255;
	dst8bit.convertTo(dst8bit, CV_8UC1);
	memcpy(result->data(), dst8bit.data, wid * hei * sizeof(uint8_t));

	std::cout << "自适应调整参数：" << std::endl;
	std::cout << "原始均值：" << mean << "，原始标准差：" << stdDev << std::endl;
	std::cout << "对比度系数alpha：" << alpha << "，亮度偏移beta：" << beta << std::endl;
	return true;
}

#if 0
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

#endif

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