#include "ImageProcessByOpenCV.h"

#include <opencv2/opencv.hpp>
#include <algorithm>
#include <iostream>

namespace
{
	// 脉冲（椒盐）判定阈值：降噪强度越大阈值越小，去噪越积极。16bit 灰度尺度。
	inline double ImpulseThreshold(int nDenoise)
	{
		const double d = std::clamp(nDenoise, 0, 100);
		return 2500.0 - 22.0 * d;   // d=0 -> 2500, d=100 -> 300
	}

	// 非锐化掩模增益：由增强滑块(0~21)映射到 0~1.26。
	inline double UnsharpAmount(int nEnhance)
	{
		const double e = std::clamp(nEnhance, 0, 100);
		return e * 0.06;
	}

	// 平坦/边缘软过渡的下/上倍率（相对噪声σ）：低于2σ视为噪声抑制，高于5σ视为真实边缘增强。
	constexpr double kNoiseLowMul = 2.0;
	constexpr double kNoiseHighMul = 5.0;
	// 噪声区保留的细节比例（抑制颗粒但不清除弱细节）。
	constexpr float kFlatGain = 0.5f;
	// 多尺度非锐化：细尺度σ与中尺度σ（中尺度负责抬升金属内部空腔/边界线等弱结构）。
	constexpr double kSigmaFine = 1.2;
	constexpr double kSigmaMid = 12.0;
	// 暗部提升 gamma（<1 抬升金属等暗区，使其内部空腔/边界线进入可见灰阶）。
	constexpr double kGamma = 0.5;
}

CImageProcessByOpenCV::CImageProcessByOpenCV()
{
	m_bImgNeedReverse = false;
}

bool CImageProcessByOpenCV::ImageProcess1(int wid, int hei, uint8_t* picData,
	CImageProcessParam1* imageProcessParam, std::vector<uint8_t>* v)
{
	if (wid <= 0 || hei <= 0 || picData == nullptr || imageProcessParam == nullptr || v == nullptr)
	{
		return false;
	}

	// 输入为 16bit 小端灰度数据
	cv::Mat src(hei, wid, CV_16UC1, picData);

	// ---- 1) 开关中值：去除背景黑噪点与金属区白噪点（脉冲噪声），边缘保持 ----
	cv::Mat med;
	cv::medianBlur(src, med, 3);
	cv::Mat diff;
	cv::absdiff(src, med, diff);
	cv::Mat impulseMask;
	cv::compare(diff, cv::Scalar::all(ImpulseThreshold(imageProcessParam->m_nDenoise)), impulseMask, cv::CMP_GT);
	cv::Mat den;
	src.copyTo(den);
	med.copyTo(den, impulseMask);   // 仅脉冲像素替换为中值

	// ---- 2) 多尺度非锐化：中尺度增益抬升金属内部空腔/边界线，细尺度软阈值抑制噪声 ----
	cv::Mat denF, l1, l2;
	den.convertTo(denF, CV_32F);
	cv::GaussianBlur(denF, l1, cv::Size(), kSigmaFine);
	cv::GaussianBlur(denF, l2, cv::Size(), kSigmaMid);
	cv::Mat dFine = denF - l1;   // 细尺度：噪声 + 锐利边缘
	cv::Mat dMid = l1 - l2;      // 中尺度：空腔、边界线等弱结构

	// 细尺度软增益：低于2σ视为噪声抑制，高于5σ视为真实边缘增强
	cv::Mat absFine;
	cv::absdiff(denF, l1, absFine);
	const double sigma = 1.2533 * cv::mean(absFine)[0];
	const double tLow = kNoiseLowMul * sigma;
	const double tHigh = kNoiseHighMul * sigma;
	cv::Mat tmp = (absFine - tLow) / std::max(1e-6, (tHigh - tLow));
	cv::threshold(tmp, tmp, 1.0, 1.0, cv::THRESH_TRUNC);
	cv::threshold(tmp, tmp, 0.0, 0.0, cv::THRESH_TOZERO);
	cv::Mat w;
	tmp.convertTo(w, CV_32F);
	const double gFine = 1.0 + UnsharpAmount(imageProcessParam->m_nEnhance) * 0.5;
	const double gMid = 1.0 + UnsharpAmount(imageProcessParam->m_nEnhance);   // 中尺度增益更强
	cv::Mat gainFine = (w * (gFine - kFlatGain)) + kFlatGain;
	cv::Mat outF = l2 + dMid * gMid + gainFine.mul(dFine);

	// 饱和截断回 16bit
	cv::Mat out16;
	outF.convertTo(out16, CV_16UC1);

	const size_t byteLen = static_cast<size_t>(wid) * hei * sizeof(uint16_t);
	if (v->size() != byteLen)
	{
		v->resize(byteLen);
	}
	memcpy(v->data(), out16.data, byteLen);
	return true;
}

