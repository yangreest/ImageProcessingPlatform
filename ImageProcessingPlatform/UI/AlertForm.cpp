#include "AlertForm.h"

AlertForm::AlertForm(const std::string & title, const std::string & content, QWidget *parent)
	: CFormWithShowModal(parent)
{
	ui.setupUi(this);
	setWindowTitle(QString::fromStdString(title));
	ui.label->setText(QString::fromStdString(content));
	connect(ui.pushButton, &QPushButton::clicked, this, &AlertForm::onCloseButtonClicked);
}

AlertForm::~AlertForm()
{}

void AlertForm::onCloseButtonClicked()
{
	close();
}

