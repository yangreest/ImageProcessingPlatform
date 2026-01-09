#include "SD_Raw.h"

#define  SD_ROW_HEADER_LEN 127

bool CSD_Raw::LoadSDRaw(const std::vector<uint8_t>* p)
{
	auto dt = p->data();

	if (dt[0] != 'S' || dt[1] != 'T' || dt[2] != 'A' || dt[3] != 'R')
	{
		return false;
	}
	int index = 4;

	memcpy(&m_wPicWidth, dt + index, sizeof(m_wPicWidth));
	index = index + sizeof(m_wPicWidth);

	memcpy(&m_wPicHeight, dt + index, sizeof(m_wPicHeight));
	index = index + sizeof(m_wPicHeight);

	SetOriginalRawData(dt + SD_ROW_HEADER_LEN, p->size() - SD_ROW_HEADER_LEN);
	Clear();
	return true;
}

void CSD_Raw::SetOriginalRawData(const uint8_t* p, int len)
{
	m_vector_Original_RawData.resize(len);
	memcpy(m_vector_Original_RawData.data(), p, len);
}

void CSD_Raw::SetOriginalRawData(const std::vector<uint8_t>* p)
{
	m_vector_Original_RawData.resize(p->size());
	memcpy(m_vector_Original_RawData.data(), p->data(), p->size());
}

std::vector<uint8_t> CSD_Raw::GetSaveOriginalData()
{
	std::vector<uint8_t> result;
	GetSaveData(&m_vector_Original_RawData, &result);
	return result;
}

void CSD_Raw::SetTempRawData(const uint8_t* p, int len)
{
	m_vector_Temp_RawData.resize(len);
	memcpy(m_vector_Temp_RawData.data(), p, len);
}

void CSD_Raw::SetTempRawData(const std::vector<uint8_t>* p)
{
	m_vector_Temp_RawData.resize(p->size());
	memcpy(m_vector_Temp_RawData.data(), p->data(), p->size());
}

std::vector<uint8_t> CSD_Raw::GetTempRawData()
{
	return m_vector_Temp_RawData;
}

std::vector<uint8_t> CSD_Raw::GetSaveTempData()
{
	std::vector<uint8_t> result;
	GetSaveData(&m_vector_Temp_RawData, &result);
	return result;
}

void CSD_Raw::Clear()
{
	//m_vector_Temp_RawData.resize(m_vector_Original_RawData.data(0));
	m_vector_Temp_RawData = m_vector_Original_RawData;
}

void CSD_Raw::GetSaveData(const std::vector<uint8_t>* pRawData, std::vector<uint8_t>* pResult)
{
	auto allSize = SD_ROW_HEADER_LEN + pRawData->size();
	pResult->resize(allSize);
	auto dt = pResult->data();
	auto index = 0;
	dt[index] = 'S';
	index = index + 1;

	dt[index] = 'T';
	index = index + 1;

	dt[index] = 'A';
	index = index + 1;

	dt[index] = 'R';
	index = index + 1;

	memcpy(dt + index, &m_wPicWidth, sizeof(m_wPicWidth));
	index = index + sizeof(m_wPicWidth);

	memcpy(dt + index, &m_wPicHeight, sizeof(m_wPicHeight));
	index = index + sizeof(m_wPicHeight);

	memcpy(dt + SD_ROW_HEADER_LEN, pRawData->data(), pRawData->size());
}

std::vector<uint8_t> CSD_Raw::GetOriginalRawData()
{
	return m_vector_Original_RawData;
}