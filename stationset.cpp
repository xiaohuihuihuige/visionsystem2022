#include "stationset.h"
#include "ui_stationset.h"
#include"general/generalfunc.h"
#include<QMessageBox>
#include"general/configfileoperate.h"
#include"general/xmloperate.h"
#include"publicstruct.h"

StationSet::StationSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::StationSet)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
    m_ExePath=qApp->applicationDirPath();
}

StationSet::~StationSet()
{
    delete ui;
}
//初始化参数
void StationSet::initPar()
{
    QString allProDir=m_ExePath+"\\Pro";
    QStringList proList=GeneralFunc::GetAllFolderName(allProDir);
    ConfigFileOperate tconfig(allProDir+"\\Pro.ini");
    m_UsePro=tconfig.readKeyValue(QString("UsePro"));
    m_ProPath=allProDir+"\\"+m_UsePro;
    GeneralFunc::addItemsToCombobox(ui->cbboxProSelect,proList,m_UsePro);
    ui->cbboxFunctionSelect->addItems(funtion_list);
}

//读取相机列表
QStringList StationSet::readCamList(QString campath)
{
    QStringList camList=QStringList();
    XmlOperate m_xml;
    m_xml.openXml(campath+"\\CamConfig.xml");
    camList=m_xml.getChild(QStringList());
    m_xml.closeXml();
    return  camList;
}
//读取通信列表
QStringList StationSet::readComList(QString compath)
{
    QStringList comList=QStringList();
    XmlOperate m_xml;
    m_xml.openXml(compath+"\\ComConfig.xml");
    comList=m_xml.getChild(QStringList());
    m_xml.closeXml();
    return comList;
}


void StationSet::closeEvent(QCloseEvent *event)
{ 
    close();
}

//配置得项目更改
void StationSet::on_cbboxProSelect_currentTextChanged(const QString &arg1)
{
    m_UsePro=arg1;
    m_ProPath=m_ExePath+"\\Pro\\"+m_UsePro;
    ui->cbboxStationSelect->clear();
    ui->cbboxStationSelect->addItems(readComList(m_ProPath+"\\station"));
    ui->cbboxCamSelect->clear();
    ui->cbboxCamSelect->addItems(readCamList(m_ProPath+"\\camera"));
    m_station_path=m_ProPath+"\\station\\"+m_station_name;

}
//读取原有得工位配置
void StationSet::readStationConfig(QString stationpath)
{
    m_cam_fun.clear();
    ConfigFileOperate config(stationpath+"\\StationConfig.ini");
    QStringList camsteplist=QStringList();
    camsteplist=config.childKeys();
    for (int i=0;i<camsteplist.size();i++)
    {
        m_cam_fun[camsteplist[i]]=config.readKeyValue(camsteplist[i]);
    }
    return ;
}

//工位选择
void StationSet::on_cbboxStationSelect_currentTextChanged(const QString &arg1)
{
    m_station_name=arg1;
    m_station_path=m_ProPath+"\\station\\"+m_station_name;
    readStationConfig(m_station_path);
    GeneralFunc::removeTableAllRow(ui->tabwgtCamUse);
    GeneralFunc::setTableWidgetHeader(ui->tabwgtCamUse,QStringList()<<"Cam-Step"<<"Func");
    ui->tabwgtCamUse->setEditTriggers(QAbstractItemView::NoEditTriggers);
    QMap<QString,QString>::iterator iter=m_cam_fun.begin();
    while(iter!=m_cam_fun.end())
    {
        int row=ui->tabwgtCamUse->rowCount();
        ui->tabwgtCamUse->insertRow(row);
        QTableWidgetItem *item= new QTableWidgetItem(iter.key());
        ui->tabwgtCamUse->setItem(row,0,item);
        item= new QTableWidgetItem(iter.value());
        ui->tabwgtCamUse->setItem(row,1,item);
        QStringList cam_steplist=iter.key().split('-');
        if(cam_steplist.size()==2)
        {
            QString camFolder=m_station_path+"\\"+cam_steplist[0]+"\\"+cam_steplist[1];
            GeneralFunc::isDirExist(camFolder,true);
        }
        iter++;
    }
}

//加入选用列表
void StationSet::on_pbtnAddToUse_clicked()
{
    QString camuse=ui->cbboxCamSelect->currentText();
    QString camStepUse=ui->cbboxCamStep->currentText();
    QString funcuse=ui->cbboxFunctionSelect->currentText();
    int row=ui->tabwgtCamUse->rowCount();
    ui->tabwgtCamUse->insertRow(row);
    QTableWidgetItem *item= new QTableWidgetItem(camuse+"-"+camStepUse);
    ui->tabwgtCamUse->setItem(row,0,item);
    item= new QTableWidgetItem(funcuse);
    ui->tabwgtCamUse->setItem(row,1,item);
    QString camstepFolder=m_station_path+"\\"+camuse+"\\"+camStepUse;
    GeneralFunc::isDirExist(camstepFolder,true);
}
//从选用列表删除
void StationSet::on_pbtnDeleteFromUse_clicked()
{
    int row=ui->tabwgtCamUse->currentRow();
    ui->tabwgtCamUse->removeRow(row);
}

//保存配置
void StationSet::on_pbtnSave_clicked()
{
    ConfigFileOperate config(m_station_path+"\\StationConfig.ini");
    config.removeAllKey();
    int row=ui->tabwgtCamUse->rowCount();
    for (int i=0;i<row;i++)
    {
        QString key=ui->tabwgtCamUse->item(i,0)->text();
        QString value=ui->tabwgtCamUse->item(i,1)->text();
        config.setKeyValue(key,value);
    }
    emit signal_station_change();
}

void StationSet::on_cbboxCamSelect_currentTextChanged(const QString &arg1)
{
    XmlOperate m_xml;
    m_xml.openXml(m_ProPath+"\\camera\\CamConfig.xml");
    QStringList stepList=m_xml.readNode(QStringList()<<arg1<<"CamStepAll").split(':');
    m_xml.closeXml();
    ui->cbboxCamStep->clear();
    ui->cbboxCamStep->addItems(stepList);
}
