#pragma once

#include <mutex>

#include "CameraFrom.h"
#include "ControlBoardConfigForm.h"
#include "ui_MainForm.h"
#include "SampleBoard/SampleBoardBase.h"
#include "KeyEventFilter.h"
#include "MouseEventFilter.h"
#include "Config/ConfigManager.h"
#include "DeviceCom/IDeviceCom.h"
#include "ImageProcessing/ImageProcessBase.h"
#include "Log/ScanS_FC.h"
#include "Log/ScanS_WriteLog.h"
#include "Model/SD_Raw.h"
#include "Model/ImageTag.h"
#include "Model/ImageCapture.h"
#include "Protocol/WHSDControlBoradProtocol.h"
#include "Tools/USBKey.h"

#define MaxControlBoardCount 2

class MainForm : public QMainWindow
{
	Q_OBJECT

public:
	MainForm(const std::string& guid, int model, QWidget* parent = nullptr);
	~MainForm() override;

private slots:
	void ConnectDevice();
	void On_timer_timeout();
	void On_Up_PressDown();
	void On_Up_Release();
	void On_Down_PressDown();
	void On_Down_Release();
	void On_Left_PressDown();
	void On_Left_Release();
	void On_Right_PressDown();
	void On_Right_Release();
	void On_ImgLabelMouseMove(Qt::MouseButton button, const QPoint& pos);
	void On_ImgLabelMousePress(Qt::MouseButton button, const QPoint& pos);
	void On_ImgLabelMouseRelease(Qt::MouseButton button, const QPoint& pos);
	void On_ImgLabelWheelUp();
	void On_ImgLabelWheelDown();
	void On_ImgRightDoubleClick();
	void On_ImgLeftDoubleClick();
	void On_SafeChanged_On();
	void On_SafeChanged_Stop();
	void On_SafeChanged_Off();
	void On_ManuallyTrigger_Clicked();
	void On_LoadPic_Clicked();
	void On_ControlOnly_Pressed();
	void On_ControlOnly_Released();
	void On_SampleBoard_Change_Clicked1();
	void On_SampleBoard_Change_Clicked2();
	void On_SampleBoard_Change_Stop();
	void On_NumberOfPulses_clicked();
	void On_DelayTime_clicked();
	void On_XRayStart_clicked();
	void On_XRayStop_clicked();
	void On_LeftRightMirror_clicked();
	void On_UpDownMirror_clicked();
	void On_TurnOnAll_Click();
	void On_TurnOffAll_Click();
	void On_Upgrade_DoubleClick();
	void On_TurnOnCamera_Click();
	void On_TurnOffCamera_Click();
	void On_ShowConfigFormClick();
	void On_SliderValueChanged2(int value);
	void On_SavePicByGuid_Click();
	void On_DeletePic_Click();
	void On_PreviousPic_Click();
	void On_NextPic_Click();
	void On_UploadAllPic_Click();
	void On_UploadAllPic_Click2();
	void On_ResetPic_Click();
	void On_SavePic_Click();
	void On_DeletePic_Click2();
	void On_Pic_Receive_Slots();
	void On_PreviousPic_Click2();
	void On_NextPic_Click2();
	void On_OnLineLoadFailed_Slots();
	void On_SaveRaw_Clicked();
	void On_SavePNG_Clicked();
	void On_SetFMode();
	void On_NoSetFMode();

	void On_DenoiseAndEnhance0();
	void On_Denoise1();
	void On_Denoise2();
	void On_Denoise3();
	void On_Enhance1();
	void On_Enhance2();
	void On_Enhance3();

	void On_DrawLine_Click();
	void On_Rect_Click();
	void On_Ellipse_Click();
	void On_MoveLast_Click();
	void On_InputText_Click();
	void On_Curvature_Click();
	void On_Angle_Click();

	void On_Capture_Click();
	void On_NoCapture_Click();
	void On_ImgReverse_Click();

	void On_RotateLeft_Click();
	void On_RotateRight_Click();

	void On_RecoverImgTag_Click();

	void On_deleteTag_Click();

	void On_SaveDealedPic();
signals:
	void On_Pic_Receive();
	void On_OnLineLoadSuccess();
	void On_OnLineLoadFailed();

protected:
	// 重写显示事件
	void showEvent(QShowEvent* event) override;

private:
	void On_SliderValueChanged();

	void ReadPicFromMem(int index);

	void Callback_ComDeviceConnectionChanged(bool connected, int guid, int nComdeviceIndex);

	void Callback_SampleBoardNewImg(uint8_t* data, int width, int height, int pixLen);

	void Callback_SampleBoardDeviceConnectStatus(DeviceConnectStatus s);

	void Callback_SampleBoardDeviceRunStatus(DeviceRunStatus s);

	void Callback_DeviceHeartBeat(const CDeviceHeartBeat& b, int nComdeviceIndex);

	void Callback_GetSampleBoardValue(DeviceValue v, double d);

