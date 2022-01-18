#ifndef PRO_NEEDLESEATCRACK_H
#define PRO_NEEDLESEATCRACK_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<opencv/qtopencv.h>
#include<halcon/qthalcon.h>

class Pro_NeedleSeatCrack : public QObject
{
    Q_OBJECT
public:
    explicit Pro_NeedleSeatCrack(QObject *parent = nullptr);
    ~Pro_NeedleSeatCrack();

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
    QString  m_weightfile,m_namesfile;//品种文件

    //参数
    QMap<QString,int> m_IntParameter;//int参数
    QMap<QString,float> m_FloatParameter;//float参数
    QMap<QString,QString> m_StringParameter;//string参数
    double time_start,time_end;
    QString m_runInfo;

    HalconCpp::HObject m_OriginImage,m_SceneImage;
    cv::Mat m_SceneImageShow;

    QMap<QString,HObject> map_hobj;//选区
    QMap<QString,QtHalcon::ModelPar> map_model_par;
    QMap<int,class Detector*> map_detector;
    QMap<int,class yolov5openvino*> map_openvino;
    QMap<int,QThread*> map_Thread;

    QList<HalconCpp::HObject> v_obj_result;
    QVector<int> v_obj_bool;
    QList<int> m_send_data;
    QList<cv::Mat> v_result_image;
    int check_count=0;
    bool check_err;

    void initGlobalPar(QString pro,QString station,QString cam,QString step,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();


    void getCheckObj(HObject ho_Image, HObject ho_ROI, HObject *ho_ObjectsConcat, HObject *ho_ObjectsConcat1,
                     HTuple hv_back_thr_min, HTuple hv_back_close_size, HTuple hv_back_thr_dif, HTuple hv_seat_close_size_h,
                     HTuple hv_seat_close_size_v, HTuple hv_seat_select_width_min, HTuple hv_seat_select_height_min,
                     HTuple hv_obj_height, HTuple *hv_Number);
    void openHalconWindow();
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
    void slot_check_result(int num, cv::Mat img_result, QStringList name ,QList<cv::Rect> rect);

};

#endif // PRO_NEEDLESEATCRACK_H
