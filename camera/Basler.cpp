//Basler.cpp


#include "Basler.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CBaslerGigeNative::CBaslerGigeNative()
{
    IsAcquisitionStartAvail = false;
    IsFrameStartStartAvail = false;
    IsCameraRemoved = false;
}


CBaslerGigeNative::~CBaslerGigeNative()
{

}


void CBaslerGigeNative::PylonLibInit()
{
    try
    {
        PylonInitialize();
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());

    }
}
void CBaslerGigeNative::PylonLibTerminate()
{
    try
    {
        PylonTerminate();
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());

    }
}

BOOL CBaslerGigeNative::OpenDevice()
{
    // Only look for cameras supported by Camera_t.
    try
    {
        CDeviceInfo info;
        info.SetDeviceClass( Camera_t::DeviceClass());
        if(camera.IsPylonDeviceAttached())
        {
            camera.DetachDevice();
        }
        camera.Attach( CTlFactory::GetInstance().CreateFirstDevice( info));

        camera.Open();
        IsAcquisitionStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_AcquisitionStart));
        IsFrameStartStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_FrameStart));
        // register configuration event handler that handles device removal.
        camera.RegisterConfiguration(this, RegistrationMode_Append, Cleanup_None);
        camera.RegisterImageEventHandler(this, RegistrationMode_Append, Cleanup_None);
    }
    catch (exception* e)
    {
        return 1;
    }
    return 0;
}

BOOL CBaslerGigeNative::OpenDeviceBySn(string serialNumber)
{
    try
    {
        CBaslerGigEDeviceInfo info;
        info.SetSerialNumber(String_t(serialNumber.c_str()));

        if(camera.IsPylonDeviceAttached())
        {
            camera.DetachDevice();
        }
        camera.Attach( CTlFactory::GetInstance().CreateFirstDevice( info));
        camera.Open();

        IsAcquisitionStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_AcquisitionStart));
        IsFrameStartStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_FrameStart));

        // register configuration event handler that handles device removal.
        camera.RegisterConfiguration(this, RegistrationMode_Append, Cleanup_None);
        camera.RegisterImageEventHandler(this, RegistrationMode_Append, Cleanup_None);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        return 1;
    }
    return 0;
}

BOOL  CBaslerGigeNative::OpenDeviceByUserID(string UserDefinedName)
{
    try
    {
        CBaslerGigEDeviceInfo info;
        info.SetUserDefinedName(String_t(UserDefinedName.c_str()));

        if(camera.IsPylonDeviceAttached())
        {
            camera.DetachDevice();
        }
        camera.Attach( CTlFactory::GetInstance().CreateFirstDevice( info));
        camera.Open();


        IsAcquisitionStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_AcquisitionStart));
        IsFrameStartStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_FrameStart));

        // register configuration event handler that handles device removal.
        camera.RegisterConfiguration(this, RegistrationMode_Append, Cleanup_None);
        camera.RegisterImageEventHandler(this, RegistrationMode_Append, Cleanup_None);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        return 1;
    }
    return 0;
}

BOOL CBaslerGigeNative::OpenDeviceByIPAddress(string IPAddress, string subnetMask)
{
    try
    {
        CBaslerGigEDeviceInfo info;
        info.SetIpAddress(String_t(IPAddress.c_str()));
        info.SetSubnetMask(String_t(subnetMask.c_str()));

        if(camera.IsPylonDeviceAttached())
        {
            camera.DetachDevice();
        }
        camera.Attach( CTlFactory::GetInstance().CreateFirstDevice( info));
        camera.Open();

        IsAcquisitionStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_AcquisitionStart));
        IsFrameStartStartAvail = IsAvailable(camera.TriggerSelector.GetEntry(TriggerSelector_FrameStart));

        // register configuration event handler that handles device removal.
        camera.RegisterConfiguration(this, RegistrationMode_Append, Cleanup_None);
        camera.RegisterImageEventHandler(this, RegistrationMode_Append, Cleanup_None);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        return 1;
    }
    return 0;
}

