#ifndef EXCELOPERATE_H
#define EXCELOPERATE_H

#include <QObject>
#include <ActiveQt/QAxObject>
#include<QVector>
#include<QList>

class ExcelOperate : public QObject
{
    Q_OBJECT
public:
    explicit ExcelOperate(QObject *parent = nullptr);

    QAxObject *pApplication = nullptr;
    QAxObject *pWorkBooks = nullptr;
    QAxObject *pWorkBook = nullptr;
    QAxObject *pSheets = nullptr;
    QAxObject *pSheet = nullptr;


    void newExcel(const QString &fileName);
    void saveAs(QString fileName);
    void save();
    void appendSheet(const QString &sheetName);
    void closeFile();
    void setCellValue(int row, int column, const QString &value);
    void freeExcel();
    QList<QList<QVariant> > readAll();
    void castVariant2ListListVariant(const QVariant &var, QList<QList<QVariant> > &res);
    QString to26AlphabetString(int data);
    void convertToColName(int data, QString &res);
    void castListListVariant2Variant(const QList<QList<QVariant> > &res, QVariant &var);
    bool writeCurrentSheet(const QList<QList<QVariant> > &cells);
signals:

public slots:
};

#endif // EXCELOPERATE_H
