#include "xmloperate.h"

XmlOperate::XmlOperate()
{



}
XmlOperate::~XmlOperate()
{

}
//创建一个xml文件，根节点为“root”
//path:文件路径
bool XmlOperate::creatXml(const QString& path)
{
    //创建xml文件
    QFile file(path);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) return false;
    QDomDocument doc;
    //写入xml头部
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");
    doc.appendChild(instruction);
    //添加根节点
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);
    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream, 4); //缩进4格
    file.close();
    return true;
}
//打开一个xml文件，并获取根节点root
//fileName:文件路径
void XmlOperate::openXml(QString fileName)
{
    m_fileName=fileName;
    QFile *m_file=new QFile(m_fileName);
    if(!m_file->exists())
    {
        delete m_file;
        m_file=nullptr;
        creatXml(m_fileName);
        m_file=new QFile(m_fileName);
    }
    if(m_file->open(QIODevice::ReadWrite|QIODevice::ReadOnly))
    {
        m_dom=new QDomDocument();
        if(!m_dom->setContent(m_file))
        {
            delete m_dom;
            m_dom=nullptr;

            m_file->close();
            delete m_file;
            m_file=nullptr;

            creatXml(m_fileName);
            m_file=new QFile(m_fileName);
            m_file->open(QIODevice::ReadWrite|QIODevice::ReadOnly);

            m_dom=new QDomDocument();
            m_dom->setContent(m_file);
        }
        rootNode= m_dom->documentElement();
        m_file->close();
        return;
    }
    return;
}

//添加节点
//node：QStringList()<<"第一层级"<<"第二层级"
//value:节点的值
void XmlOperate::addNode(QStringList node,QString value)
{
    QDomNode element;
    element=rootNode.elementsByTagName(node[0]).at(0);//查找名字为node[0]的第一个节点
    if(element==QDomNode())//判断是否节点为空
    {
        QDomElement node1=m_dom->createElement(node[0]);//创建节点
        rootNode.appendChild(node1);//添加到父级
        element=rootNode.elementsByTagName(node[0]).at(0);//创建之后再查找，就不可能为空节点了
    }
    for (int i=1;i<node.size();i++)
    {
        QDomElement tnode=element.toElement();
        element=tnode.elementsByTagName(node[i]).at(0);//和上面是一样的操作
        if(element==QDomNode())
        {
            QDomElement node2=m_dom->createElement(node[i]);
            tnode.appendChild(node2);
            element=tnode.elementsByTagName(node[i]).at(0);
        }
    }
    if(element.firstChild().isNull())
    {
        QDomText text=m_dom->createTextNode(value);
        element.appendChild(text);
    }
    else
    {
        element.firstChild().setNodeValue(value);
    }

}

//删除节点
//node：QStringList()<<"第一层级"<<"第二层级"
void XmlOperate::removeNode(QStringList node)
{
    QDomNode element;
    element=rootNode.elementsByTagName(node[0]).at(0);
    for (int i=1;i<node.size();i++)
    {
        QDomElement tnode=element.toElement();
        element=tnode.elementsByTagName(node[i]).at(0);
    }
    QDomElement nodeEle=element.parentNode().toElement();
    nodeEle.removeChild(element);
}

//更新节点
//node：QStringList()<<"第一层级"<<"第二层级"
//value:节点的新值
void XmlOperate::updateNode(QStringList node,QString value)
{
    QDomNode element;
    element=rootNode.elementsByTagName(node[0]).at(0);
    for (int i=1;i<node.size();i++)
    {
        QDomElement tnode=element.toElement();
        element=tnode.elementsByTagName(node[i]).at(0);
    }
    QDomElement nodeEle=element.toElement();
    nodeEle.firstChild().setNodeValue(value);
}

//读取节点
//node：QStringList()<<"第一层级"<<"第二层级"
QString XmlOperate::readNode(QStringList node)
{
    QDomNode element;
    element=rootNode.elementsByTagName(node[0]).at(0);
    for (int i=1;i<node.size();i++)
    {
        QDomElement tnode=element.toElement();
        element=tnode.elementsByTagName(node[i]).at(0);
    }
    QDomElement nodeEle=element.toElement();
    return nodeEle.text();
}
//清楚xml文件，root下的内容
void XmlOperate::clearXml()
{
    //创建xml文件
    QDomDocument doc;
    //写入xml头部
    QDomProcessingInstruction instruction;
    instruction = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"utf-8\"");
    doc.appendChild(instruction);
    //添加根节点
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);
    *m_dom=doc;
    closeXml();
    openXml(m_fileName);
}
//关闭xml文件并保存
void XmlOperate::closeXml()
{
    QFile m_file(m_fileName);
    if (!m_file.open(QFile::WriteOnly | QFile::Truncate)) return;
    QTextStream outstream(&m_file);
    m_dom->save(outstream,4);
    m_file.close();
}
QStringList XmlOperate::getChild(QStringList node)
{
    QStringList str_list;
    QDomNode element;
    QDomNode nameRoot;
    if(node.size()>0)
    {
        element=rootNode.elementsByTagName(node[0]).at(0);
        for (int i=1;i<node.size();i++)
        {
            QDomElement tnode=element.toElement();
            element=tnode.elementsByTagName(node[i]).at(0);
        }
        nameRoot=element.firstChild();
    }
    else
    {
        nameRoot=rootNode.firstChild();
    }
    while(!nameRoot.isNull())
    {
        QDomElement eleComID=nameRoot.toElement();
        QString name=eleComID.tagName();
        str_list<<name;
        nameRoot=nameRoot.nextSiblingElement();
    }
    return str_list;
}


