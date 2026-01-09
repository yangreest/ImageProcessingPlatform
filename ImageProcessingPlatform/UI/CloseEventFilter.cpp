#include "CloseEventFilter.h"

CloseEventFilter::CloseEventFilter(QObject* parent) : QObject(parent)
{
}

bool CloseEventFilter::eventFilter(QObject* obj, QEvent* event)
{
	{
		if (event->type() == QEvent::Close)
		{
			emit windowAboutToClose();
			// 不阻止默认关闭行为（若要阻止，调用 event->ignore() 并返回 true）
			return false;
		}
		return QObject::eventFilter(obj, event);
	}
}