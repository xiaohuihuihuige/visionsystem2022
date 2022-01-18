#include "jsonoperate.h"

JsonOperate::JsonOperate()
{

}
//创建一个xml文件，根节点为“root”
//path:文件路径
bool JsonOperate::creatJson(const QString& path)
{
    /*// 定义 { } 对象
    QJsonObject interestObj;
    // 插入元素，对应键值对
    interestObj.insert("basketball", "篮球");
    interestObj.insert("badminton", "羽毛球");
    // 定义 [ ] 对象
    QJsonArray colorArray;
    // 往数组中添加元素
    colorArray.append("black");
    colorArray.append("white");
    // 定义 { } 对象
    QJsonObject likeObject1;
    likeObject1.insert("game", "三国杀");
    likeObject1.insert("price", 58.5);

    QJsonObject likeObject2;
    likeObject2.insert("game", "海岛奇兵");
    likeObject2.insert("price", 66.65);

    // 定义 [ ] 对象
    QJsonArray likeArray;
    likeArray.append(likeObject1);
    likeArray.append(likeObject2);

    // 定义 { } 对象
    QJsonObject language1;
    language1.insert("language", "汉语");
    language1.insert("grade", 10);

    QJsonObject language2;
    language2.insert("language", "英语");
    language2.insert("grade", 6);

    QJsonObject languages;
    // 将{ } 插入 { } 中
    languages.insert("serialOne", language1);
    languages.insert("serialTwo", language2);

    QJsonObject rootObject;
    // 插入元素
    rootObject.insert("name", "老王");
    rootObject.insert("age", 26);
    rootObject.insert("interest", interestObj);
    rootObject.insert("color", colorArray);
    rootObject.insert("like", likeArray);
    rootObject.insert("languages", languages);
    rootObject.insert("vip", true);
    rootObject.insert("address", QJsonValue::Null);


    // 将json对象里的数据转换为字符串
    QJsonDocument doc;
    // 将object设置为本文档的主要对象
    doc.setObject(rootObject);
    QFile file("e:\\aaa.json");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;

    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");		// 设置写入编码是UTF8
    // 写入文件
    stream << doc.toJson();
    file.close();*/

    //创建xml文件
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) return false;
    // 将json对象里的数据转换为字符串
    QJsonDocument doc;
    QJsonObject rootObject;
    // 将object设置为本文档的主要对象
    doc.setObject(rootObject);

    QTextStream stream(&file);
    stream.setCodec("UTF-8");		// 设置写入编码是UTF8
    // 写入文件
    stream << doc.toJson();
    file.close();
    return true;
}
//打开一个xml文件，并获取根节点root
//fileName:文件路径
void JsonOperate::openJson(QString fileName)
{
    m_fileName=fileName;
    QFile file(m_fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        return;
    }
    // 读取文件的全部内容
    QTextStream stream(&file);
    stream.setCodec("UTF-8");		// 设置读取编码是UTF8
    QString str = stream.readAll();
    file.close();

    // QJsonParseError类用于在JSON解析期间报告错误。
    QJsonParseError jsonError;
    // 将json解析为UTF-8编码的json文档，并从中创建一个QJsonDocument。
    // 如果解析成功，返回QJsonDocument对象，否则返回null
    m_dom = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    // 判断是否解析失败
    if (jsonError.error != QJsonParseError::NoError && !m_dom.isNull())
    {
        return;
    }
    rootObj = m_dom.object();
    return;
}

