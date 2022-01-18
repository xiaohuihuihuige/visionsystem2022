#ifndef STATIONRUNNING_H
#define STATIONRUNNING_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>

class StationRunning : public QObject
{
    Q_OBJECT
public:
    explicit StationRunning(QObject *parent = nullptr);
    ~StationRunning();
    QString m_ExePath;//程序路径
    QString m_ProPath;//项目文件路径
    QString m_ProName;//项目名称
    QString m_AllStationPath;
    QString m_StationName;//工位名称
    QString m_station_path; //工位路径
    QString m_type_name;//品种名称
    QString m_cur_cam_name;//当前主界面上选择的相机
    QString m_cur_cam_step;//主界面上选择得步骤
    QStringList cam_step_list;//配置的相机列表
    QMap<QString,CamInfo> m_cam;//运行的相机列表

    EnumChange::PLCCom m_plcType;  //PLC名对应的PLC类型
    ComInfo m_com_info;//PLC
    QThread *thread_plc=nullptr;

    bool is_station_exist=false;

    class CamRunning *m_CamRunning;//相机运行类

    QMap<QString,class Pro_PipeDetect*> map_Pro_PipeDetect;
    QMap<QString,class Pro_HeparingCap*> map_Pro_HeparingCap;
    QMap<QString,class Pro_NeedleSeatGlue*> map_Pro_NeedleSeatGlue;
    QMap<QString,class Pro_ConnectSeatGlue*> map_Pro_ConnectSeatGlue;
    QMap<QString,class Pro_Distance*> map_Pro_Distance;
    QMap<QString,class Pro_NeedleSeatCrack*> map_Pro_NeedleSeatCrack;
    QMap<QString,class Pro_ConnectSeatCrack*> map_Pro_ConnectSeatCrack;
    QMap<QString,class Pro_TubeBase*> map_Pro_TubeBase;
    QMap<QString,class Pro_SiliconeTube*> map_Pro_SiliconeTube;
    QMap<QString,class Pro_DecodeText*> map_Pro_DecodeText;
    QMap<QString,class Pro_DistanceOnly*> map_Pro_DistanceOnly;
    QMap<QString,class Pro_GlueWithSeat*> map_Pro_GlueWithSeat;
    QMap<QString,class Pro_NeedleDirection*> map_Pro_NeedleDirection;
    QMap<QString,class Pro_MetalCheck*> map_Pro_MetalCheck;
    QMap<QString,class Pro_MeltPlugRowCheck*> map_Pro_MeltPlugRowCheck;
    QMap<QString,class Pro_OppositeDetect*> map_Pro_OppositeDetect;
    QMap<QString,class Pro_PositiveDetect*> map_Pro_PositiveDetect;
    QMap<QString,class CrnnTest*> map_CrnnTest;
    QMap<QString,class Pro_AllObjDetect*> map_Pro_AllObjDetect;

    QMap<QString,QThread*> map_Thread;
    QMap<QString,QString> map_cam_function;

    double time_start,time_end;

    QString m_runInfo;

    bool m_isRunning=false;//是否处于运行状态
    bool isReadCheckOnce=false;//手动读取检测一次
    QMap<QString,bool> cam_ischeck;//自动检测一次
    QMap<QString,QString> g_cam_step;//相机进行的步骤
    
    void initStationCom(QString proname, QString usestation);
    void initStationType(QString usetype);
    void readStationConfig();
    void readCamInfo();
    void readComInfo(QString m_UseCom);
    void initType();
    void initCamRunning();
    void delete_com();
    void check_cam_is_photo(QList<int> data);
signals:
    void signal_cam_change(QString cam);
    void signal_com_start();
    void signal_com_stop();
    void signal_roi_change(QString cam,QString step);
    void signal_type_change(QString type);
    void signal_par_change(QString cam,QString step);
    void signal_init_cam(bool isinit);
    void signal_init_cam_result(bool isright);
    void signal_photo(int type,QString cam);
    void signal_one_frame(QImage image,QString camname,bool ischeck,QString step);
    void signal_read_image(QString cam,QString step,QString path);

    void signal_check(QString cam,QString step);
    void signal_program_exit();

    void signal_information_text(QString camname,int type,QString information);
    void signal_information_image(QString camname,int type,QImage showImage);

    void signal_press_point(QString cam,QPoint point,QSize size);

    void signal_write(QString comname,int startaddr, QList<int> writedata);
    void signal_read(QString comname);

public slots:
    void slot_cam_change(QString cam);
    void slot_com_change(QString com);
    void slot_roi_change(QString station,QString cam,QString step);
    void slot_type_change(QString type);
    void slot_par_change(QString station,QString cam,QString step);

    void slot_init_cam(bool isinit);
    void slot_init_cam_result(bool isright);
    void slot_init_com(bool isinit);
    void slot_start_running(bool isRunning);

    void slot_photo(QString station,QString cam,QString step);
    void slot_one_frame(QImage image,QString camname);
    void slot_read_image(QString station,QString cam,QString step,QString imgPath);
    void slot_check(QString station,QString cam,QString step);

    void slot_read_plc_photo(QString station,QString cam,QString step);
    void slot_receive_plc_data(QList<int> data,QString comName);
    void slot_com_state(QString comname,bool state);
    void slot_write_result(QList<int> senddata,QString cam,QString step);
    void slot_force_set(QString station,QString cam,QString step,bool isright);

    void slot_information_image(QString camname,int type,QImage showImage);
    void slot_information_text(QString cam, int type,QString runinfo);

    void slot_press_point(QString cam,QPoint point,QSize size);
    void slot_program_exit(); 

};

#endif // STATIONRUNNING_H
