#pragma once
class CImageCapture
{
public:
	CImageCapture();
	void Clear();
	int m_nPosX;
	int m_nPosY;
	int m_nPicW;
	int m_nPicH;
	bool m_bSaved;
};