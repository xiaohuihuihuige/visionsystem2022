#ifndef TYPESET_H
#define TYPESET_H

#include <QWidget>


namespace Ui {
class TypeSet;
}

class TypeSet : public QWidget
{
    Q_OBJECT

public:
    explicit TypeSet(QWidget *parent = nullptr);
    ~TypeSet();
    QString m_ProPath;
    QStringList m_StationList;
    QStringList m_CamList;
    QStringList m_StepList;
    QStringList m_TypeList;

    void initPar(QString proPath);
signals:
    void signal_type_change(QString type_name,bool isNew);



private slots:
    void on_pbtnNewOne_clicked();

    void on_pbtnNewOneBase_clicked();

    void on_pbtnDelete_clicked();




private:
    Ui::TypeSet *ui;
};

#endif // TYPESET_H
