#include "weikongplc.h"

WeiKongPLC::WeiKongPLC(QObject *parent) : QObject(parent)
{
    serialPort=new QSerialPort(this);
    connect(serialPort,&QSerialPort::readyRead,this,&WeiKongPLC::slot_startTimer);
    connect(serialPort, &QSerialPort::errorOccurred, this, &WeiKongPLC::slot_readError);

    _p_read_write_timer=new QTimer(this);
    connect(_p_read_write_timer, &QTimer::timeout, this, &WeiKongPLC::slot_read_write);

    _p_write_timer=new QTimer(this);
    connect(_p_write_timer, &QTimer::timeout, this, &WeiKongPLC::slot_timeto_write);

    _p_timer_rev=new QTimer(this);
    connect(_p_timer_rev, &QTimer::timeout, this, &WeiKongPLC::slot_readyRead);
}
WeiKongPLC::~WeiKongPLC()
{
    disconnectWeiKongPLC();
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
    if(_p_write_timer!=nullptr)
    {
        delete _p_write_timer;
        _p_write_timer=nullptr;
    }
}
void WeiKongPLC::initPar(QString comname,int serverID, QString portName, QString parity, int baud, int databits, int stopbit,int readspace,bool readsignal,QMap<QString,int> addrInfo)
{
     _isAnswer=false;
    _name=comname;
    _serverID=serverID;
    _portName=portName;
    _parity=parity;
    _baudRate=baud;
    _databit=databits;
    _stopbit=stopbit;
    _read_startaddr=addrInfo[QString::fromLocal8Bit("读取起始地址")];
    _read_size=addrInfo[QString::fromLocal8Bit("读取长度")];
    _write_startaddr=addrInfo[QString::fromLocal8Bit("写入起始地址")];
    _write_size=addrInfo[QString::fromLocal8Bit("写入长度")];
    _read_space=readspace;
    _is_read_TriggerSignal=readsignal;
    _is_write_data_change=false;
    zeroQList(&_read_data,_read_size);
    zeroQList(&_write_data,_write_size);
}


//初始化fx3U的通讯
int8_t WeiKongPLC::initWeiKongPLC()
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
bool WeiKongPLC::disconnectWeiKongPLC()
{
    _isConnected=false;
    if(serialPort==nullptr)return true;
    if(serialPort->isOpen())
    {
        serialPort->close();
    }
    return 0;
}
void WeiKongPLC::slot_startTimer()
{
    _p_timer_rev->start(20);
}
//接收数据并转化为int数组
void WeiKongPLC::slot_readyRead()
{
    _p_timer_rev->stop();
    QByteArray readByte = serialPort->readAll();
    if(!readByte.isEmpty())
    {
        if(readByte.size()<5)return;
        _isAnswer=true;
        emit signal_com_state(_name,_isAnswer);
        if(_isWrite)
        {
            _isWrite=false;
            return;
        }
        QByteArray buf=QByteArray::fromHex(readByte.mid(3,readByte.size()-5));
        QList<int> data=transToIntList(buf);
        QList<int> compData=compareReaddata(data,_read_data);
        _read_data=data;
        emit signal_readFinish(compData,_name);

    }
}
QList<int> WeiKongPLC::compareReaddata(QList<int> data1,QList<int> data2)
{
    if(data1.size()!=data2.size())return QList<int>();
    QList<int> data;
    for (int i=0;i<data1.size();i++)
    {
        if(data1.at(i)==data2.at(i))
        {
            data<<0;
        }
        else
        {
            data<<data1.at(i);
        }
    }
    return data;
}

void WeiKongPLC::slot_readError()
{
    _isConnected=false;
}
//串口发送十六进制
void WeiKongPLC::sendHex(QString sendText)
{
    _isAnswer=false;
    emit signal_com_state(_name,_isAnswer);
    QByteArray sendByte;
    sendByte=QByteArray::fromHex(sendText.toLocal8Bit());
    serialPort->write(sendByte);
}
void WeiKongPLC::slot_write(QString comname,int startaddr, QList<int> writedata)
{
    if(_name!=comname)return;
    for (int i=startaddr;i<startaddr+writedata.size();i++)
    {
        _write_data[i]=writedata[i-startaddr];
    }
    _is_write_data_change=true;
}

void WeiKongPLC::slot_read(QString comname)
{
    if(_name!=comname)return;
    readWord(_read_startaddr,_read_size);
}
void WeiKongPLC::zeroQList(QList<int> *list, int size)
{
    (*list).clear();
    for (int i=0;i<size;i++)
    {
        (*list)<<0;
    }
}
void WeiKongPLC::slot_read_write()
{
    if(!_isAnswer)
    {
        disconnectWeiKongPLC();
        initWeiKongPLC();
        readWord(_read_startaddr,1);
        return;
    }

    if(_is_read_TriggerSignal)
    {
        readWord(_read_startaddr,_read_size);
    }
    _p_write_timer->start(int(_read_space/2));

}
void WeiKongPLC::slot_timeto_write()
{

    //if(_is_write_data_change)
    {
        _isWrite=true;
        writeWord(_write_startaddr,_write_data);
        _is_write_data_change=false;
        zeroQList(&_write_data,_write_size);
    }
    _p_write_timer->stop();
}
void WeiKongPLC::slot_com_start()
{
    _p_read_write_timer->start(int(_read_space));
}

