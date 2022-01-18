#ifndef PRO_HEPARINGCAP_H
#define PRO_HEPARINGCAP_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<opencv/qtopencv.h>
#include<halcon/qthalcon.h>

class Pro_HeparingCap : public QObject
{
    Q_OBJECT
public:
    explicit Pro_HeparingCap(QObject *parent = nullptr);

    ~Pro_HeparingCap();

    HTuple hv_WindowHandle_UI;//虚拟窗口句柄

    QString m_ExePath;//程序路径
    QString m_ProPath;//项目文件路径
    QString m_ProName;//项目名称
    QString m_StationName;//工位名称
    QString m_station_path; //工位路径
    QString m_CamName;//相机名
    QString m_CamPath;//相机路径
    QString m_StepName;//步骤名
    QString m_StepPath;//步骤路径
    QString m_TypeName;//品种名
    QString m_TypePath;//品种路径
    QString m_ReportPath;//结果保存路径

    QString  m_weightfile,m_namesfile;//品种文件
    CamInfo m_cam_info;//相机参数
    HObject m_MapFix;//修正参数
    //参数
    QMap<QString,int> m_IntParameter;//int参数
    QMap<QString,float> m_FloatParameter;//float参数
    QMap<QString,QString> m_StringParameter;//string参数

    HalconCpp::HObject m_OriginImage,m_SceneImage,m_SceneImageShow;//图像变量
    QMap<QString,HObject> map_hobj;//选区
    QMap<QString,QtHalcon::ModelPar> map_model_par;//模板参数
    class classfiedopenvino* m_openvino;//图像分类


    double time_start,time_end;//程序运行计时
    QString m_runInfo;//运行状态变量
    QList<HalconCpp::HObject> v_obj_result;//显示结果
    QList<int> v_obj_bool;//显示结果状态
    QList<int> m_send_data;//发送数据

    void initGlobalPar(QString pro,QString station,QString cam,QString step,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();
    /*void getHeparingRegion(HObject ho_Image, HObject ho_ROI_0, HObject *ho_Rectangle,
                           HObject *ho_ImagePart, HTuple hv_back_thr_min, HTuple hv_back_open_size, HTuple hv_back_close_size,
                           HTuple hv_thr_diff, HTuple hv_select_area_min, HTuple hv_min_width, HTuple hv_obj_thr_min,
                           HTuple hv_obj_thr_max, HTuple hv_obj_open_size, HTuple hv_cut_size);*/
    void openHalconWindow();
    void get_check_roi(HObject ho_Image, HObject ho_ROI, HObject *ho_Rectangle1, HObject *ho_ImagePart,
                       HTuple hv_thr_min, HTuple hv_thr_max, HTuple hv_cut_size);
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

#endif // PRO_HEPARINGCAP_H
