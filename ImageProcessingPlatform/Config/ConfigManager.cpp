#include "ConfigManager.h"
#include "../Tools/tinyxml2.h"

CControlBoardConfig::CControlBoardConfig()
{
	m_strIp = "";
	m_wPort = 0;
	m_wDeviceHeartBeat = 200;
	m_bFactoryMode = false;
}

void CConfigManager::Read(const std::string& filePath)
{
	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError eResult = doc.LoadFile(filePath.c_str());
	int nIntTemp = 0;
	if (eResult == tinyxml2::XML_SUCCESS)
	{
		auto config = doc.RootElement();
		if (config != nullptr)
		{
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
				}
			}
		}
	}
}