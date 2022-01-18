#include "reportview.h"
#include "ui_reportview.h"
#include"general/generalfunc.h"


ReportView::ReportView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ReportView)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
    m_ExePath=qApp->applicationDirPath();
    ui->ledtFilePath->setReadOnly(true);
    initPar();
}

ReportView::~ReportView()
{
    delete ui;
}
//初始化参数
void ReportView::initPar()
{
    m_SystemList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report");
    if(m_SystemList.size()<1)return;
    ui->cbboxSystemSelect->addItems(m_SystemList);
}
//项目记录选择
void ReportView::on_cbboxSystemSelect_currentTextChanged(const QString &arg1)
{
    m_SystemSelect=arg1;
    m_StationList.clear();
    m_StationList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report\\"+m_SystemSelect);
    if(m_StationList.size()<1)return;
    ui->cbboxStationSelect->clear();
    ui->cbboxStationSelect->addItems(m_StationList);
}
//工位记录选择
void ReportView::on_cbboxStationSelect_currentTextChanged(const QString &arg1)
{
    m_StationSelect=arg1;
    m_CamList.clear();
    m_CamList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report\\"+m_SystemSelect+"\\"+m_StationSelect);
    if(m_CamList.size()<1)return;
    ui->cbboxCamSelect->clear();
    ui->cbboxCamSelect->addItems(m_CamList);
}
//相机选择
void ReportView::on_cbboxCamSelect_currentTextChanged(const QString &arg1)
{
    m_CamSelect=arg1;
    m_CamStepList.clear();
    m_CamStepList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report\\"+m_SystemSelect+"\\"+m_StationSelect+"\\"+m_CamSelect);
    if(m_CamStepList.size()<1)return;
    ui->cbboxCamStep->clear();
    ui->cbboxCamStep->addItems(m_CamStepList);
}
//步骤选择
void ReportView::on_cbboxCamStep_currentTextChanged(const QString &arg1)
{
    m_CamStep=arg1;
    m_TypeList.clear();
    m_TypeList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report\\"+m_SystemSelect+"\\"+m_StationSelect+"\\"+m_CamSelect+"\\"+m_CamStep);
    if(m_TypeList.size()<1)return;
    ui->cbboxTypeSelect->clear();
    ui->cbboxTypeSelect->addItems(m_TypeList);
}

//品种选择
void ReportView::on_cbboxTypeSelect_currentTextChanged(const QString &arg1)
{
    m_TypeSelect=arg1;
    m_DateList.clear();
    m_DateList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report\\"+m_SystemSelect+"\\"+m_StationSelect+"\\"+m_CamSelect+"\\"+m_CamStep+"\\"+m_TypeSelect);
    if(m_DateList.size()<1)return;
    ui->cbboxDateSelect->clear();
    ui->cbboxDateSelect->addItems(m_DateList);
}
//日期选择
void ReportView::on_cbboxDateSelect_currentTextChanged(const QString &arg1)
{
    m_DateSelect=arg1;
    m_ResultList.clear();
    m_ResultList=GeneralFunc::GetAllFolderName(m_ExePath+"\\Report\\"+m_SystemSelect+"\\"+m_StationSelect+"\\"+m_CamSelect+"\\"+m_CamStep+"\\"+m_TypeSelect+"\\"+m_DateSelect);
    ui->cbboxResultSelect->clear();
    ui->cbboxResultSelect->addItems(m_ResultList);
}
//结果选择
void ReportView::on_cbboxResultSelect_currentTextChanged(const QString &arg1)
{
    m_ResultSelect=arg1;
    m_fileList.clear();
    m_ResultFolder=m_ExePath+"\\Report\\"+m_SystemSelect+"\\"+m_StationSelect+"\\"+m_CamSelect+"\\"+m_CamStep+"\\"+m_TypeSelect+"\\"+m_DateSelect+"\\"+m_ResultSelect;
    m_fileList=GeneralFunc::getFileListName(m_ResultFolder,QStringList()<<"*.jpg");

    if(m_fileList.size()<1)return;
    m_fileSelectNum=0;
    readAndShow();
}
//读取并显示
void ReportView::readAndShow()
{
    showPictureInWidget();
    if(m_fileSelectNum==0)
        ui->pbtnPreviousOne->setEnabled(false);
    else
        ui->pbtnPreviousOne->setEnabled(true);

    if(m_fileSelectNum==m_fileList.size()-1)
        ui->pbtnNextOne->setEnabled(false);
    else
        ui->pbtnNextOne->setEnabled(true);
}

//显示图像在控件上
void ReportView::showPictureInWidget()
{
    QString showstring=tr(QString::fromLocal8Bit("当前:%1/%2").arg(QString::number(m_fileSelectNum+1),QString::number(m_fileList.size())).toUtf8());
    ui->lblNumNow->setText(showstring);
    m_fileSelect=m_fileList[m_fileSelectNum];
    m_fileSelectPath=m_ResultFolder+"\\"+m_fileSelect+".jpg";
    QImage image;
    image.load(m_fileSelectPath);
    ui->widget->initImage(image);
    ui->ledtFilePath->setText(m_fileSelectPath);
}
//第一个
void ReportView::on_pbtnFrontOne_clicked()
{
    if(m_fileList.size()<1)return;
    m_fileSelectNum=0;
    readAndShow();
}
//最后一个
void ReportView::on_pbtnLastOne_clicked()
{
    if(m_fileList.size()<1)return;
    m_fileSelectNum=m_fileList.size()-1;
    readAndShow();
}
//前一个
void ReportView::on_pbtnPreviousOne_clicked()
{
    if(m_fileList.size()<1)return;
    m_fileSelectNum-=1;
    readAndShow();
}
//后一个
void ReportView::on_pbtnNextOne_clicked()
{
    if(m_fileList.size()<1)return;
    m_fileSelectNum+=1;
    readAndShow();
}
void ReportView::on_pbtnJump_clicked()
{
    int jumpnum=ui->ledtJump->text().toInt();
    if(jumpnum<0||m_fileList.size()<jumpnum+1)return;
    m_fileSelectNum=jumpnum;
    readAndShow();
}

void ReportView::on_pbtnClose_clicked()
{
    close();
}


