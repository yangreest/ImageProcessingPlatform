#ifndef __DEBUG__
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <filesystem>
#include "Tools/Tools.h"
#endif


#include "./UI/MainForm.h"
#include <QtWidgets/QApplication>

#define ProgramName "Star_XRay_Drone_Image_HMI"

int main(int argc, char* argv[])
{
	// 1. 注册自定义消息处理函数（必须在创建QApplication之后、使用qDebug之前）
	qInstallMessageHandler(customMessageHandler);
	//保证程序实例只会启动一次
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, TEXT(ProgramName));
	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
		return FALSE;
	}

	std::string guid;
	int model = 1;

	if (argc > 2)
	{
		guid = argv[1];
		model = std::atoi(argv[2]);
	}

	//guid = "99100045037";
	//model = 2;

	QApplication app(argc, argv);
	MainForm window(guid, model);
#ifdef __DEBUG__
	qDebug() << "Console allocated at runtime!";
#endif
	window.show();
	return app.exec();
}