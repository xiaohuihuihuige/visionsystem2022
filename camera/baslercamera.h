#ifndef BASLERCAMERA_H
#define BASLERCAMERA_H

#include <QObject>
#include <QImage>
#include"Basler.h"

//添加到pro文件
//INCLUDEPATH += $$quote( D:/Program Files/Basler/pylon 6/Development/include\ )
//LIBS += -L'D:/Program Files/Basler/pylon 6/Development/lib/x64/' \
//-lGCBase_MD_VC141_v3_1_Basler_pylon                      \
//-lGenApi_MD_VC141_v3_1_Basler_pylon                      \
//-lPylonBase_v6_2                      \
//-lPylonC                      \
//-lPylonGUI_v6_2                      \
//-lPylonUtility_v6_2
class BaslerCamera : public QObject
{
    Q_OBJECT
public:
    explicit BaslerCamera(QObject *parent = nullptr);
    ~BaslerCamera();

    QString m_CamName;
    bool isCameraInit;
    bool isGrabStart;
    CBaslerGigeNative*  m_pcMyCamera;   // ch:CMyCamera封装了常用接口 | en:CMyCamera packed normal used interface

public:
    int openDevice(QString sn);
    int closeDevice();
    int setTriggerMode(QString triggerModel,QString triggerSource,QString triggerDelay);
    int setExposureTime(float expTime);
    int setGain(float gainValue);
    int setTriggerDelay(float delay);
    int doSoftwareOnce();
    int startGrabbing();
    int stopGrabbing();
    static void DisplayImageCallBack(void* pOwner, int* width, int* height, unsigned char* pBuffer, bool isColor);
    bool initCamera(QString camName,QString cameraModelAndSerial,QString triggerModel,QString triggerSource,QString exp,QString delay);

signals:
    void getOneFrame(QImage image,QString camName);
};

#endif // BASLERCAMERA_H
