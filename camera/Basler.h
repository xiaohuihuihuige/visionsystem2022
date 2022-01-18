//Basler.h
//MVLZ ShenZhen MiaoWei 2016.11.30 仅供参考，如有疑问，请及时联系我 miaow@mvlz.com

#pragma once

// Include files to use the PYLON API.
#include <pylon/PylonIncludes.h>
#include <pylon/gige/BaslerGigEInstantCamera.h>

// Namespace for using pylon objects.
using namespace Pylon;
// Namespace for using cout.
using namespace std;

//注意:: 此例程针对basler gige 接口相机使用，如果是其它接口相机，个别函数调用会有差别或者不能使用
// 其它接口相机使用，请参结合本例子，参考basler安装目录下例子c++ ParametrizeCamera_NativeParameterAccess或
typedef Pylon::CBaslerGigEInstantCamera Camera_t;
using namespace Basler_GigECameraParams;
#include <pylon/PylonGUI.h>

typedef void (WINAPI *BaslerGrabbedCallback)(void* pOwner, int* width, int* height, unsigned char* pBuffer, bool isColor);

// COnImaggedTestDlg 对话框
class CBaslerGigeNative: public CImageEventHandler,public Pylon::CConfigurationEventHandler
{
    // 构造
public:
    CBaslerGigeNative();	// 标准构造函数
    ~CBaslerGigeNative();

public:
    Camera_t camera;
    bool IsAcquisitionStartAvail;
    bool IsFrameStartStartAvail;
    bool IsCameraRemoved;
    void*			             m_pOwner;
    BaslerGrabbedCallback        m_DisplayImageCallBack;
    INT64        width;		// 图像宽
    INT64       height;		// 图像高
    bool       isColor;		// 判断是否彩色相机
    unsigned char * pImageBufferMono;	// 自定义黑白图像指针
    unsigned char * pImageBufferColor;	// 自定义彩色图像指针

public:
    //以下两个函数，即使是多相机情况，也只能调用一次
    void PylonLibInit();
    void PylonLibTerminate();
    //以下函数可以针对每一个相机使用
    BOOL OpenDevice(); //open first device by default;
    BOOL OpenDeviceBySn(string serialNumber); // open camera by specialized serial number
    BOOL OpenDeviceByUserID(string UserDefinedName);// open camera by user defined name
    BOOL OpenDeviceByIPAddress(string IPAddress, string subnetMask);//open camera by specilaized IP address
    void RemovedDeviceReconnect();// if device is removed, use this function to find it and connect it again
    void SetHeartbeatTimeout(int64_t valueMs); //ms, 这个函数对于pylon5.0.5并包含此版本是有bug的，暂时不要使用
    void CloseDevice();
    void GrabOne();
    void StartGrab();
    void StopGrab();
    void SetFreerunMode();
    void SetSoftwareTriggerMode();
    void SendSoftwareTriggerCommand();
    void SetExternalTriggerMode();
    void SetTriggerDelay(double nTimeUs);
    void SetLineDebouncerTimeAbs(double dTimeUs);
    void SetExposureTimeRaw(int64_t nExpTimeUs);
    void SetGainRaw(int64_t nGainRaw);
    void SetAcquisitionFrameRate(double fps);

    void SetOwner(void* pOwner, BaslerGrabbedCallback pDisplayImageCallBack);

    virtual void OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult);//included in CImageEventHandler
    //virtual void OnImagesSkipped( CInstantCamera& camera, size_t countOfSkippedImages); //included in CConfigurationEventHandler
    virtual void OnCameraDeviceRemoved(CInstantCamera& /*camera*/); //included in CImageEventHandler

};
