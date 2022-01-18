#ifndef CAMERACALIBRATE_H
#define CAMERACALIBRATE_H

#include <QWidget>
#include<QMouseEvent>
#include"halcon/qthalcon.h"
#include"general/configfileoperate.h"
#include"publicstruct.h"

namespace Ui {
class CameraCalibrate;
}

class CameraCalibrate : public QWidget
{
    Q_OBJECT

public:
    explicit CameraCalibrate(QWidget *parent = nullptr);
    ~CameraCalibrate();
    HTuple hv_WindowHandle;//窗口句柄
    QPoint lastPoint;//鼠标在控件中的位置
    QPoint mousePoint;//鼠标在控件中的位置
    bool mouseDown;//鼠标按下标志位
    float zoom_scale;//放大倍数
    QPoint windowPoint,firstPoint;//图像移动点和初始点
    int m_dDispImagePartRow0=0, m_dDispImagePartCol0=0, m_dDispImagePartRow1, m_dDispImagePartCol1;//显示图像的区域
    int m_dYOffset, m_dXOffset;//图像偏移，与初始位置
    void mousePressEvent(QMouseEvent *event);//鼠标按下事件
    void mouseMoveEvent(QMouseEvent *event);//鼠标移动事件
    void mouseReleaseEvent(QMouseEvent *event);//鼠标松开事件
    void wheelEvent(QWheelEvent *event);//鼠标滚轮事件
    void displayImage(HImage srcImg,HTuple hv_Window);//显示图像
    void moveWnd(QPoint pointStart,QPoint pointEnd, HImage srcImg, HTuple hWindow);//移动显示区域
    void resizeEvent(QResizeEvent *event);


    //
    HTuple hv_cam_start_par,hv_cam_par;//相机内参
    HTuple hv_cam_pose;//位姿
    HTuple hv_cal_data_ID;//标定句柄


    QString g_pro_path;//项目路径
    QString cam_config_path;//相机配置文件路径
    QString g_cam_select;//相机选择
    QString g_cam_path;//选择相机得路径
    QString g_cam_step_select;//相机步骤
    QString g_cam_step_path;//相机步骤路径


    ConfigFileOperate *g_cal_par_config;//标定参数配置文件
    QStringList g_cam_list;//相机列表

    CamInfo m_cam_info;//相机信息

    HObject m_OriginImage,m_SceneImage;//原始图像，操作图像
    QMap<QString,HObject> map_hobj;//标定图片


    void initPar(QString propath);//初始化
    void initUI();//初始化UI
    void slot_one_frame(QImage image, QString cam);//接收一帧图像
    void initCalPar();//初始化标定参数
private slots:
    void on_pbtn_GenCalTab_clicked();

    void on_pbtn_CalTabSelect_clicked();

    void on_cbboxCamList_currentTextChanged(const QString &arg1);

    void on_cbboxCamStep_currentTextChanged(const QString &arg1);

    void on_pbtn_SaveConfig_clicked();

    void on_pbtnCamOpen_clicked();

    void on_pbtn_SavePhoto_clicked();

    void on_pbtn_SetReferencePose_clicked();

    void on_pbtn_RemoveImageOne_clicked();

    void on_pbtn_RemoveImageAll_clicked();

    void on_pbtn_Cal_clicked();

    void on_tblwgt_ImageList_cellClicked(int row, int column);


private:
    Ui::CameraCalibrate *ui;
};

#endif // CAMERACALIBRATE_H
