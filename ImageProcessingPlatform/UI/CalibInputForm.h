#pragma once

#include <QMainWindow>

#include "FormWithShowModal.h"
#include "ui_CalibInputForm.h"

class  CalibInputForm : public CFormWithShowModal
{
	Q_OBJECT

public:
	 CalibInputForm(const int pexLenght, QWidget* parent = nullptr);
	~ CalibInputForm();
	// 计算的比例
	static float fRatio;

private:
	Ui:: CalibInputFormClass ui;
	void LoadJsonFile(QString filePath);
	void SaveJsonFile(int pexl ,float real_pexl);

signals:
	// 窗口关闭时发射的信号
	void windowClosed();

private slots:
	// 关闭按钮点击事件
	void onCloseButtonClicked();
	void onYesButtonClicked();

};
