#include "TragListDialog.h"
#include <QFileDialog>

TragListDialog::TragListDialog(QWidget* parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	setupUI();
}

TragListDialog::~TragListDialog()
{
}

void TragListDialog::setupUI()
{
	// 将界面设置置顶
    setWindowFlags(Qt::WindowStaysOnTopHint);
}
