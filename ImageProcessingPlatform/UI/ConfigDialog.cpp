#include "ConfigDialog.h"
#include <QFileDialog>
#include <Tools/Tools.h>

ConfigDialog::ConfigDialog(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	// 将界面设置置顶
	setWindowFlags(Qt::WindowStaysOnTopHint);
	setupUI();
	connect(ui.pushButton_3, &QPushButton::clicked, this, &ConfigDialog::onApplyButtonClicked);
    connect(ui.pushButton, &QPushButton::clicked, this, &ConfigDialog::onCloseButtonClicked);
    connect(ui.pushButton_2, &QPushButton::clicked, this, &ConfigDialog::onSaveButtonClicked);
}

ConfigDialog::~ConfigDialog()
{
}

void ConfigDialog::UpdataConfig(CConfigManager* mCfig)
{
	m_mCfig = mCfig;
}

void ConfigDialog::onApplyButtonClicked()
{
	getDataFromUI();
    m_mCfig->Write(WHSD_Tools::GetAbsolutePath("Config.xml"));
}

void ConfigDialog::showEvent(QShowEvent* event)
{
	setupUI();
}

void ConfigDialog::onCloseButtonClicked()
{
    close();
}

 void ConfigDialog::onSaveButtonClicked()
 {
	 onApplyButtonClicked();
	 close();
 }

void ConfigDialog::setupUI()
{
	if (!m_mCfig)
		return;
	if (m_mCfig->m_memTimsConfig.nLanguage == 1)
	{
		ui.radioButton->setChecked(true);
	}
	else
	{
		ui.radioButton_2->setChecked(true);
	}
}

void ConfigDialog::getDataFromUI()
{
    m_mCfig->m_memTimsConfig.nLanguage = ui.radioButton->isChecked() ? 1 : 0;
}


