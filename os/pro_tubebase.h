#ifndef PRO_TUBEBASE_H
#define PRO_TUBEBASE_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<opencv/qtopencv.h>
#include<halcon/qthalcon.h>

class Pro_TubeBase : public QObject
{
    Q_OBJECT
public:
    explicit Pro_TubeBase(QObject *parent = nullptr);
    ~Pro_TubeBase();

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
    QString  m_weightfile,m_namesfile;//品种文件
    class yolov5openvino* m_openvino;


    QList<HalconCpp::HObject> v_obj_result;
    QList<int> v_obj_bool;

    QList<cv::Mat> v_result_image;
    QList<HalconCpp::HObject> v_result_image_rect;

    QMap<QString,QPoint> map_string_point;
    QMap<QString,int> map_string_color;

    QStringList v_str_result;
    QList<int> v_str_bool;

    QList<int> m_send_data;
    int _final_result;

    void initGlobalPar(QString pro,QString station,QString cam,QString step,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();

    void openHalconWindow();
    void check_seat_and_get_pipe(HObject ho_Image, HObject ho_ROI_0, HObject *ho_seat_rect,
                                 HObject *ho_pipe_rect, HObject *ho_ImagePart, HTuple hv_seat_width_min, HTuple hv_seat_height_max,
                                 HTuple hv_check_pipe, HTuple hv_pipe_width, HTuple hv_check_size, HTuple hv_metal_height,
                                 HTuple hv_rotate_angle, HTuple hv_seat_thr, HTuple hv_offset_dis, HTuple *hv_type_is,
                                 HTuple *hv_seat_width, HTuple *hv_seat_height);
signals:
    void signal_information_text(QString camname,int type,QString information);
    void signal_information_image(QString camname,int type,QImage showImage);
    void signal_result(QList<int> data,QString cam,QString step);
    void signal_setCheckPar(int num,cv::Mat src,int imageSize,float conf_threshold, float iou_threshold);
    void signal_check_image();
public slots:
    void slot_roi_change(QString cam,QString step);
    void slot_type_change(QString type);
    void slot_par_change(QString cam,QString step);
    void slot_read_image(QString cam,QString step,QString imgPath);
    void slot_one_frame(QImage src,QString camName,bool ischeck,QString step);

    void slot_check(QString camName,QString step);
    void slot_press_point(QString cam,QPoint point, QSize size);

    void slot_program_exit();
    void slot_check_result(int num, cv::Mat img_result, QStringList name);
};

#endif // PRO_TUBEBASE_H
