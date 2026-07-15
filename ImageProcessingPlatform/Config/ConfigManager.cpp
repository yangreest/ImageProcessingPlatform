#include "ConfigManager.h"
#include "../Tools/tinyxml2.h"
#include <qDebug.h>

CControlBoardConfig::CControlBoardConfig()
{
	m_strIp = "";
	m_wPort = 0;
	m_wDeviceHeartBeat = 200;
	m_bFactoryMode = false;
}

CSampleBoardConfig::CSampleBoardConfig()
{
	m_nManufacturer = 0;
	m_nModel = 0;
	m_nMapType = 0;
	m_nExposureType = 0;
}

void CConfigManager::Read(const std::string& filePath)
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError eResult = doc.LoadFile(filePath.c_str());
	int nIntTemp = 0;
	if (eResult != tinyxml2::XML_SUCCESS)
		return;
	auto config = doc.RootElement();
	if (config == nullptr)
		return;
	{
		auto deviceBoard = config->FirstChildElement("DeviceControlBoard");
		if (deviceBoard != nullptr)
		{
			auto ipElement = deviceBoard->FirstChildElement("Ip");
			if (ipElement && ipElement->GetText())
			{
				m_memControlBoardConfig.m_strIp = ipElement->GetText();
			}
			auto portElement = deviceBoard->FirstChildElement("Port");
			if (portElement != nullptr && portElement->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memControlBoardConfig.m_wPort = nIntTemp;
			}
			auto ipElement2 = deviceBoard->FirstChildElement("Ip2");
			if (ipElement2 && ipElement2->GetText())
			{
				m_memControlBoardConfig.m_strIp2 = ipElement2->GetText();
			}
			auto portElement2 = deviceBoard->FirstChildElement("Port2");
			if (portElement2 != nullptr && portElement2->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memControlBoardConfig.m_wPort2 = nIntTemp;
			}
			auto ht = deviceBoard->FirstChildElement("DeviceHeartBeat");
			if (ht != nullptr && ht->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memControlBoardConfig.m_wDeviceHeartBeat = nIntTemp;
			}

			auto dm = deviceBoard->FirstChildElement("FactoryMode");
			if (dm != nullptr && dm->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memControlBoardConfig.m_bFactoryMode = nIntTemp > 0;
			}
			auto d2m = deviceBoard->FirstChildElement("EnableIp2");
			if (d2m != nullptr && d2m->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memControlBoardConfig.m_bEnableIp2 = nIntTemp > 0;
			}
		}
	}
	{
		auto img = config->FirstChildElement("ImageProcess");
		if (img != nullptr)
		{
			auto iType = img->FirstChildElement("Type");
			if (iType != nullptr && iType->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCImageProcessConfig.m_nType = nIntTemp;
			}
			auto nRawFile = img->FirstChildElement("RawFile");
			if (nRawFile != nullptr && nRawFile->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCImageProcessConfig.m_nRawFile = nIntTemp;
			}

			auto rawFileNode = img->FirstChildElement("RawFileSize");
			if (rawFileNode != nullptr)
			{
				auto sizeElement = rawFileNode->FirstChildElement("Size");
				while (sizeElement != nullptr)
				{
					CRawFileSize info;

					// 提取FileSize
					const tinyxml2::XMLElement* fileSizeElem = sizeElement->FirstChildElement("FileSize");
					if (fileSizeElem && fileSizeElem->GetText())
					{
						info.m_nFileSize = std::stoull(fileSizeElem->GetText());
					}

					// 提取RawFileWidth
					const tinyxml2::XMLElement* widthElem = sizeElement->FirstChildElement("RawFileWidth");
					if (widthElem && widthElem->GetText())
					{
						info.m_nRawFileWidth = std::stoi(widthElem->GetText());
					}

					// 提取RawFileHeight
					const tinyxml2::XMLElement* heightElem = sizeElement->FirstChildElement("RawFileHeight");
					if (heightElem && heightElem->GetText())
					{
						info.m_nRawFileHeight = std::stoi(heightElem->GetText());
					}

					// 添加到vector
					m_memCImageProcessConfig.m_vec_RawFileSize.push_back(info);
					sizeElement = sizeElement->NextSiblingElement("Size");
				}
			}

			auto UploadPic = img->FirstChildElement("UploadPic");
			if (UploadPic && UploadPic->GetText())
			{
				m_memTimsConfig.m_strUploadPic = UploadPic->GetText();
			}

			auto DownloadPic = img->FirstChildElement("DownloadPic");
			if (DownloadPic && DownloadPic->GetText())
			{
				m_memTimsConfig.m_strDownloadPic = DownloadPic->GetText();
			}

			auto GetGuidInfo = img->FirstChildElement("GetGuidInfo");
			if (GetGuidInfo && GetGuidInfo->GetText())
			{
				m_memTimsConfig.m_strGetGuidInfo = GetGuidInfo->GetText();
			}


			auto dm = img->FirstChildElement("DownLoadTimeOut");
			if (dm != nullptr && dm->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memTimsConfig.m_nDownloadTimeOut = nIntTemp;
			}
		}
	}

	{
		auto sb = config->FirstChildElement("SampleBoard");
		if (sb != nullptr)
		{
			auto nManufacturer = sb->FirstChildElement("Manufacturer");
			if (nManufacturer != nullptr && nManufacturer->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCSampleBoardConfig.m_nManufacturer = nIntTemp;
			}

			auto nModel = sb->FirstChildElement("Model");
			if (nModel != nullptr && nModel->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCSampleBoardConfig.m_nModel = nIntTemp;
			}

			auto nMapType = sb->FirstChildElement("MapType");
			if (nMapType != nullptr && nMapType->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCSampleBoardConfig.m_nMapType = nIntTemp;
			}

			auto nExposureType = sb->FirstChildElement("ExposureType");
			if (nExposureType != nullptr && nExposureType->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCSampleBoardConfig.m_nExposureType = nIntTemp;
			}

			auto nExposureTime = sb->FirstChildElement("ExposureTime");
			if (nExposureTime != nullptr && nExposureTime->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memCSampleBoardConfig.m_nExposureTime = nIntTemp;
			}
		}
	}
	{
		auto sb = config->FirstChildElement("Camera");
		if (sb != nullptr)
		{
			auto nLeft = sb->FirstChildElement("Left");
			if (nLeft != nullptr)
			{
				m_memCCameraConfig.m_strLeftIp = nLeft->GetText();
			}

			auto nMid = sb->FirstChildElement("Mid");
			if (nMid != nullptr)
			{
				m_memCCameraConfig.m_strMidIp = nMid->GetText();
			}

			auto nRight = sb->FirstChildElement("Right");
			if (nRight != nullptr)
			{
				m_memCCameraConfig.m_strRightIp = nRight->GetText();
			}
		}
	}
	{
		auto sb = config->FirstChildElement("Tims");
		if (sb != nullptr)
		{
			auto nForceGuid = sb->FirstChildElement("ForceGuid");
			if (nForceGuid != nullptr && nForceGuid->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memTimsConfig.m_nForceGuid = nIntTemp;
			}
			auto nLanguage = sb->FirstChildElement("Language");
			if (nLanguage != nullptr && nLanguage->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_memTimsConfig.nLanguage = nIntTemp;
			}
		}
	}
	{
		auto sb = config->FirstChildElement("TcpClient");
		if (sb != nullptr)
		{
			auto ipElement = sb->FirstChildElement("Ip");
			if (ipElement && ipElement->GetText())
			{
				m_mCTcpClientParam.m_strIp = ipElement->GetText();
			}

			auto nPort = sb->FirstChildElement("Port");
			if (nPort != nullptr && nPort->QueryIntText(&nIntTemp) == tinyxml2::XML_SUCCESS)
			{
				m_mCTcpClientParam.m_nPort = nIntTemp;
			}
		}
	}
}

