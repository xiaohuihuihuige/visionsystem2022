#ifndef PRO_POSITIVEDETECT_H
#define PRO_POSITIVEDETECT_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<halcon/qthalcon.h>
class Pro_PositiveDetect : public QObject
{
    Q_OBJECT
public:
    explicit Pro_PositiveDetect(QObject *parent = nullptr);
    ~Pro_PositiveDetect();

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

    int _final_result;
    QList<int> m_send_data;
    QMap<QString,HalconCpp::HObject> map_result_image;
    int ng_num;

    QMap<QString,QPoint> str_write_pos;
    QMap<QString,int>    str_write_pos_bool;
    QMap<QString,HObject> roi_obj;

    QStringList v_str_result;
    QVector<int> v_str_bool;
    QList<HalconCpp::HObject> v_obj_result;
    QVector<int> v_obj_bool;



    void initGlobalPar(QString pro,QString station,QString cam,QString step,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();

    void openHalconWindow();
    void check_circle_hole (HObject ho_Image, HObject ho_ROI, HObject *ho_hole_region,
                            HTuple hv_opening_size, HTuple hv_hole_thr_min, HTuple hv_hole_thr_max, HTuple hv_hole_area_min,
                            HTuple hv_hole_area_max, HTuple hv_roi_type, HTuple hv_check_best, HTuple *hv_is_have,
                            HTuple *hv_Area_hole);
    void check_is_positive(HObject ho_Image2, HObject *ho_ModelAtNewPosition, HObject *ho_Cross, HTuple hv_ModelID_1,
                           HTuple hv_ModelID_2, HTuple hv_MovementOfObject_Model_M_1, HTuple hv_MovementOfObject_Model_M_2,
                           HTuple hv_NumToFind, HTuple hv_Angle_Start, HTuple hv_Min_Score, HTuple hv_Max_Overlap,
                           HTuple hv_Sub_Pixel, HTuple hv_Num_Levels, HTuple hv_Greediness, HTuple *hv_Success,
                           HTuple *hv_MovementOfObject, HTuple *hv_Row, HTuple *hv_Column, HTuple *hv_Angle,
                           HTuple *hv_Score, HTuple *hv_Obj1_To_Obj2, HTuple *hv_is_positive);
    void check_distance_H_V(HObject ho_Image, HObject ho_region_0, HObject ho_region_1,
                            HObject ho_region_dir, HObject *ho_line_dir, HObject *ho_line_H, HObject *ho_line_V,
                            HTuple hv_edge_strength, HTuple *hv_Distance_H, HTuple *hv_Distance_V, HTuple *hv_point_h,
                            HTuple *hv_point_v);
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

#endif // PRO_POSITIVEDETECT_H
