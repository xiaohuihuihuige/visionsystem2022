#ifndef IMAGECALIBRATE_H
#define IMAGECALIBRATE_H

#include <QWidget>
#include<QMouseEvent>
#include"halcon/qthalcon.h"
#include"general/configfileoperate.h"
#include"publicstruct.h"

namespace Ui {
class ImageCalibrate;
}

class ImageCalibrate : public QWidget
{
    Q_OBJECT

public:
    explicit ImageCalibrate(QWidget *parent = nullptr);
    ~ImageCalibrate();

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
    HObject m_OriginImage,m_SceneImage;//原始图像，操作图像

    QString g_pro_path;//项目文件夹
    QString cam_config_path;//相机配置文件
    QString g_cam_select;//相机选择
    QString g_cam_path;//相机路径
    QString g_cam_step_select;//相机步骤选择
    QString g_cam_step_path;//相机步骤路径

    QStringList g_cam_list;//相机列表

    HTuple hv_cam_par;//相机内参
    HTuple hv_cam_pose;//相机位姿
    double image_width,image_height,world_width,world_height;//图像宽，图像高，世界宽，世界高
    double ratio,x_offset,y_offset;//比率，x偏移，y偏移

    HObject hv_mappedfix;//校正图像


    void initUI();
    void initPar(QString propath);
    void initCalPar();
private slots:
    void on_pbtn_Read_Image_clicked();

    void on_cbbox_CamList_currentTextChanged(const QString &arg1);

    void on_cbbox_CamStep_currentTextChanged(const QString &arg1);

    void on_pbtn_Cal_Image_clicked();

    void on_pbtn_Save_clicked();


private:
    Ui::ImageCalibrate *ui;
};

#endif // IMAGECALIBRATE_H
