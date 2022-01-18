#include "hikvisioncamera.h"

HikvisionCamera::HikvisionCamera(QObject *parent) :
    QObject(parent),
    m_pcMyCamera(nullptr)
{
    useCamID=-1;
    isCameraInit=false;
    isGrabStart=false;
}

HikvisionCamera::~HikvisionCamera()
{
    closeDevice();
}
//回调函数取图
void __stdcall HikvisionCamera::ImageCallBackEx(unsigned char * pData, MV_FRAME_OUT_INFO_EX* pFrameInfo, void* pUser)
{
    HikvisionCamera *pcam = (HikvisionCamera*)(pUser);
    if (pFrameInfo)
    {
        QImage image,image1;
        if(pFrameInfo->enPixelType==PixelType_Gvsp_Mono8)
        {
            //灰度图转化
            image=QImage(pData, pFrameInfo->nWidth, pFrameInfo->nHeight, QImage::Format_Indexed8);
            image1=image.copy(QRect(0,0,image.width(),image.height()));
        }
        else
        {
            //彩色图转化
            unsigned char *pDataForRGB = NULL;
            pDataForRGB = (unsigned char*)malloc(pFrameInfo->nWidth * pFrameInfo->nHeight * 3);
            if (NULL == pDataForRGB)
            {
                return;
            }
            unsigned int nDataSizeForRGB = pFrameInfo->nWidth * pFrameInfo->nHeight * 3;

            // ch:像素格式转换 | en:Convert pixel format
            MV_CC_PIXEL_CONVERT_PARAM stConvertParam = {0};
            memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

            stConvertParam.nWidth = pFrameInfo->nWidth;                 //ch:图像宽 | en:image width
            stConvertParam.nHeight = pFrameInfo->nHeight;               //ch:图像高 | en:image height
            stConvertParam.pSrcData = pData;                            //ch:输入数据缓存 | en:input data buffer
            stConvertParam.nSrcDataLen = pFrameInfo->nFrameLen;         //ch:输入数据大小 | en:input data size
            stConvertParam.enSrcPixelType = pFrameInfo->enPixelType;    //ch:输入像素格式 | en:input pixel format
            stConvertParam.enDstPixelType = PixelType_Gvsp_RGB8_Packed; //ch:输出像素格式 | en:output pixel format
            stConvertParam.pDstBuffer = pDataForRGB;                    //ch:输出数据缓存 | en:output data buffer
            stConvertParam.nDstBufferSize = nDataSizeForRGB;            //ch:输出缓存大小 | en:output buffer size
            pcam->m_pcMyCamera->ConvertPixelType(&stConvertParam);
            image=QImage(stConvertParam.pDstBuffer, pFrameInfo->nWidth, pFrameInfo->nHeight, QImage::Format_RGB888);
            image1=image.copy(QRect(0,0,image.width(),image.height()));
            free(pDataForRGB);
        }
        emit pcam->getOneFrame(image1,pcam->m_CamName);
    }
}



//获取相机列表 型号:序列号
int HikvisionCamera::getCamList(QVector<QString> *listDeviceName)
{
    int aaaa=0;
    aaaa=MvCamera::GetSDKVersion();
    int nRet= MvCamera::EnumDevices(&m_stDevList);
    if (MV_OK != nRet)
    {
        return STATUS_ERROR;
    }
    for (unsigned int i = 0; i < m_stDevList.nDeviceNum; i++)
    {
        MV_CC_DEVICE_INFO* pDeviceInfo = m_stDevList.pDeviceInfo[i];
        if (nullptr == pDeviceInfo)
        {
            return STATUS_ERROR;
        }
        QString deviceName;
        if(pDeviceInfo->nTLayerType == MV_GIGE_DEVICE)
        {
            for (int j=0;j<16;j++)
            {
                deviceName += char(pDeviceInfo->SpecialInfo.stGigEInfo.chModelName[j]);
            }
            deviceName=deviceName.mid(0,deviceName.indexOf(QChar('\0')));
            deviceName+=":";
            for (int var = 0; var < 16; var++)
            {
                deviceName +=char(pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber[var]);
            }
        }
        else if (pDeviceInfo->nTLayerType == MV_USB_DEVICE)
        {
            for (int j=0;j<16;j++)
            {
                deviceName += char(pDeviceInfo->SpecialInfo.stUsb3VInfo.chModelName[j]);
            }
            deviceName=deviceName.mid(0,deviceName.indexOf(QChar('\0')));
            deviceName+=":";
            for (int var = 0; var < 16; var++)
            {
                deviceName +=char(pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber[var]);
            }
        }
        deviceName=deviceName.mid(0,deviceName.indexOf(QChar('\0')));
        (*listDeviceName)<<deviceName;
    }
    return MV_OK;
}

