#pragma once
#ifndef LOGDISPLAYDIALOG_H
#define LOGDISPLAYDIALOG_H

#include <ui_LogDisplayDialog.h>
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

class LogDisplayDialog : public QWidget
{
	Q_OBJECT

public:
	LogDisplayDialog(QWidget* parent = nullptr);
	~LogDisplayDialog();

	// 添加日志条目
	void addLogEntry(const QString& level, const QString& message, const QString& timestamp, const QString& file, const QString& function, const QString& line);
	//void addLogEntry(const QString& level, const QString& message, const QDateTime& timestamp, const QString& file, const QString& function, const QString& line);

	// 清空所有日志
	void clearLogs();

	// 保存日志到文件
	bool saveLogsToFile(const QString& fileName);

public slots:
	void onClearButtonClicked();
	void onSaveButtonClicked();
	//void onCopySelectedRows();

private:
	Ui::LogDisplayClass ui;
	void setupUI();
	void setupConnections();
	int m_entryCount;
};

#endif // LOGDISPLAYDIALOG_H