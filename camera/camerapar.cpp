#include "camerapar.h"
#include "ui_camerapar.h"
#include"general/xmloperate.h"
#include<QMessageBox>
#include"general/generalfunc.h"

CameraPar::CameraPar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraPar)

{
    ui->setupUi(this);
    //设置窗口状态
    //setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
    m_ExePath=qApp->applicationDirPath();
}

CameraPar::~CameraPar()
{
    delete ui;
}
//初始化变量
void CameraPar::initPar(QString pro_path)
{
    GeneralFunc::isDirExist(pro_path+"\\camera",true);
    cam_config_path=pro_path+"\\camera\\CamConfig.xml";
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    camList=m_xml.getChild(QStringList());
    m_xml.closeXml();
    initUI();
}
//初始化UI
void CameraPar::initUI()
{
    ui->cbboxCamList->clear();
    ui->cbboxCamList->addItems(camList);
}
//在关闭前发送设置变化的信号
void CameraPar::closeEvent(QCloseEvent *event)
{
    event->accept();
}

//写入相机设置
void CameraPar::on_pbtnWrite_clicked()
{

    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    QString text=ui->cbboxCamList->currentText();
    m_xml.addNode(QStringList()<<text<<"CamType",ui->cbboxCamType->currentText());
    m_xml.addNode(QStringList()<<text<<"CameraModelSerial",ui->ledtSerialNum->text());
    m_xml.addNode(QStringList()<<text<<"Resolution",ui->ledtResolution->text());
    m_xml.addNode(QStringList()<<text<<"ExposureTime",ui->ledtExpTime->text());
    m_xml.addNode(QStringList()<<text<<"Gain",ui->ledtGain->text());
    m_xml.addNode(QStringList()<<text<<"GrabDelay",ui->ledtTimeDelay->text());
    m_xml.addNode(QStringList()<<text<<"TriggerMode",ui->cbboxTriggerMode->currentText());
    m_xml.addNode(QStringList()<<text<<"TriggerSource",ui->cbboxTriggerSource->currentText());
    m_xml.addNode(QStringList()<<text<<"CamStepAll",ui->ledtCamStepAll->text());
    m_xml.closeXml();
    emit signal_cam_change(text);

}

//增加一个相机配置
void CameraPar::on_pbtnAddCam_clicked()
{
    QString settext=ui->ledtCamName->text();
    ui->cbboxCamList->addItem(settext);
    camList<<settext;
    ui->cbboxCamList->setCurrentText(settext);
    ui->cbboxCamType->setCurrentIndex(0);
    ui->ledtSerialNum->setText(tr(QString::fromLocal8Bit("型号:序列号").toUtf8()));
    ui->ledtResolution->setText(tr(QString::fromLocal8Bit("width:height").toUtf8()));
    ui->ledtExpTime->setText(QString::fromLocal8Bit("10000"));
    ui->ledtGain->setText("0");
    ui->ledtTimeDelay->setText(QString::fromLocal8Bit("10"));
    ui->cbboxTriggerMode->setCurrentText("0");
    ui->cbboxTriggerSource->setCurrentText("7");
    ui->ledtCamStepAll->setText("step0");
}

//选择相机时更新界面
void CameraPar::on_cbboxCamList_currentTextChanged(const QString &arg1)
{
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    ui->cbboxCamType->setCurrentText(m_xml.readNode(QStringList()<<arg1<<"CamType"));
    ui->ledtSerialNum->setText(m_xml.readNode(QStringList()<<arg1<<"CameraModelSerial"));
    ui->ledtResolution->setText(m_xml.readNode(QStringList()<<arg1<<"Resolution"));
    ui->ledtExpTime->setText(m_xml.readNode(QStringList()<<arg1<<"ExposureTime"));
    ui->ledtGain->setText(m_xml.readNode(QStringList()<<arg1<<"Gain"));
    ui->ledtTimeDelay->setText(m_xml.readNode(QStringList()<<arg1<<"GrabDelay"));
    ui->cbboxTriggerMode->setCurrentText(m_xml.readNode(QStringList()<<arg1<<"TriggerMode"));
    ui->cbboxTriggerSource->setCurrentText(m_xml.readNode(QStringList()<<arg1<<"TriggerSource"));
    ui->ledtCamStepAll->setText(m_xml.readNode(QStringList()<<arg1<<"CamStepAll"));
    m_xml.closeXml();



}
//删除一个相机配置
void CameraPar::on_pbtnDeleteCam_clicked()
{
    QMessageBox::StandardButton btn=QMessageBox::question(this,
                                                          QString::fromLocal8Bit("提示"),
                                                          QString::fromLocal8Bit("确实要删除当前相机吗?"),
                                                          QMessageBox::Yes|QMessageBox::No);
    if(btn==QMessageBox::Yes)
    {
        XmlOperate m_xml;
        m_xml.openXml(cam_config_path);
        m_xml.removeNode(QStringList()<<ui->cbboxCamList->currentText());
        m_xml.closeXml();
        ui->cbboxCamList->removeItem(ui->cbboxCamList->currentIndex());
    }
}
