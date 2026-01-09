#pragma once

#include <QMouseEvent>

class MouseEventFilter : public QObject
{
	Q_OBJECT

public:
	explicit MouseEventFilter(QObject* parent = nullptr);

signals:
	void mousePositionChanged(Qt::MouseButton button, const QPoint& pos);
	void mousePressed(Qt::MouseButton button, const QPoint& pos);
	void mouseReleased(Qt::MouseButton button, const QPoint& pos);
	void wheelUp();
	void wheelDown();
	void leftDoubleClicked();
	void rightDoubleClicked();

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;
};
