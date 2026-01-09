#pragma once
#include <QKeyEvent>

class KeyEventFilter : public QObject
{
	Q_OBJECT

public:
	explicit KeyEventFilter(QObject* parent = nullptr);

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

signals:
	// 按键按下信号
	void upKeyPressed();
	void downKeyPressed();
	void leftKeyPressed();
	void rightKeyPressed();

	// 按键释放信号
	void upKeyReleased();
	void downKeyReleased();
	void leftKeyReleased();
	void rightKeyReleased();

	void ctrlAEvent();
};
