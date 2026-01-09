#pragma once

#include <QMainWindow>
#include <QEventLoop>

class CFormWithShowModal : public QMainWindow
{
public:
	CFormWithShowModal(QWidget* parent = nullptr);
	~CFormWithShowModal();
	void showModal();
private:

	QEventLoop* m_eventLoop;
protected:
	// 重写关闭事件处理函数
	void closeEvent(QCloseEvent* event) override;
};


