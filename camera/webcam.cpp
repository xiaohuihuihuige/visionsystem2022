#include "webcam.h"
#include<QFile>

WebCam::WebCam(QObject *parent) : QObject(parent)
{
    _cam_name="";
    _pTimer=nullptr;
    triggerDelay=50;
    cam_list.clear();
    GetSerialNum(cam_list);
}
WebCam::~WebCam()
{
    closeDevice();
}
bool WebCam::closeDevice()
{
    if(_pTimer!=nullptr)
    {
        _pTimer->stop();
        delete _pTimer;
        _pTimer=nullptr;
    }
    if(_pCam!=nullptr)
    {
        _pCam->release();
        delete _pCam;
        _pCam=nullptr;
    }
    return true;
}

bool WebCam::initCamera(QString camName,QString cameraModelAndSerial,QString resolution, QString triggerModel, QString exp,QString delay)
{
    try
    {
        triggerDelay=delay.toInt();
        _cam_name=camName;
        QString serial_up=cameraModelAndSerial.toUpper();
        int  cam_num=-1;
        if(cameraModelAndSerial.length()<3)
        {
            cam_num=cameraModelAndSerial.toInt();
        }
        else
        {
            for (int var = 0; var < cam_list.size(); var++)
            {
                if(cam_list[var]==serial_up)
                {
                    cam_num=var;
                    break;
                }
            }
        }
        if(cam_num==-1)return false;

        bool isopen=openWebCam(cam_num,resolution,triggerModel.toInt());
        if(isopen)
            setExp(exp);
        return isopen;
    }
    catch (cv::Exception e)
    {
        return false;
    }
}

bool WebCam::openWebCam(int camID,QString resolution,int triggerModel)
{
    try
    {
        _pCam=new cv::VideoCapture(camID);
        QStringList sizelist=resolution.split(':');
        if (sizelist.size()<2)return false;
        _pCam->set(cv::CAP_PROP_FRAME_WIDTH, sizelist[0].toInt());
        _pCam->set(cv::CAP_PROP_FRAME_HEIGHT, sizelist[1].toInt());
        if(triggerModel==0)
        {
            _pTimer =new QTimer(this);
            connect(_pTimer,&QTimer::timeout,this,&WebCam::doSoftwareOnce);
            _pTimer->start(triggerDelay);
        }
        else if (triggerModel==1)
        {

        }
        return _pCam->isOpened();
    }
    catch (cv::Exception e)
    {
        return false;
    }
}

bool WebCam::setExp(QString exp)
{
    if(exp.toInt()==0)
    {
        _pCam->set(cv::CAP_PROP_AUTO_EXPOSURE,1);
    }
    else
    {

        _pCam->set(cv::CAP_PROP_AUTO_EXPOSURE,0.25);
        _pCam->set(cv::CAP_PROP_EXPOSURE,exp.toDouble());
    }
    return true;
}

void WebCam::doSoftwareOnce()
{
    cv::Mat src;
    *_pCam>>src;
    QImage image=cvMat2QImage(src);
    emit getOneFrame(image,_cam_name);
}
QImage WebCam::cvMat2QImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        return QImage();
    }
}


#define INITGUID
#ifdef DEFINE_DEVPROPKEY
#undef DEFINE_DEVPROPKEY
#endif
#ifdef INITGUID
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY name
#endif // INITGUID

DEFINE_DEVPROPKEY(DEVPKEY_Device_BusReportedDeviceDesc, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 4);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_ContainerId, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c, 2);     // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_Device_FriendlyName, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 14);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_DeviceDisplay_Category, 0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57, 0x5a);  // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_SecuritySDS, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 26);    // DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING
#define GUID_CAMERA_STRING L"{65e8773d-8f56-11d0-a3b9-00a0c9223196}"
#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

typedef BOOL(WINAPI* FN_SetupDiGetDevicePropertyW)(
        __in       HDEVINFO DeviceInfoSet,
        __in       PSP_DEVINFO_DATA DeviceInfoData,
        __in       const DEVPROPKEY* PropertyKey,
        __out      DEVPROPTYPE* PropertyType,
        __out_opt  PBYTE PropertyBuffer,
        __in       DWORD PropertyBufferSize,
        __out_opt  PDWORD RequiredSize,
        __in       DWORD Flags
        );

void WebCam::GetSerialNum(QVector <QString>& list)
{

    FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
            GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");
    QFile outFile("camlist.txt");
    //添加记录到文件
    outFile.open(QIODevice::WriteOnly);

    USES_CONVERSION;
    GUID guid;
    CLSIDFromString(GUID_CAMERA_STRING, &guid);

    HDEVINFO devInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    WCHAR szDeviceInstanceId[200];

    if (devInfo != INVALID_HANDLE_VALUE)
    {
        DWORD devIndex = 0;
        SP_DEVINFO_DATA devInfoData;
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        // 获取设备容器ID
        DWORD dwSize;
        DEVPROPTYPE ulPropertyType;
        TCHAR szDesc[1024];
        WCHAR szBuffer[2048];
        QString usbCameraPath, usbCameraContainerID;

        while (SetupDiEnumDeviceInfo(devInfo, devIndex, &devInfoData))
        {
            memset(szDeviceInstanceId, 0, 200);
            SetupDiGetDeviceInstanceIdW(devInfo, &devInfoData, szDeviceInstanceId, MAX_PATH, 0);

            //usbCameraPath = WcharToString(szDeviceInstanceId);
            //std::cout << WcharToString(szDeviceInstanceId) << std::endl;
            usbCameraPath=QString::fromWCharArray(szBuffer);

            if (fn_SetupDiGetDevicePropertyW(devInfo, &devInfoData, &DEVPKEY_Device_ContainerId,
                                             &ulPropertyType, (BYTE*)szDesc, sizeof(szDesc), &dwSize, 0))
            {
                StringFromGUID2((REFGUID)szDesc, szBuffer, ARRAY_SIZE(szBuffer));
                //usbCameraContainerID = WcharToString(szBuffer);
                usbCameraContainerID = QString::fromWCharArray(szBuffer);
                //QString ret2 = QString((QChar*)szBuffer, wcslen(szBuffer));
                //std::cout <<" ContainerId: "<< usbCameraContainerID << std::endl;
            }
            //如果时系统相机或者显示适配器，直接跳过
            if(usbCameraContainerID.toUpper()!="{00000000-0000-0000-FFFF-FFFFFFFFFFFF}")
            {
                //auto tuple = make_tuple(usbCameraPath, usbCameraContainerID);
                if(!list.contains(usbCameraContainerID))
                {
                    list.push_back(usbCameraContainerID);
                    outFile.write(QString( usbCameraContainerID+"\n").toLocal8Bit());
                }
            }
            devIndex++;
        }
        outFile.close();
    }
}

std::string WebCam::WcharToString(LPCWSTR pwszsrc)
{
    int nLen = WideCharToMultiByte(CP_ACP, 0, pwszsrc, -1, NULL, 0, NULL, NULL);
    if (nLen <= 0)
        return std::string("");

    char* pszDst = new char[nLen];
    if (NULL == pszDst)
        return std::string("");

    WideCharToMultiByte(CP_ACP, 0, pwszsrc, -1, pszDst, nLen, NULL, NULL);
    pszDst[nLen - 1] = 0;

    std::string strTmp(pszDst);
    delete[] pszDst;

    return strTmp;
}
