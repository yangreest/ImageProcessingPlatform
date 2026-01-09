#pragma once

#include <QMainWindow>
#include "ui_AlertForm.h"
#include <string>

#include "FormWithShowModal.h"

class AlertForm : public CFormWithShowModal
{
	Q_OBJECT

public:
	AlertForm(const std::string & title,const std::string & content, QWidget *parent = nullptr);
	~AlertForm();

private:
	Ui::AlertFormClass ui;

private slots:
	// 关闭按钮点击事件
	void onCloseButtonClicked();
};