void WeiKongPLC::slot_com_stop()
{
    _p_read_write_timer->stop();

}

//bytearray转化为int数组
QList<int> WeiKongPLC::transToIntList(QByteArray buf)
{
    QList<int> intdata=QList<int>();
    for (int i=0;i<buf.size()/2;i++)
    {
        QByteArray array;
        array.resize(2);
        array[0]=buf[i*2];
        array[1]=buf[i*2+1];
        bool ok;
        //QByteArray hex=QByteArray::fromHex( array);
        int data=array.toHex().toInt(&ok,16);
        intdata<<data;
    }
    return intdata;
}


//请求帧格式：从机地址+0x03+寄存器起始地址+寄存器数量+CRC检验
//从机地址1个字节取值1~247，由D8121设定
// 0x03（命令码） 1个字节读寄存器
// 寄存器起始地址2个字节高位在前，低位在后，见寄存器编址
// 寄存器数量2个字节高位在前，低位在后（N）
// CRC校验2个字节高位在前，低位在后

//响应帧格式：从机地址+0x03+字节数+寄存器值+CRC检验
//从机地址1个字节取值1~247，由D8121设定
//0x03（命令码） 1个字节读寄存器
//字节数1个字节值：N*2
//寄存器值N*2个字节
//每两字节表示一个寄存器值，高位在前低位在后。寄存器地址小的排在前面
//CRC校验2个字节高位在前，低位在后
void WeiKongPLC::readWord(int startAddr,int num)
{

    //读D位地址
    QString sendstr=QString("%1").arg(_serverID,2,16,QLatin1Char('0'));
    //命令
    sendstr+="03";
    //起始地址
    sendstr+=QString("%1").arg(startAddr,4,16,QLatin1Char('0'));
    //数据长度
    sendstr+=QString("%1").arg(num,4,16,QLatin1Char('0'));
    //计算校验和
    QString strall=sendstr + crc16(sendstr);
    sendHex(strall);
}
//写入字
//请求帧格式：从机地址+0x10+寄存器起始地址+寄存器数量+字节数+寄存器值+CRC检验。
//1 从机地址1个字节取值1~247，由D8121设定
//2 0x10（命令码） 1个字节写多个寄存器
//3 寄存器起始地址2个字节高位在前，低位在后，见寄存器编址
//4 寄存器数量2个字节高位在前，低位在后。N，最大为120
//5 字节数1个字节值：N*2
//6 寄存器值N*2（N*4）
//7 CRC校验2个字节高位在前，低位在后

//响应帧格式：从机地址+0x10+寄存器起始地址+寄存器数量+CRC检验
//从机地址1个字节取值1~247，由D8121设定
//2 0x10（命令码） 1个字节写多个寄存器
//3 寄存器起始地址2个字节高位在前，低位在后，见寄存器编址
//4 寄存器数量2个字节高位在前，低位在后。N，最大为120
//5 CRC校验2个字节高位在前，低位在后

void WeiKongPLC::writeWord(int startAddr,QList<int> writedata)
{
    //读D位地址
    QString sendstr=QString("%1").arg(_serverID,2,16,QLatin1Char('0'));
    //命令
    sendstr+="10";
    //起始地址
    sendstr+=QString("%1").arg(startAddr,4,16,QLatin1Char('0'));
    //数据长度
    sendstr+=QString("%1").arg(writedata.size(),4,16,QLatin1Char('0'));
    //字节数
    sendstr+=QString("%1").arg(writedata.size()*2,2,16,QLatin1Char('0'));
    //数据
    for (int i=0;i<writedata.size();i++)
    {
        QString ss=QString("%1").arg(writedata[i],4,16,QLatin1Char('0'));
        sendstr+=ss;
    }
    //计算校验和
    QString strall=sendstr + crc16(sendstr);
    sendHex(strall);
}
//校验和计算
QString WeiKongPLC::crc16(QString linkstring)
{
    QByteArray bytedata;
    bytedata=QByteArray::fromHex(linkstring.toLocal8Bit());
    unsigned int crc = 0xFFFF;
    for (int pos = 0; pos < bytedata.size(); pos++)
    {
        crc ^= (unsigned int)bytedata[pos]; // XOR byte into least sig. byte of crc
        for (int i = 8; i != 0; i--)   // Loop over each bit
        {
            if ((crc & 0x0001) != 0)   // If the LSB is set
            {
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else // Else LSB is not set
            {
                crc >>= 1;    // Just shift right
            }
        }
    }
    //高低字节转换
    //crc = ((crc & 0x00ff) << 8) | ((crc & 0xff00) >> 8);
    QString back=QString("%1").arg(crc,4,16,QLatin1Char('0'));
    return back;
}




