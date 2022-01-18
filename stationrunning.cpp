#include"os/pro_pipedetect.h"
#include"os/pro_heparingcap.h"
#include"os/pro_needleseatglue.h"
#include"os/pro_connectseatglue.h"
#include"os/pro_distance.h"
#include"os/pro_needleseatcrack.h"
#include"os/pro_connectseatcrack.h"
#include"os/pro_tubebase.h"
#include"os/pro_siliconetube.h"
#include"os/pro_decodetext.h"
#include"os/pro_distanceonly.h"
#include"os/pro_gluewithseat.h"
#include"os/pro_needledirection.h"
#include"os/pro_metalcheck.h"
#include"os/pro_meltplugrowcheck.h"
#include"os/pro_oppositedetect.h"
#include"os/pro_positivedetect.h"
#include"os/crnntest.h"
#include"os/pro_allobjdetect.h"
#include"stationrunning.h"
#include<QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"
#include"camera/camrunning.h"

StationRunning::StationRunning(QObject *parent) : QObject(parent),
    m_CamRunning(nullptr)
{
    m_ExePath=qApp->applicationDirPath();
    m_isRunning=false;
    is_station_exist=false;
    cam_ischeck.clear();
}
StationRunning::~StationRunning()
{
    //slot_program_exit();
}
void StationRunning::initStationCom(QString proname,QString usestation)
{
    m_ProName=proname;
    m_ProPath=m_ExePath+"\\Pro\\"+m_ProName;
    m_StationName=usestation;
    m_AllStationPath=m_ProPath+"\\station";
    m_station_path=m_ProPath+"\\station\\"+m_StationName;
    readComInfo(m_StationName);

}
//初始化工位
void StationRunning::initStationType(QString usetype)
{
    m_type_name=usetype;
    readStationConfig();
    readCamInfo();
    initType();
    initCamRunning();
    slot_init_com(true);
}

