#include "dahuacamera.h"

#define DEFAULT_ERROR_STRING ("N/A")

using namespace Dahua::GenICam;
using namespace Dahua::Infra;
DaHuaCamera::DaHuaCamera(QObject *parent) : QObject(parent)
{

}
DaHuaCamera::~DaHuaCamera()
{

}

// 取流回调函数
// get frame callback function
void DaHuaCamera::FrameCallback(const CFrame& frame)
{
    CFrameInfo frameInfo;
    frameInfo.m_nWidth = frame.getImageWidth();
    frameInfo.m_nHeight = frame.getImageHeight();
    frameInfo.m_nBufferSize = frame.getImageSize();
    frameInfo.m_nPaddingX = frame.getImagePadddingX();
    frameInfo.m_nPaddingY = frame.getImagePadddingY();
    frameInfo.m_ePixelType = frame.getImagePixelFormat();
    frameInfo.m_pImageBuf = (BYTE *)malloc(sizeof(BYTE)* frameInfo.m_nBufferSize);
    frameInfo.m_nTimeStamp = frame.getImageTimeStamp();

    // 内存申请失败，直接返回
    // memory application failed, return directly
    if (frameInfo.m_pImageBuf != NULL)
    {
        memcpy(frameInfo.m_pImageBuf, frame.getImage(), frame.getImageSize());
    }
    QImage image,image1;
    if (Dahua::GenICam::gvspPixelMono8 == frameInfo.m_ePixelType)
    {
        image = QImage(frameInfo.m_pImageBuf,frameInfo.m_nWidth, frameInfo.m_nHeight, QImage::Format_Indexed8);
        image1=image.copy(QRect(0,0,image.width(),image.height()));
    }
    else
    {
        // 转码
        uint8_t *pRGBbuffer = nullptr;
        int nRgbBufferSize = 0;
        nRgbBufferSize = frameInfo.m_nWidth * frameInfo.m_nHeight * 3;
        pRGBbuffer = (uint8_t *)malloc(nRgbBufferSize);
        if (pRGBbuffer == nullptr)
        {
            // 释放内存
            free(frameInfo.m_pImageBuf);
        }
        IMGCNV_SOpenParam openParam;
        openParam.width = frameInfo.m_nWidth;
        openParam.height = frameInfo.m_nHeight;
        openParam.paddingX = frameInfo.m_nPaddingX;
        openParam.paddingY = frameInfo.m_nPaddingY;
        openParam.dataSize = frameInfo.m_nBufferSize;
        openParam.pixelForamt = frameInfo.m_ePixelType;
        IMGCNV_EErr status = IMGCNV_ConvertToBGR24(frameInfo.m_pImageBuf, &openParam, pRGBbuffer, &nRgbBufferSize);
        image = QImage(pRGBbuffer,frameInfo.m_nWidth, frameInfo.m_nHeight, QImage::Format_RGB888);
        image1=image.copy(QRect(0,0,image.width(),image.height()));
        free(pRGBbuffer);
    }
    free(frameInfo.m_pImageBuf);
    emit getOneFrame(m_camname,image1);
}



// 设置曝光
// set exposeTime
bool DaHuaCamera::SetExposeTime(double dExposureTime)
{
    if (NULL == m_pCamera)
    {
        return false;
    }
    CDoubleNode nodeExposureTime(m_pCamera, "ExposureTime");
    if (false == nodeExposureTime.isValid())
    {
        return false;
    }
    if (false == nodeExposureTime.isAvailable())
    {
        return false;
    }
    if (false == nodeExposureTime.setValue(dExposureTime))
    {
        return false;
    }
    return true;
}

// 设置增益
// set gain
bool DaHuaCamera::SetAdjustPlus(double dGainRaw)
{
    if (NULL == m_pCamera)
    {
        return false;
    }

    CDoubleNode nodeGainRaw(m_pCamera, "GainRaw");

    if (false == nodeGainRaw.isValid())
    {
        return false;
    }

    if (false == nodeGainRaw.isAvailable())
    {
        return false;
    }

    if (false == nodeGainRaw.setValue(dGainRaw))
    {
        return false;
    }
    return true;
}
// 打开相机
// open camera
bool DaHuaCamera::CameraOpen(void)
{
    if (NULL == m_pCamera)
    {
        return false;
    }
    if (true == m_pCamera->isConnected())
    {
        return true;
    }
    if (false == m_pCamera->connect())
    {
        return false;
    }
    return true;
}
// 关闭相机
// close camera
bool DaHuaCamera::CameraClose(void)
{
    if (NULL == m_pCamera)
    {
        return false;
    }
    if (false == m_pCamera->isConnected())
    {
        return true;
    }
    if (false == m_pCamera->disConnect())
    {
        return false;
    }
    return true;
}

