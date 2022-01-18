#include "camrunning.h"
#include"camera/camerapar.h"
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include<QThread>
#include<QTime>
#include <QApplication>
#include<general/xmloperate.h>

CamRunning::CamRunning(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
}
CamRunning::~CamRunning()
{

}
//初始化参数
void CamRunning::initGlobalPar(QString pro,QString station)
{
    m_ProName=pro;
    m_ProPath=m_ExePath+"\\Pro\\"+m_ProName;
    m_StationName=station;
    m_station_path=m_ProPath+"\\station\\"+m_StationName;
    ConfigFileOperate tconfig(m_station_path+"\\StationConfig.ini");
    m_station_cam_list=tconfig.childKeys();
    cam_config_path=m_ProPath+"\\camera\\CamConfig.xml";
    readCamInfo(m_station_cam_list);
}
//读取相机配置
void CamRunning::readCamInfo(QStringList cam_use_list)
{
    QStringList camall=cam_use_list;
    m_cam.clear();
    //读取相机配置
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    for (int i=0;i<camall.size();i++)
    {
        QString cam_name=camall[i].split('-').at(0);
        m_cam[cam_name].camType=m_xml.readNode(QStringList()<<cam_name<<"CamType");
        m_cam[cam_name].cameraModelSerial=m_xml.readNode(QStringList()<<cam_name<<"CameraModelSerial");
        if(m_cam[cam_name].cameraModelSerial=="")
        {
            m_cam.remove(cam_name);
            continue;
        }
        m_cam[cam_name].resolution=m_xml.readNode(QStringList()<<cam_name<<"Resolution");
        m_cam[cam_name].exposureTime=m_xml.readNode(QStringList()<<cam_name<<"ExposureTime");
        m_cam[cam_name].grabDelay=m_xml.readNode(QStringList()<<cam_name<<"GrabDelay");
        m_cam[cam_name].gain=m_xml.readNode(QStringList()<<cam_name<<"Gain");
        m_cam[cam_name].triggerMode=m_xml.readNode(QStringList()<<cam_name<<"TriggerMode");
        m_cam[cam_name].triggerSource=m_xml.readNode(QStringList()<<cam_name<<"TriggerSource");
    }
    m_xml.closeXml();
}



//相机配置更改
void CamRunning::slot_cam_change(QString cam)
{
    if(!m_cam.contains(cam))return;
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    m_cam[cam].camType=m_xml.readNode(QStringList()<<cam<<"CamType");
    m_cam[cam].cameraModelSerial=m_xml.readNode(QStringList()<<cam<<"CameraModelSerial");
    m_cam[cam].resolution=m_xml.readNode(QStringList()<<cam<<"Resolution");
    m_cam[cam].exposureTime=m_xml.readNode(QStringList()<<cam<<"ExposureTime");
    m_cam[cam].grabDelay=m_xml.readNode(QStringList()<<cam<<"GrabDelay");
    m_cam[cam].gain=m_xml.readNode(QStringList()<<cam<<"Gain");
    m_cam[cam].triggerMode=m_xml.readNode(QStringList()<<cam<<"TriggerMode");
    m_cam[cam].triggerSource=m_xml.readNode(QStringList()<<cam<<"TriggerSource");
    m_xml.closeXml();
    if(m_cam[cam].isOpen)
    {
        EnumChange::CamType camtype=EnumChange::string2enum_cam(m_cam[cam].camType);
        switch (camtype)
        {
        case EnumChange::HikCam:
            m_cam[cam].hikvisionCamera->stopGrabbing();
            m_cam[cam].hikvisionCamera->closeDevice();
            m_cam[cam].hikvisionCamera->initCamera(cam,m_cam[cam].cameraModelSerial,m_cam[cam].triggerMode,m_cam[cam].triggerSource,m_cam[cam].exposureTime,m_cam[cam].gain,m_cam[cam].grabDelay);
            break;
        case EnumChange::WebCam:
            m_cam[cam].webCam->closeDevice();
            m_cam[cam].webCam->initCamera(cam,m_cam[cam].cameraModelSerial,m_cam[cam].resolution,m_cam[cam].triggerMode,m_cam[cam].exposureTime,m_cam[cam].grabDelay);
            break;
        case EnumChange::DahuaCam:
            break;
        case EnumChange::DahengCam:
            break;
        case EnumChange::BaslerCam:
            break;
        }
    }
}

