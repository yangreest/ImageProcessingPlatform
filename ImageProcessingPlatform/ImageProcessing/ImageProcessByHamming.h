#pragma once
#include "ImageProcessBase.h"

class CImageProcessByHamming : public CImageProcessBase
{
public:
	CImageProcessByHamming();
	bool ImageProcess1(int wid, int hei, uint8_t* picData, CImageProcessParam1* imageProcessParam,
		std::vector<uint8_t>* v) override;
	bool BrightAndContrastProcess(int wid, int hei, uint16_t* picData, CImageProcessParam2* imageProcessParam,
		std::vector<uint8_t>* result) override;
	bool calculateMaxandMinBright(const std::vector<uint8_t>& data, int& nMean, int& nStdDev) override;
	std::string GetVersion() override;

	void SetReverse(bool bImgNeedReverse)override;
private:
	bool m_bImgNeedReverse;
};
