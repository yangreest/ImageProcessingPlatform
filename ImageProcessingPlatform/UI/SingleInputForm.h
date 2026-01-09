#pragma once

#include <QMainWindow>

#include "FormWithShowModal.h"
#include "ui_SingleInputForm.h"

class SingleInputForm : public CFormWithShowModal
{
	Q_OBJECT

public:
	SingleInputForm(const std::string& title, QWidget* parent = nullptr);
	~SingleInputForm();
	static int result;

private:
	Ui::SingleInputFormClass ui;

signals:
	// 窗口关闭时发射的信号
	void windowClosed();

private slots:
	// 关闭按钮点击事件
	void onCloseButtonClicked();
	void onYesButtonClicked();

};
