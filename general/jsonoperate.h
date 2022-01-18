#ifndef JSONOPERATE_H
#define JSONOPERATE_H

#include<QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include<QTextStream>


class JsonOperate
{
public:
    JsonOperate();


    QJsonDocument m_dom;//文件变量
    QJsonObject rootObj;//根节点
    QString m_fileName;//文件名
    //创建xml文件
    static bool creatJson(const QString &path);
    //打开xml文件
    void openJson(QString fileName);
    //添加节点
    void addNode(QStringList node, QString value);
    //移除节点
    void removeNode(QStringList node);
    //更新节点
    void updateNode(QStringList node, QString value);
    //读取节点
    QString readNode(QStringList node);
    //清楚xml文件
    void clearJson();
    //关闭文件
    void closeJson();
    //获取节点下的子节点
    QStringList getChild(QStringList node);


    void addMapList(QStringList node, QList<QMap<QString, QString> > listmap);
    void addArray(QStringList node, QStringList valueList);
};

#endif // JSONOPERATE_H
