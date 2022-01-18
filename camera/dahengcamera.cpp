#include "dahengcamera.h"

DaHengCamera::DaHengCamera(QObject *parent) : QObject(parent)
{
    IGXFactory::GetInstance().Init();
    useCamID=0;
    isCameraInit=false;
    isGrabStart=false;
}

DaHengCamera::~DaHengCamera()
{
    closeDevice();
}

int DaHengCamera::openDevice(QString cameraModelAndSerial)
{

    GxIAPICPP::gxdeviceinfo_vector vectorDeviceInfo;
    IGXFactory::GetInstance().UpdateDeviceList(1000, vectorDeviceInfo);
    if(vectorDeviceInfo.size()<1)return 1;
    uint idnum=0;
    std::string snnum=cameraModelAndSerial.split(":").at(1).toStdString().data();
    for (int i=0;i<vectorDeviceInfo.size();i++)
    {
        if(snnum==vectorDeviceInfo[i].GetSN().c_str())
        {
            idnum=i;
            break;
        }
    }
    //打开设备
    m_objDevicePtr = IGXFactory::GetInstance().OpenDeviceBySN(vectorDeviceInfo[idnum].GetSN(), GX_ACCESS_EXCLUSIVE);
    m_objFeatureControlPtr = m_objDevicePtr->GetRemoteFeatureControl();
    m_nImageWidth = (int64_t)m_objDevicePtr->GetRemoteFeatureControl()->GetIntFeature("Width")->GetValue();
    m_nImageHeight = (int64_t)m_objDevicePtr->GetRemoteFeatureControl()->GetIntFeature("Height")->GetValue();
        //获取是否为彩色相机
        if (m_objDevicePtr->GetRemoteFeatureControl()->IsImplemented("PixelColorFilter"))
        {
            gxstring strValue = m_objDevicePtr->GetRemoteFeatureControl()->GetEnumFeature("PixelColorFilter")->GetValue();

            if ("None" != strValue)
            {
                m_bIsColorFilter = true;
            }
        }

    //判断设备流是否大于零，如果大于零则打开流
    int nStreamCount = m_objDevicePtr->GetStreamCount();
    if (nStreamCount > 0)
    {
        m_objStreamPtr = m_objDevicePtr->OpenStream(0);
        m_objStreamFeatureControlPtr = m_objStreamPtr->GetFeatureControl();
    }
    else
    {
        return 2;
    }
    GX_DEVICE_CLASS_LIST objDeviceClass = m_objDevicePtr->GetDeviceInfo().GetDeviceClass();
    if(GX_DEVICE_CLASS_GEV == objDeviceClass)
    {
        // 判断设备是否支持流通道数据包功能
        if(true == m_objFeatureControlPtr->IsImplemented("GevSCPSPacketSize"))
        {
            // 获取当前网络环境的最优包长值
            int nPacketSize = m_objStreamPtr->GetOptimalPacketSize();
            // 将最优包长值设置为当前设备的流通道包长值
            m_objFeatureControlPtr->GetIntFeature("GevSCPSPacketSize")->SetValue(nPacketSize);
            isCameraInit=true;
        }
    }
    return 0;

}

int DaHengCamera::closeDevice()
{
    try
    {
        //关闭流对象
        m_objStreamPtr->Close();

    }
    catch(CGalaxyException)
    {
        //do noting
    }
    try
    {
        //关闭设备
        m_objDevicePtr->Close();
    }
    catch(CGalaxyException)
    {
        //do noting
    }
    return 0;
}
// ch:设置触发模式 | en:Set trigger mode  0为连续，1为触发
int DaHengCamera::setTriggerMode(QString triggerModel, QString triggerSource, QString triggerDelay)
{

    if(triggerModel.toInt()==0)
    {

        m_objFeatureControlPtr->GetEnumFeature("TriggerMode")->SetValue("Off");
    }
    else
    {
        m_objFeatureControlPtr->GetEnumFeature("TriggerMode")->SetValue("On");
        if(triggerSource.toInt()==7)
        {
            m_objFeatureControlPtr->GetEnumFeature("TriggerSource")->SetValue("Software");
        }
        else
        {
            m_objFeatureControlPtr->GetEnumFeature("TriggerSource")->SetValue("Line0");
        }
    }
    return 0;
}