void CBaslerGigeNative::RemovedDeviceReconnect()
{
    try
    {
        if ( camera.IsCameraDeviceRemoved() || IsCameraRemoved)
        {
            // 提示相机掉线
            //CString strMsg;
            //strMsg = "UserID为“"+ camera.GetDeviceInfo().GetUserDefinedName() + "”的相机已掉线！！！";
            //AfxMessageBox(strMsg);

            // Now try to find the detached camera after it has been attached again:
            // Create a device info object for remembering the camera properties.
            CDeviceInfo info;

            // Remember the camera properties that allow detecting the same camera again.
            info.SetDeviceClass( camera.GetDeviceInfo().GetDeviceClass());
            info.SetSerialNumber( camera.GetDeviceInfo().GetSerialNumber());

            // Destroy the Pylon Device representing the detached camera device.
            // It cannot be used anymore.
            camera.DestroyDevice();

            // Ask the user to connect the same device.
            int loopCount = 100;	// 遍历相机100次，每次间隔250ms
            // cout << endl << "Please connect the same device to the PC again (timeout " << loopCount / 4 << "s) " << endl;

            // Create a filter containing the CDeviceInfo object info which describes the properties of the device we are looking for.
            DeviceInfoList_t filter;
            filter.push_back( info);

            for ( ; loopCount > 0; --loopCount)
            {
                // Try to find the camera we are looking for.
                DeviceInfoList_t devices;
                if (  CTlFactory::GetInstance().EnumerateDevices(devices, filter) > 0 )
                {
                    // Print two new lines, just for improving printed output.
                    //cout << endl << endl;

                    // The camera has been found. Create and attach it to the Instant Camera object.
                    camera.Attach( CTlFactory::GetInstance().CreateDevice( devices[0]));
                    //Exit waiting
                    break;
                }

                WaitObject::Sleep(250);
            }

            // If the camera has been found.
            if ( camera.IsPylonDeviceAttached())
            {
                // All configuration objects and other event handler objects are still registered.
                // The configuration objects will parameterize the camera device and the instant
                // camera will be ready for operation again.

                // Open the camera.
                camera.Open();
                camera.GetTLParams().HeartbeatTimeout.SetValue(5000);
                // Now the Instant Camera object can be used as before.

                // 提示相机已重新连接
                //CString strMsg;
                //strMsg = "已经重新连接上UserID为“"+ camera.GetDeviceInfo().GetUserDefinedName() + "”的相机！";
                //AfxMessageBox(strMsg);

            }
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}
void CBaslerGigeNative::SetHeartbeatTimeout(int64_t valueMs)
{
    try
    {
        camera.GetTLParams().HeartbeatTimeout.SetValue(valueMs);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::CloseDevice()
{
    try
    {
        if(camera.IsGrabbing())
        {
            camera.StopGrabbing();
        }

        camera.DeregisterConfiguration(this);
        camera.DeregisterImageEventHandler(this);
        camera.Close();
        camera.DetachDevice();
        camera.DestroyDevice();
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
    }
}

void CBaslerGigeNative::GrabOne()
{
    try
    {
        // For demonstration purposes only, register another image event handler.
        camera.AcquisitionMode.SetValue(AcquisitionMode_SingleFrame);
        camera.StartGrabbing(1, GrabStrategy_LatestImageOnly, GrabLoop_ProvidedByInstantCamera);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
    }
}

void CBaslerGigeNative::StartGrab()
{
    try
    {
        // For demonstration purposes only, register another image event handler.
        camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);
        //有时候电脑资源不足的时候，需要设置为MaxNumBuffer = 1 + GrabStrategy_LatestImageOnly，否则可能会返回上一张图片的异常情况
        camera.MaxNumBuffer = 30;
        camera.StartGrabbing( GrabStrategy_LatestImageOnly, GrabLoop_ProvidedByInstantCamera);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
    }
}

void CBaslerGigeNative::StopGrab()
{
    try
    {
        if(camera.IsGrabbing())
        {
            camera.StopGrabbing();
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
    }
}

void CBaslerGigeNative::SetFreerunMode()
{
    try
    {
        if(IsAcquisitionStartAvail && IsFrameStartStartAvail)
        {
            camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
            camera.TriggerMode.SetValue(TriggerMode_Off);

            camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
            camera.TriggerMode.SetValue(TriggerMode_Off);
            camera.TriggerSource.SetValue(TriggerSource_Software);
        }
        else if(IsAcquisitionStartAvail && !IsFrameStartStartAvail)
        {
            camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
            camera.TriggerMode.SetValue(TriggerMode_Off);
            camera.TriggerSource.SetValue(TriggerSource_Software);
            camera.AcquisitionFrameCount.SetValue(1);
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::SetSoftwareTriggerMode()
{
    try
    {
        if(IsAcquisitionStartAvail && IsFrameStartStartAvail)
        {
            camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
            camera.TriggerMode.SetValue(TriggerMode_Off);

            camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
            camera.TriggerMode.SetValue(TriggerMode_On);
            camera.TriggerSource.SetValue(TriggerSource_Software);
        }
        else if(IsAcquisitionStartAvail && !IsFrameStartStartAvail)
        {
            camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
            camera.TriggerMode.SetValue(TriggerMode_On);
            camera.TriggerSource.SetValue(TriggerSource_Software);
            camera.AcquisitionFrameCount.SetValue(1);
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::SendSoftwareTriggerCommand()
{
    try
    {
        /*if(IsAcquisitionStartAvail && IsFrameStartStartAvail)
        {
        camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
        }
        else if(IsAcquisitionStartAvail && !IsFrameStartStartAvail)
        {
        camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
        }*/
        camera.TriggerSoftware();
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::SetExternalTriggerMode()
{
    try
    {
        if(IsAcquisitionStartAvail && IsFrameStartStartAvail)
        {
            camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
            camera.TriggerMode.SetValue(TriggerMode_Off);

            camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
            camera.TriggerMode.SetValue(TriggerMode_On);
            camera.TriggerSource.SetValue(TriggerSource_Line1);
        }
        else if(IsAcquisitionStartAvail && !IsFrameStartStartAvail)
        {
            camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
            camera.TriggerMode.SetValue(TriggerMode_On);
            camera.TriggerSource.SetValue(TriggerSource_Line1);
            camera.AcquisitionFrameCount.SetValue(1);
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }

}

void CBaslerGigeNative::SetTriggerDelay(double nTimeUs)
{
    try
    {
        double min = camera.TriggerDelayAbs.GetMin();
        double max = camera.TriggerDelayAbs.GetMax();

        if(nTimeUs > min)
        {
            nTimeUs = min;
        }
        else if(nTimeUs>max)
        {
            nTimeUs = max;
        }

        camera.TriggerDelayAbs.SetValue(nTimeUs);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::SetLineDebouncerTimeAbs(double dTimeUs)
{
    try
    {
        double min = camera.TriggerDelayAbs.GetMin();
        double max = camera.TriggerDelayAbs.GetMax();

        if(dTimeUs > min)
        {
            dTimeUs = min;
        }
        else if(dTimeUs>max)
        {
            dTimeUs = max;
        }

        if(IsAvailable(camera.LineSelector.GetEntry(LineSelector_Line1)))
        {
            camera.LineSelector.SetValue(LineSelector_Line1);

            camera.LineDebouncerTimeAbs.SetValue(dTimeUs);
            //double d = camera.LineDebouncerTimeAbs.GetValue();
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}
void CBaslerGigeNative::SetExposureTimeRaw(int64_t nExpTimeUs)
{
    try
    {
        int64_t min= camera.ExposureTimeRaw.GetMin();
        int64_t max= camera.ExposureTimeRaw.GetMax();
        int64_t inc= camera.ExposureTimeRaw.GetInc();
        int64_t correctedValue = nExpTimeUs;
        if(nExpTimeUs < min)
        {
            nExpTimeUs = min;
        }
        else if(nExpTimeUs > max)
        {
            nExpTimeUs = max;
        }

        if(inc > 1)
        {
            // Apply the increment and cut off invalid values if neccessary.
            correctedValue = nExpTimeUs - (nExpTimeUs % inc);
        }
        camera.ExposureTimeRaw.SetValue(correctedValue);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::SetGainRaw(int64_t nGainRaw)
{
    try
    {
        int64_t min= camera.GainRaw.GetMin();
        int64_t max= camera.GainRaw.GetMax();
        int64_t inc= camera.GainRaw.GetInc();
        int64_t correctedValue = nGainRaw;
        if(nGainRaw < min)
        {
            nGainRaw = min;
        }
        else if(nGainRaw > max)
        {
            nGainRaw = max;
        }

        if(inc > 1)
        {
            // Apply the increment and cut off invalid values if neccessary.
            correctedValue = nGainRaw - (nGainRaw % inc);
        }
        camera.ExposureTimeRaw.SetValue(correctedValue);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}

void CBaslerGigeNative::SetAcquisitionFrameRate(double fps)
{
    try
    {
        camera.AcquisitionFrameRateEnable.SetValue(true);
        camera.AcquisitionFrameRateAbs.SetValue(fps);
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }
}
void CBaslerGigeNative::OnImageGrabbed( CInstantCamera& camera, const CGrabResultPtr& ptrGrabResult)
{

    try
    {
        if (ptrGrabResult->GrabSucceeded())
        {
            // 判断是哪个相机进入采集图像回调函数
            //if(camera.GetDeviceInfo().GetUserDefinedName() == "c1")
            if (ptrGrabResult->GetPixelType() == PixelType_Mono8)
            {
                int x = (int)width;
                int y = (int)height;
                pImageBufferMono = (uint8_t *)ptrGrabResult->GetBuffer();

                isColor = false;
                // 1、通过函数指针传出，转bitmap显示
                m_DisplayImageCallBack( m_pOwner, &x, &y, pImageBufferMono, isColor);

                //// 2、pylon自带显示窗体
                //Pylon::DisplayImage(0, ptrGrabResult);
            }
            else if (ptrGrabResult->GetPixelType() == PixelType_BayerGR8 || ptrGrabResult->GetPixelType() == PixelType_BayerRG8
                     || ptrGrabResult->GetPixelType() == PixelType_BayerGB8 || ptrGrabResult->GetPixelType() == PixelType_BayerBG8
                     || ptrGrabResult->GetPixelType() == PixelType_YUV422packed)
            {
                int x,y;
                x = ptrGrabResult->GetWidth();
                y = ptrGrabResult->GetHeight();
                if(pImageBufferColor == NULL)
                {
                    pImageBufferColor = (unsigned char*) malloc(ptrGrabResult->GetPayloadSize() * 3);
                }
                // 彩色相机进行RGB转换
                CImageFormatConverter converter;
                converter.OutputPixelFormat = PixelType_BGR8packed;
                converter.Convert( pImageBufferColor, ptrGrabResult->GetPayloadSize() * 3, ptrGrabResult);

                isColor = true;
                m_DisplayImageCallBack( m_pOwner, &x, &y, pImageBufferColor, isColor);
            }
        }
        else
        {
            //CString strMsg;
            //strMsg ="Grab faild!!\n" + ptrGrabResult->GetErrorDescription();
            //AfxMessageBox(strMsg);
        }
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
    }
    /*try
    {
        // 所有返回数据在ptrGrabResult这里面，对应的相机信息在camera里面

        // 如果是多相机的情况，需要在这里区分相机信息,比如对比SN号码
        //if(camera.GetDeviceInfo().GetSerialNumber() == "21530810")
        {
            // Image grabbed successfully?
            if (ptrGrabResult->GrabSucceeded())
            {
                // 1、Bitmap显示
                int x,y;
                unsigned char * p;
                x = ptrGrabResult->GetWidth();
                y = ptrGrabResult->GetHeight();
                p = (uint8_t *)ptrGrabResult->GetBuffer();
                m_DisplayImageCallBack( m_pOwner, &x, &y, p);


                // 2、Pylon自带窗体显示
                //Pylon::DisplayImage(1, ptrGrabResult);


                // 3、转halcon图像并保存
                //Hobject hImage;
                //unsigned char *Pointer;
                //int width,height;

                //height = ptrGrabResult->GetHeight();
                //width = ptrGrabResult->GetWidth();
                //Pylon::PixelType pPixeltype;
                //pPixeltype = ptrGrabResult->GetPixelType();
                //Pointer = (uint8_t *) ptrGrabResult->GetBuffer();

                //if(pPixeltype== PixelType_Mono8) //For Mono
                //{
                //	gen_image1_extern(&hImage,"byte",(HTuple)width,(HTuple)height,(long)Pointer,NULL);  //Mono
                //	write_image(hImage,"bmp",0,"HalconTestMono.bmp");
                //}
                //else if(pPixeltype== PixelType_BayerGB8 || pPixeltype== PixelType_BayerBG8)//For Color
                //{
                //	CImageFormatConverter converter;
                //	CPylonImage targetImage;
                //	converter.OutputPixelFormat = PixelType_RGB8packed ;
                //	converter.Convert(targetImage,ptrGrabResult);
                //	unsigned char *Pointer = (unsigned char*)targetImage.GetBuffer(); //for Color

                //	PointerR = (unsigned char*)targetImage.GetPlane(0).GetBuffer();
                //	PointerG = (unsigned char*)targetImage.GetPlane(1).GetBuffer();
                //	PointerB = (unsigned char*)targetImage.GetPlane(2).GetBuffer();
                //	HalconCpp::GenImage3Extern(&HImage,"byte",(HTuple)width,(HTuple)height,(long)PointerR,(long)PointerG,(long)PointerB,NULL); //Color
                //

                //	gen_image_interleaved (&hImage,(long)Pointer, "rgb", (HTuple)width,(HTuple)height, 0, "byte", 0, 0, 0, 0, 8, 0);
                //	write_image(hImage,"bmp",0,"TestColor.bmp");
                //}

            }
            else
            {
                 CString strMsg;
                strMsg ="error!!"+ camera.GetDeviceInfo().GetSerialNumber() + ptrGrabResult->GetErrorCode()+ ptrGrabResult->GetErrorDescription();
                AfxMessageBox(strMsg);
                //std::cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << std::endl;
            }
        }

        //if(camera.GetDeviceInfo().GetUserDefinedName() == "camera1") //多相机对比UserID

        //if(((CBaslerGigEDeviceInfo)camera.GetDeviceInfo()).GetIpAddress() == "camera1") //多gige相机对比IPAddress
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());

    }*/

}

//This method is called when images have been skipped using the GrabStrategy_LatestImageOnly strategy or the GrabStrategy_LatestImages strategy.

//void CBaslerGigeNative::OnImagesSkipped( CInstantCamera& camera, size_t countOfSkippedImages)
//{
//	std::cout << "OnImagesSkipped event for device " << camera.GetDeviceInfo().GetSerialNumber() << std::endl;
//    std::cout << countOfSkippedImages  << " images have been skipped." << std::endl;
//    std::cout << std::endl;
//	
//	 CString strMsg, temp;
//	 temp.Format(_T("%d"),countOfSkippedImages);
//	 strMsg = camera.GetDeviceInfo().GetModelName() + "countOfSkippedImages = "+String_t(temp);
//     AfxMessageBox(strMsg);
//}

void CBaslerGigeNative::OnCameraDeviceRemoved(CInstantCamera& /*camera*/)
{
    try
    {
        /*IsCameraRemoved = true;
        std::cout << "OnCameraDeviceRemoved event for device " << camera.GetDeviceInfo().GetModelName() << std::endl;
        CString strMsg;
        strMsg = "OnCameraDeviceRemoved event for device"+ camera.GetDeviceInfo().GetSerialNumber();
        AfxMessageBox(strMsg);*/

        RemovedDeviceReconnect();
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());
    }
}

void CBaslerGigeNative::SetOwner(void* pOwner, BaslerGrabbedCallback pDisplayImageCallBack)
{
    try
    {
        m_pOwner = pOwner;
        m_DisplayImageCallBack = pDisplayImageCallBack;
    }
    catch (GenICam::GenericException &e)
    {
        string  str(e.GetDescription());
        //TRACE("%s",str.c_str());
    }
}
