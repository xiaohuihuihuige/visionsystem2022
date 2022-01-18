#ifndef DAHUACAMERA_H
#define DAHUACAMERA_H
//将一下配置粘贴到pro文件中
//LIBS += $$quote( D:/Program Files/DahuaTech/MV Viewer/Development/Lib/x64/MVSDKmd.lib)
//LIBS += $$quote( D:/Program Files/DahuaTech/MV Viewer/Development/Lib/x64/VideoRender.lib)
//LIBS += $$quote( D:/Program Files/DahuaTech/MV Viewer/Development/Lib/x64/ImageConvert.lib)
//#DESTDIR = $$quote( D:/Program Files/DahuaTech/MV Viewer/Development/DLL)
//INCLUDEPATH = $$quote(  D:/Program Files/DahuaTech/MV Viewer/Development/Include\ )
//DEPENDPATH += $$quote( D:/Program Files/DahuaTech/MV Viewer/Development/Lib/x64 )



#include <QObject>
#include <QWidget>
#include "GenICam/System.h"
#include "Media/VideoRender.h"
#include "Media/ImageConvert.h"
#include <QElapsedTimer>
#include <QMutex>


// 帧信息
// frame imformation
class CFrameInfo : public Dahua::Memory::CBlock
{
public:
    CFrameInfo()
    {
        m_pImageBuf = NULL;
        m_nBufferSize = 0;
        m_nWidth = 0;
        m_nHeight = 0;
        m_ePixelType = Dahua::GenICam::gvspPixelMono8;
        m_nPaddingX = 0;
        m_nPaddingY = 0;
        m_nTimeStamp = 0;
    }
    ~CFrameInfo()
    {
    }

public:
    BYTE		*m_pImageBuf;
    int			m_nBufferSize;
    int			m_nWidth;
    int			m_nHeight;
    Dahua::GenICam::EPixelType	m_ePixelType;
    int			m_nPaddingX;
    int			m_nPaddingY;
    uint64_t	m_nTimeStamp;
};
class DaHuaCamera : public QObject
{
    Q_OBJECT
public:
    explicit DaHuaCamera(QObject *parent = nullptr);
    ~DaHuaCamera();
    enum ETrigType
    {
        trigContinous = 0,	// 连续拉流 | continue grabbing
        trigSoftware = 1,	// 软件触发 | software trigger
        trigLine = 2,		// 外部触发	| external trigger
    };
    // 打开相机
    // open cmaera
    bool CameraOpen(void);
    // 关闭相机
    // close camera
    bool CameraClose(void);
    // 开始采集
    // start grabbing
    bool CameraStart(void);
    // 停止采集
    // stop grabbing
    bool CameraStop(void);
    // 取流回调函数
    // get frame callback function
    void FrameCallback(const Dahua::GenICam::CFrame & frame);
    // 切换采集方式、触发方式 （连续采集、外部触发、软件触发）
    // Switch acquisition mode and triggering mode (continuous acquisition, external triggering and software triggering)
    bool CameraChangeTrig( QString triggerModel,QString triggerSource);
    // 执行一次软触发
    // Execute a soft trigger
    bool ExecuteSoftTrig(void);
    // 设置曝光
    // set exposuse time
    bool SetExposeTime(double dExposureTime);
    // 设置增益
    // set gain
    bool SetAdjustPlus(double dGainRaw);
    // 设置当前相机
    // set current camera
    void SetCamera(const QString& strKey);
    //设置相机列表
    bool setPixelFormat(int type);
    //获取相机列表
    QStringList getCameraList();
    //设置延时
    bool setDelay(double delay);

    Dahua::GenICam::ICameraPtr m_pCamera;							// 当前相机 | current camera
    Dahua::GenICam::IStreamSourcePtr m_pStreamSource;				// 流对象   |  stream object
    Dahua::Infra::TVector<Dahua::GenICam::ICameraPtr> m_vCameraPtrList;

    QString m_camname;
    void takePhoto(QImage *ho_Image,int sleeptime);
    bool initCamera(QString camName,QString strKey, QString triggerModel,QString triggerSource,QString exp,QString delay);


signals:
     void getOneFrame(QString camname,QImage ho_Image);
public slots:
};

#endif // DAHUACAMERA_H
