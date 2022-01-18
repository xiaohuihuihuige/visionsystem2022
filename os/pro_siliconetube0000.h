#ifndef PRO_SILICONETUBE_H
#define PRO_SILICONETUBE_H

#include <QObject>
#include<publicstruct.h>
#include<QMap>
#include<halcon/qthalcon.h>

class Pro_SiliconeTube : public QObject
{
    Q_OBJECT
public:
    explicit Pro_SiliconeTube(QObject *parent = nullptr);
    ~Pro_SiliconeTube();

    HTuple hv_WindowHandle_UI;//窗口句柄

    QString m_ExePath;//程序路径
    QString m_ProPath;//项目文件路径
    QString m_ProName;//项目名称
    QString m_StationName;//工位名称
    QString m_station_path; //工位路径
    QString m_CamName;
    QString m_CamPath;
    QString m_TypeName;
    QString m_TypePath;
    QString m_ReportPath;

    QString  m_weightfile,m_namesfile;//品种文件

    //参数
    QMap<QString,int> m_IntParameter;//int参数
    QMap<QString,float> m_FloatParameter;//float参数
    QMap<QString,QString> m_StringParameter;//string参数
    double time_start,time_end;
    QString m_runInfo;

    HObject m_OriginImage,m_SceneImage,m_SceneImageShow;

    QMap<QString,QString> map_obj;//选区
    QMap<int,class Classified*> map_Classified;
    QMap<int,class classfiedopenvino*> map_openvino;
    QMap<int,QThread*> map_Thread;

    QList<HalconCpp::HObject> v_obj_result;
    QList<int> v_obj_bool;
    QList<HalconCpp::HObject> v_result_image;
    int ng_num=0;
    int check_count=0;
    QList<int> m_send_data;
    bool check_err;


    void initGlobalPar(QString pro,QString station,QString cam,QString type);
    void readPar();
    void readROI();
    void readTypeInfo();
    void writeImageAndReport();
    void writeCheckInfoToImage();
    void send_result();
    void getCheckROI(HObject ho_Image1, HObject ho_ROI_0, HObject *ho_Rectangle1, HObject *ho_ImagePart, HTuple hv_min_thr);
signals:
    void signal_information_text(QString camname,int type,QString information);
    void signal_information_image(QString camname,int type,QImage showImage);
    void signal_result(QList<int> data,QString cam);
    void signal_setCheckPar(int num,cv::Mat src,int imageSize,int crop_size );
    void signal_check_image();
public slots:
    void slot_roi_change(QString cam);
    void slot_type_change(QString type);
    void slot_par_change(QString cam);
    void slot_read_image(QString cam,QString imgPath);
    void slot_one_frame(QImage src,QString camName,bool ischeck);

    void slot_check(QString camName);
    void slot_press_point(QString cam,QPoint point, QSize size);

    void slot_program_exit();
    void slot_check_result(int num, QString name);
};

#endif // PRO_SILICONETUBE_H
