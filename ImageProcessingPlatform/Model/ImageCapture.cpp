#include "ImageCapture.h"

CImageCapture::CImageCapture()
{
	Clear();
}

void CImageCapture::Clear()
{
	m_nPosX = 0;
	m_nPosY = 0;
	m_nPicW = 0;
	m_nPicH = 0;
	m_bSaved = false;
}