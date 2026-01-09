#pragma once

#include <QMainWindow>
#include "ui_ControlBoardConfigForm.h"
#include "DeviceCom/IDeviceCom.h"

class ControlBoardConfigForm : public QMainWindow
{
	Q_OBJECT

public:
	ControlBoardConfigForm(IDeviceCom* p, QWidget* parent = nullptr);
	~ControlBoardConfigForm();

private slots:
	void On_SendCmd_Click();

private:
	Ui::ControlBoardConfigFormClass ui;

	IDeviceCom* m_pControlBoardCom;
};
