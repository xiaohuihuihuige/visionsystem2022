#ifndef WEBCAM_H
#define WEBCAM_H

#include <QObject>
#include"opencv2/opencv.hpp"
#include"opencv2/highgui.hpp"
#include<QImage>
#include<QTimer>

#include<iostream>
#include<atlconv.h> // W2A USES_CONVERSION
#include<SetupAPI.h> // HDEVINFO
#include<vector>
#include<algorithm>
#include<tuple>
#include<string>
#include <Windows.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#include <tchar.h>
#include <stdio.h>
#pragma comment (lib, "setupapi.lib")


class WebCam : public QObject
{
    Q_OBJECT
public:
    explicit WebCam(QObject *parent = nullptr);
    ~WebCam();
    QVector<QString> cam_list;
    QString _cam_name;
    QTimer* _pTimer;
    cv::VideoCapture  *_pCam;
    int triggerDelay;//相机采集延时
    bool initCamera(QString camName,QString cameraModelAndSerial,QString resolution, QString triggerModel, QString exp,QString delay);
    bool openWebCam(int camID,QString resolution,int triggerModel);
    bool setExp(QString exp);
    void doSoftwareOnce();
    QImage cvMat2QImage(const cv::Mat &mat);
    bool closeDevice();

    void GetSerialNum(QVector <QString>&);
    std::string WcharToString(LPCWSTR pwszsrc);

signals:
    void getOneFrame(QImage image,QString camName);
public slots:
};

#endif // WEBCAM_H
