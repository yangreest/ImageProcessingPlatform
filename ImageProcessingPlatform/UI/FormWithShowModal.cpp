#include "FormWithShowModal.h"
#include <QCloseEvent>


CFormWithShowModal::CFormWithShowModal(QWidget* parent ):QMainWindow(parent)
{
	setWindowModality(Qt::ApplicationModal);

	this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint);
	QIcon appIcon("./1.png");
	if (!appIcon.isNull())
	{
		setWindowIcon(appIcon);
	}
}

CFormWithShowModal::~CFormWithShowModal()
{
}

void CFormWithShowModal::showModal()
{
	QEventLoop loop;
	m_eventLoop = &loop;

	// 显示窗口
	show();

	// 进入事件循环，阻塞当前线程
	loop.exec();

	// 事件循环退出后重置指针
	m_eventLoop = nullptr;
}

void CFormWithShowModal::closeEvent(QCloseEvent* event)
{
	// 接受关闭事件
	event->accept();

	// 如果事件循环存在，则退出事件循环
	if (m_eventLoop)
	{
		m_eventLoop->quit();
		qDebug() << "窗口关闭，退出事件循环";
	}
	QMainWindow::closeEvent(event);
}
