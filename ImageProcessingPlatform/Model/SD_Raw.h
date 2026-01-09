#pragma once
#include <vector>

class CSD_Raw
{
public:
	bool LoadSDRaw(const std::vector<uint8_t>* p);

	uint16_t m_wPicWidth;

	uint16_t m_wPicHeight;

	void SetOriginalRawData(const uint8_t* p, int len);

	void SetOriginalRawData(const std::vector<uint8_t>* p);

	std::vector<uint8_t> GetOriginalRawData();

	std::vector<uint8_t> GetSaveOriginalData();

	void SetTempRawData(const uint8_t* p, int len);

	void SetTempRawData(const std::vector<uint8_t>* p);

	std::vector<uint8_t> GetTempRawData();

	std::vector<uint8_t> GetSaveTempData();

	void Clear();

private:
	void GetSaveData(const std::vector<uint8_t>* pRawData, std::vector<uint8_t>* pResult);
	std::vector<uint8_t> m_vector_Original_RawData;
	std::vector<uint8_t> m_vector_Temp_RawData;
};
