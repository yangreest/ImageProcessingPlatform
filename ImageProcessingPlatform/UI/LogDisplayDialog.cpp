#include "LogDisplayDialog.h"
#include <QFileDialog>

LogDisplayDialog::LogDisplayDialog(QWidget* parent)
	: QWidget(parent)
	, m_entryCount(0)
{
	ui.setupUi(this);
	setupUI();
	setupConnections();
}

LogDisplayDialog::~LogDisplayDialog()
{
}

void LogDisplayDialog::setupUI()
{
	// 将界面设置置顶
    setWindowFlags(Qt::WindowStaysOnTopHint);

	// 创建表格
	//m_logTable = new QTableWidget(this);
	ui.m_logTable->setColumnCount(7);
	ui.m_logTable->setHorizontalHeaderLabels(QStringList() << tr("序号") << tr("时间") << tr("级别") << tr("消息") << tr("文件名") << tr("函数名") << tr("行号"));
	ui.m_logTable->setSelectionBehavior(QAbstractItemView::SelectRows);  // 整行选择
	ui.m_logTable->setSelectionMode(QAbstractItemView::ExtendedSelection); // 支持多选
	ui.m_logTable->setAlternatingRowColors(true);
	ui.m_logTable->horizontalHeader()->setStretchLastSection(true);
	ui.m_logTable->verticalHeader()->setVisible(false);
	ui.m_logTable->setColumnWidth(0, 50);
	ui.m_logTable->setColumnWidth(3, 400);

}

void LogDisplayDialog::setupConnections()
{
	connect(ui.m_clearButton, &QPushButton::clicked, this, &LogDisplayDialog::onClearButtonClicked);
	connect(ui.m_saveButton, &QPushButton::clicked, this, &LogDisplayDialog::onSaveButtonClicked);
}

void LogDisplayDialog::addLogEntry(const QString& level, const QString& message, const QString& timestamp, const QString& file, const QString& function, const QString& line)
{
	// 对传入的
	// 设置级别（根据级别设置颜色）
	QTableWidgetItem* levelItem = new QTableWidgetItem(level);
	levelItem->setTextAlignment(Qt::AlignCenter);
	if (level.toUpper() == "[CRIT]") {
		if (!ui.cBError->isChecked()) return;
		levelItem->setBackground(QColor(255, 200, 200));  // 红色背景
		levelItem->setForeground(QColor(255, 200, 200));
	}
	else if (level.toUpper() == "[WARN]") {
        if (!ui.cBWaring->isChecked()) return;
		levelItem->setBackground(QColor(255, 255, 200));  // 黄色背景
        levelItem->setForeground(QColor(255, 255, 200));
	}
	else if (level.toUpper() == "[INFO]") {
        if (!ui.cBInfo->isChecked()) return;
		levelItem->setBackground(QColor(200, 255, 200));  // 绿色背景
        levelItem->setForeground(QColor(200, 255, 200));
	}
	else if(level.toUpper() == "[DEBUG]")
	{
		if (!ui.cBDebug->isChecked()) return;
	}

	int row = ui.m_logTable->rowCount();
	ui.m_logTable->insertRow(row);

	// 设置序号
	QTableWidgetItem* indexItem = new QTableWidgetItem(QString::number(++m_entryCount));
	indexItem->setTextAlignment(Qt::AlignCenter);
	ui.m_logTable->setItem(row, 0, indexItem);

	// 设置时间
	QTableWidgetItem* timeItem = new QTableWidgetItem(timestamp);
	timeItem->setTextAlignment(Qt::AlignCenter);
	ui.m_logTable->setItem(row, 1, timeItem);

	
	ui.m_logTable->setItem(row, 2, levelItem);

	// 设置消息
	QTableWidgetItem* messageItem = new QTableWidgetItem(message);
	ui.m_logTable->setItem(row, 3, messageItem);

	// 设置文件名
	QTableWidgetItem* fileItem = new QTableWidgetItem(file);
	ui.m_logTable->setItem(row, 4, fileItem);
	QTableWidgetItem* functionItem = new QTableWidgetItem(function);
	ui.m_logTable->setItem(row, 5, functionItem);
	QTableWidgetItem* lineItem = new QTableWidgetItem(line);
	ui.m_logTable->setItem(row, 6, lineItem);
	// 自动滚动到底部
	ui.m_logTable->scrollToBottom();
}

void LogDisplayDialog::clearLogs()
{
	ui.m_logTable->setRowCount(0);
	m_entryCount = 0;

}

void LogDisplayDialog::onClearButtonClicked()
{
	if (ui.m_logTable->rowCount() > 0) {
		QMessageBox::StandardButton reply = QMessageBox::question(
			this,
			tr("确认清空"),
			tr("确定要清空所有日志吗？"),
			QMessageBox::Yes | QMessageBox::No
		);

		if (reply == QMessageBox::Yes) {
			clearLogs();
		}
	}
}

void LogDisplayDialog::onSaveButtonClicked()
{
	QString fileName = QFileDialog::getSaveFileName(
		this,
		tr("保存日志文件"),
		QDir::homePath() + "/logs_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv",
		tr("文本文件 (*.csv);;所有文件 (*)")
	);

	if (!fileName.isEmpty()) {
		if (saveLogsToFile(fileName)) {
			QMessageBox::information(this, tr("成功"), tr("日志已保存到: %1").arg(fileName));
		}
		else {
			QMessageBox::critical(this, tr("错误"), tr("保存日志失败！"));
		}
	}
}

bool LogDisplayDialog::saveLogsToFile(const QString& fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return false;
	}

	QTextStream out(&file);

	// 写入表头
	out << "序号,时间,级别,消息,文件名,函数名,行号\n";

	// 写入日志数据
	for (int row = 0; row < ui.m_logTable->rowCount(); ++row) {
		QString index = ui.m_logTable->item(row, 0) ? ui.m_logTable->item(row, 0)->text() : "";
		QString time = ui.m_logTable->item(row, 1) ? ui.m_logTable->item(row, 1)->text() : "";
		QString level = ui.m_logTable->item(row, 2) ? ui.m_logTable->item(row, 2)->text() : "";
		QString message = ui.m_logTable->item(row, 3) ? ui.m_logTable->item(row, 3)->text() : "";
		QString file = ui.m_logTable->item(row, 4) ? ui.m_logTable->item(row, 4)->text() : "";
		QString function = ui.m_logTable->item(row, 5) ? ui.m_logTable->item(row, 5)->text() : "";
		QString line = ui.m_logTable->item(row, 6) ? ui.m_logTable->item(row, 6)->text() : "";
		out << index << "," << time << "," << level << "," << message << "," << file << "," << function << "," << line << "\n";
	}

	file.close();
	return true;
}
