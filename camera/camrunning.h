#ifndef CAMRUNNING_H
#define CAMRUNNING_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>

class CamRunning : public QObject
{
    Q_OBJECT
public:
    explicit CamRunning(QObject *parent = nullptr);

    ~CamRunning();
    QString m_ExePath;//程序路径
    QString m_ProPath;//项目文件路径
    QString m_ProName;//项目名称
    QString m_StationName;//工位名称
    QString m_station_path; //工位路径
    QStringList m_station_cam_list;

    QString cam_config_path;
    QMap<QString,CamInfo> m_cam;
    //QMap<QString,QThread*> m_map_Thread;

    double time_start,time_end;
    QString m_runInfo;
    void initGlobalPar(QString pro, QString station);
    void readCamInfo(QStringList cam_use_list);



signals:
    void signal_init_cam_result(bool isright);
    void signal_one_frame(QImage image,QString camname);
    void signal_information_text(QString camname,int type,QString info);

public slots:
    void slot_cam_change(QString cam);
    void slot_init_cam(bool init);
    void slot_one_frame(QImage src, QString camName);
    void slot_photo(int type,QString cam);
    void slot_program_exit();


};

#endif // CAMRUNNING_H
