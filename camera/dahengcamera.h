#ifndef DAHENGCAMERA_H
#define DAHENGCAMERA_H

#include <QObject>
#include<QImage>
#include "GalaxyIncludes.h"


//#添加daheng sdk
//INCLUDEPATH += $$quote( D:/Program Files/Daheng Imaging/GalaxySDK/Samples/C++ SDK/inc )
//DEPENDPATH += $$quote( D:/Program Files/Daheng Imaging/GalaxySDK/Samples/C++ SDK/lib/x64 )
//win32:CONFIG(release, debug|release): LIBS += -L'D:/Program Files/Daheng Imaging/GalaxySDK/Samples/C++ SDK/lib/x64/' \
//-lGxIAPICPPEx \

class DaHengCamera : public QObject
{
    Q_OBJECT
public:
    explicit DaHengCamera(QObject *parent = nullptr);
    ~DaHengCamera();
    class CSampleCaptureEventHandler :public ICaptureEventHandler
    {
        //回调函数
        void DoOnImageCaptured(CImageDataPointer& objImageDataPointer, void* pUserParam)
        {
            try
            {
                DaHengCamera* pcam = (DaHengCamera*)pUserParam;
                GX_VALID_BIT_LIST emValidBits = GX_BIT_0_7;
                BYTE* pBuffer = NULL;

                if ((objImageDataPointer.IsNull()))
                {
                    return;
                }
                emValidBits = pcam->GetBestValudBit(objImageDataPointer->GetPixelFormat());
                QImage image,image1;
                if (pcam->m_bIsColorFilter)
                {
                    pBuffer = (BYTE*)objImageDataPointer->ConvertToRGB24(emValidBits, GX_RAW2RGB_NEIGHBOUR, true);
                    image=QImage(pBuffer, pcam->m_nImageWidth, pcam->m_nImageHeight, QImage::Format_RGB888);
                    image1=image.copy(QRect(0,0,image.width(),image.height()));
                }
                else
                {
                        //pBuffer = (BYTE*)objImageDataPointer->GetBuffer();
                    pBuffer = (BYTE*)objImageDataPointer->ConvertToRaw8(emValidBits);
                    image=QImage(pBuffer, pcam->m_nImageWidth, pcam->m_nImageHeight, QImage::Format_Indexed8);
                    image1=image.copy(QRect(0,0,image.width(),image.height()));
                    // 黑白相机需要翻转数据后显示
                    /*for(int i =0;i < m_nImageHeight;i++)
                    {
                        memcpy(m_pImageBuffer + i * m_nImageWidth, pBuffer + (m_nImageHeight - i -1) * m_nImageWidth,(size_t)m_nImageWidth);
                    }*/
                    //pBuffer = m_pImageBuffer;
                }
                 emit pcam->getOneFrame(image1,pcam->m_CamName);

            }
            catch (CGalaxyException)
            {
                //do nothing

            }
        }
    };

    QString m_CamName;
    int useCamID;
    bool isCameraInit;
    bool isGrabStart;
    int triggerDelay;
    CGXDevicePointer                  m_objDevicePtr;             ///< 设备句柄
    CGXStreamPointer                  m_objStreamPtr;             ///< 设备流
    CGXFeatureControlPointer          m_objFeatureControlPtr;     ///< 属性控制器
    CImageDataPointer                 m_objImageDataPtr;
    CGXFeatureControlPointer          m_objStreamFeatureControlPtr; ///< 流层控制器对象
    ///< 回调函数指针
    CSampleCaptureEventHandler*       m_pSampleCaptureEventHandle;
    bool m_bIsColorFilter;
    int m_nImageWidth,m_nImageHeight;


public:

    int getCamList(QVector<QString> *listDeviceName);
    int openDevice(QString cameraModelAndSerial);
    int closeDevice();
    int setTriggerMode(QString triggerModel,QString triggerSource,QString triggerDelay);
    int setExposureTime(float expTime);
    int setGain(float gainValue);
    int setTriggerDelay(double delay);
    int doSoftwareOnce();
    int startGrabbing();
    int stopGrabbing();
    GX_VALID_BIT_LIST GetBestValudBit(GX_PIXEL_FORMAT_ENTRY emPixelFormatEntry);
    bool initCamera(QString camName,QString cameraModelAndSerial,QString triggerModel,QString triggerSource,QString exp,QString delay);

signals:
    void getOneFrame(QImage image,QString camName);
};

#endif // DAHENGCAMERA_H
