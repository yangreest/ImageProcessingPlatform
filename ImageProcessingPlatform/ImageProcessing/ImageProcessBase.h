#pragma once
#include <string>
#include <vector>

class CImageProcessParam1
{
public:
	CImageProcessParam1();
	int m_nBalanced;
	int m_nDetail;
	int m_nEnhance;
	int m_nDenoise;
};

class CImageProcessParam2
{
public:
	CImageProcessParam2();
	uint16_t m_wMaxBright;
	uint16_t m_wMinBright;
};

class CImageProcessBase
{
public:
	virtual ~CImageProcessBase() = default;
	virtual bool ImageProcess1(int wid, int hei, uint8_t* picData, CImageProcessParam1* imageProcessParam,
		std::vector<uint8_t>* v) = 0;
	virtual bool BrightAndContrastProcess(int wid, int hei, uint16_t* picData, CImageProcessParam2* imageProcessParam,
		std::vector<uint8_t>* result) = 0;
	virtual bool calculateMaxandMinBright(const std::vector<uint8_t>& data, int& nMean, int& nStdDev) = 0;
	virtual std::string GetVersion() = 0;
	virtual void SetReverse(bool bImgNeedReverse) = 0;

	static CImageProcessBase* GetCImageProcessObj(int type);
};
