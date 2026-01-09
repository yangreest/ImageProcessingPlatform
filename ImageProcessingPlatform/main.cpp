#ifdef _DEBUG
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
#ifdef _DEBUG

	AllocConsole();
	freopen("CONOUT$", "w", stdout); // 重定向标准输出到控制台
	std::cout << "Console allocated at runtime!" << std::endl;
	auto tp=WHSD_Tools::GetExeDirectory();
#endif

	//保证程序实例只会启动一次
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, ProgramName);
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

	//guid = "40990";
	//model = 2;

	QApplication app(argc, argv);
	MainForm window(guid, model);
	window.show();
	return app.exec();
}