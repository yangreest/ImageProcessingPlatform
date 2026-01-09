#pragma once
#include <QString>
#include <string>
#include <vector>

namespace HttpClient
{
	std::tuple<bool, std::string, std::string> Get(
		const std::string& url,
		const std::vector<std::pair<std::string, std::string>>& query_params = {},
		const std::vector<std::pair<std::string, std::string>>& headers = {},
		int timeout_ms = 3000);
	std::tuple<bool, std::string, std::string> Post(
		const std::string& url,
		const QString& post_data,
		const std::string& content_type = "application/json",
		const std::vector<std::pair<QString, QString>>& headers = {},
		int timeout_ms = 3000);

	std::vector<uchar> DownloadFileToVector(const std::string& urlString);
}
