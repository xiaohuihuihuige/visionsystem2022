#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <QWidget>
#include<QMap>
#include<QTableWidgetItem>
#include"publicstruct.h"

namespace Ui {
class Communication;
}

class Communication : public QWidget
{
    Q_OBJECT

public:
    explicit Communication(QWidget *parent = nullptr);
    ~Communication();
    //所有工位得路径
    QString m_AllStationPath;
    QString m_ComConfigPath;//通讯配置文件路径

    QMap<QString,ComInfo> m_com;//通讯对象信息

    void initPar(QString path);
    void initUI();
    void writeComPar();
    static QStringList getAvailableSerial();
    int readComConfigFromXML(QString fileName);
    void closeEvent(QCloseEvent *event);
signals:
    void signal_com_change(QString com);

private slots:
    void on_pbtnChange_clicked();

    void on_pbtnAddAddr_clicked();

    void on_pbtnDeleteAddr_clicked();

    void on_cbboxComList_currentTextChanged(const QString &arg1);

    void on_pbtnAddOne_clicked();
    void on_cbboxPLCType_currentTextChanged(const QString &arg1);

    void on_pbtnDeleteOne_clicked();


private:
    Ui::Communication *ui;
};

#endif // COMMUNICATION_H
