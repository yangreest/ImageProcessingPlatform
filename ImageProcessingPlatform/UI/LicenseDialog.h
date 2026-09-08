#pragma once

#include <QMainWindow>
#include <string>

#include "FormWithShowModal.h"
#include "ui_LicenseDialog.h"

class LicenseDialog : public CFormWithShowModal
{
	Q_OBJECT

public:
	LicenseDialog(const std::string& hardwareId,
		const std::string& hardwareIdHash,
		bool expired,
		QWidget* parent = nullptr);
	~LicenseDialog();

	// 用户输入的授权码
	static std::string inputCode;
	// 用户是否点击了“Activate”确认
	static bool confirmed;

private:
	Ui::LicenseDialogClass ui;

private slots:
	// 激活按钮点击事件
	void onActivateClicked();
	// 取消按钮点击事件
	void onCancelClicked();
};
