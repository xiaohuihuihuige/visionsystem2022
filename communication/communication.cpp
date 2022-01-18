#include "communication.h"
#include "ui_communication.h"
#include"general/configfileoperate.h"
#include"mainwindow.h"
#include <QSerialPort>
#include <QSerialPortInfo>
#include"general/generalfunc.h"
#include"general/xmloperate.h"
#include<QMessageBox>



Communication::Communication(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Communication)
{
    ui->setupUi(this);
    //设置窗口为唯一窗口
    setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
}

Communication::~Communication()
{
    delete ui;
}
//初始化参数
void Communication::initPar(QString path)
{
    m_AllStationPath=path;
    GeneralFunc::isDirExist(m_AllStationPath,true);
    m_ComConfigPath=path+"\\ComConfig.xml";
    //readComConfigFromXML(m_ComConfigPath);
    initUI();
}

//初始化UI
void Communication::initUI()
{
    GeneralFunc::setTableWidgetHeader(ui->tblwgtAddr,QStringList()<<tr(QString::fromLocal8Bit("地址标记").toUtf8())
                                      <<tr(QString::fromLocal8Bit("地址").toUtf8()));
    ui->cbboxPLCType->addItem("");
    ui->cbboxPLCType->addItems(protocol_type_list);
    readComConfigFromXML(m_ComConfigPath);
    QMap<QString,ComInfo>::iterator iter=m_com.begin();
    while(iter!=m_com.end())
    {
        ui->cbboxComList->addItem(iter.key());
        ++iter;
    }
}

//将通讯设置写入文件
void Communication::writeComPar()
{

    XmlOperate m_xml;
    m_xml.openXml(m_ComConfigPath);
    QString text=ui->cbboxComList->currentText();
    QString plctype=ui->cbboxPLCType->currentText();
    EnumChange::PLCCom plcType=EnumChange::string2enum_plc(plctype);
    switch (plcType)
    {
    case EnumChange::ModbusTCP:
    case EnumChange::MELSEC_MC:
    case EnumChange::TCP:
        m_xml.addNode(QStringList()<<text<<"TcpIPPort",ui->ledtIPPort->text());
        break;
    case EnumChange::ModbusRTU:
    case EnumChange::MELSEC_FX3U:
    case EnumChange::HostLink:
    case EnumChange::Serial:
    case EnumChange::SerialRelay:
    case EnumChange::WeiKongPLC:
        m_xml.addNode(QStringList()<<text<<"PortName",ui->cbboxPort->currentText());
        m_xml.addNode(QStringList()<<text<<"Parity",ui->cbboxParity->currentText());
        m_xml.addNode(QStringList()<<text<<"BaudRate",ui->cbboxBaud->currentText());
        m_xml.addNode(QStringList()<<text<<"DataBits",ui->cbboxDataBits->currentText());
        m_xml.addNode(QStringList()<<text<<"StopBits",ui->cbboxStopBit->currentText());
        break;
    }
    m_xml.addNode(QStringList()<<text<<"PLCType",plctype);
    m_xml.addNode(QStringList()<<text<<"ReadTriggerSignal",ui->cbboxReadTriggerSignal->currentText());
    m_xml.addNode(QStringList()<<text<<"ServerID",ui->ledtServerID->text());
    m_xml.addNode(QStringList()<<text<<"Sec",ui->ledtReadSec->text());
    m_xml.removeNode(QStringList()<<text<<"Addr");
    int count=ui->tblwgtAddr->rowCount();
    for (int i=0;i<count;i++)
    {
        QString key=ui->tblwgtAddr->item(i,0)->text();
        QString value=ui->tblwgtAddr->item(i,1)->text();
        if(key=="")continue;
        m_xml.addNode(QStringList()<<text<<"Addr"<<key,value);
    }
    m_xml.closeXml();

}
//写入之后再读取一遍，更新变量
void Communication::on_pbtnChange_clicked()
{
    writeComPar();
    readComConfigFromXML(m_ComConfigPath);
    QString com=ui->cbboxComList->currentText();
    emit signal_com_change(com);
    //GeneralFunc::isDirExist(m_AllStationPath+"\\"+com,true);
}

