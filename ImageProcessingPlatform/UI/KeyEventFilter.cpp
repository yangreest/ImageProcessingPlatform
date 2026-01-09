#include "KeyEventFilter.h"

KeyEventFilter::KeyEventFilter(QObject* parent) : QObject(parent)
{
}

bool KeyEventFilter::eventFilter(QObject* obj, QEvent* event)
{
	//键盘的事件处理有问题，这段代码屏蔽掉
	switch (event->type())
	{
	case QEvent::KeyPress:
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		switch (keyEvent->key())
		{
		case Qt::Key_Up:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit upKeyPressed();
			return true;
		}
		case Qt::Key_Down:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit downKeyPressed();
			return true;
		}
		case Qt::Key_Left:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit leftKeyPressed();
			return true;
		}
		case Qt::Key_Right:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit rightKeyPressed();
			return true;
		}
		case Qt::Key_A:
		{
			if (keyEvent->modifiers() == Qt::ControlModifier)
			{
				emit ctrlAEvent();
			}
			return true;
		}
		default:
			break;
		}
		break;
	}
	case QEvent::KeyRelease:
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
		switch (keyEvent->key())
		{
		case Qt::Key_Up:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit upKeyReleased();
			return true;
		}
		case Qt::Key_Down:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit downKeyReleased();
			return true;
		}
		case Qt::Key_Left:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit leftKeyReleased();
			return true;
		}
		case Qt::Key_Right:
		{
			if (keyEvent->isAutoRepeat())
			{
				return true; // 事件被过滤掉，不再继续传播
			}
			emit rightKeyReleased();
			return true;
		}
		default:
			break;
		}
		break;
	}
	}

	// 继续处理其他类型的事件
	return QObject::eventFilter(obj, event);
}