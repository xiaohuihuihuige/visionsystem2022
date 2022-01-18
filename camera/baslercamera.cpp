#include "baslercamera.h"

BaslerCamera::BaslerCamera(QObject *parent) : QObject(parent),
    m_pcMyCamera(nullptr)
{
    isCameraInit=false;
    isGrabStart=false;
}

BaslerCamera::~BaslerCamera()
{
    closeDevice();
}

int BaslerCamera::openDevice(QString sn)
{
    if (true == isCameraInit)
    {
        return -1;
    }
    int nRet = 0;
    if (nullptr == m_pcMyCamera)
    {
        m_pcMyCamera = new CBaslerGigeNative;
    }
    QString snNum=sn.split(":").at(1);
    nRet=m_pcMyCamera->OpenDeviceBySn(snNum.toStdString());
    m_pcMyCamera->SetHeartbeatTimeout(6000);
    if(nRet!=0)
        return 1;
    isCameraInit=true;
    return 0;
}

int BaslerCamera::closeDevice()
{
    int nRet = 0;
    if(isGrabStart)
    {
        stopGrabbing();
    }
    if(isCameraInit)
    {
        m_pcMyCamera->CloseDevice();
    }
    isGrabStart = false;
    isCameraInit = false;
    delete m_pcMyCamera;
    m_pcMyCamera=nullptr;
    return nRet;
}

int BaslerCamera::setTriggerMode(QString triggerModel, QString triggerSource, QString triggerDelay)
{
    if (false == isCameraInit)
    {
        return 1;
    }
    int nRet = 0;
    if(triggerModel.toInt()==0)
    {
        m_pcMyCamera->SetFreerunMode();
    }
    else
    {
        if(triggerSource.toInt()!=7)
        {
            m_pcMyCamera->SetExternalTriggerMode();
        }
        else
        {
            m_pcMyCamera->SetSoftwareTriggerMode();
        }
        nRet=setTriggerDelay(triggerDelay.toInt());
        if(nRet!=0)return false;
    }
    return 0;
}

int BaslerCamera::setExposureTime(float expTime)
{
    m_pcMyCamera->SetExposureTimeRaw(expTime);
    return 0;
}

int BaslerCamera::setGain(float gainValue)
{
    m_pcMyCamera->SetGainRaw(gainValue);
    return 0;
}

int BaslerCamera::setTriggerDelay(float delay)
{
    m_pcMyCamera->SetTriggerDelay(delay);
    return 0;
}

int BaslerCamera::doSoftwareOnce()
{
    m_pcMyCamera->SendSoftwareTriggerCommand();
    return 0;
}

int BaslerCamera::startGrabbing()
{
    // 注册图像传出方法
    BaslerGrabbedCallback pDisplayImageCallBack = (BaslerGrabbedCallback)DisplayImageCallBack;
    m_pcMyCamera->SetOwner(this, pDisplayImageCallBack);
    m_pcMyCamera->StartGrab();
    return 0;
}

int BaslerCamera::stopGrabbing()
{
    m_pcMyCamera->StopGrab();
    return 0;
}


void BaslerCamera::DisplayImageCallBack(void *pOwner, int *width, int *height, unsigned char *pBuffer, bool isColor)
{
    BaslerCamera *pcam = (BaslerCamera*)(pOwner);
    QImage image,image1;
    if(!isColor)
    {
        //灰度图转化
        image=QImage(pBuffer, *width, *height, QImage::Format_Indexed8);
        image1=image.copy(QRect(0,0,image.width(),image.height()));
        //ho_Image=QtOpencv::unsignchar2Mat(pFrameInfo->nHeight ,pFrameInfo->nWidth,1,pData);
    }
    else
    {
        image=QImage(pBuffer, *width, *height, QImage::Format_RGB888);
        image1=image.copy(QRect(0,0,image.width(),image.height()));
        //ho_Image=QtOpencv::unsignchar2Mat(pFrameInfo->nHeight ,pFrameInfo->nWidth,3,stConvertParam.pDstBuffer);
    }
    emit pcam->getOneFrame(image1,pcam->m_CamName);

}

bool BaslerCamera::initCamera(QString camName, QString cameraModelAndSerial, QString triggerModel, QString triggerSource, QString exp, QString delay)
{

    int ret;
    ret=openDevice(cameraModelAndSerial);
    if(ret!=0)return false;
    ret=setTriggerMode(triggerModel,triggerSource,delay);
    if(ret!=0)return false;
    ret=setExposureTime(exp.toFloat());
    if(ret!=0)return false;
    ret=startGrabbing();
    if(ret!=0)return false;
    m_CamName=camName;
    return true;
}