int DaHengCamera::setExposureTime(float expTime)
{
    m_objFeatureControlPtr->GetFloatFeature("ExposureTime")->SetValue(expTime);
    return 0;
}

int DaHengCamera::setGain(float gainValue)
{
    m_objFeatureControlPtr->GetFloatFeature("Gain")->SetValue(gainValue);
}

int DaHengCamera::setTriggerDelay(double delay)
{
    m_objFeatureControlPtr->GetFloatFeature("TriggerDelay")->SetValue(delay);
}

int DaHengCamera::doSoftwareOnce()
{
    m_objFeatureControlPtr->GetCommandFeature("TriggerSoftware")->Execute();
}

int DaHengCamera::startGrabbing()
{
    //注册回调函数
    m_objStreamPtr->RegisterCaptureCallback(m_pSampleCaptureEventHandle, this);
    //开启流层通道
    m_objStreamPtr->StartGrab();
    //发送开采命令
    m_objFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();
    isGrabStart=true;
    return  0;


}

int DaHengCamera::stopGrabbing()
{
    try
    {
        //判断是否已停止采集
        if (isGrabStart)
        {
            //发送停采命令
            m_objFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
            //关闭流层采集
            m_objStreamPtr->StopGrab();
            //注销采集回调
            m_objStreamPtr->UnregisterCaptureCallback();
        }
    }
    catch(CGalaxyException)
    {
        //do noting
    }
}
GX_VALID_BIT_LIST DaHengCamera::GetBestValudBit(GX_PIXEL_FORMAT_ENTRY emPixelFormatEntry)
{
    GX_VALID_BIT_LIST emValidBits = GX_BIT_0_7;
    switch (emPixelFormatEntry)
    {
    case GX_PIXEL_FORMAT_MONO8:
    case GX_PIXEL_FORMAT_BAYER_GR8:
    case GX_PIXEL_FORMAT_BAYER_RG8:
    case GX_PIXEL_FORMAT_BAYER_GB8:
    case GX_PIXEL_FORMAT_BAYER_BG8:
        {
            emValidBits = GX_BIT_0_7;
            break;
        }
    case GX_PIXEL_FORMAT_MONO10:
    case GX_PIXEL_FORMAT_BAYER_GR10:
    case GX_PIXEL_FORMAT_BAYER_RG10:
    case GX_PIXEL_FORMAT_BAYER_GB10:
    case GX_PIXEL_FORMAT_BAYER_BG10:
        {
            emValidBits = GX_BIT_2_9;
            break;
        }
    case GX_PIXEL_FORMAT_MONO12:
    case GX_PIXEL_FORMAT_BAYER_GR12:
    case GX_PIXEL_FORMAT_BAYER_RG12:
    case GX_PIXEL_FORMAT_BAYER_GB12:
    case GX_PIXEL_FORMAT_BAYER_BG12:
        {
            emValidBits = GX_BIT_4_11;
            break;
        }
    case GX_PIXEL_FORMAT_MONO14:
        {
            //暂时没有这样的数据格式待升级
            break;
        }
    case GX_PIXEL_FORMAT_MONO16:
    case GX_PIXEL_FORMAT_BAYER_GR16:
    case GX_PIXEL_FORMAT_BAYER_RG16:
    case GX_PIXEL_FORMAT_BAYER_GB16:
    case GX_PIXEL_FORMAT_BAYER_BG16:
        {
            //暂时没有这样的数据格式待升级
            break;
        }
    default:
        break;
    }
    return emValidBits;
}


bool DaHengCamera::initCamera(QString camName, QString cameraModelAndSerial, QString triggerModel, QString triggerSource, QString exp, QString delay)
{
    m_CamName=camName;
    m_pSampleCaptureEventHandle = new CSampleCaptureEventHandler();
    QVector<QString> cameraList;
    int ret;
    ret=openDevice(cameraModelAndSerial);
    if(ret!=0)return false;
    ret=setTriggerMode(triggerModel,triggerSource,delay);
    if(ret!=0)return false;
    ret=setExposureTime(exp.toFloat());
    if(ret!=0)return false;
    ret=startGrabbing();
    if(ret!=0)return false;
    return true;
}

