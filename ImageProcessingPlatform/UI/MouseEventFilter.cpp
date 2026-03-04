#include "MouseEventFilter.h"

MouseEventFilter::MouseEventFilter(QObject* parent) : QObject(parent)
{
}

bool MouseEventFilter::eventFilter(QObject* obj, QEvent* event)
{
	switch (event->type())
	{
	case QEvent::MouseMove:
	{
		auto mouseEvent = (QMouseEvent*)(event);
		emit mousePositionChanged(mouseEvent->button(), mouseEvent->pos());
		return false; // 继续处理事件
	}
	case QEvent::MouseButtonPress:

	{
		auto mouseEvent = (QMouseEvent*)(event);
		emit mousePressed(mouseEvent->button(), mouseEvent->pos());
		return false; // 继续处理事件
	}
	case QEvent::MouseButtonRelease:
	{
		auto mouseEvent = (QMouseEvent*)(event);
		emit mouseReleased(mouseEvent->button(), mouseEvent->pos());
		return false; // 继续处理事件
	}
	case QEvent::Wheel:
	{
		auto wheelEvent = dynamic_cast<QWheelEvent*>(event);
		auto mouseEvent = (QMouseEvent*)(event);
		// Qt 5.14+ 推荐方法
		if (wheelEvent->angleDelta().y() > 0)
		{
			emit wheelUp(mouseEvent->pos());
		}
		else
		{
			emit wheelDown(mouseEvent->pos());
		}
		return false; // 继续处理事件
	}
	case QEvent::MouseButtonDblClick:
	{
		QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
		switch (mouseEvent->button())
		{
		case Qt::RightButton:
		{
			emit rightDoubleClicked();
			break;
		}
		case Qt::LeftButton:
		{
			emit leftDoubleClicked();
			break;
		}
		default:
		{
			break;
		}
		}
		return false; // 继续处理事件
	}
	default:
	{
		break;
	}
	}
	return QObject::eventFilter(obj, event);
}