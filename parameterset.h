#ifndef PARAMETERSET_H
#define PARAMETERSET_H

#include <QWidget>
#include<QComboBox>

namespace Ui {
class ParameterSet;
}

class ParameterSet : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterSet(QWidget *parent = nullptr);
    ~ParameterSet();
    QString m_ConfigPath;//配置文件路径
    QStringList parTypeList;


    //QMap<QString,int> map_int;//int参数
    //QMap<QString,float> map_float;//float参数
    //QMap<QString,QString> map_string;//string参数
    QComboBox *m_Combobox_cur;
    int rowLast;
    QMetaObject::Connection textchange;


    QMap<QString,int> m_IntParameter;//int参数
    QMap<QString,float> m_FloatParameter;//float参数
    QMap<QString,QString> m_StringParameter;//string参数

    void initUI();
    void writePar();
    void showIntPar(QMap<QString, int> map_int);
    void showFloatPar(QMap<QString, float> map_float);
    void showStringPar(QMap<QString, QString> map_string);


    void initPar(QString path);
    void readPar(QString typePath);
    void closeEvent(QCloseEvent *event);
private slots:
    void on_pbtnEditPar_clicked();

    void on_pbtnAddPar_clicked();

    void on_pbtnDeletePar_clicked();

    void on_tblwgtPar_cellClicked(int row, int column);

signals:
    void signal_par_change();

private:
    Ui::ParameterSet *ui;
};

#endif // PARAMETERSET_H