//根据相机ID开启相机
int HikvisionCamera::openDevice(int camID)
{
    if (true == isCameraInit)
    {
        return -1;
    }
    int nRet = MV_OK;
    if (nullptr == m_pcMyCamera)
    {
        m_pcMyCamera = new MvCamera;
    }
    nRet = m_pcMyCamera->Open(m_stDevList.pDeviceInfo[useCamID]);
    if (MV_OK != nRet)
    {
        delete(m_pcMyCamera);
        m_pcMyCamera = nullptr;
        return nRet;
    }
    else
    {
        // ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
        if (m_stDevList.pDeviceInfo[useCamID]->nTLayerType == MV_GIGE_DEVICE)
        {
            unsigned int nPacketSize = 0;
            nRet = m_pcMyCamera->GetOptimalPacketSize(&nPacketSize);
            if (nPacketSize > 0)
            {
                nRet = m_pcMyCamera->SetIntValue("GevSCPSPacketSize",nPacketSize);
                if(nRet != MV_OK)
                {
                    return 1;
                }
                isCameraInit=true;
            }
            else
            {
                return 2;
            }
        }
    }
    return MV_OK;
}
//关闭设备
int HikvisionCamera::closeDevice()
{
    int nRet = MV_OK;
    if(isGrabStart)
    {
        stopGrabbing();
    }
    if(isCameraInit)
    {
        nRet = m_pcMyCamera->Close();
    }
    isGrabStart = false;
    isCameraInit = false;
    delete m_pcMyCamera;
    m_pcMyCamera=nullptr;
    return nRet;
}

// ch:设置触发模式 | en:Set trigger mode  0为连续，1为触发
int HikvisionCamera::setTriggerMode(QString triggerModel,QString triggerSource,QString triggerDelay)
{
    if (false == isCameraInit)
    {
        return STATUS_ERROR;
    }
    int nRet = MV_OK;

    if(triggerModel.toInt()==0)
    {
        nRet = m_pcMyCamera->SetEnumValue("TriggerMode", TRIGGER_OFF);
    }
    else
    {
        nRet = m_pcMyCamera->SetEnumValue("TriggerMode", TRIGGER_ON);
        nRet = m_pcMyCamera->SetEnumValue("TriggerSource", triggerSource.toInt());
        if(triggerSource.toInt()!=7)
        {
            nRet = m_pcMyCamera->SetEnumValue("TriggerActivation", 0);
        }
        nRet=setTriggerDelay(triggerDelay.toInt());
        if(nRet!=0)return false;
    }
    return MV_OK;
}


// ch:设置曝光时间 | en:Set exposure time
int HikvisionCamera::setExposureTime(float expTime)
{
    if (false == isCameraInit)
    {
        return STATUS_ERROR;
    }
    int nRet = MV_OK;
    nRet = m_pcMyCamera->SetEnumValue("ExposureMode", 0);
    nRet = m_pcMyCamera->SetFloatValue("ExposureTime", expTime);
    return nRet;
}
// ch:设置曝光时间 | en:Set exposure time
int HikvisionCamera::setTriggerDelay(int delay)
{
    if (false == isCameraInit)
    {
        return STATUS_ERROR;
    }
    int nRet = MV_OK;
    nRet = m_pcMyCamera->SetFloatValue("TriggerDelay", delay*1000);
    return nRet;
}


