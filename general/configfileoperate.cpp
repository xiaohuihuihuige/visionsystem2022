#include "configfileoperate.h"
#include<QTextCodec>
ConfigFileOperate::ConfigFileOperate(QString iniFileName)
{
    m_IniFile = new QSettings(iniFileName, QSettings::IniFormat);
    //m_IniFile->setIniCodec("UTF-8");
}

ConfigFileOperate::~ConfigFileOperate()
{
    delete m_IniFile;
}
//读取节点下的键值
QString ConfigFileOperate::readSection(QString section,QString name)
{
    QString addr=m_IniFile->value(section + "/" + name).toString();
    return addr;
}
//写入节点下的键值
void ConfigFileOperate::writeSection(QString section,QString name,QString value)
{
    m_IniFile->setValue(section + "/" + name, value);
}
//设置节点
void ConfigFileOperate::setSection(QString section)
{
    m_IniFile->beginGroup(section);
}
//设置键值
void ConfigFileOperate::setKeyValue(QString key,QString value)
{
    m_IniFile->setValue(key, value);
}
//读取值
QString ConfigFileOperate::readKeyValue(QString key)
{
    QString value;
    value=m_IniFile->value(key).toString();
    return value;
}
//结束节点
void ConfigFileOperate::resetSection()
{
    m_IniFile->endGroup();
}
//读取节点下的所有键值
void ConfigFileOperate::readSection(QString section,QStringList *keys,QStringList *values)
{
    setSection(section);
    QStringList tkeys,tvalues;
    tkeys = m_IniFile->childKeys();
    for(int i=0; i<tkeys.size(); i++)
    {
        tvalues<<readKeyValue(tkeys[i]);
    }
    *keys=tkeys;
    *values=tvalues;
    resetSection();
}
//移除不在节点下得所有键值
void ConfigFileOperate::removeAllKey()
{
    QStringList tkeys,tvalues;
    tkeys = m_IniFile->childKeys();
    for(int i=0; i<tkeys.size(); i++)
    {
        removeKey(tkeys[i]);
    }
}
//清空
void ConfigFileOperate::clear()
{
    m_IniFile->clear();
}
//移除节点键值
void ConfigFileOperate::removeKey(QString section,QString key)
{
    setSection(section);
    m_IniFile->remove(key);
    resetSection();
}
//移除键值
void ConfigFileOperate::removeKey(QString key)
{
    m_IniFile->remove(key);
}
//移除节点
void ConfigFileOperate::removeSection(QString section)
{
    m_IniFile->remove(section);
}
//子节点
QStringList ConfigFileOperate::childGroup()
{
    return m_IniFile->childGroups();
}
//子键值
QStringList ConfigFileOperate::childKeys()
{
    return m_IniFile->childKeys();
}