	void Callback_OTAStatus(uint8_t, uint32_t, uint32_t);

	void Callback_XRaySendResult(uint8_t re);

	void InitUI(int model);

	void SetAllVis(bool v);

	void Set4SplitNotVisible(bool v);

	void BindAction();

	void InitParam();

	void SendData2ComDevice(uint8_t* p, int len);

	void PaintImg();

	QImage PaintTag();

	void UpdateMotorRunStatus(const CDeviceHeartBeat& c);

	void UpdateOTAStatus();

	void ReadSavedFiles(int index);

	void DownloadPic();

	bool CheckPassword();

	void Callback_ShowImgOnLabel_2();

	void ShowImgOnLabel();

	void ClearPicOpt();

	void AddOneImgTag(const CImageTag & t);

	void LoadOnlinePicByThread();

	std::string ChangeUIImg(const std::string& s, bool ac);

	Ui::MainFormClass ui;

	CameraFrom* m_pCameraFrom;

	CConfigManager* m_pConfig;

	KeyEventFilter* m_pKeyEventFilter;

	MouseEventFilter* m_pMouseEventFilter;

	QTimer* m_pTimer;

	IDeviceCom* m_apDeviceCom[MaxControlBoardCount];

	CWHSDControlBoardProtocol* m_apWHSDControlBoardProtocol[MaxControlBoardCount];

	bool m_abControlBroadConnected[MaxControlBoardCount];

	CDeviceHeartBeat m_amemDeviceHeartBeat[MaxControlBoardCount];

	uint64_t m_anHeartBeatCount[MaxControlBoardCount];

	CCFRD_Time m_atime_LastHeartBeatTime[MaxControlBoardCount];

	ISampleBoardBase* m_pSampleBoardBase;

	CImageProcessBase* m_pImageProcess;

	std::mutex m_mutexLastImgMutex;

	int m_nWifi;

	int m_nBattery;

	int m_nTemperature;

	bool m_bLeftRightMirror;

	bool m_bUpDownMirror;

	bool m_bControlPressed;// 是否按下了控制键 用于按住旋转图像

	bool m_bNeedRoteImg; // 是否需要旋转图片

	double m_dImgScale; // 显示图片的缩放比例

	int m_nRotate;

	int m_nImgXOffset;

	int m_nImgYOffset;

	int m_nLastMouseX;

	int m_nLastMouseY;

	std::mutex m_mutexDeviceInfoLock;

	std::vector<std::pair<std::string, std::string>> m_vecDeviceBaseInfo;

	DeviceConnectStatus m_enumDeviceConnectStatus;

	DeviceRunStatus m_enumDeviceRunStatus;

	CWriteLog* m_pDeviceLog;

	int m_nPicCount;

	int m_nShowImgTips;

	std::string m_strWorkGuid;

	int m_nWorkMode;

	int m_nPicIndex;

	uint8_t m_cOTAStatus;

	uint32_t m_nOTAAllPackNumber;

	uint32_t m_nOTAPackIndex;

	ControlBoardConfigForm* m_pControlBoard;

	CImageProcessParam1 m_memImageProcessParam1;

	CImageProcessParam2 m_memImageProcessParam2;

	CSD_Raw m_memSDRaw;

	CUSBKey* m_pUSBKey;

	uint64_t m_nXRaySendCout;

	/// <summary>
	/// 这个是用于显示的8bit灰度图
	/// </summary>
	std::vector<uint8_t> m_vecNeedShowBuffer;

	bool m_bNeedChangePicInfo;

	std::vector<std::string> m_vectorReadedPicPath;

	/// <summary>
	/// 已经加载好，等待用于处理的raw原图
	/// </summary>
	std::map<int, std::vector<uint8_t>> m_mapLoadedMap;

	std::vector<std::string> m_vector_NeedDownLoadPic;

	int m_nLoadedMapType;



	/// <summary>
	/// 已处理好的图片
	/// </summary>
	std::map<int, std::vector<uint8_t>> m_mapDealedPic;

	QImage m_memMainQImage;

	QImage m_memDealedImg;

	QImage m_memShowedImg;

	uint8_t m_cWorkModel;

	CUSBKeyData m_memUSBKeyData;

	/// <summary>
	/// 图像操作部分
	/// </summary>
	std::vector<CImageTag> m_vector_ImgTag;

	/// <summary>
	/// 用于恢复操作的
	/// </summary>
	std::vector<CImageTag> m_vector_ImgTag_bak;



	/// <summary>
	/// 鼠标的操作方式
	/// </summary>
	int m_nMouseMode;

	/// <summary>
	/// 鼠标操作模式下的点击计数
	/// </summary>
	int m_nMouseClickCount;

	/// <summary>
	/// 图片截图的位置
	/// </summary>
	CImageCapture m_memCImageCapture;

	bool m_bImgNeedReverse;

	bool m_bDownloadedPic;
};