//添加节点
//node：QStringList()<<"第一层级"<<"第二层级"
//value:节点的值
void JsonOperate::addNode(QStringList node,QString value)
{    
    /*if(node.size()<1)return;
    QJsonValue nodeValue;
    QList<QJsonObject> nodelist;
    for (int i=0;i<node.size()-1;i++)
    {
        if(!nodeObj.contains(node[i]))
        {
            nodeObj.insert(node[i],QJsonObject());
        }
        nodeValue=nodeObj.value(node[i]);
        if(nodeValue.type() == QJsonValue::Object)
        {
            nodeObj=nodeValue.toObject();
        }
    }
    if(nodeObj.contains(node[node.size()-1]))
    {
        nodeObj[node[node.size()-1]]=value;
    }
    else
    {
        nodeObj.insert(node[node.size()-1],value);
    }*/
    if(node.size()<1)return;
    QJsonObject nodeObj;
    nodeObj.insert(node[node.size()-1],value);
    for (int i=node.size()-2;i>0;i--)
    {
        nodeObj.insert(node[i],nodeObj);
        nodeObj.remove(node[i+1]);
    }
     rootObj.insert(node[0],nodeObj);
}
void JsonOperate::addArray(QStringList node,QStringList valueList)
{
    /*JsonOperate m_json;
    m_json.openJson("E:\\aa.json");
    m_json.addNode(QStringList()<<"huang"<<"gui"<<"hui"<<"ddd","aaa");
    m_json.closeJson();*/

}
void JsonOperate::addMapList(QStringList node,QList<QMap<QString,QString>> listmap)
{

}

//删除节点
//node：QStringList()<<"第一层级"<<"第二层级"
void JsonOperate::removeNode(QStringList node)
{
    if(node.size()<1)return;
    QJsonValue nodeValue;
    QJsonObject nodeObj=rootObj;
    for (int i=0;i<node.size()-1;i++)
    {
        nodeValue=nodeObj.value(node[i]);
        if(nodeValue.type() == QJsonValue::Object)
        {
            nodeObj=nodeValue.toObject();
        }
    }
    nodeObj.remove(node[node.size()-1]);
}

//更新节点
//node：QStringList()<<"第一层级"<<"第二层级"
//value:节点的新值
void JsonOperate::updateNode(QStringList node,QString value)
{
    if(node.size()<1)return;
    QJsonValue nodeValue;
    QJsonObject nodeObj=rootObj;
    for (int i=0;i<node.size()-1;i++)
    {
        nodeValue=nodeObj.value(node[i]);
        if(nodeValue.type() == QJsonValue::Object)
        {
            nodeObj=nodeValue.toObject();
        }
    }
    nodeObj[node[node.size()-1]]=value;
}

//读取节点
//node：QStringList()<<"第一层级"<<"第二层级"
QString JsonOperate::readNode(QStringList node)
{
    QJsonValue nodeValue;
    QJsonObject nodeObj=rootObj;
    for (int i=0;i<node.size();i++)
    {
        nodeValue=nodeObj.value(node[i]);
        if(nodeValue.type() == QJsonValue::Object)
        {
            nodeObj=nodeValue.toObject();
            continue;
        }
    }
    return nodeValue.toString();
}
//清楚xml文件，root下的内容
void JsonOperate::clearJson()
{
    QJsonDocument doc;
    QJsonObject rootObject;
    // 将object设置为本文档的主要对象
    doc.setObject(rootObject);
    m_dom=doc;
    closeJson();
    openJson(m_fileName);
}
//关闭xml文件并保存
void JsonOperate::closeJson()
{
    QFile m_file(m_fileName);
    if (!m_file.open(QFile::WriteOnly | QFile::Truncate)) return;
    QTextStream stream(&m_file);
    stream.setCodec("UTF-8");		// 设置写入编码是UTF8
    // 写入文件
    m_dom.setObject(rootObj);
    stream << m_dom.toJson();
    m_file.close();
}
QStringList JsonOperate::getChild(QStringList node)
{
    QStringList str_list;
    QJsonValue nodeValue;
    QJsonObject nodeObj=rootObj;
    for (int i=0;i<node.size();i++)
    {
        nodeValue=nodeObj.value(node[i]);
        if(nodeValue.type() == QJsonValue::Object)
        {
            nodeObj=nodeValue.toObject();
            continue;
        }
    }
    QJsonObject::iterator iter=nodeObj.begin();
    while(iter!=nodeObj.end())
    {
        str_list<<iter.key();
        iter++;
    }

    return str_list;
}
