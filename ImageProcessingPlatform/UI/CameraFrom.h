#pragma once

#include <QMainWindow>
#include "ui_CameraFrom.h"
#include "Camera/CameraBase.h"
#include "Config/ConfigManager.h"

class CameraFrom : public QMainWindow
{
	Q_OBJECT

public:
	CameraFrom(const CConfigManager* nConfig, QWidget* parent = nullptr);
	~CameraFrom();

private:
	Ui::CameraFromClass ui;
	void Connect();
	ICameraBase* m_pC1;
	ICameraBase* m_pC2;
	ICameraBase* m_pC3;
	std::string m_strLeftIp;
	std::string m_strMidIp;
	std::string m_strRightIp;
};
