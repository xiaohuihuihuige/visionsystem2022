#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QMap>
#include<QLabel>
#include<QTimer>
#include<QMouseEvent>
#include<QThread>
#include"drawingpapereditor.h"
#include"controls/ringsprogressbar.h"
#include"controls/animationprogressbar.h"
#include"stationrunning.h"
#include<QGridLayout>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QImage m_ImageShow;//图像变量
    QLabel *lblPosInfo;//位置信息
    QMap<QString,QLabel*> map_lblConnectInfo;//工位运行状态

    class StationSet *g_p_StationSet;
    class Communication *g_p_Communication;
    class CamOperate *g_p_CamOperate;
    class ParameterSet *g_p_Par;
    class ModelParSet *g_p_ModelParSet;
    class ReportView *g_p_ReportView;
    class TypeSet *g_p_TypeSet;
    class About *g_p_about;
    class LightControl *g_p_LightControl;
    class Login *g_p_Login;//登录页面类

    QStringList m_ProList;//项目列表
    QStringList m_StationList;//工位列表
    QStringList m_TypeList;//品种列表
    QStringList m_camList;//相机列表

    QString m_usePro;//使用项目
    QString m_station_name;//当前选择工位
    QString m_cam_name;//下拉框中选择的相机
    QString m_cam_step;//当前选择步骤
    QString m_useType;//当前品种

    QString m_useFolder;//上次打开的文件夹
    QString m_ExePath;//exe的路径
    QString m_AllProPath;//所有项目路径
    QString m_ProPath;//当前项目路径
    QString m_AllStationPath;//所有工位的路径
    QString m_edit_type_path;//编辑品种文件夹路径

    QMap<QString,QtDrawingPaperEditor*> map_show_widget;//显示窗口的列表
    QMap<QString,QLabel*> map_show_label;//显示结果label的列表
    QMap<QString,QLabel*> map_ok_label;//显示ok计数label列表
    QMap<QString,QLabel*> map_ng_label;//显示ng计数label列表
    QMap<QString,QLabel*> map_qu_label;//显示ng计数label列表
    QMap<QString,QMap<QString,int>> cam_re_count;//相机运行计数

    QGridLayout *gLayout1;//网格布局
    QMap<QString,StationRunning*> map_stations;//工位
    QMap<QString, QThread*> map_Thread;//工位线程
    RingsProgressbar *bar=nullptr;//加载进度条
    bool m_isRunning;//是否处于运行状态
    bool login_state=false;//登录状态

    void showReasult(QString camname,int showtype);
    void readProConfig();
    void readStationConfig();
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent *event);
    void initStation();
    void show_progress_bar(int precent);
    void initShowWindow();
    void changeSkin(QString str_skin);
    void userLogin();
    void changeControlEnable(bool isEnabe);
    //void initLightSet();
    void readDataCount();
    void startWork();
    void initMes();
private slots:
    //菜单栏点击事件
    void on_actionHandbook_triggered();
    void on_actionAbout_triggered();
    void on_actionChinese_triggered();
    void on_actionEnglish_triggered();
    void on_actionSkinChange_triggered();


    //控件事件
    void on_pbtnComSet_clicked();

    void on_pbtnCamSet_clicked();

    void on_pbtnTakePhoto_clicked();
    void on_chkboxInitCam_clicked();

    void on_pbtnCheck_clicked();

    void on_pbtnRunning_clicked();

    void on_pbtnReadImage_clicked();

    void on_pbtnEditCheckPar_clicked();

    void on_pbtnForceOK_clicked();

    void on_pbtnForceNG_clicked();

    void on_cbboxTypeList_currentTextChanged(const QString &arg1);

    void on_cbboxProList_currentTextChanged(const QString &arg1);

    void on_pbtnEditROI_clicked();

    void on_pbtnReportView_clicked();

    void on_pbtnStationSet_clicked();

    void on_cbboxStationList_currentTextChanged(const QString &arg1);

    void on_cbboxStationCamList_currentTextChanged(const QString &arg1);

    void on_cbboxCamStep_currentTextChanged(const QString &arg1);

    void on_pbtnReadOnceTakePhoto_clicked();

    void on_actionLogin_triggered();

    void on_pbtnLightSet_clicked();

    void on_pbtnReadImageFolder_clicked();

    void on_pbtnTypeSet_clicked();

signals:
    void signal_init_cam(bool isinit);
    void signal_start_running(bool isrunning);

    void signal_photo(QString station,QString cam,QString step);
    void signal_read_image(QString station,QString cam,QString step,QString imgPath);
    void signal_check(QString station,QString cam,QString step);
    void signal_read_plc_photo(QString station,QString cam,QString step);
    void signal_force_set(QString station,QString cam,QString step,bool isright);

    void signal_type_change(QString type);
    void signal_com_change(QString com);
    void signal_cam_change(QString cam);
    void signal_par_change(QString station,QString cam,QString step);
    void signal_roi_change(QString station,QString cam,QString step);

    void signal_press_point(QString cam,QPoint point,QSize size);
    void signal_closing();
public slots:
    void slot_init_cam_result(bool isSucceed);
    void slot_cam_change(QString cam);
    void slot_com_change(QString com);
    void slot_par_change();
    void slot_roi_change();
    void slot_type_change(QString type_name,bool isNew);

    void slot_information_text(QString camname,int type,QString text);
    void slot_information_image(QString camname,int type,QImage image);

    void slot_press_point(QString camname,QPoint point);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
