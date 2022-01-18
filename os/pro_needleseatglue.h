#ifndef PRO_NEEDLESEATGLUE_H
#define PRO_NEEDLESEATGLUE_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<opencv/qtopencv.h>
#include<halcon/qthalcon.h>

class Pro_NeedleSeatGlue : public QObject
{
    Q_OBJECT
public:
    explicit Pro_NeedleSeatGlue(QObject *parent = nullptr);

    ~Pro_NeedleSeatGlue();
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

    HalconCpp::HObject m_OriginImage,m_SceneImage,m_SceneImageShow;
    QMap<QString,HObject> map_hobj;//选区
    QMap<QString,QtHalcon::ModelPar> map_model_par;
    QMap<int,class Classified*> map_Classified;
    QMap<int,class classfiedopenvino*> map_openvino;
    QMap<int,QThread*> map_Thread;


    QList<HalconCpp::HObject> v_obj_result;
    QList<int> v_obj_bool;
    QMap<int,QStringList> v_str_result;
    QMap<int,QList<int>> v_str_bool;
    QList<HalconCpp::HObject> v_contour_result;
    QList<int> v_contour_bool;
    int check_count=0;
    QList<int> m_send_data;
    bool  check_err;


    void initGlobalPar(QString pro,QString station,QString cam,QString step,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();

    void getCheckRegion(HObject ho_Image, HObject ho_ROI_0, HObject *ho_Rectangle, HObject *ho_ImagePart1,
                        HTuple hv_thr_dif, HTuple hv_select_area_min, HTuple hv_seat_size, HTuple hv_needle_size,
                        HTuple hv_check_size);
    void openHalconWindow();
    void check_needle_and_angle(HObject ho_Image, HObject ho_Rectangle, HObject *ho_ResultContour,
                                HTuple hv_needle_length, HTuple hv_needle_size, HTuple hv_angle_min, HTuple hv_angle_std,
                                HTuple *hv_is_angle_right, HTuple *hv_is_needle_right, HTuple *hv_Length2, HTuple *hv_angle);
signals:
    void signal_information_text(QString camname,int type,QString information);
    void signal_information_image(QString camname,int type,QImage showImage);
    void signal_result(QList<int> data,QString cam,QString step);
    void signal_setCheckPar(int num,cv::Mat src,int imageSize);
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
    void slot_check_result(int num, QString name);
};

#endif // PRO_NEEDLESEATGLUE_H
