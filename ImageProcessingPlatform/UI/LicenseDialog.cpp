#include "LicenseDialog.h"
#include <QCloseEvent>

std::string LicenseDialog::inputCode = "";
bool LicenseDialog::confirmed = false;

LicenseDialog::LicenseDialog(const std::string& hardwareId,
	const std::string& hardwareIdHash,
	bool expired,
	QWidget* parent)
	: CFormWithShowModal(parent)
{
	ui.setupUi(this);

	connect(ui.pushButtonActivate, &QPushButton::clicked, this, &LicenseDialog::onActivateClicked);
	connect(ui.pushButtonCancel, &QPushButton::clicked, this, &LicenseDialog::onCancelClicked);

	setWindowTitle("License Activation");

	// 显示本机硬件信息，供用户发送给供应商生成授权码
	ui.labelHardwareId->setText(QString::fromStdString(hardwareId));
	ui.labelHardwareHash->setText(QString::fromStdString(hardwareIdHash));

	ui.lineEditCode->clear();
	ui.lineEditCode->setPlaceholderText("XXXXX-XXXXX-XXXXX-XXXXX-XXXXX");

	if (expired)
	{
		ui.labelStatus->setText(
			"Your license has expired. Please contact your vendor to renew, "
			"then enter the new license code below.");
	}
	else
	{
		ui.labelStatus->setText(
			"This software is not activated. Please provide the Hardware ID and "
			"Hardware ID Hash below to your vendor, then enter the license code.");
	}

	inputCode = "";
	confirmed = false;
}

LicenseDialog::~LicenseDialog()
{
}

void LicenseDialog::onActivateClicked()
{
	inputCode = ui.lineEditCode->text().trimmed().toStdString();
	confirmed = true;
	close();
}

void LicenseDialog::onCancelClicked()
{
	confirmed = false;
	close();
}
