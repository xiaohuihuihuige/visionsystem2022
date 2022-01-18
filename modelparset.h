#ifndef MODELPARSET_H
#define MODELPARSET_H

#include <QWidget>

#include "QMouseEvent"
#include<QTableWidgetItem>
#include"halcon/qthalcon.h"
#include <QKeyEvent>
#include"publicstruct.h"
#include<QComboBox>

namespace Ui {
class ModelParSet;
}

class ModelParSet : public QWidget
{
    Q_OBJECT

public:
    explicit ModelParSet(QWidget *parent = nullptr);
    ~ModelParSet();

    bool isDrawing;//是否在绘制
    HTuple hv_WindowHandle;//窗口句柄
    QPoint lastPoint;//鼠标在控件中的位置
    QPoint mousePoint;//鼠标在控件中的位置
    bool mouseDown;//鼠标按下标志位
    float zoom_scale;//放大倍数
    QPoint windowPoint,firstPoint;//图像移动点和初始点
    int m_dDispImagePartRow0, m_dDispImagePartCol0, m_dDispImagePartRow1, m_dDispImagePartCol1;//显示图像的区域
    int m_dYOffset, m_dXOffset;//图像偏移，与初始位置
    void mousePressEvent(QMouseEvent *event);//鼠标按下事件
    void mouseMoveEvent(QMouseEvent *event);//鼠标移动事件
    void mouseReleaseEvent(QMouseEvent *event);//鼠标松开事件
    void wheelEvent(QWheelEvent *event);//鼠标滚轮事件
    void displayImage(HImage srcImg,HTuple hv_Window);//显示图像
    void moveWnd(QPoint pointStart,QPoint pointEnd, HImage srcImg, HTuple hWindow);//移动显示区域
    void resizeEvent(QResizeEvent *event);
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *obj, QEvent *e);



    QString m_cam;//相机
    QString m_step;//步骤
    QString m_type;//品种
    QString m_ProPath;//项目文件路径
    QString m_type_path;//品种文件夹
    QString m_Typeroi_path;//品种roi文件夹

    CamInfo m_cam_info;//相机信息

    HObject m_OriginImage,m_SceneImage_Draw;//原始图像，操作图像
    QMetaObject::Connection m_readImage;//连接变量

    QString keyLast,keyNow;//单元格修改之前的内容和修改之后的内容
    QComboBox *m_Combobox_cur=nullptr;
    int rowLast;
    QMetaObject::Connection textchange;

    bool isNew;//是否为创建
    QMap<QString,HObject> map_hobj;//Roi

    void initUI();
    void readROI();
    void initPar(QString pro,QString station,QString cam ,QString step,QString type);
    void create_and_show_model();
    void readModelPar(QString roi_name);
    void readCamInfo();
private slots:
    void on_pbtnReadImage_clicked();

    void on_pbtnTakePhoto_clicked();

    void on_pbtnAdd_clicked();

    void on_pbtnEdit_clicked();


    void on_tblwgtROI_cellChanged(int row, int column);

    void on_tblwgtROI_cellClicked(int row, int column);

    void on_pbtnSave_clicked();

    void on_pbtnDelete_clicked();

    void on_pbtnCamOpen_clicked();


    void on_pbtnWriteModelPar_clicked();

    void on_pbtnCalImage_clicked();


    void mouse_right_click_in_tblwgt();


    void on_pbtnSendSignal_clicked();

protected:
    virtual void keyPressEvent(QKeyEvent *ev);
    virtual void keyReleaseEvent(QKeyEvent *ev);

private:
    Ui::ModelParSet *ui;

signals:
    void signal_roi_change();
public slots:
    void slot_one_frame(QImage image,QString camname);
};

#endif // MODELPARSET_H
