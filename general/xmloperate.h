#ifndef XMLOPERATE_H
#define XMLOPERATE_H
#include<QFile>
#include<QDomDocument>
#include<QTextStream>

class XmlOperate
{
public:
    XmlOperate();
    ~XmlOperate();

    QDomDocument *m_dom;//文件变量
    QDomElement rootNode;//根节点
    QString m_fileName;//文件名
    //创建xml文件
    static bool creatXml(const QString &path);
    //打开xml文件
    void openXml(QString fileName);
    //添加节点
    void addNode(QStringList node, QString value);
    //移除节点
    void removeNode(QStringList node);
    //更新节点
    void updateNode(QStringList node, QString value);
    //读取节点
    QString readNode(QStringList node);
    //清楚xml文件
    void clearXml();
    //关闭文件
    void closeXml();
    //获取节点下的子节点
    QStringList getChild(QStringList node);
};

#endif // XMLOPERATE_H
