#include "lightcontrol.h"
#include "ui_lightcontrol.h"
#include"general/configfileoperate.h"

LightControl::LightControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LightControl)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    setWindowModality(Qt::WindowModal);
    _pSerial=new SerialPortNet(this);
}

LightControl::~LightControl()
{
    delete _pSerial;
    _pSerial=nullptr;
    delete ui;
}
//初始化参数
void LightControl::initPar(QString path)
{
    m_LightConfigPath=path+"\\LightConfig.ini";
    ConfigFileOperate config(m_LightConfigPath);
    ui->ledtLightCom->setText(config.readKeyValue("LightCom"));
    ui->cbboxLightChannel->setCurrentIndex(0);
    ui->ledtLightValue->setText(config.readKeyValue("CH"+ui->cbboxLightChannel->currentText()));
}

bool LightControl::openCom(QString comname)
{
    int errnum=_pSerial->initSerialPort(comname,"No",9600,8,1);
    if(errnum==0)
        return true;
    else
        return false;
}

void LightControl::on_pbtnSave_clicked()
{
    ConfigFileOperate config(m_LightConfigPath);
    config.setKeyValue("LightCom",ui->ledtLightCom->text());
    config.setKeyValue("CH"+ui->cbboxLightChannel->currentText(),ui->ledtLightValue->text());
}

void LightControl::on_pbtnOpenCom_clicked()
{
    bool isopen=openCom(ui->ledtLightCom->text());
    if(isopen)
        ui->ledtInfo->setText("open");
    else
        ui->ledtInfo->setText("open failed");
}

void LightControl::on_pbtnCloseCom_clicked()
{
    _pSerial->disconnectSerial();
}
unsigned char dosCheck(QString data)
{
    unsigned char sum=0x00;
    for (int i=0;i<data.length();i++)
    {
        unsigned char add=data[i].toLatin1();
        sum^=add;
    }
    return sum;
}

void LightControl::on_pbtnSet_clicked()
{
    QString command=QString::number(ui->cbboxCommand->currentIndex()+1);
    QString channel=ui->cbboxLightChannel->currentText();
    QString value=QString("%1").arg(ui->ledtLightValue->text().toInt(),3,16,QLatin1Char('0'));
    setCommand(command,channel,value);
}
void LightControl::setCommand(QString command,QString channel,QString value)
{
    unsigned char sum=dosCheck(command+channel+value);
    QString checkdata=QString("%1").arg(sum,2,16,QLatin1Char('0'));
    _pSerial->sendStr("$"+command+channel+value+checkdata);
}

void LightControl::on_cbboxLightChannel_currentTextChanged(const QString &arg1)
{
    ConfigFileOperate config(m_LightConfigPath);
    ui->ledtLightValue->setText(config.readKeyValue("CH"+arg1));
}
