#include "typeset.h"
#include "ui_typeset.h"
#include"general/generalfunc.h"
#include<QMessageBox>

TypeSet::TypeSet(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TypeSet)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
}

TypeSet::~TypeSet()
{
    delete ui;
}
//初始化参数
void TypeSet::initPar(QString proPath)
{
    m_ProPath=proPath;
    m_StationList=GeneralFunc::GetAllFolderName(m_ProPath+"\\station");
    if(m_StationList.size()>0)
        m_CamList<<GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[0]);
    if(m_CamList.size()>0)
        m_StepList=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[0]+"\\"+m_CamList[0]);
    if(m_StepList.size()>0)
        m_TypeList=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[0]+"\\"+m_CamList[0]+"\\"+m_StepList[0]);
    ui->lstwgtTypeList->addItems(m_TypeList);
}

void TypeSet::on_pbtnNewOne_clicked()
{
    QString newname=ui->ledtTypeName->text();
    if(newname=="")
    {
        QMessageBox::warning(this,"warning",tr(QString::fromLocal8Bit("请输入新品种的名称").toUtf8()));
        return;
    }
    else if (m_TypeList.contains(newname))
    {
        QMessageBox::warning(this,"warning",tr(QString::fromLocal8Bit("此品种已存在").toUtf8()));
        return;
    }
    m_TypeList<<newname;
    for (int i=0;i<m_StationList.size();i++)
    {
        QStringList cam_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[i]);
        for (int j=0;j<cam_name_list.size();j++)
        {
            QStringList step_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]);
            for (int k=0;k<step_name_list.size();k++)
            {
                QString folderpath=m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]+"\\"+step_name_list[k]+"\\"+newname;
                GeneralFunc::isDirExist(folderpath,true);
            }
        }
    }
    ui->lstwgtTypeList->addItem(newname);
    emit signal_type_change(newname,true);

}

void TypeSet::on_pbtnNewOneBase_clicked()
{
    QString newname=ui->ledtTypeName->text();
    if(newname=="")
    {
        QMessageBox::warning(this,"warning",tr(QString::fromLocal8Bit("请输入新品种的名称").toUtf8()));
        return;
    }
    else if (m_TypeList.contains(newname))
    {
        QMessageBox::warning(this,"warning",tr(QString::fromLocal8Bit("此品种已存在").toUtf8()));
        return;
    }
    m_TypeList<<newname;
    QString selecttype=ui->lstwgtTypeList->currentItem()->text();
    if(selecttype=="")
    {
        QMessageBox::warning(this,"warning",tr(QString::fromLocal8Bit("请选择一个品种进行复制").toUtf8()));
        return;
    }
    for (int i=0;i<m_StationList.size();i++)
    {
        QStringList cam_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[i]);
        for (int j=0;j<cam_name_list.size();j++)
        {
            QStringList step_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]);
            for (int k=0;k<step_name_list.size();k++)
            {
                QString folderpathfrom=m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]+"\\"+step_name_list[k]+"\\"+selecttype;
                QString folderpathto=m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]+"\\"+step_name_list[k]+"\\"+newname;
                GeneralFunc::copyDirectoryFiles(folderpathfrom,folderpathto,true);
            }
        }
    }
    ui->lstwgtTypeList->addItem(newname);
    emit signal_type_change(newname,true);

}

void TypeSet::on_pbtnDelete_clicked()
{
    QString selecttype=ui->lstwgtTypeList->currentItem()->text();
    if(selecttype=="")
    {
        QMessageBox::warning(this,"warning",tr(QString::fromLocal8Bit("请选择一个品种").toUtf8()));
        return;
    }
    for (int i=0;i<m_StationList.size();i++)
    {
        QStringList cam_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[i]);
        for (int j=0;j<cam_name_list.size();j++)
        {
            QStringList step_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]);
            for (int k=0;k<step_name_list.size();k++)
            {
                QString folderpathdelete=m_ProPath+"\\station\\"+m_StationList[i]+"\\"+cam_name_list[j]+"\\"+step_name_list[k]+"\\"+selecttype;
                GeneralFunc::deleteFileOrFolder(folderpathdelete);
            }
        }
    }
    m_TypeList.removeOne(selecttype);
    QListWidgetItem *itemTmp=ui->lstwgtTypeList->currentItem();
    ui->lstwgtTypeList->removeItemWidget(itemTmp);
    delete itemTmp;
    emit signal_type_change(selecttype,false);

}
