#ifndef PRO_NEEDLEDIRECTION_H
#define PRO_NEEDLEDIRECTION_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<halcon/qthalcon.h>

class Pro_NeedleDirection : public QObject
{
    Q_OBJECT
public:
    explicit Pro_NeedleDirection(QObject *parent = nullptr);
    ~Pro_NeedleDirection();
    HTuple hv_WindowHandle_UI;//窗口句柄

    QString m_ExePath;//程序路径
    QString m_ProPath;//项目文件路径
    QString m_ProName;//项目名称
    QString m_StationName;//工位名称
    QString m_station_path; //工位路径
    QString m_CamName;
    QString m_CamPath;
    QString m_StepName;
    QString m_StepPath;
    QString m_TypeName;
    QString m_TypePath;
    QString m_ReportPath;
    CamInfo m_cam_info;//相机参数
    HObject m_MapFix;//修正参数
    //参数
    QMap<QString,int> m_IntParameter;//int参数
    QMap<QString,float> m_FloatParameter;//float参数
    QMap<QString,QString> m_StringParameter;//string参数
    double time_start,time_end;
    QString m_runInfo;

    HalconCpp::HObject m_OriginImage,m_SceneImage,m_SceneImageShow;
    QMap<QString,HObject> map_hobj;//选区
    QMap<QString,QtHalcon::ModelPar> map_model_par;

    QStringList v_str_result;
    QVector<int> v_str_bool;
    QList<HalconCpp::HObject> v_obj_result;
    QVector<int> v_obj_bool;
    QList<int> m_send_data;
    QMap<QString,HalconCpp::HObject > map_result_image;



    void initGlobalPar(QString pro,QString station,QString cam,QString step,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();
    void openHalconWindow();


    void get_cant_rotate_angle(HObject ho_Image, HObject *ho_cant_rect, HObject *ho_needle_rect,
                               HTuple hv_cant_height_min, HTuple hv_cant_height_max, HTuple hv_cant_width_min,
                               HTuple hv_cant_width_max, HTuple hv_back_thr, HTuple *hv_cant_height,
                               HTuple *hv_cant_width, HTuple *hv_needle_angle, HTuple *hv_cant_angle,
                               HTuple *hv_angle_dif, HTuple *hv_dir, HTuple *hv_angle, HTuple *hv_c, HTuple *hv_r);
signals:
    void signal_information_text(QString camname,int type,QString information);
    void signal_information_image(QString camname,int type,QImage showImage);
    void signal_result(QList<int> data,QString cam,QString step);
public slots:
    void slot_roi_change(QString cam,QString step);
    void slot_type_change(QString type);
    void slot_par_change(QString cam,QString step);
    void slot_read_image(QString cam,QString step,QString imgPath);
    void slot_one_frame(QImage src,QString camName,bool ischeck,QString step);

    void slot_check(QString camName,QString step);
    void slot_press_point(QString cam,QPoint point, QSize size);

    void slot_program_exit();
};

#endif // PRO_NEEDLEDIRECTION_H