// 开始采集
// start grabbing
bool DaHuaCamera::CameraStart()
{
    if (NULL == m_pCamera)
    {
        return false;
    }
    if (NULL == m_pStreamSource)
    {
        m_pStreamSource = CSystem::getInstance().createStreamSource(m_pCamera);
    }
    if (NULL == m_pStreamSource)
    {
        return false;
    }
    if (m_pStreamSource->isGrabbing())
    {
        return true;
    }
    bool bRet = m_pStreamSource->attachGrabbing(IStreamSource::Proc(&DaHuaCamera::FrameCallback, this));
    if (!bRet)
    {
        return false;
    }
    if (!m_pStreamSource->startGrabbing())
    {
        return false;
    }
    return true;
}

// 停止采集
// stop grabbing
bool DaHuaCamera::CameraStop()
{
    if (m_pStreamSource != NULL)
    {
        m_pStreamSource->detachGrabbing(IStreamSource::Proc(&DaHuaCamera::FrameCallback, this));
        m_pStreamSource->stopGrabbing();
        m_pStreamSource.reset();
    }
    return true;
}

// 切换采集方式、触发方式 （连续采集、外部触发、软件触发）
// Switch acquisition mode and triggering mode (continuous acquisition, external triggering and software triggering)
// ch:设置触发模式 | en:Set trigger mode  0为连续，1为触发
bool DaHuaCamera::CameraChangeTrig( QString triggerModel,QString triggerSource)
{
    if (NULL == m_pCamera)
    {
        return false;
    }

    if (triggerModel.toInt() == 0)
    {
        // 设置触发模式
        // set trigger mode
        CEnumNode nodeTriggerMode(m_pCamera, "TriggerMode");
        if (false == nodeTriggerMode.isValid())
        {
            return false;
        }
        if (false == nodeTriggerMode.setValueBySymbol("Off"))
        {
            return false;
        }
    }
    else
    {
        // 设置触发器
        // set trigger
        CEnumNode nodeTriggerSelector(m_pCamera, "TriggerSelector");
        if (false == nodeTriggerSelector.isValid())
        {
            return false;
        }
        if (false == nodeTriggerSelector.setValueBySymbol("FrameStart"))
        {
            return false;
        }
        // 设置触发模式
        // set trigger mode
        CEnumNode nodeTriggerMode(m_pCamera, "TriggerMode");
        if (false == nodeTriggerMode.isValid())
        {
            return false;
        }
        if (false == nodeTriggerMode.setValueBySymbol("On"))
        {
            return false;
        }
        if (triggerSource.toInt() == 7)
        {
            // 设置触发源为软触发
            // set triggerSource as software trigger
            CEnumNode nodeTriggerSource(m_pCamera, "TriggerSource");
            if (false == nodeTriggerSource.isValid())
            {
                return false;
            }
            if (false == nodeTriggerSource.setValueBySymbol("Software"))
            {
                return false;
            }
        }
        else
        {
            // 设置触发源为Line1触发
            // set trigggerSource as Line1 trigger
            CEnumNode nodeTriggerSource(m_pCamera, "TriggerSource");
            if (false == nodeTriggerSource.isValid())
            {
                return false;
            }
            QString linestr=QString("Line%1").arg(triggerSource);
            if (false == nodeTriggerSource.setValueBySymbol(linestr.toStdString().data()))
            {
                return false;
            }
        }
    }
    return true;
}

// 执行一次软触发
// execute one software trigger
bool DaHuaCamera::ExecuteSoftTrig(void)
{
    if (NULL == m_pCamera)
    {
        return false;
    }
    CCmdNode nodeTriggerSoftware(m_pCamera, "TriggerSoftware");
    if (false == nodeTriggerSoftware.isValid())
    {
        return false;
    }
    if (false == nodeTriggerSoftware.execute())
    {
        return false;
    }
    return true;
}



