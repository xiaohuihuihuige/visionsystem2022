#ifndef REPORTVIEW_H
#define REPORTVIEW_H

#include <QWidget>

namespace Ui {
class ReportView;
}

class ReportView : public QWidget
{
    Q_OBJECT

public:
    explicit ReportView(QWidget *parent = nullptr);
    ~ReportView();
    QStringList m_SystemList;
    QStringList m_StationList;
    QStringList m_CamList;
    QStringList m_CamStepList;
    QStringList m_TypeList;
    QStringList m_DateList;
    QStringList m_ResultList;
    QStringList m_fileList;

    QString m_SystemSelect;
    QString m_StationSelect;
    QString m_CamSelect;
    QString m_CamStep;
    QString m_TypeSelect;
    QString m_DateSelect;
    QString m_ResultSelect;
    QString m_fileSelect;

    QString m_ResultFolder;
    int m_fileSelectNum;
    QString m_fileSelectPath;
    QString m_ExePath;


    void initPar();
    void readAndShow();
    void showPictureInWidget();
private slots:
    void on_cbboxSystemSelect_currentTextChanged(const QString &arg1);

    void on_cbboxTypeSelect_currentTextChanged(const QString &arg1);

    void on_cbboxDateSelect_currentTextChanged(const QString &arg1);

    void on_cbboxResultSelect_currentTextChanged(const QString &arg1);


    void on_pbtnLastOne_clicked();

    void on_pbtnNextOne_clicked();

    void on_pbtnPreviousOne_clicked();

    void on_pbtnFrontOne_clicked();

    void on_cbboxStationSelect_currentTextChanged(const QString &arg1);

    void on_cbboxCamSelect_currentTextChanged(const QString &arg1);

    void on_pbtnJump_clicked();

    void on_pbtnClose_clicked();

    void on_cbboxCamStep_currentTextChanged(const QString &arg1);

private:
    Ui::ReportView *ui;
};

#endif // REPORTVIEW_H
