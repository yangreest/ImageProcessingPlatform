#pragma once
#include "ImageProcessBase.h"

// 纯 OpenCV 实现的优化派生算法（不依赖 Hamming SDK），针对三类显示问题：
// 1) 背景白区黑噪点 / 金属区白噪点（脉冲-椒盐噪声）：开关中值滤波，仅替换偏离中值过大的像素，去噪且不糊边；
// 2) 平坦区颗粒感：以高斯模糊为低频基准，平坦区向平滑值收敛；
// 3) 边界不清晰：边缘区非锐化掩模（增益>1）增强；自动窗宽窗位改用百分位统计，避免离群噪点拉宽窗口。
class CImageProcessByOpenCV : public CImageProcessBase
{
public:
	CImageProcessByOpenCV();
	bool ImageProcess1(int wid, int hei, uint8_t* picData, CImageProcessParam1* imageProcessParam,
		std::vector<uint8_t>* v) override;
	bool BrightAndContrastProcess(int wid, int hei, uint16_t* picData, CImageProcessParam2* imageProcessParam,
		std::vector<uint8_t>* result) override;
	bool calculateMaxandMinBright(const std::vector<uint8_t>& data, int& nMean, int& nStdDev) override;
	std::string GetVersion() override;

	void SetReverse(bool bImgNeedReverse) override;
private:
	bool m_bImgNeedReverse;
};
