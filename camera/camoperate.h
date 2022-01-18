#ifndef CAMOPERATE_H
#define CAMOPERATE_H

#include <QWidget>
#include<QTabWidget>
#include"camera/camerapar.h"
#include"cameracalibrate.h"
#include"imagecalibrate.h"

namespace Ui {
class CamOperate;
}

class CamOperate : public QWidget
{
    Q_OBJECT

public:
    explicit CamOperate(QWidget *parent = nullptr);
    ~CamOperate();
    QTabWidget *tabWidget;//tab控件
    CameraPar *cameraPar;//相机配置定义
    CameraCalibrate *cameracalibrate;//相机标定定义
    ImageCalibrate *imageCal;//图像校正类定义

    QString g_pro_path;//项目路径


    void initPar(QString path);
    void initUI();
    void tab_clicked(int index);
    void closeEvent(QCloseEvent *event);
private:
    Ui::CamOperate *ui;
};

#endif // CAMOPERATE_H
