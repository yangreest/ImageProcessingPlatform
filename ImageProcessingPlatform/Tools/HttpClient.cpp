#include "HttpClient.h"
#include <QCoreApplication>
#include <QTimer>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QEventLoop>
#include <QByteArray>


std::vector<uchar>  HttpClient::DownloadFileToVector(const std::string& urlStringStd)
{
	std::vector<uchar> result;
	QUrl url(QString::fromStdString(urlStringStd));
	if (!url.isValid()) {
		return result; // URL无效
	}

	QNetworkAccessManager manager;
	QNetworkRequest request(url);
	QNetworkReply* reply = manager.get(request);

	// 使用事件循环实现同步下载
	QEventLoop loop;
	QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	// 检查下载是否成功
	if (reply->error() == QNetworkReply::NoError) {
		QByteArray data = reply->readAll();
		// 转换为std::vector<uchar>
		result.assign(data.begin(), data.end());
	}

	reply->deleteLater();
	return result;
}

std::tuple<bool, std::string, std::string> HttpClient::Get(
	const std::string& url,
	const std::vector<std::pair<std::string, std::string>>& query_params,
	const std::vector<std::pair<std::string, std::string>>& headers,
	int timeout_ms)
{
	// -------------------- 步骤 1：构造完整 URL（含查询参数） --------------------
	QUrl request_url(QString::fromStdString(url));

	// 添加查询参数（自动编码键值）
	if (!query_params.empty())
	{
		QUrlQuery url_query;
		for (const auto& param : query_params)
		{
			QString key = QString::fromStdString(param.first);
			QString value = QString::fromStdString(param.second);
			url_query.addQueryItem(key, value); // 自动处理特殊字符编码（如空格→%20）
		}
		request_url.setQuery(url_query); // 覆盖原 URL 中的查询参数（如需追加需额外处理）
	}

	// 校验 URL 有效性
	if (!request_url.isValid())
	{
		return std::tuple<bool, std::string, std::string>(false, "", "Invalid URL format");
	}

	// -------------------- 步骤 2：初始化网络组件 --------------------
	QNetworkAccessManager manager; // 栈上分配，自动释放
	QNetworkRequest request(request_url);

	// 设置自定义请求头（如 User-Agent、Authorization）
	for (const auto& header : headers)
	{
		QString key = QString::fromStdString(header.first);
		QString value = QString::fromStdString(header.second);
		request.setRawHeader(key.toUtf8(), value.toUtf8());
	}

	// -------------------- 步骤 3：发送 GET 请求（同步阻塞） --------------------
	QNetworkReply* reply = manager.get(request); // 发送 GET 请求

	// 事件循环：阻塞等待请求完成或超时
	QEventLoop loop;
	bool is_timeout = false;

	// 连接请求完成信号（无论成功/失败都会触发）
	QObject::connect(reply, &QNetworkReply::finished, [&]()
		{
			loop.quit(); // 退出事件循环
		});

	// 超时处理（防止永久阻塞）
	QTimer timeout_timer;
	QObject::connect(&timeout_timer, &QTimer::timeout, [&]()
		{
			is_timeout = true;
			reply->abort(); // 强制终止请求
			loop.quit(); // 退出事件循环
		});
	timeout_timer.start(timeout_ms); // 启动超时计时器

	// 阻塞当前线程，等待请求完成或超时
	loop.exec();

	// -------------------- 步骤 4：处理响应结果 --------------------
	if (is_timeout)
	{
		reply->deleteLater();
		return std::tuple<bool, std::string, std::string>(false, "", "Request timed out");
	}

	if (reply->error() != QNetworkReply::NoError)
	{
		// 网络错误（如 DNS 失败、连接拒绝等）
		reply->deleteLater();
		return std::tuple<bool, std::string, std::string>(
			false, "", "HTTP Error: " + reply->errorString().toStdString());
	}

	// 检查 HTTP 状态码是否为 2xx（成功）
	int http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (http_status < 200 || http_status >= 300)
	{
		reply->deleteLater();
		return std::tuple<bool, std::string,
			std::string>(false, "", "HTTP Status Code: " + std::to_string(http_status));
	}

	// 成功：读取响应数据（转换为 UTF-8 字符串）
	reply->deleteLater();
	return std::tuple<bool, std::string, std::string>(true, QByteArray(reply->readAll()).toStdString(), "");
}

std::tuple<bool, std::string, std::string> HttpClient::Post(const std::string& url, const QString& post_data,
	const std::string& content_type,
	const std::vector<std::pair<QString, QString>>&
	headers, int timeout_ms)
{
	// -------------------- 步骤 1：校验 URL 有效性 --------------------
	QUrl request_url(QString::fromStdString(url));
	if (!request_url.isValid())
	{
		return std::tuple<bool, std::string, std::string>(false, "", "Invalid URL format");
	}

	// -------------------- 步骤 2：初始化网络组件 --------------------
	QNetworkAccessManager manager; // 栈上分配，自动释放
	QNetworkRequest request(request_url);

	// 设置请求体类型（Content-Type）
	request.setHeader(QNetworkRequest::ContentTypeHeader, QString::fromStdString(content_type));

	// 设置自定义请求头（如 User-Agent、Authorization）
	for (const auto& header : headers)
	{
		request.setRawHeader(header.first.toUtf8(), header.second.toUtf8());
	}

	// -------------------- 步骤 3：发送 POST 请求（同步阻塞） --------------------
	QNetworkReply* reply = manager.post(request, post_data.toUtf8());

	// 事件循环：阻塞等待请求完成或超时
	QEventLoop loop;
	bool is_timeout = false;

	// 连接请求完成信号（无论成功/失败都会触发）
	QObject::connect(reply, &QNetworkReply::finished, [&]()
		{
			loop.quit(); // 退出事件循环
		});

	// 超时处理（防止永久阻塞）
	QTimer timeout_timer;
	QObject::connect(&timeout_timer, &QTimer::timeout, [&]()
		{
			is_timeout = true;
			reply->abort(); // 强制终止请求
			loop.quit(); // 退出事件循环
		});
	timeout_timer.start(timeout_ms); // 启动超时计时器

	// 阻塞当前线程，等待请求完成或超时
	loop.exec();

	// -------------------- 步骤 4：处理响应结果 --------------------
	if (is_timeout)
	{
		return std::tuple<bool, std::string, std::string>(false, "", "Request timed out");
	}

	if (reply->error() != QNetworkReply::NoError)
	{
		// 网络错误（如 DNS 失败、连接拒绝等）
		return std::tuple<bool, std::string, std::string>(
			false, "", "HTTP Error: " + reply->errorString().toStdString());
	}

	// 检查 HTTP 状态码是否为 2xx（成功）
	int http_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (http_status < 200 || http_status >= 300)
	{
		return std::tuple<bool, std::string,
			std::string>(false, "", "HTTP Status Code: " + std::to_string(http_status));
	}

	return std::tuple<bool, std::string, std::string>(true, QByteArray(reply->readAll()).toStdString(), "");
}