void Communication::closeEvent(QCloseEvent *event)
{
    close();
}
//获取串口列表
QStringList Communication::getAvailableSerial()
{
    QStringList portList;
    foreach (const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        portList<<info.portName();
    }
    return portList;
}
//增加通讯地址
void Communication::on_pbtnAddAddr_clicked()
{

    int row=ui->tblwgtAddr->rowCount();
    ui->tblwgtAddr->insertRow(row);
    QTableWidgetItem *item=new QTableWidgetItem(tr(QString::fromLocal8Bit("地址").toUtf8()));
    ui->tblwgtAddr->setItem(row,0,item);
    item=new QTableWidgetItem(QString("100"));
    ui->tblwgtAddr->setItem(row,1,item);
}
//移除通讯地址
void Communication::on_pbtnDeleteAddr_clicked()
{
    int row=ui->tblwgtAddr->currentRow();
    ui->tblwgtAddr->removeRow(row);
}
//从文件中读取通讯设置
int Communication::readComConfigFromXML(QString fileName)
{

    m_com.clear();
    XmlOperate m_xml;
    m_xml.openXml(fileName);
    QStringList com_all=m_xml.getChild(QStringList());
    for (int i=0;i<com_all.size();i++)
    {
        m_com[com_all[i]].plcType=m_xml.readNode(QStringList()<<com_all[i]<<"PLCType");
        m_com[com_all[i]].serverID=m_xml.readNode(QStringList()<<com_all[i]<<"ServerID").toInt();
        m_com[com_all[i]].readTriggerSignal=m_xml.readNode(QStringList()<<com_all[i]<<"ReadTriggerSignal").toInt();
        m_com[com_all[i]].sec=m_xml.readNode(QStringList()<<com_all[i]<<"Sec").toInt();
        EnumChange::PLCCom plcType=EnumChange::string2enum_plc(m_com[com_all[i]].plcType);
        switch (plcType)
        {
        case EnumChange::ModbusTCP:
        case EnumChange::MELSEC_MC:
        case EnumChange::TCP:
            m_com[com_all[i]].tcpIPPort=m_xml.readNode(QStringList()<<com_all[i]<<"TcpIPPort");
            break;
        case EnumChange::ModbusRTU:
        case EnumChange::MELSEC_FX3U:
        case EnumChange::HostLink:
        case EnumChange::Serial:
        case EnumChange::SerialRelay:
        case EnumChange::WeiKongPLC:
            m_com[com_all[i]].portname=m_xml.readNode(QStringList()<<com_all[i]<<"PortName");
            m_com[com_all[i]].parity=m_xml.readNode(QStringList()<<com_all[i]<<"Parity");
            m_com[com_all[i]].baudrate=m_xml.readNode(QStringList()<<com_all[i]<<"BaudRate").toInt();
            m_com[com_all[i]].dataBits=m_xml.readNode(QStringList()<<com_all[i]<<"DataBits").toInt();
            m_com[com_all[i]].stopBits=m_xml.readNode(QStringList()<<com_all[i]<<"StopBits").toInt();
            break;
        }
        QStringList addr_all=m_xml.getChild(QStringList()<<com_all[i]<<"Addr");
        for(int j=0;j<addr_all.size();j++)
        {
            m_com[com_all[i]].addr[addr_all[j]]=m_com[com_all[i]].stopBits=m_xml.readNode(QStringList()<<com_all[i]<<"Addr"<<addr_all[j]).toInt();
        }
    }
    m_xml.closeXml();
    return 1;
}

//
void Communication::on_cbboxComList_currentTextChanged(const QString &arg1)
{   
    ui->cbboxPLCType->setCurrentText("");
    ui->cbboxPLCType->setCurrentText(m_com[arg1].plcType);
}

