#ifndef PRO_DISTANCE_H
#define PRO_DISTANCE_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<halcon/qthalcon.h>

class Pro_Distance : public QObject
{
    Q_OBJECT
public:
    explicit Pro_Distance(QObject *parent = nullptr);
    ~Pro_Distance();

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

    void check_distance_melt_top(HObject ho_Image, HObject *ho_distance_arrow, HObject *ho_melt_contours,
                                 HObject *ho_top_region, HObject *ho_fold_region, HObject *ho_slant_rect, HObject *ho_needle_rect,
                                 HTuple hv_is_direction, HTuple hv_is_check_distance, HTuple hv_is_check_melt,
                                 HTuple hv_is_check_slant, HTuple hv_is_check_top, HTuple hv_is_check_fold, HTuple hv_needle_angle_rect_height,
                                 HTuple hv_needle_width, HTuple hv_top_thr, HTuple hv_top_h_min, HTuple hv_top_h_max,
                                 HTuple hv_top_w_min, HTuple hv_top_w_max, HTuple hv_top_opening_size, HTuple hv_top_width_min,
                                 HTuple hv_slant_width_min, HTuple hv_pipe_thick, HTuple hv_pipe_position, HTuple hv_distance_min,
                                 HTuple hv_distance_max, HTuple hv_fold_pos, HTuple hv_fold_thr, HTuple hv_fold_area_min,
                                 HTuple hv_melt_thr, HTuple hv_melt_min_area, HTuple hv_melt_side_edge_strength,
                                 HTuple hv_melt_angle_min, HTuple *hv_is_distance_ok, HTuple *hv_distance, HTuple *hv_is_top_ok,
                                 HTuple *hv_top_width, HTuple *hv_is_fold_ok, HTuple *hv_fold_area, HTuple *hv_is_slant_ok,
                                 HTuple *hv_slant_width, HTuple *hv_is_melt_ok, HTuple *hv_melt_angle);
    void openHalconWindow();
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

#endif // PRO_DISTANCE_H
