#include "SingleInputForm.h"
#include <QCloseEvent>

int SingleInputForm::result = 0;

SingleInputForm::SingleInputForm(const std::string& title, QWidget* parent)
	: CFormWithShowModal(parent)
{
	ui.setupUi(this);
	// 设置窗口模态性：只阻塞父窗口及其子窗口
	connect(ui.pushButton, &QPushButton::clicked, this, &SingleInputForm::onCloseButtonClicked);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &SingleInputForm::onYesButtonClicked);
	setWindowTitle(QString::fromStdString(title));
	result = 0;
}

SingleInputForm::~SingleInputForm()
{
}


void SingleInputForm::onCloseButtonClicked()
{
	// 关闭窗口
	close();
}

void SingleInputForm::onYesButtonClicked()
{
	result = ui.lineEdit->text().toInt();
	close();
}
