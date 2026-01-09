#pragma once

#include <QEvent>
#include <QMainWindow>

class CloseEventFilter : public QObject
{
	Q_OBJECT

public:
	explicit CloseEventFilter(QObject* parent = nullptr);

signals:
	void windowAboutToClose();

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;
};
