#ifndef LIGHTCONTROL_H
#define LIGHTCONTROL_H

#include <QWidget>
#include"serialportnet.h"

namespace Ui {
class LightControl;
}

class LightControl : public QWidget
{
    Q_OBJECT

public:
    explicit LightControl(QWidget *parent = nullptr);
    ~LightControl();

    SerialPortNet* _pSerial;
    QString m_LightConfigPath;

    bool openCom(QString comname);

    void initPar(QString path);
    void setCommand(QString command, QString channel, QString value);
private slots:
    void on_pbtnSave_clicked();

    void on_pbtnOpenCom_clicked();

    void on_pbtnCloseCom_clicked();

    void on_pbtnSet_clicked();

    void on_cbboxLightChannel_currentTextChanged(const QString &arg1);

private:
    Ui::LightControl *ui;
};

#endif // LIGHTCONTROL_H
