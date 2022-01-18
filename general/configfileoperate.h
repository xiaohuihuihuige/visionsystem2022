#ifndef CONFIGFILEOPERATE_H
#define CONFIGFILEOPERATE_H
#include<QSettings>

class ConfigFileOperate
{
public:
    ConfigFileOperate(QString iniFileName="");
    ~ConfigFileOperate();
     QSettings  *m_IniFile;
     QString readSection(QString section,QString name);
     void writeSection(QString section,QString name,QString value);
     void setSection(QString section);
     void setKeyValue(QString key, QString value);
     void resetSection();
     QString readKeyValue(QString key);
     void readSection(QString section, QStringList *keys, QStringList *values);
     void removeKey(QString section, QString key);
     QStringList childGroup();
     void removeSection(QString section);
     QStringList childKeys();
     void removeKey(QString key);
     void removeAllKey();
     void clear();
};

#endif // CONFIGFILEOPERATE_H