//读取工位配置
void StationRunning::readStationConfig()
{
    ConfigFileOperate tconfig(m_station_path+"\\StationConfig.ini");
    cam_step_list=tconfig.childKeys();
    for (int i=0;i<cam_step_list.size();i++)
    {
        QString value=tconfig.readKeyValue(cam_step_list[i]);
        map_cam_function[cam_step_list[i]]=value;
    }
}
//读取相机配置
void StationRunning::readCamInfo()
{
    m_cam.clear();
    QString cam_config_path=m_ProPath+"\\camera\\CamConfig.xml";
    //读取相机配置
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    for (int i=0;i<cam_step_list.size();i++)
    {
        QString cam_name=cam_step_list[i].split('-').at(0);
        m_cam[cam_name].triggerMode=m_xml.readNode(QStringList()<<cam_name<<"TriggerMode");
        m_cam[cam_name].triggerSource=m_xml.readNode(QStringList()<<cam_name<<"TriggerSource");
    }
    m_xml.closeXml();
}
//读取通信配置
void StationRunning::readComInfo(QString useCom)
{
    XmlOperate m_xml;
    m_xml.openXml(m_AllStationPath+"\\ComConfig.xml");
    m_com_info.plcType=m_xml.readNode(QStringList()<<useCom<<"PLCType");
    if(protocol_type_list.contains( m_com_info.plcType))
    {
        is_station_exist=true;
    }
    m_com_info.serverID=m_xml.readNode(QStringList()<<useCom<<"ServerID").toInt();
    m_com_info.readTriggerSignal=m_xml.readNode(QStringList()<<useCom<<"ReadTriggerSignal").toInt();
    m_com_info.sec=m_xml.readNode(QStringList()<<useCom<<"Sec").toInt();
    m_plcType=EnumChange::string2enum_plc(m_com_info.plcType);
    switch (m_plcType)
    {
    case EnumChange::ModbusTCP:
    case EnumChange::MELSEC_MC:
    case EnumChange::TCP:
        m_com_info.tcpIPPort=m_xml.readNode(QStringList()<<useCom<<"TcpIPPort");
        break;
    case EnumChange::ModbusRTU:
    case EnumChange::MELSEC_FX3U:
    case EnumChange::HostLink:
    case EnumChange::Serial:
    case EnumChange::SerialRelay:
    case EnumChange::WeiKongPLC:
        m_com_info.portname=m_xml.readNode(QStringList()<<useCom<<"PortName");
        m_com_info.parity=m_xml.readNode(QStringList()<<useCom<<"Parity");
        m_com_info.baudrate=m_xml.readNode(QStringList()<<useCom<<"BaudRate").toInt();
        m_com_info.dataBits=m_xml.readNode(QStringList()<<useCom<<"DataBits").toInt();
        m_com_info.stopBits=m_xml.readNode(QStringList()<<useCom<<"StopBits").toInt();
        break;
    }
    QStringList addr_all=m_xml.getChild(QStringList()<<useCom<<"Addr");
    for(int j=0;j<addr_all.size();j++)
    {
        m_com_info.addr[addr_all[j]]=m_com_info.stopBits=m_xml.readNode(QStringList()<<useCom<<"Addr"<<addr_all[j]).toInt();
    }
    m_xml.closeXml();

    return;
}
//初始化相机运行类
void StationRunning::initCamRunning()
{
    m_CamRunning=new CamRunning(this);
    m_CamRunning->initGlobalPar(m_ProName,m_StationName);
    connect(this,&StationRunning::signal_init_cam,m_CamRunning,&CamRunning::slot_init_cam);
    connect(this,&StationRunning::signal_cam_change,m_CamRunning,&CamRunning::slot_cam_change);
    connect(this,&StationRunning::signal_photo,m_CamRunning,&CamRunning::slot_photo);
    connect(this,&StationRunning::signal_program_exit,m_CamRunning,&CamRunning::slot_program_exit);
    connect(m_CamRunning,&CamRunning::signal_init_cam_result,this,&StationRunning::slot_init_cam_result);
    connect(m_CamRunning,&CamRunning::signal_one_frame,this,&StationRunning::slot_one_frame);
    connect(m_CamRunning,&CamRunning::signal_information_text,this,&StationRunning::slot_information_text);

}
//初始化品种类，并加入线程
void StationRunning::initType()
{

    QMap<QString,CamInfo>::iterator iter=m_cam.begin();
    while (iter!=m_cam.end())
    {
        QString cam_name=iter.key();
        QStringList cam_step_list=GeneralFunc::GetAllFolderName(m_station_path+"\\"+cam_name);
        for (int i=0;i<cam_step_list.size();i++)
        {
            QString key=cam_name+"-"+cam_step_list[i];
            map_Thread[key]=new QThread();
            EnumChange::Func func=EnumChange::string2enum_pro(map_cam_function[key]);
            //创建检测类
            switch (func)
            {
            case EnumChange::Pro_PipeDetect:
                map_Pro_PipeDetect[key]=new Pro_PipeDetect();
                map_Pro_PipeDetect[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_PipeDetect[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_PipeDetect[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_PipeDetect[key],&Pro_PipeDetect::slot_program_exit);
                connect(map_Pro_PipeDetect[key],&Pro_PipeDetect::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_PipeDetect[key],&Pro_PipeDetect::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_PipeDetect[key],&Pro_PipeDetect::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_HeparingCap:
                map_Pro_HeparingCap[key]=new Pro_HeparingCap();
                map_Pro_HeparingCap[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_HeparingCap[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_HeparingCap[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_HeparingCap[key],&Pro_HeparingCap::slot_program_exit);
                connect(map_Pro_HeparingCap[key],&Pro_HeparingCap::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_HeparingCap[key],&Pro_HeparingCap::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_HeparingCap[key],&Pro_HeparingCap::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_NeedleSeatGlue:
                map_Pro_NeedleSeatGlue[key]=new Pro_NeedleSeatGlue();
                map_Pro_NeedleSeatGlue[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_NeedleSeatGlue[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_NeedleSeatGlue[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::slot_program_exit);
                connect(map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_NeedleSeatGlue[key],&Pro_NeedleSeatGlue::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_ConnectSeatGlue:
                map_Pro_ConnectSeatGlue[key]=new Pro_ConnectSeatGlue();
                map_Pro_ConnectSeatGlue[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_ConnectSeatGlue[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_ConnectSeatGlue[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::slot_program_exit);
                connect(map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_ConnectSeatGlue[key],&Pro_ConnectSeatGlue::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_Distance:
                map_Pro_Distance[key]=new Pro_Distance();
                map_Pro_Distance[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_Distance[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_Distance[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_Distance[key],&Pro_Distance::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_Distance[key],&Pro_Distance::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_Distance[key],&Pro_Distance::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_Distance[key],&Pro_Distance::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_Distance[key],&Pro_Distance::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_Distance[key],&Pro_Distance::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_Distance[key],&Pro_Distance::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_Distance[key],&Pro_Distance::slot_program_exit);
                connect(map_Pro_Distance[key],&Pro_Distance::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_Distance[key],&Pro_Distance::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_Distance[key],&Pro_Distance::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_NeedleSeatCrack:
                map_Pro_NeedleSeatCrack[key]=new Pro_NeedleSeatCrack();
                map_Pro_NeedleSeatCrack[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_NeedleSeatCrack[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_NeedleSeatCrack[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::slot_program_exit);
                connect(map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_NeedleSeatCrack[key],&Pro_NeedleSeatCrack::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_ConnectSeatCrack:
                map_Pro_ConnectSeatCrack[key]=new Pro_ConnectSeatCrack();
                map_Pro_ConnectSeatCrack[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_ConnectSeatCrack[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_ConnectSeatCrack[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::slot_program_exit);
                connect(map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_ConnectSeatCrack[key],&Pro_ConnectSeatCrack::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_TubeBase:
                map_Pro_TubeBase[key]=new Pro_TubeBase();
                map_Pro_TubeBase[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_TubeBase[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_TubeBase[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_TubeBase[key],&Pro_TubeBase::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_TubeBase[key],&Pro_TubeBase::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_TubeBase[key],&Pro_TubeBase::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_TubeBase[key],&Pro_TubeBase::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_TubeBase[key],&Pro_TubeBase::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_TubeBase[key],&Pro_TubeBase::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_TubeBase[key],&Pro_TubeBase::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_TubeBase[key],&Pro_TubeBase::slot_program_exit);
                connect(map_Pro_TubeBase[key],&Pro_TubeBase::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_TubeBase[key],&Pro_TubeBase::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_TubeBase[key],&Pro_TubeBase::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_SiliconeTube:
                map_Pro_SiliconeTube[key]=new Pro_SiliconeTube();
                map_Pro_SiliconeTube[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_SiliconeTube[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_SiliconeTube[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_SiliconeTube[key],&Pro_SiliconeTube::slot_program_exit);
                connect(map_Pro_SiliconeTube[key],&Pro_SiliconeTube::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_SiliconeTube[key],&Pro_SiliconeTube::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_SiliconeTube[key],&Pro_SiliconeTube::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_DecodeText:
                map_Pro_DecodeText[key]=new Pro_DecodeText();
                map_Pro_DecodeText[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_DecodeText[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_DecodeText[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_DecodeText[key],&Pro_DecodeText::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_DecodeText[key],&Pro_DecodeText::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_DecodeText[key],&Pro_DecodeText::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_DecodeText[key],&Pro_DecodeText::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_DecodeText[key],&Pro_DecodeText::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_DecodeText[key],&Pro_DecodeText::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_DecodeText[key],&Pro_DecodeText::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_DecodeText[key],&Pro_DecodeText::slot_program_exit);
                connect(map_Pro_DecodeText[key],&Pro_DecodeText::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_DecodeText[key],&Pro_DecodeText::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_DecodeText[key],&Pro_DecodeText::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_DistanceOnly:
                map_Pro_DistanceOnly[key]=new Pro_DistanceOnly();
                map_Pro_DistanceOnly[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_DistanceOnly[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_DistanceOnly[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_DistanceOnly[key],&Pro_DistanceOnly::slot_program_exit);
                connect(map_Pro_DistanceOnly[key],&Pro_DistanceOnly::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_DistanceOnly[key],&Pro_DistanceOnly::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_DistanceOnly[key],&Pro_DistanceOnly::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_GlueWithSeat:
                map_Pro_GlueWithSeat[key]=new Pro_GlueWithSeat();
                map_Pro_GlueWithSeat[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_GlueWithSeat[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_GlueWithSeat[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::slot_program_exit);
                connect(map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_GlueWithSeat[key],&Pro_GlueWithSeat::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_NeedleDirection:
                map_Pro_NeedleDirection[key]=new Pro_NeedleDirection();
                map_Pro_NeedleDirection[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_NeedleDirection[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_NeedleDirection[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_NeedleDirection[key],&Pro_NeedleDirection::slot_program_exit);
                connect(map_Pro_NeedleDirection[key],&Pro_NeedleDirection::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_NeedleDirection[key],&Pro_NeedleDirection::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_NeedleDirection[key],&Pro_NeedleDirection::signal_result,this,&StationRunning::slot_write_result);
                break;

            case EnumChange::Pro_MetalCheck:
                map_Pro_MetalCheck[key]=new Pro_MetalCheck();
                map_Pro_MetalCheck[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_MetalCheck[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_MetalCheck[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_MetalCheck[key],&Pro_MetalCheck::slot_program_exit);
                connect(map_Pro_MetalCheck[key],&Pro_MetalCheck::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_MetalCheck[key],&Pro_MetalCheck::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_MetalCheck[key],&Pro_MetalCheck::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_MeltPlugRowCheck:
                map_Pro_MeltPlugRowCheck[key]=new Pro_MeltPlugRowCheck();
                map_Pro_MeltPlugRowCheck[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_MeltPlugRowCheck[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_MeltPlugRowCheck[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::slot_program_exit);
                connect(map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_MeltPlugRowCheck[key],&Pro_MeltPlugRowCheck::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_PositiveDetect:
                map_Pro_PositiveDetect[key]=new Pro_PositiveDetect();
                map_Pro_PositiveDetect[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_PositiveDetect[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_PositiveDetect[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_PositiveDetect[key],&Pro_PositiveDetect::slot_program_exit);
                connect(map_Pro_PositiveDetect[key],&Pro_PositiveDetect::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_PositiveDetect[key],&Pro_PositiveDetect::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_PositiveDetect[key],&Pro_PositiveDetect::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_OppositeDetect:
                map_Pro_OppositeDetect[key]=new Pro_OppositeDetect();
                map_Pro_OppositeDetect[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_OppositeDetect[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_OppositeDetect[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_OppositeDetect[key],&Pro_OppositeDetect::slot_program_exit);
                connect(map_Pro_OppositeDetect[key],&Pro_OppositeDetect::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_OppositeDetect[key],&Pro_OppositeDetect::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_OppositeDetect[key],&Pro_OppositeDetect::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::CrnnTest:
                map_CrnnTest[key]=new CrnnTest();
                map_CrnnTest[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_CrnnTest[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_CrnnTest[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_CrnnTest[key],&CrnnTest::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_CrnnTest[key],&CrnnTest::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_CrnnTest[key],&CrnnTest::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_CrnnTest[key],&CrnnTest::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_CrnnTest[key],&CrnnTest::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_CrnnTest[key],&CrnnTest::slot_check);
                connect(this,&StationRunning::signal_press_point,map_CrnnTest[key],&CrnnTest::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_CrnnTest[key],&CrnnTest::slot_program_exit);
                connect(map_CrnnTest[key],&CrnnTest::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_CrnnTest[key],&CrnnTest::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_CrnnTest[key],&CrnnTest::signal_result,this,&StationRunning::slot_write_result);
                break;
            case EnumChange::Pro_AllObjDetect:
                map_Pro_AllObjDetect[key]=new Pro_AllObjDetect();
                map_Pro_AllObjDetect[key]->initGlobalPar(m_ProName,m_StationName,cam_name,cam_step_list[i],m_type_name);
                map_Pro_AllObjDetect[key]->moveToThread(map_Thread[key]);
                connect(map_Thread[key],&QThread::finished,map_Thread[key],&QObject::deleteLater);
                connect(map_Thread[key],&QThread::finished,map_Pro_AllObjDetect[key],&QObject::deleteLater);
                connect(this,&StationRunning::signal_roi_change,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_roi_change);
                connect(this,&StationRunning::signal_type_change,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_type_change);
                connect(this,&StationRunning::signal_par_change,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_par_change);
                connect(this,&StationRunning::signal_read_image,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_read_image);
                connect(this,&StationRunning::signal_one_frame,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_one_frame);
                connect(this,&StationRunning::signal_check,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_check);
                connect(this,&StationRunning::signal_press_point,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_press_point);
                connect(this,&StationRunning::signal_program_exit,map_Pro_AllObjDetect[key],&Pro_AllObjDetect::slot_program_exit);
                connect(map_Pro_AllObjDetect[key],&Pro_AllObjDetect::signal_information_image,this,&StationRunning::slot_information_image);
                connect(map_Pro_AllObjDetect[key],&Pro_AllObjDetect::signal_information_text,this,&StationRunning::slot_information_text);
                connect(map_Pro_AllObjDetect[key],&Pro_AllObjDetect::signal_result,this,&StationRunning::slot_write_result);
                break;
            }
            map_Thread[key]->start();
        }
        iter++;
    }
}

//初始化通信
void StationRunning::slot_init_com(bool init)
{
    //"ModbusTCP"<<"MELSEC-MC"<<"TCP"<<"ModbusRTU"<<"MELSEC-FX3U"<<"HostLink"<<"Serial";
    QString comname=m_StationName;
    if(init)
    {
        thread_plc=new QThread();
        switch (m_plcType)
        {
        case EnumChange::ModbusTCP:
            m_com_info.modbusTcp=new ModbusTcp();
            m_com_info.modbusTcp->initPar(comname,
                                          m_com_info.tcpIPPort,
                                          m_com_info.serverID,
                                          m_com_info.sec,
                                          m_com_info.readTriggerSignal,
                                          m_com_info.addr);

            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.modbusTcp,&QObject::deleteLater);
            connect(m_com_info.modbusTcp,&ModbusTcp::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            connect(m_com_info.modbusTcp,&ModbusTcp::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_read,m_com_info.modbusTcp,&ModbusTcp::slot_read);
            connect(this,&StationRunning::signal_write,m_com_info.modbusTcp,&ModbusTcp::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.modbusTcp,&ModbusTcp::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.modbusTcp,&ModbusTcp::slot_com_stop);
            m_com_info.modbusTcp->moveToThread(thread_plc);
            break;
        case EnumChange::MELSEC_MC:
            m_com_info.mcNet=new MCNet();
            m_com_info.mcNet->initPar(comname,
                                      m_com_info.tcpIPPort,
                                      m_com_info.serverID,
                                      m_com_info.sec,
                                      m_com_info.readTriggerSignal,
                                      m_com_info.addr);
            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.mcNet,&QObject::deleteLater);
            connect(m_com_info.mcNet,&MCNet::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            connect(m_com_info.mcNet,&MCNet::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_read,m_com_info.mcNet,&MCNet::slot_read);
            connect(this,&StationRunning::signal_write,m_com_info.mcNet,&MCNet::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.mcNet,&MCNet::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.mcNet,&MCNet::slot_com_stop);
            m_com_info.mcNet->moveToThread(thread_plc);
            break;
        case EnumChange::TCP:

            break;
        case EnumChange::ModbusRTU:
            m_com_info.modbusRTU=new ModbusRTU();
            m_com_info.modbusRTU->initPar(comname,
                                          m_com_info.portname,
                                          m_com_info.parity,
                                          m_com_info.baudrate,
                                          m_com_info.dataBits,
                                          m_com_info.stopBits,
                                          m_com_info.serverID,
                                          m_com_info.sec,
                                          m_com_info.readTriggerSignal,
                                          m_com_info.addr);
            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.modbusRTU,&QObject::deleteLater);
            connect(m_com_info.modbusRTU,&ModbusRTU::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            connect(m_com_info.modbusRTU,&ModbusRTU::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_read,m_com_info.modbusRTU,&ModbusRTU::slot_read);
            connect(this,&StationRunning::signal_write,m_com_info.modbusRTU,&ModbusRTU::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.modbusRTU,&ModbusRTU::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.modbusRTU,&ModbusRTU::slot_com_stop);
            m_com_info.modbusRTU->moveToThread(thread_plc);
            break;
        case EnumChange::MELSEC_FX3U:
            m_com_info.fxProtocol=new FXProtocol();
            m_com_info.fxProtocol->initPar(comname,
                                           m_com_info.portname,
                                           m_com_info.parity,
                                           m_com_info.baudrate,
                                           m_com_info.dataBits,
                                           m_com_info.stopBits,
                                           m_com_info.sec,
                                           m_com_info.readTriggerSignal,
                                           m_com_info.addr);
            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.fxProtocol,&QObject::deleteLater);
            connect(m_com_info.fxProtocol,&FXProtocol::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            connect(m_com_info.fxProtocol,&FXProtocol::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_read,m_com_info.fxProtocol,&FXProtocol::slot_read);
            connect(this,&StationRunning::signal_write,m_com_info.fxProtocol,&FXProtocol::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.fxProtocol,&FXProtocol::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.fxProtocol,&FXProtocol::slot_com_stop);
            m_com_info.fxProtocol->moveToThread(thread_plc);
            break;
        case EnumChange::HostLink:
            m_com_info.hostLink=new HostLink();
            m_com_info.hostLink->initPar(comname,
                                         m_com_info.portname,
                                         m_com_info.parity,
                                         m_com_info.baudrate,
                                         m_com_info.dataBits,
                                         m_com_info.stopBits,
                                         m_com_info.sec,
                                         m_com_info.readTriggerSignal,
                                         m_com_info.addr);
            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.hostLink,&QObject::deleteLater);
            connect(m_com_info.hostLink,&HostLink::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            connect(m_com_info.hostLink,&HostLink::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_read,m_com_info.hostLink,&HostLink::slot_read);
            connect(this,&StationRunning::signal_write,m_com_info.hostLink,&HostLink::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.hostLink,&HostLink::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.hostLink,&HostLink::slot_com_stop);

            m_com_info.hostLink->moveToThread(thread_plc);
            break;
        case EnumChange::Serial:
            break;
        case EnumChange::SerialRelay:
            m_com_info.serialRelay=new SerialRelay();
            m_com_info.serialRelay->initPar(comname,
                                            m_com_info.portname,
                                            m_com_info.parity,
                                            m_com_info.baudrate,
                                            m_com_info.dataBits,
                                            m_com_info.stopBits,
                                            m_com_info.readTriggerSignal,
                                            m_com_info.sec,
                                            m_com_info.addr);
            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.serialRelay,&QObject::deleteLater);
            connect(m_com_info.serialRelay,&SerialRelay::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_write,m_com_info.serialRelay,&SerialRelay::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.serialRelay,&SerialRelay::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.serialRelay,&SerialRelay::slot_com_stop);
            m_com_info.serialRelay->moveToThread(thread_plc);
            break;
        case EnumChange::WeiKongPLC:
            m_com_info.weiKongPLC=new WeiKongPLC();
            m_com_info.weiKongPLC->initPar(comname,
                                           m_com_info.serverID,
                                           m_com_info.portname,
                                           m_com_info.parity,
                                           m_com_info.baudrate,
                                           m_com_info.dataBits,
                                           m_com_info.stopBits,
                                           m_com_info.sec,
                                           m_com_info.readTriggerSignal,
                                           m_com_info.addr);
            connect(thread_plc,&QThread::finished,thread_plc,&QObject::deleteLater);
            connect(thread_plc,&QThread::finished,m_com_info.weiKongPLC,&QObject::deleteLater);
            connect(m_com_info.weiKongPLC,&WeiKongPLC::signal_com_state,this,&StationRunning::slot_com_state);
            connect(this,&StationRunning::signal_write,m_com_info.weiKongPLC,&WeiKongPLC::slot_write);
            connect(this,&StationRunning::signal_com_start,m_com_info.weiKongPLC,&WeiKongPLC::slot_com_start);
            connect(this,&StationRunning::signal_com_stop,m_com_info.weiKongPLC,&WeiKongPLC::slot_com_stop);
            m_com_info.weiKongPLC->moveToThread(thread_plc);
            break;
        }
        emit signal_com_start();
        thread_plc->start();

    }
    else
    {
        emit signal_com_stop();
        switch (m_plcType)
        {
        case EnumChange::ModbusTCP:
            disconnect(m_com_info.modbusTcp,&ModbusTcp::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            disconnect(m_com_info.modbusTcp,&ModbusTcp::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_read,m_com_info.modbusTcp,&ModbusTcp::slot_read);
            disconnect(this,&StationRunning::signal_write,m_com_info.modbusTcp,&ModbusTcp::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.modbusTcp,&ModbusTcp::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.modbusTcp,&ModbusTcp::slot_com_stop);

            break;
        case EnumChange::MELSEC_MC:
            disconnect(m_com_info.mcNet,&MCNet::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            disconnect(m_com_info.mcNet,&MCNet::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_read,m_com_info.mcNet,&MCNet::slot_read);
            disconnect(this,&StationRunning::signal_write,m_com_info.mcNet,&MCNet::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.mcNet,&MCNet::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.mcNet,&MCNet::slot_com_stop);
            break;
        case EnumChange::TCP:
            break;
        case EnumChange::ModbusRTU:
            disconnect(m_com_info.modbusRTU,&ModbusRTU::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            disconnect(m_com_info.modbusRTU,&ModbusRTU::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_read,m_com_info.modbusRTU,&ModbusRTU::slot_read);
            disconnect(this,&StationRunning::signal_write,m_com_info.modbusRTU,&ModbusRTU::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.modbusRTU,&ModbusRTU::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.modbusRTU,&ModbusRTU::slot_com_stop);
            break;
        case EnumChange::MELSEC_FX3U:
            disconnect(m_com_info.fxProtocol,&FXProtocol::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            disconnect(m_com_info.fxProtocol,&FXProtocol::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_read,m_com_info.fxProtocol,&FXProtocol::slot_read);
            disconnect(this,&StationRunning::signal_write,m_com_info.fxProtocol,&FXProtocol::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.fxProtocol,&FXProtocol::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.fxProtocol,&FXProtocol::slot_com_stop);
            break;
        case EnumChange::HostLink:
            disconnect(m_com_info.hostLink,&HostLink::signal_readFinish,this,&StationRunning::slot_receive_plc_data);
            disconnect(m_com_info.hostLink,&HostLink::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_read,m_com_info.hostLink,&HostLink::slot_read);
            disconnect(this,&StationRunning::signal_write,m_com_info.hostLink,&HostLink::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.hostLink,&HostLink::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.hostLink,&HostLink::slot_com_stop);
            break;
        case EnumChange::Serial:
            break;
        case EnumChange::SerialRelay:
            disconnect(m_com_info.serialRelay,&SerialRelay::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_write,m_com_info.serialRelay,&SerialRelay::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.serialRelay,&SerialRelay::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.serialRelay,&SerialRelay::slot_com_stop);
            break;
        case EnumChange::WeiKongPLC:
            disconnect(m_com_info.weiKongPLC,&WeiKongPLC::signal_com_state,this,&StationRunning::slot_com_state);
            disconnect(this,&StationRunning::signal_write,m_com_info.weiKongPLC,&WeiKongPLC::slot_write);
            disconnect(this,&StationRunning::signal_com_start,m_com_info.weiKongPLC,&WeiKongPLC::slot_com_start);
            disconnect(this,&StationRunning::signal_com_stop,m_com_info.weiKongPLC,&WeiKongPLC::slot_com_stop);
            break;
        }
        delete_com();
    }
}
//通信异常
void StationRunning::slot_com_state(QString comname,bool state)
{
    if(state)
    {
        m_com_info.isConnect=true;
        emit signal_information_text(comname,4,comname+tr(QString::fromLocal8Bit("连接成功").toUtf8()).toUtf8());
    }
    else
    {
        m_com_info.isConnect=false;
        emit signal_information_text(comname,5,comname+tr(QString::fromLocal8Bit("连接异常").toUtf8()).toUtf8());
    }
}

//初始化相机结果
void StationRunning::slot_init_cam_result(bool isright)
{
    emit signal_init_cam_result(isright);
}

//初始化相机
void StationRunning::slot_init_cam(bool isinit)
{
    emit signal_init_cam(isinit);
}

//接收PLC数据
void StationRunning::slot_receive_plc_data(QList<int> data,QString comName)
{  
    if(comName!=m_StationName)
        return;
    check_cam_is_photo(data);
    if(isReadCheckOnce)
    {
        QString rev;
        for (int i = 0; i < data.size(); i++)
        {
            const QString entry = tr("%1-%2; ").arg(i)
                    .arg(QString::number(data.value(i),10));
            rev+=entry;
        }
        emit signal_information_text(comName,1,rev);
        isReadCheckOnce=false;
    }
}

//判断拍照得相机，并发送拍照信号
void StationRunning::check_cam_is_photo(QList<int> data)
{
    time_start=double(clock());
    for (QMap<QString,CamInfo>::iterator iter=m_cam.begin();iter!=m_cam.end();iter++)
    {
        QString camname=iter.key();
        CamInfo caminfo=iter.value();
        if(data.size()>0&&data[m_com_info.addr[camname+QString::fromLocal8Bit("拍照地址")]]!=0)
        {
            g_cam_step[camname]="step"+QString::number(data[m_com_info.addr[camname+QString::fromLocal8Bit("拍照地址")]]-1);
            emit signal_information_text(camname,1,"");
            cam_ischeck[camname]=true;
            if(caminfo.triggerMode=="1"&&caminfo.triggerSource=="7")
            {
                emit signal_photo(1,camname);
            }
        }
    }
}

//写入结果到PLC
void StationRunning::slot_write_result(QList<int> sendlist,QString cam,QString step)
{
    if(!m_cam.contains(cam))
        return;
    if(m_isRunning)
        emit signal_write(m_StationName,m_com_info.addr[cam+"-"+step+QString::fromLocal8Bit("结果地址")],sendlist);

    if(m_ProName=="DistanceAndGlue")
    {
        if(sendlist[0]==2)
            m_isRunning=false;
    }

}

//开始运行
void StationRunning::slot_start_running(bool isRunning)
{
    m_isRunning=isRunning;
    if(m_ProName=="DistanceAndGlue")
    {
        if(isRunning)
            emit signal_write(m_StationName,m_com_info.addr[m_cur_cam_name+"-"+m_cur_cam_step+QString::fromLocal8Bit("结果地址")],QList<int>()<<1<<1<<1<<1);
        else
            emit signal_write(m_StationName,m_com_info.addr[m_cur_cam_name+"-"+m_cur_cam_step+QString::fromLocal8Bit("结果地址")],QList<int>()<<2<<2<<2<<2);
    }
}

//拍照
void StationRunning::slot_photo(QString station, QString cam,QString step)
{
    if(station!=m_StationName)
        return;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    g_cam_step[m_cur_cam_name]=m_cur_cam_step;
    time_start=double(clock());
    emit signal_photo(1,m_cur_cam_name);
}

//接收一帧
void StationRunning::slot_one_frame(QImage image, QString camname)
{
    if(m_cam[camname].triggerMode=="1")
    {
        //清空
        emit signal_information_text(camname,1,"");
        //时间
        QDateTime time=QDateTime::currentDateTime();
        QString strtime=time.toString("hh:mm:ss");
        emit signal_information_text(camname,1,strtime);
        //检测
        emit signal_one_frame(image,camname,m_isRunning,g_cam_step[camname]);
        //拍照时间
        time_end=double(clock());
        if(m_cam[camname].triggerSource=="7")
        {
            m_runInfo=tr(QString::fromLocal8Bit("%1采集时间:%2ms").arg(camname,QString::number(time_end-time_start)).toUtf8());
            emit signal_information_text(camname,1,m_runInfo);
        }
    }
    else
    {
        if(cam_ischeck[camname])
        {
            emit signal_one_frame(image,camname,cam_ischeck[camname],g_cam_step[camname]);
            emit signal_information_text(camname,1,"");
            time_end=double(clock());
            m_runInfo=tr(QString::fromLocal8Bit("%1采集时间:%2ms").arg(camname,QString::number(time_end-time_start)).toUtf8());
            emit signal_information_text(camname,1,m_runInfo);
        }
        else
        {
            emit signal_information_image(camname,0,image);
        }
        cam_ischeck[camname]=false;
    }
}

//读取图片
void StationRunning::slot_read_image(QString station, QString cam,QString step, QString imgPath)
{
    if(station!=m_StationName)
        return;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    emit signal_read_image(cam,step,imgPath);
}

//检测
void StationRunning::slot_check(QString station, QString cam,QString step)
{
    if(station!=m_StationName)
        return;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    emit signal_check(cam,step);
}

//读取PLC
void StationRunning::slot_read_plc_photo(QString station, QString cam,QString step)
{
    if(station!=m_StationName)
        return;
    isReadCheckOnce=true;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    emit signal_read(station);
}

//强制
void StationRunning::slot_force_set(QString station, QString cam,QString step, bool isright)
{
    if(station!=m_StationName)
        return;
    if(!m_com_info.isConnect)
        return;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    int writedata=0;
    //isright?writedata=1:writedata=2;
    isright?writedata=1:writedata=2;
    QList<int> data;
    if(m_ProName=="DistanceAndGlue")
    {
        data<<writedata<<writedata;
        emit signal_write(m_StationName,m_com_info.addr[m_cur_cam_name+"-"+m_cur_cam_step+QString::fromLocal8Bit("结果地址")],data);
    }
    else
    {
        data<<writedata;
        emit signal_write(station,m_com_info.addr[m_cur_cam_name+"-"+m_cur_cam_step+QString::fromLocal8Bit("结果地址")],data);
    }
}

//图像信息
void StationRunning::slot_information_image(QString camname,int type, QImage showImage)
{
    emit signal_information_image(camname,type,showImage);
}

//文字信息
void StationRunning::slot_information_text(QString cam,int type, QString runinfo)
{
    emit signal_information_text(cam,type,runinfo);
}

//点击事件
void StationRunning::slot_press_point( QString cam, QPoint point, QSize size)
{
    emit signal_press_point(cam,point,size);
}

//程序结束前，断开一切链接
void StationRunning::slot_program_exit()
{
    emit signal_program_exit();
    delete_com();
    QMap<QString,QThread*>::iterator iter=map_Thread.begin();
    while(iter!=map_Thread.end())
    {
        if(map_Thread[iter.key()]!=nullptr)
        {
            map_Thread[iter.key()]->quit();
            map_Thread[iter.key()]->wait();
            //map_Thread[iter.key()]->requestInterruption();
            //map_Thread[iter.key()]->wait();
            //map_Thread[iter.key()]->terminate();
            delete map_Thread[iter.key()];
            map_Thread[iter.key()]=nullptr;
        }
        iter++;
    }
    if(m_CamRunning!=nullptr)
    {
        delete  m_CamRunning;
        m_CamRunning=nullptr;
    }
}


//相机参数变化
void StationRunning::slot_cam_change(QString cam)
{
    readCamInfo();
    emit signal_cam_change(cam);
}

//通信参数变化
void StationRunning::slot_com_change(QString com)
{
    if(com!=m_StationName)
        return;
    slot_init_com(false);
    readComInfo(m_StationName);
    slot_init_com(true);
}

//选区变化
void StationRunning::slot_roi_change(QString station, QString cam,QString step)
{
    if(station!=m_StationName)
        return;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    emit signal_roi_change(cam,step);
}


//启用品种变化
void StationRunning::slot_type_change(QString type)
{
    emit signal_type_change(type);
}
//参数变化
void StationRunning::slot_par_change(QString station, QString cam,QString step)
{
    if(station!=m_StationName)
        return;
    m_cur_cam_name=cam;
    m_cur_cam_step=step;
    emit signal_par_change(cam,step);
}
//删除通信指针
void StationRunning::delete_com()
{
    if(thread_plc!=nullptr)
    {
        m_isRunning=false;
        thread_plc->exit(0);
        thread_plc->wait();
        delete thread_plc;
        thread_plc=nullptr;
        m_com_info.isConnect=false;
    }
}