// 设置当前相机
// set current camera
void DaHuaCamera::SetCamera(const QString& strKey)
{
    QStringList camlist= getCameraList();
    if(camlist.contains(strKey.toStdString().c_str()))
    {
        CSystem &systemObj = CSystem::getInstance();
        m_pCamera = systemObj.getCameraPtr(strKey.toStdString().c_str());
    }
}
//获取相机列表
QStringList DaHuaCamera::getCameraList()
{
    QStringList cameraList;
    CSystem &systemObj = CSystem::getInstance();
    if (false == systemObj.discovery(m_vCameraPtrList))
    {
        return QStringList();
    }
    if (m_vCameraPtrList.size() < 1)
    {
        return QStringList();
    }
    else
    {
        for (int i = 0; i < m_vCameraPtrList.size(); i++)
        {
            cameraList<<m_vCameraPtrList[i]->getKey();
        }
        return cameraList;
    }
}
bool DaHuaCamera::setPixelFormat(int type)
{
    CEnumNode nodePixelFormat(m_pCamera, "PixelFormat");
    if (false == nodePixelFormat.isValid())
    {
        return false;
    }
    bool setsuccess=false;
    switch (type)
    {
    case 0:
        setsuccess= nodePixelFormat.setValueBySymbol("Mono8");
        break;
    case 1:
        setsuccess= nodePixelFormat.setValueBySymbol("BayerRG10");
        break;
    }
    return setsuccess;

}
bool DaHuaCamera::setDelay(double delay)
{
    if (NULL == m_pCamera)
    {
        return false;
    }
    CDoubleNode nodeTriggerDelay(m_pCamera, "ExposureTime");
    if (false == nodeTriggerDelay.isValid())
    {
        return false;
    }
    if (false == nodeTriggerDelay.isAvailable())
    {
        return false;
    }
    if (false == nodeTriggerDelay.setValue(delay))
    {
        return false;
    }
    return true;

}
//(QString camName,QString cameraModelAndSerial,QString triggerModel,QString triggerSource,QString exp,QString delay)
bool DaHuaCamera::initCamera(QString camName,QString strKey, QString triggerModel,QString triggerSource,QString exp,QString delay)
{
    m_camname=camName;
    SetCamera(strKey);
    bool iscamopensuccess=CameraOpen();
    if(!iscamopensuccess)return false;
    bool issettriggersuccess=false;
    issettriggersuccess=CameraChangeTrig(triggerModel,triggerSource);
    if(!issettriggersuccess)return false;
    SetExposeTime(exp.toDouble());
    setDelay(delay.toDouble());
    //SetAdjustPlus(gain);
    //setPixelFormat(pixeltype);
    bool iscamstartsuccess=CameraStart();
    if(!iscamstartsuccess)return false;
    return true;
}
void DaHuaCamera::takePhoto(QImage *ho_Image,int sleeptime)
{

    //ExecuteSoftTrig();
    //Sleep(200);
    //HalconCpp::GenEmptyObj(ho_Image);
    CFrame frame;
    //double time_Start = (double)clock();
    //Sleep(sleeptime);
    m_pStreamSource->getFrame(frame, 3000);

    //double time_End = (double)clock();
    //double time=time_End-time_Start;
    CFrameInfo frameInfo;
    frameInfo.m_nWidth = frame.getImageWidth();
    frameInfo.m_nHeight = frame.getImageHeight();
    frameInfo.m_nBufferSize = frame.getImageSize();
    frameInfo.m_nPaddingX = frame.getImagePadddingX();
    frameInfo.m_nPaddingY = frame.getImagePadddingY();
    frameInfo.m_ePixelType = frame.getImagePixelFormat();
    frameInfo.m_pImageBuf = (BYTE *)malloc(sizeof(BYTE)* frameInfo.m_nBufferSize);
    frameInfo.m_nTimeStamp = frame.getImageTimeStamp();

    // 内存申请失败，直接返回
    // memory application failed, return directly
    if (frameInfo.m_pImageBuf == nullptr||(frame.getImageSize()<(frameInfo.m_nWidth*frameInfo.m_nHeight-1)))
    {
        return;
    }
    memcpy(frameInfo.m_pImageBuf, frame.getImage(), frame.getImageSize());
    //HalconCpp::GenEmptyObj(ho_Image);
    if (Dahua::GenICam::gvspPixelMono8 == frameInfo.m_ePixelType)
    {
        //QtHalcon::hobjectMonoFromBuffer(ho_Image,frameInfo.m_nWidth,frameInfo.m_nHeight,frameInfo.m_pImageBuf);
        QImage image = QImage(frameInfo.m_pImageBuf,frameInfo.m_nWidth, frameInfo.m_nHeight, QImage::Format_Mono);
        //HImage hi_image;
        //QtHalcon::qImage2HImage(image,hi_image);
        //CopyImage(hi_image,ho_Image);
    }
    else
    {
        // 转码
        uint8_t *pRGBbuffer = nullptr;
        int nRgbBufferSize = 0;
        nRgbBufferSize = frameInfo.m_nWidth * frameInfo.m_nHeight * 3;
        pRGBbuffer = (uint8_t *)malloc(nRgbBufferSize);
        if (pRGBbuffer == nullptr)
        {
            // 释放内存
            free(frameInfo.m_pImageBuf);
            return;
        }
        IMGCNV_SOpenParam openParam;
        openParam.width = frameInfo.m_nWidth;
        openParam.height = frameInfo.m_nHeight;
        openParam.paddingX = frameInfo.m_nPaddingX;
        openParam.paddingY = frameInfo.m_nPaddingY;
        openParam.dataSize = frameInfo.m_nBufferSize;
        openParam.pixelForamt = frameInfo.m_ePixelType;
        IMGCNV_EErr status = IMGCNV_ConvertToBGR24(frameInfo.m_pImageBuf, &openParam, pRGBbuffer, &nRgbBufferSize);
        //HImage hi_image;
        double time_Start = (double)clock();
        //hi_image.GenImageInterleaved((void*)pRGBbuffer, "bgrx", frameInfo.m_nWidth, frameInfo.m_nHeight, 0,  "byte", frameInfo.m_nWidth, frameInfo.m_nHeight, 0, 0, 8, 0);
        *ho_Image = QImage(pRGBbuffer,frameInfo.m_nWidth, frameInfo.m_nHeight, QImage::Format_RGB888);
        //HImage hi_image;
        //QtHalcon::qImage2HImage(image,hi_image);
        //CopyImage(hi_image,ho_Image);
        double time_End = (double)clock();
        double time=time_End-time_Start;
        free(pRGBbuffer);
    }
    free(frameInfo.m_pImageBuf);

}