//更改PLC类型
void Communication::on_cbboxPLCType_currentTextChanged(const QString &arg1)
{
    if(arg1=="")return;
    bool istcp=false,isserial=false;
    QString comName=ui->cbboxComList->currentText();
    QMap<QString,int> taddr;

    EnumChange::PLCCom plcType=EnumChange::string2enum_plc(arg1);
    switch (plcType)
    {
    case EnumChange::ModbusTCP:
    case EnumChange::MELSEC_MC:
    case EnumChange::TCP:
        istcp=true;
        isserial=false;
        ui->ledtIPPort->setText(m_com[comName].tcpIPPort);
        break;
    case EnumChange::ModbusRTU:
    case EnumChange::MELSEC_FX3U:
    case EnumChange::HostLink:
    case EnumChange::Serial:
    case EnumChange::SerialRelay:
    case EnumChange::WeiKongPLC:
        istcp=false;
        isserial=true;
        ui->cbboxPort->clear();
        ui->cbboxPort->addItems(getAvailableSerial());
        ui->cbboxPort->setCurrentText(m_com[comName].portname);
        ui->cbboxParity->setCurrentText(m_com[comName].parity);
        ui->cbboxBaud->setCurrentText(QString::number(m_com[comName].baudrate));
        ui->cbboxDataBits->setCurrentText(QString::number(m_com[comName].dataBits));
        ui->cbboxStopBit->setCurrentText(QString::number(m_com[comName].stopBits));
        break;
    }

    ui->ledtServerID->setText(QString::number(m_com[comName].serverID));
    ui->cbboxReadTriggerSignal->setCurrentText(QString::number(m_com[comName].readTriggerSignal));
    ui->ledtReadSec->setText(QString::number(m_com[comName].sec));
    taddr=m_com[comName].addr;

    ui->ledtIPPort->setEnabled(istcp);
    ui->cbboxPort->setEnabled(isserial);
    ui->cbboxParity->setEnabled(isserial);
    ui->cbboxBaud->setEnabled(isserial);
    ui->cbboxDataBits->setEnabled(isserial);
    ui->cbboxStopBit->setEnabled(isserial);
    GeneralFunc::removeTableAllRow(ui->tblwgtAddr);
    if(taddr.size()<1)return;
    QMap<QString,int>::iterator itAddr=taddr.begin();
    while (itAddr!=taddr.end())
    {
        int row=ui->tblwgtAddr->rowCount();
        ui->tblwgtAddr->insertRow(row);
        QTableWidgetItem *item=new QTableWidgetItem(itAddr.key());
        ui->tblwgtAddr->setItem(row,0,item);
        item=new QTableWidgetItem(QString::number(itAddr.value()));
        ui->tblwgtAddr->setItem(row,1,item);
        itAddr++;
    }

}


//增加一个通讯
void Communication::on_pbtnAddOne_clicked()
{
    QString settext=ui->ledtComName->text();
    if(m_com.contains(settext))
    {
        QMessageBox::warning(this,"warning",
                             tr(QString::fromLocal8Bit("已存在命名为\"%1\"的通信,请修改命名再创建新的通信").arg(settext).toUtf8()));
        return;
    }
    ComInfo cominfo;
    cominfo.tcpIPPort="127.0.0.1:502";
    cominfo.sec=100;
    cominfo.serverID=1;
    cominfo.readTriggerSignal=0;
    cominfo.addr=QMap<QString,int>();
    cominfo.plcType=protocol_type_list[0];
    m_com[settext]=cominfo;
    ui->cbboxComList->addItem(settext);
    ui->cbboxComList->setCurrentText(settext);
}
//删除一个通讯
void Communication::on_pbtnDeleteOne_clicked()
{

    XmlOperate m_xml;
    m_xml.openXml(m_ComConfigPath);
    QString usename=ui->cbboxComList->currentText();
    m_xml.removeNode(QStringList()<<usename);
    m_xml.closeXml();
    m_com.remove(usename);
    ui->cbboxComList->removeItem(ui->cbboxComList->currentIndex());
}

