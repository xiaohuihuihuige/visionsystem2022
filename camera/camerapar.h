#ifndef CAMERAPAR_H
#define CAMERAPAR_H

#include <QWidget>
#include<QCloseEvent>


namespace Ui {
class CameraPar;
}

class CameraPar : public QWidget
{
    Q_OBJECT

public:
    explicit CameraPar(QWidget *parent = nullptr);
    ~CameraPar();
    QString m_ExePath;//exe路径
    QStringList camList;//相机列表
    QString cam_config_path;//相机配置文件路径

    void initUI();
    void writeCamInfo();//写入相机信息
    void initPar(QString pro_path);//初始化
    void closeEvent(QCloseEvent *event);
private slots:
    void on_pbtnWrite_clicked();

    void on_pbtnAddCam_clicked();

    void on_cbboxCamList_currentTextChanged(const QString &arg1);

    void on_pbtnDeleteCam_clicked();

signals:
    void signal_cam_change(QString cam);
private:
    Ui::CameraPar *ui;
};

#endif // CAMERAPAR_H
