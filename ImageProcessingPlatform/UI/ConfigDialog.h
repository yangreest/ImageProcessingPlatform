#pragma once
#ifndef ConfigDialog_H
#define ConfigDialog_H

#include <ui_ConfigDialog.h>
#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <Config/ConfigManager.h>

class ConfigDialog : public QWidget
{
	Q_OBJECT

public:
	ConfigDialog(QWidget* parent = nullptr);
	~ConfigDialog();
	void UpdataConfig(CConfigManager* mCfig);

public slots:
	void onSaveButtonClicked();
	void onCloseButtonClicked();
	void onApplyButtonClicked();

protected:
	void showEvent(QShowEvent* event) override;

private:
	Ui::ConfigClass ui;
	void setupUI();
	// 从UI上获取数据
    void getDataFromUI();
	CConfigManager* m_mCfig;
};
#endif // ConfigDialog_H