//获得曝光值
float HikvisionCamera::getExposureTime()
{
    if (false == isCameraInit)
    {
        return STATUS_ERROR;
    }
    int nRet = MV_OK;
    MVCC_FLOATVALUE floatStruct;
    float expos;
    nRet=m_pcMyCamera->GetFloatValue("ExposureTime", &floatStruct);
    expos=floatStruct.fCurValue;
    return expos;
}

// ch:设置增益 | en:Set gain
int HikvisionCamera::setGain(float gainValue)
{
    if (false == isCameraInit)
    {
        return STATUS_ERROR;
    }
    int nRet = MV_OK;
    nRet = m_pcMyCamera->SetEnumValue("GainAuto", 0);
    nRet = m_pcMyCamera->SetFloatValue("Gain", gainValue);
    return nRet;
}

// ch:获得增益
float HikvisionCamera::getGain()
{
    if (false == isCameraInit)
    {
        return -1;
    }
    int nRet = MV_OK;
    MVCC_FLOATVALUE floatStruct;
    float gain;
    nRet=m_pcMyCamera->GetFloatValue("Gain", &floatStruct);
    gain=floatStruct.fCurValue;
    return gain;
}

// ch:软触发一次 | en:Software trigger once
int HikvisionCamera::doSoftwareOnce()
{
    if (false == isCameraInit)
    {
        return 1;
    }
    int nRet = MV_OK;
    nRet = m_pcMyCamera->CommandExecute("TriggerSoftware");
    return nRet;
}

// ch:开始采集 | en:Start grabbing
int HikvisionCamera::startGrabbing()
{
    if (false == isCameraInit || true == isGrabStart)
    {
        return 1;
    }
    int nRet = MV_OK;
    nRet = m_pcMyCamera->RegisterImageCallBack(ImageCallBackEx, this);
    if (MV_OK != nRet)
    {
        return nRet;
    }
    nRet = m_pcMyCamera->StartGrabbing();
    if(nRet!=MV_OK)
    {
        return nRet;
    }
    isGrabStart = true;
    return MV_OK;
}

// ch:结束采集 | en:stop grabbing
int HikvisionCamera::stopGrabbing()
{
    if (false == isGrabStart)
    {
        return STATUS_ERROR;
    }
    int nRet = MV_OK;
    nRet = m_pcMyCamera->StopGrabbing();
    if(nRet!=MV_OK)
    {
        return nRet;
    }
    isGrabStart = false;
    return nRet;
}

//初始化相机，匹配型号和序列号-》获得相机ID-》打开相机-》设置出发模式-》设置曝光-》开启采集
bool HikvisionCamera::initCamera(QString camName,QString cameraModelAndSerial,QString triggerModel,QString triggerSource,QString exp,QString gain,QString delay)
{
    QVector<QString> cameraList;
    getCamList(&cameraList);
    useCamID=-1;
    for (int var = 0; var < cameraList.size(); var++)
    {
        if(cameraModelAndSerial==cameraList[var])
        {
            useCamID=var;
        }
    }
    if(useCamID==-1)
    {
        return false;
    }
    int ret;
    ret=openDevice(useCamID);
    if(ret!=0)return false;
    ret=setTriggerMode(triggerModel,triggerSource,delay);
    if(ret!=0)return false;

    ret=setGain(gain.toFloat());
    if(ret!=0)return false;
    ret=setExposureTime(exp.toFloat());
    if(ret!=0)return false;

    ret=startGrabbing();
    if(ret!=0)return false;
    m_CamName=camName;
    return true;
}