void CConfigManager::Write(const std::string& filePath)
{
	tinyxml2::XMLDocument doc;
	if (filePath.empty())
	{
		qWarning() << "No config file path specified";
		return;
	}
	// 创建根节点
	tinyxml2::XMLElement* root = doc.NewElement("Config");
	doc.InsertEndChild(root);
	{
		tinyxml2::XMLElement* deviceBoard = root->InsertNewChildElement("DeviceControlBoard");

		tinyxml2::XMLElement* dBIP = deviceBoard->InsertNewChildElement("Ip");
		dBIP->SetText(m_memControlBoardConfig.m_strIp.c_str());
		tinyxml2::XMLElement* dBPort = deviceBoard->InsertNewChildElement("Port");
		dBPort->SetText(m_memControlBoardConfig.m_wPort);
		tinyxml2::XMLElement* dBIP2 = deviceBoard->InsertNewChildElement("Ip2");
		dBIP2->SetText(m_memControlBoardConfig.m_strIp2.c_str());
		tinyxml2::XMLElement* dBPort2 = deviceBoard->InsertNewChildElement("Port2");
		dBPort2->SetText(m_memControlBoardConfig.m_wPort2);
		tinyxml2::XMLElement* dBDeviceHeartBeat = deviceBoard->InsertNewChildElement("DeviceHeartBeat");
		dBDeviceHeartBeat->SetText(m_memControlBoardConfig.m_wDeviceHeartBeat);
		tinyxml2::XMLElement* dBFactroyMode = deviceBoard->InsertNewChildElement("FactoryMode");
		dBFactroyMode->SetText(m_memControlBoardConfig.m_bFactoryMode ? 1 : 0);
		tinyxml2::XMLElement* dBEnableIp2 = deviceBoard->InsertNewChildElement("EnableIp2");
		dBEnableIp2->SetText(m_memControlBoardConfig.m_bEnableIp2 ? 1 : 0);
	}
	{
		tinyxml2::XMLElement* img = root->InsertNewChildElement("ImageProcess");
		tinyxml2::XMLElement* nRawFile = img->InsertNewChildElement("RawFile");
		nRawFile->SetText(m_memCImageProcessConfig.m_nRawFile);
		tinyxml2::XMLElement* nType = img->InsertNewChildElement("Type");
		nType->SetText(m_memCImageProcessConfig.m_nType);
		tinyxml2::XMLElement* RawFileSizeElement = img->InsertNewChildElement("RawFileSize");
		for (auto& info : m_memCImageProcessConfig.m_vec_RawFileSize)
		{
			tinyxml2::XMLElement* sizeElement = RawFileSizeElement->InsertNewChildElement("Size");
			sizeElement->InsertNewChildElement("RawFileHeight")->SetText(info.m_nRawFileHeight);
			sizeElement->InsertNewChildElement("RawFileWidth")->SetText(info.m_nRawFileWidth);
			sizeElement->InsertNewChildElement("FileSize")->SetText(info.m_nFileSize);
		}
		tinyxml2::XMLElement* nUploadPic = img->InsertNewChildElement("UploadPic");
		nUploadPic->SetText(m_memTimsConfig.m_strUploadPic.c_str());
		tinyxml2::XMLElement* DownloadPic = img->InsertNewChildElement("DownloadPic");
		DownloadPic->SetText(m_memTimsConfig.m_strDownloadPic.c_str());
		tinyxml2::XMLElement* GetGuidInfo = img->InsertNewChildElement("GetGuidInfo");
		GetGuidInfo->SetText(m_memTimsConfig.m_strGetGuidInfo.c_str());
		tinyxml2::XMLElement* nGetGuidInfo = img->InsertNewChildElement("DownLoadTimeOut");
		nGetGuidInfo->SetText(m_memTimsConfig.m_nDownloadTimeOut);
	}

	{
		tinyxml2::XMLElement* sb = root->InsertNewChildElement("SampleBoard");
		tinyxml2::XMLElement* nManufacturer = sb->InsertNewChildElement("Manufacturer");
		nManufacturer->SetText(m_memCSampleBoardConfig.m_nManufacturer);
		tinyxml2::XMLElement* nModel = sb->InsertNewChildElement("Model");
		nModel->SetText(m_memCSampleBoardConfig.m_nModel);
		tinyxml2::XMLElement* nMapType = sb->InsertNewChildElement("MapType");
		nMapType->SetText(m_memCSampleBoardConfig.m_nMapType);
		tinyxml2::XMLElement* nExposureType = sb->InsertNewChildElement("ExposureType");
		nExposureType->SetText(m_memCSampleBoardConfig.m_nExposureType);
		tinyxml2::XMLElement* nExposureTime = sb->InsertNewChildElement("ExposureTime");
		nExposureTime->SetText(m_memCSampleBoardConfig.m_nExposureTime);
	}

	{
		tinyxml2::XMLElement* sb = root->InsertNewChildElement("Camera");
		tinyxml2::XMLElement* nLeft = sb->InsertNewChildElement("Left");
		nLeft->SetText(m_memCCameraConfig.m_strLeftIp.c_str());
		tinyxml2::XMLElement* nMid = sb->InsertNewChildElement("Mid");
		nMid->SetText(m_memCCameraConfig.m_strMidIp.c_str());
		tinyxml2::XMLElement* nRight = sb->InsertNewChildElement("Right");
		nRight->SetText(m_memCCameraConfig.m_strRightIp.c_str());
	}

	{
		tinyxml2::XMLElement* sb = root->InsertNewChildElement("Tims");
		tinyxml2::XMLElement* nForceGuid = sb->InsertNewChildElement("ForceGuid");
		nForceGuid->SetText(m_memTimsConfig.m_nForceGuid);
		tinyxml2::XMLElement* nLanguage = sb->InsertNewChildElement("Language");
		nLanguage->SetText(m_memTimsConfig.nLanguage);
	}

	{
		tinyxml2::XMLElement* sb = root->InsertNewChildElement("TcpClient");
		tinyxml2::XMLElement* nIp = sb->InsertNewChildElement("Ip");
		nIp->SetText(m_mCTcpClientParam.m_strIp.c_str());
		tinyxml2::XMLElement* nPort = sb->InsertNewChildElement("Port");
		nPort->SetText(m_mCTcpClientParam.m_nPort);
	}

	doc.SaveFile(filePath.c_str());
}