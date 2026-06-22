#include "CalibInputForm.h"
#include <QCloseEvent>
#include <Tools/Tools.h>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>

  float CalibInputForm::fRatio =0.0f;

CalibInputForm::CalibInputForm(const int pexLenght, QWidget* parent)
	: CFormWithShowModal(parent)
{
	ui.setupUi(this);
	// 设置窗口模态性：只阻塞父窗口及其子窗口
	connect(ui.pushButton, &QPushButton::clicked, this, &CalibInputForm::onCloseButtonClicked);
	connect(ui.pushButton_2, &QPushButton::clicked, this, &CalibInputForm::onYesButtonClicked);
	setWindowTitle(tr("尺寸标定"));
	ui.lineEdit->setText(QString::number(pexLenght));
	//result = 0;
	QString jsonPath = QString("%1\\calibration.json").arg(WHSD_Tools::GetExeDirectory());
	LoadJsonFile(jsonPath);
}

CalibInputForm::~CalibInputForm()
{
}

void CalibInputForm::LoadJsonFile(QString filePath)
{
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly)) {
		qDebug() << "Failed to open file: " << filePath;
		return;
	}

	QByteArray data = file.readAll();
	QJsonDocument doc = QJsonDocument::fromJson(data);

	int pexl = doc.object()["Pxel"].toInt();
	float real_pxel = doc.object()["Real_Pxel"].toDouble();
	fRatio = doc.object()["fRatio"].toDouble();

	//ui.lineEdit->setText(QString::number(pexl));
	ui.lineEdit_2->setText(QString::number(real_pxel));
}

void CalibInputForm::SaveJsonFile(int pexl, float real_pexl)
{
	//将数据保存到JSON文件中
    QJsonObject obj;
    obj["Pxel"] = pexl;
    obj["Real_Pxel"] = real_pexl;
	fRatio = real_pexl / pexl;
    obj["fRatio"] = fRatio;
    QJsonDocument doc(obj);
	QString jsonPath = QString("%1\\calibration.json").arg(WHSD_Tools::GetExeDirectory());
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
    qDebug() << "Data saved to JSON file: " << jsonPath;
}


void CalibInputForm::onCloseButtonClicked()
{
	// 关闭窗口
	close();
}

void CalibInputForm::onYesButtonClicked()
{
	// 将数据保存到json文件中
	SaveJsonFile(ui.lineEdit->text().toInt(), ui.lineEdit_2->text().toFloat());
	close();
}
