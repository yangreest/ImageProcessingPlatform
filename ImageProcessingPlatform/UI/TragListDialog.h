#pragma once
#ifndef TRAGLISTDIALOG_H
#define TRAGLISTDIALOG_H

#include <ui_TragListDialog.h>
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

class TragListDialog : public QWidget
{
	Q_OBJECT

public:
	TragListDialog(QWidget* parent = nullptr);
	~TragListDialog();

	// 显示日志
	void ShowTrag();

private:
	Ui::TragListClass ui;
	void setupUI();
};

#endif // LOGDISPLAYDIALOG_H