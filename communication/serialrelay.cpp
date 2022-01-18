#include "serialrelay.h"

SerialRelay::SerialRelay(QObject *parent) : QObject(parent)
  ,_p_read_write_timer(nullptr)
{
    serialPort=new QSerialPort(this);
    connect(serialPort,&QSerialPort::readyRead,this,&SerialRelay::slot_startTimer);
    connect(serialPort, &QSerialPort::errorOccurred, this, &SerialRelay::slot_readError);

    _p_timer_rev=new QTimer(this);
    connect(_p_timer_rev, &QTimer::timeout, this, &SerialRelay::slot_readyRead);

}
SerialRelay::~SerialRelay()
{
    if(_is_read_TriggerSignal)
    {
        disconnect(_p_read_write_timer, &QTimer::timeout, this, &SerialRelay::slot_read_write);
    }

    disconnectSerialRelay();
    if(_p_timer_rev!=nullptr)
    {
        _p_timer_rev->stop();
        delete _p_timer_rev;
        _p_timer_rev=nullptr;
    }
    if(serialPort!=nullptr)
    {
        delete serialPort;
        serialPort=nullptr;
    }
    if(_p_read_write_timer!=nullptr)
    {
        _p_read_write_timer->stop();
        delete _p_read_write_timer;
        _p_read_write_timer=nullptr;
    }
}
void SerialRelay::initPar(QString comname, QString portName, QString parity ,int baud,int databits,int stopbit,int readsignal,int readspace,QMap<QString,int> addrInfo)
{
    _name=comname;
    _portName=portName;
    _parity=parity;
    _baudRate=baud;
    _databit=databits;
    _stopbit=stopbit;

    _write_startaddr=addrInfo[QString::fromLocal8Bit("写入起始地址")];
    _write_size=addrInfo[QString::fromLocal8Bit("写入长度")];
    _is_read_TriggerSignal=readsignal;
    _read_space=readspace;
    zeroQList(&_write_data,_write_size);

    if(_is_read_TriggerSignal)
    {
        _p_read_write_timer=new QTimer(this);
        connect(_p_read_write_timer, &QTimer::timeout, this, &SerialRelay::slot_read_write);
    }

}


//初始化fx3U的通讯
int8_t SerialRelay::initSerialRelay()
{
    //初始化串口
    if(serialPort->isOpen())//如果串口已经打开 先关闭
    {
        serialPort->close();
    }
    serialPort->setPortName(_portName);
    if(!serialPort->open(QIODevice::ReadWrite)) return 1;
    serialPort->setBaudRate(_baudRate,QSerialPort::AllDirections);//设置波特率为115200
    switch(_databit)
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
    if(_parity=="No")
    {
        serialPort->setParity(QSerialPort::NoParity); //无校验
    }
    else if(_parity=="Odd")
    {
        serialPort->setParity(QSerialPort::OddParity); //奇校验
    }
    else if(_parity=="Even")
    {
        serialPort->setParity(QSerialPort::EvenParity); //偶校验
    }
    else if(_parity=="Space")
    {
        serialPort->setParity(QSerialPort::SpaceParity); //校验位始终为0
    }
    else if(_parity=="Mark")
    {
        serialPort->setParity(QSerialPort::MarkParity); //校验位始终为1
    }
    switch(_stopbit)
    {
    case 1 :
        serialPort->setStopBits(QSerialPort::OneStop);//停止位设置为1
        break;
    case 2 :
        serialPort->setStopBits(QSerialPort::TwoStop);//停止位设置为2
        break;
    }
    serialPort->setFlowControl(QSerialPort::NoFlowControl);//设置为无流控制
    return 0;
}
bool SerialRelay::disconnectSerialRelay()
{

    if(serialPort==nullptr)return true;
    if(serialPort->isOpen())
    {
        serialPort->close();
    }
    return 0;
}
void SerialRelay::slot_startTimer()
{
    _p_timer_rev->start(20);
}
//接收数据并转化为int数组
void SerialRelay::slot_readyRead()
{
    _p_timer_rev->stop();
    QByteArray readByte = serialPort->readAll();
    if(!readByte.isEmpty())
    {

    }
}

void SerialRelay::slot_readError()
{
    _isConnected=false;
    emit signal_com_state(_name,false);
}
//串口发送十六进制
void SerialRelay::sendHex(QString sendText)
{
    QByteArray sendByte;
    sendByte=QByteArray::fromHex(sendText.toLocal8Bit());
    serialPort->write(sendByte);
}
void SerialRelay::slot_write(QString comname,int startaddr, QList<int> writedata)
{
    if(_name!=comname)return;
    for (int i=startaddr;i<startaddr+writedata.size();i++)
    {
        _write_data[i]=writedata[i-startaddr];
    }
    if(!_is_read_TriggerSignal)
    {
        if(!_isConnected)
        {
            int issucceed=initSerialRelay();
            _isConnected=issucceed==0?true:false;
            emit signal_com_state(_name,_isConnected);
            return;
        }
        writeRegister(_write_data);
        zeroQList(&_write_data,_write_size);
    }

}

void SerialRelay::zeroQList(QList<int> *list, int size)
{
    (*list).clear();
    for (int i=0;i<size;i++)
    {
        (*list)<<1;
    }
}
void SerialRelay::slot_read_write()
{ 

    if(!_isConnected)
    {
        int issucceed=initSerialRelay();
        _isConnected=issucceed==0?true:false;
        emit signal_com_state(_name,_isConnected);
        return;
    }
    writeRegister(_write_data);
    zeroQList(&_write_data,_write_size);
}
void SerialRelay::slot_com_start()
{
    if(_p_read_write_timer!=nullptr)
    {
        _p_read_write_timer->start(_read_space);
    }
}

void SerialRelay::slot_com_stop()
{
    if(_p_read_write_timer!=nullptr)
    {
        _p_read_write_timer->stop();
    }
}

//写入字
void SerialRelay::writeRegister(QList<int> writedata)
{
    //M位地址
    QString sendstr="24010601";
    //数据
    for (int i=0;i<writedata.size();i++)
    {
        QString ss=QString("%1").arg(writedata[i],2,16,QLatin1Char('0')).toUpper();
        sendstr+=ss;
    }
    //计算校验和
    //QString sendstr="2401060102000000";
    QString strall=sendstr + computeSum(sendstr);
    sendHex(strall);
}
//校验和计算
QString SerialRelay::computeSum(QString linkstring)
{
    QByteArray bytedata;
    bytedata=QByteArray::fromHex(linkstring.toLocal8Bit());
    int sunResult=0;
    for (int i=0;i<bytedata.size();i++)
    {
        sunResult+=(int)bytedata[i];
    }
    QString sumLastTwo=QString("%1").arg(sunResult,2,16,QLatin1Char('0')).right(2).toUpper();
    return sumLastTwo;
}
