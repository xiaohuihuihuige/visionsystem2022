#include "serialportnet.h"

SerialPortNet::SerialPortNet(QObject *parent) : QObject(parent)
{
    serialPort=new QSerialPort(this);
    timer_rev=new QTimer(this);
    QObject::connect(serialPort,&QSerialPort::readyRead,this,&SerialPortNet::slot_startTimer);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialPortNet::slot_readError);
    connect(timer_rev, &QTimer::timeout, this, &SerialPortNet::slot_readyRead);
}

SerialPortNet::~SerialPortNet()
{
    disconnectSerial();
    if(timer_rev!=nullptr)
    {
        timer_rev->stop();
        delete timer_rev;
        timer_rev=nullptr;
    }
    if(serialPort!=nullptr)
    {
        delete serialPort;
        serialPort=nullptr;
    }
}
//获取串口列表
QStringList SerialPortNet::getAvailableSerial()
{  
    QStringList portList;
    foreach (const QSerialPortInfo &info,QSerialPortInfo::availablePorts())
    {
        portList<<info.portName();
    }
    return portList;
}
//初始化串口通讯
int8_t SerialPortNet::initSerialPort(QString portName,QString checkBit,int baudRate, int dataBit,int stopBit)
{
    if(serialPort->isOpen())//如果串口已经打开 先关闭
    {
        serialPort->close();
    }
    serialPort->setPortName(portName);
    if(!serialPort->open(QIODevice::ReadWrite)) return 1;
    serialPort->setBaudRate(baudRate,QSerialPort::AllDirections);//设置波特率为115200
    switch(dataBit)
    {
    case 6 :
        serialPort->setDataBits(QSerialPort::Data6);//设置数据位8
        break;
    case 7 :
        serialPort->setDataBits(QSerialPort::Data7);//设置数据位8
        break;
    case 8 :
        serialPort->setDataBits(QSerialPort::Data8);//设置数据位8
        break;
    }
    if(checkBit=="No")
    {
        serialPort->setParity(QSerialPort::NoParity); //校验位设置为0
    }
    else if(checkBit=="Odd")
    {
        serialPort->setParity(QSerialPort::OddParity); //校验位设置为0
    }
    else if(checkBit=="Even")
    {
        serialPort->setParity(QSerialPort::EvenParity); //校验位设置为0
    }
    else if(checkBit=="Space")
    {
        serialPort->setParity(QSerialPort::SpaceParity); //校验位设置为0
    }
    else if(checkBit=="Mark")
    {
        serialPort->setParity(QSerialPort::MarkParity); //校验位设置为0
    }
    switch(stopBit)
    {
    case 1 :
        serialPort->setStopBits(QSerialPort::OneStop);//停止位设置为1
        break;
    case 2 :
        serialPort->setStopBits(QSerialPort::TwoStop);//停止位设置为1
        break;
    }
    serialPort->setFlowControl(QSerialPort::NoFlowControl);//设置为无流控制
    return 0;
}
//串口断开
bool SerialPortNet::disconnectSerial()
{
    if(serialPort==nullptr)return true;
    if(serialPort->isOpen())
    {
        serialPort->close();
    }
    return true;
}
//读取接收到的信息
void SerialPortNet::slot_readyRead()
{
    timer_rev->stop();
    QByteArray buf = serialPort->readAll();
    if(!buf.isEmpty())
    {
        emit signal_readByteArryFinish(buf);
    }
}

void SerialPortNet::slot_startTimer()
{
    timer_rev->start(20);
}
//串口发送字符
void SerialPortNet::sendStr(QString sendText)
{
    QByteArray byte=sendText.toLocal8Bit();
    int a=serialPort->write(byte); //qt5去除了.toAscii()
}
//串口发送十六进制
void SerialPortNet::sendHex(QString sendText)
{
    QByteArray sendByte;
    sendByte=QByteArray::fromHex(sendText.toLocal8Bit());
    serialPort->write(sendByte);
}
//tcp发送字节
void SerialPortNet::sendByte(QByteArray sendByte)
{
    serialPort->write(sendByte);
}
//接收tcp错误并发送信号
void SerialPortNet::slot_readError(QSerialPort::SerialPortError )
{
    disconnectSerial();
    emit signal_errOccurred();
}
