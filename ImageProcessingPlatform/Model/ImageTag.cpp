#include "ImageTag.h"

uint64_t CImageTag::g_nUID = 0;

CImageTag::CImageTag()
{
	m_nUID = g_nUID++;
	m_nTagType = 0;
	m_nStartX = 0;
	m_nStartY = 0;
	m_nEndX = 0;
	m_nEndY = 0;
	m_nEndX2 = 0;
	m_nEndY2 = 0;
	m_strContent = "";
	m_bSelected = false;
}