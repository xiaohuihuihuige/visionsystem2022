#ifndef HIKVISIONCAMERA_H
#define HIKVISIONCAMERA_H

#include <QObject>
#include "MvCameraDefine.h"
#include"MvCamera.h"
#include<QImage>


class HikvisionCamera : public QObject
{
    Q_OBJECT

public:
    explicit HikvisionCamera(QObject *parent = nullptr);
    ~HikvisionCamera();

    QString m_CamName;//相机名
    int useCamID;//相机ID
    bool isCameraInit;//相机是否初始化
    bool isGrabStart;//相机是否开始采集
    int triggerDelay;//相机采集延时
    MvCamera*  m_pcMyCamera;   // ch:CMyCamera封装了常用接口 | en:CMyCamera packed normal used interface
    MV_CC_DEVICE_INFO_LIST m_stDevList;   // ch:设备信息列表结构体变量，用来存储设备列表

public:
    /**
     * @brief 获取相机列表
     */
    int getCamList(QVector<QString> *listDeviceName );
    /**
     * @brief 打开设备
     */
    int openDevice(int camID);
    /**
     * @brief 关闭设备
     */
    int closeDevice();
    /**
     * @brief 设置触发模式
     */
    int setTriggerMode(QString triggerModel,QString triggerSource,QString triggerDelay);
    /**
     * @brief 设置曝光
     */
    int setExposureTime(float expTime);
    /**
     * @brief 设置增益
     */
    int setGain(float gainValue);
    /**
     * @brief 设置触发延时
     */
    int setTriggerDelay(int delay);
    /**
     * @brief 获取曝光
     */
    float getExposureTime();
    /**
     * @brief 获取增益
     */
    float getGain();
    /**
     * @brief 软触发
     */
    int doSoftwareOnce();
    /**
     * @brief 开始采集
     */
    int startGrabbing();
    /**
     * @brief 停止采集
     */
    int stopGrabbing();
    /**
     * @brief 回调函数
     */
    static void __stdcall ImageCallBackEx(unsigned char *pData, MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser);
    /**
     * @brief 初始化相机
     */
    bool initCamera(QString camName,QString cameraModelAndSerial,QString triggerModel,QString triggerSource,QString exp,QString gain,QString delay);

signals:
    void getOneFrame(QImage image,QString camName);
};

#endif // HIKVISIONCAMERA_H