bool CImageProcessByOpenCV::BrightAndContrastProcess(int wid, int hei, uint16_t* picData,
	CImageProcessParam2* imageProcessParam,
	std::vector<uint8_t>* result)
{
	cv::Mat src(hei, wid, CV_16UC1, picData);

	uint16_t targetMax = imageProcessParam->m_wMaxBright;
	uint16_t targetMin = imageProcessParam->m_wMinBright;

	if (src.type() != CV_16UC1 || src.empty())
	{
		std::cerr << "错误：输入图像必须是16位单通道（CV_16UC1）且非空！" << std::endl;
		return false;
	}
	if (targetMax <= targetMin)
	{
		std::cerr << "错误：窗宽窗位参数非法（max 必须大于 min）！" << std::endl;
		return false;
	}

	const int MAX_16BIT = 65535;

	// 窗宽窗位线性映射到全范围
	cv::Mat dst;
	src.convertTo(dst, CV_16UC1, 65535.0 / (targetMax - targetMin), -targetMin * 65535.0 / (targetMax - targetMin));
	if (m_bImgNeedReverse)
	{
		dst.convertTo(dst, CV_16UC1, -1, 65535);
	}
	// 裁剪超出0~65535的像素值，避免溢出
	threshold(dst, dst, MAX_16BIT, MAX_16BIT, cv::THRESH_TRUNC);
	threshold(dst, dst, 0, 0, cv::THRESH_TOZERO);

	// 暗部提升：gamma<1 抬升金属等暗区，使其内部空腔/边界线的弱起伏进入可见灰阶
	cv::Mat gf;
	dst.convertTo(gf, CV_32F, 1.0 / 65535.0);
	cv::pow(gf, kGamma, gf);
	gf.convertTo(dst, CV_16UC1, 65535.0);

	result->resize(wid * hei);
	cv::Mat dst8bit;
	dst8bit = dst / 255;
	dst8bit.convertTo(dst8bit, CV_8UC1);
	memcpy(result->data(), dst8bit.data, wid * hei * sizeof(uint8_t));
	return true;
}

bool CImageProcessByOpenCV::calculateMaxandMinBright(const std::vector<uint8_t>& data, int& nMean, int& nStdDev)
{
	nMean = 0;
	nStdDev = 0;
	if (data.size() < 2)
	{
		std::cerr << "错误：输入数组为空，无法计算窗宽窗位！" << std::endl;
		return false;
	}

	// 16bit 直方图统计
	std::vector<uint32_t> hist(65536, 0);
	const size_t n = data.size() / 2;
	for (size_t i = 0; i < n; ++i)
	{
		// 小端模式：低字节在前，高字节在后
		const uint16_t val = static_cast<uint16_t>(data[2 * i]) |
			(static_cast<uint16_t>(data[2 * i + 1]) << 8);
		++hist[val];
	}

	// 百分位窗口：剔除离群噪点/坏点，避免其拉宽窗口导致对比度下降、边界发灰
	const uint64_t lowCut = static_cast<uint64_t>(n * 0.001);    // 0.1%
	const uint64_t highCut = static_cast<uint64_t>(n * 0.999);   // 99.9%

	uint64_t cum = 0;
	int minVal = 0;
	int maxVal = 65535;
	for (int i = 0; i < 65536; ++i)
	{
		cum += hist[i];
		if (cum > lowCut) { minVal = i; break; }
	}
	cum = 0;
	for (int i = 65535; i >= 0; --i)
	{
		cum += hist[i];
		if (cum > (n - highCut)) { maxVal = i; break; }
	}

	if (maxVal <= minVal)
	{
		minVal = 0;
		maxVal = 65535;
	}

	// 与既有语义保持一致：nMean 返回上限(max)，nStdDev 返回下限(min)
	nMean = maxVal;
	nStdDev = minVal;
	return true;
}

std::string CImageProcessByOpenCV::GetVersion()
{
	return "OpenCV-Optimized 1.0.0";
}

void CImageProcessByOpenCV::SetReverse(bool bImgNeedReverse)
{
	m_bImgNeedReverse = bImgNeedReverse;
}