//初始化相机
void CamRunning::slot_init_cam(bool init)
{
    if(init)
    {
        QMap<QString, CamInfo>::iterator iter = m_cam.begin();
        bool isright=true;
        while (iter != m_cam.end())
        {
            QString camName=iter.key();
            bool issuccess=true;
            EnumChange::CamType camtype=EnumChange::string2enum_cam(iter->camType);
            switch (camtype)
            {
            case EnumChange::HikCam:
                iter->hikvisionCamera=new HikvisionCamera();
                issuccess=iter->hikvisionCamera->initCamera(camName,iter->cameraModelSerial,iter->triggerMode,iter->triggerSource,iter->exposureTime,iter->gain,iter->grabDelay);
                if(issuccess)
                {
                    connect(iter->hikvisionCamera,&HikvisionCamera::getOneFrame,this,&CamRunning::slot_one_frame);
                }
                else
                {
                    isright=false;
                }
                break;
            case EnumChange::WebCam:
                iter->webCam=new WebCam();
                issuccess=iter->webCam->initCamera(camName,iter->cameraModelSerial,iter->resolution,iter->triggerMode,iter->exposureTime,iter->grabDelay);
                if(issuccess)
                {
                    connect(iter->webCam,&WebCam::getOneFrame,this,&CamRunning::slot_one_frame);
                }
                else
                {
                    isright=false;
                }
                break;
            case EnumChange::DahuaCam:
                break;
            case EnumChange::DahengCam:
                break;
            case EnumChange::BaslerCam:
                break;
            }
            if(isright)
            {
                iter->isOpen=true;
            }
            else
            {
                slot_program_exit();
                break;
            }
            iter++;
        }
        emit signal_init_cam_result(isright);
    }
    else
    {
        slot_program_exit();
        emit signal_init_cam_result(false);
    }
}

//拍照  type：0-全部拍 1-一个拍 cam：拍照得相机名
void CamRunning::slot_photo(int type,QString cam)
{
    //time_start = double(clock());
    if(type==0)
    {
        QMap<QString, CamInfo>::iterator iter = m_cam.begin();
        while (iter != m_cam.end())
        {
            if(iter->isOpen)
            {
                EnumChange::CamType camtype=EnumChange::string2enum_cam(iter->camType);
                switch (camtype)
                {
                case EnumChange::HikCam:
                    iter->hikvisionCamera->doSoftwareOnce();
                    break;
                case EnumChange::WebCam:
                    iter->webCam->doSoftwareOnce();
                    break;
                case EnumChange::DahuaCam:
                    break;
                case EnumChange::DahengCam:
                    break;
                case EnumChange::BaslerCam:
                    break;
                }
            }
            iter++;
        }
    }
    else if (type==1)
    {
        if(m_cam[cam].isOpen)
        {
            EnumChange::CamType camtype=EnumChange::string2enum_cam(m_cam[cam].camType);
            switch (camtype)
            {
            case EnumChange::HikCam:
                m_cam[cam].hikvisionCamera->doSoftwareOnce();
                break;
            case EnumChange::WebCam:
                m_cam[cam].webCam->doSoftwareOnce();
                break;
            case EnumChange::DahuaCam:
                break;
            case EnumChange::DahengCam:
                break;
            case EnumChange::BaslerCam:
                break;
            }
        }
    }
}
//接收拍照返回
void CamRunning::slot_one_frame(QImage src,QString camName)
{
    if(src.isNull()) return;
    //time_end = double(clock());//结束采图计时
    //m_runInfo=QString::fromLocal8Bit("%1采图时间:%2ms").arg(camName,QString::number(time_end-time_start));
    emit signal_one_frame(src,camName);
    //emit signal_information_text(1,m_runInfo);
}

//程序退出
void CamRunning::slot_program_exit()
{
    QMap<QString, CamInfo>::iterator iter = m_cam.begin();
    while (iter != m_cam.end())
    {
        if(iter->isOpen)
        {
            EnumChange::CamType camtype=EnumChange::string2enum_cam(iter->camType);
            switch (camtype)
            {
            case EnumChange::HikCam:
                disconnect(iter->hikvisionCamera,&HikvisionCamera::getOneFrame,this,&CamRunning::slot_one_frame);//确保只有一次链接
                iter->hikvisionCamera->stopGrabbing();
                iter->hikvisionCamera->closeDevice();
                delete  iter->hikvisionCamera;
                iter->hikvisionCamera=nullptr;
                break;
            case EnumChange::WebCam:
                disconnect(iter->webCam,&WebCam::getOneFrame,this,&CamRunning::slot_one_frame);//确保只有一次链接
                iter->webCam->closeDevice();
                delete  iter->webCam;
                iter->webCam=nullptr;
                break;
            case EnumChange::DahuaCam:
                break;
            case EnumChange::DahengCam:
                break;
            case EnumChange::BaslerCam:
                break;
            }
            iter->isOpen=false;
        }
        iter++;
    }
}
