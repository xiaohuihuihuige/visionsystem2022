#ifndef STATIONSET_H
#define STATIONSET_H

#include <QWidget>
#include<QMap>

namespace Ui {
class StationSet;
}

class StationSet : public QWidget
{
    Q_OBJECT

public:
    explicit StationSet(QWidget *parent = nullptr);
    ~StationSet();
    QString m_ExePath,m_ProPath,m_station_path;

    QString m_UsePro,m_station_name;
    QMap<QString,QString> m_cam_fun;

    void initPar();

    QStringList readCamList(QString campath);
    QStringList readComList(QString compath);
    void readStationConfig(QString stationpath);

    void closeEvent(QCloseEvent *event);
private slots:

    void on_cbboxProSelect_currentTextChanged(const QString &arg1);

    void on_cbboxStationSelect_currentTextChanged(const QString &arg1);

    void on_pbtnAddToUse_clicked();

    void on_pbtnDeleteFromUse_clicked();

    void on_pbtnSave_clicked();

    void on_cbboxCamSelect_currentTextChanged(const QString &arg1);

signals :
    void signal_station_change();

private:
    Ui::StationSet *ui;
};

#endif // STATIONSET_H
