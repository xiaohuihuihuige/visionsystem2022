#include "fxprotocol.h"
//专用协议，校验和，断电重启
FXProtocol::FXProtocol(QObject *parent) : QObject(parent)
{
    serialPort=new QSerialPort(this);
    connect(serialPort,&QSerialPort::readyRead,this,&FXProtocol::slot_startTimer);
    connect(serialPort, &QSerialPort::errorOccurred, this, &FXProtocol::slot_readError);

    _p_read_write_timer=new QTimer(this);
    connect(_p_read_write_timer, &QTimer::timeout, this, &FXProtocol::slot_read_write);

    _p_write_timer=new QTimer(this);
    connect(_p_write_timer, &QTimer::timeout, this, &FXProtocol::slot_timeto_write);

    _p_timer_rev=new QTimer(this);
    connect(_p_timer_rev, &QTimer::timeout, this, &FXProtocol::slot_readyRead);

}
FXProtocol::~FXProtocol()
{
    disconnectFXProtocol();
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
void FXProtocol::initPar(QString comname,QString portName, QString parity, int baud, int databits, int stopbit,int readspace,bool readsignal,QMap<QString,int> addrInfo)
{

    _isAnswer=false;
    _name=comname;
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
int8_t FXProtocol::initFXProtocol()
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
bool FXProtocol::disconnectFXProtocol()
{
    _isConnected=false;
    if(serialPort==nullptr)return true;
    if(serialPort->isOpen())
    {
        serialPort->close();
    }
    return 0;
}
void FXProtocol::slot_startTimer()
{
    _p_timer_rev->start(20);
}
//接收数据并转化为int数组
void FXProtocol::slot_readyRead()
{
    _p_timer_rev->stop();
    QByteArray readByte = serialPort->readAll();
    if(!readByte.isEmpty())
    {
        if(readByte.size()<5)return;
        if(readByte.at(0)==0x06)
        {
        }
        _isAnswer=true;
        emit signal_com_state(_name,_isAnswer);
        if(_isWrite)
        {
            _isWrite=false;
            return;
        }
        if(readByte.at(0)==0x02 && readByte.at(readByte.size()-3)==0x03)
        {
            QByteArray buf=QByteArray::fromHex(readByte.mid(5,readByte.size()-8));
            QList<int> data=transToIntList(buf);
            QList<int> compData=compareReaddata(data,_read_data);
            _read_data=data;
            emit signal_readFinish(compData,_name);
        }
    }
}
QList<int> FXProtocol::compareReaddata(QList<int> data1,QList<int> data2)
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

void FXProtocol::slot_readError()
{
    _isConnected=false;
    //emit signal_com_state(_name,false);
}
//串口发送十六进制
void FXProtocol::sendHex(QString sendText)
{
    _isAnswer=false;
    emit signal_com_state(_name,_isAnswer);
    QByteArray sendByte;
    sendByte=QByteArray::fromHex(sendText.toLocal8Bit());
    serialPort->write(sendByte);
}
void FXProtocol::slot_write(QString comname,int startaddr, QList<int> writedata)
{
    if(_name!=comname)return;
    for (int i=startaddr;i<startaddr+writedata.size();i++)
    {
        _write_data[i]=writedata[i-startaddr];
    }
    _is_write_data_change=true;
}

void FXProtocol::slot_read(QString comname)
{
    if(_name!=comname)return;
    readWord(_read_startaddr,_read_size);
}
void FXProtocol::zeroQList(QList<int> *list, int size)
{
    (*list).clear();
    for (int i=0;i<size;i++)
    {
        (*list)<<0;
    }
}
void FXProtocol::slot_read_write()
{
    if(!_isAnswer)
    {
        disconnectFXProtocol();
        initFXProtocol();
        readWord(1000,1);
        return;
    }

    if(_is_read_TriggerSignal)
    {
        readWord(_read_startaddr,_read_size);
    }
    _p_write_timer->start(_read_space/2);
}
void FXProtocol::slot_timeto_write()
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

void FXProtocol::slot_com_start()
{
    _p_read_write_timer->start(_read_space);
}

void FXProtocol::slot_com_stop()
{
    _p_read_write_timer->stop();

}

//bytearray转化为int数组
QList<int> FXProtocol::transToIntList(QByteArray buf)
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
QString FXProtocol::addrNumStr(int startAddr,int num)
{
    QString sendstr="";
    //起始地址
    QByteArray array0,array1;
    array0=QString("%1").arg(startAddr,4,10,QLatin1Char('0')).toLatin1();
    array1.resize(1);
    array1[0]=array0.at(0);
    sendstr+=array1.toHex();
    array1[0]=array0.at(1);
    sendstr+=array1.toHex();
    array1[0]=array0.at(2);
    sendstr+=array1.toHex();
    array1[0]=array0.at(3);
    sendstr+=array1.toHex();
    //数据长度
    array0=QString("%1").arg(num,2,16,QLatin1Char('0')).toUpper().toLatin1();
    array1.resize(1);
    array1[0]=array0.at(0);
    sendstr+=array1.toHex();
    array1[0]=array0.at(1);
    sendstr+=array1.toHex();
    return sendstr;
}
//读取位状态
void FXProtocol::readBit(int startAddr,int num)
{
    //读M位地址
    QString sendstr="05303046464252304d";
    //起始地址+数据长度
    sendstr+=addrNumStr(startAddr,num);
    //计算校验和
    QString strall=sendstr + computeSum(sendstr.mid(2));
    sendHex(strall);
}
//写入位状态
void FXProtocol::writeBit(int startAddr,QList<int> writedata)
{
    //M位地址
    QString sendstr="05303046464257304d";
    //起始地址+数据长度
    sendstr+=addrNumStr(startAddr,writedata.size());
    //数据
    for (int i=0;i<writedata.size();i++)
    {
        sendstr+=QString("%1").arg(writedata[i]+48,2,16,QLatin1Char('0'));
    }
    //计算校验和
    QString strall=sendstr + computeSum(sendstr.mid(2));
    sendHex(strall);
}
//读取字状态
void FXProtocol::readWord(int startAddr,int num)
{

    //读D位地址
    QString sendstr="053030464657523044";
    sendstr+=addrNumStr(startAddr,num);
    //计算校验和
    QString strall=sendstr + computeSum(sendstr.mid(2));
    sendHex(strall);
}
//写入字
void FXProtocol::writeWord(int startAddr,QList<int> writedata)
{
    //M位地址
    QString sendstr="053030464657573044";
    sendstr+=addrNumStr(startAddr,writedata.size());
    //数据
    for (int i=0;i<writedata.size();i++)
    {
        QString ss=QString("%1").arg(writedata[i],4,16,QLatin1Char('0')).toUpper();
        for (int j=0;j<ss.length();j++)
        {
            sendstr+=QString("%1").arg(ss.at(j).toLatin1(),2,16,QLatin1Char('0'));
        }
    }
    //计算校验和
    QString strall=sendstr + computeSum(sendstr.mid(2));
    sendHex(strall);
}
//校验和计算
QString FXProtocol::computeSum(QString linkstring)
{
    QByteArray bytedata;
    bytedata=QByteArray::fromHex(linkstring.toLocal8Bit());
    int sunResult=0;
    for (int i=0;i<bytedata.size();i++)
    {
        sunResult+=(int)bytedata[i];
    }
    QString sumLastTwo=QString("%1").arg(sunResult,50,16,QLatin1Char('0')).right(2).toUpper();
    QString back;
    back=QString("%1").arg(sumLastTwo.at(0).toLatin1(),2,16,QLatin1Char('0'))
            +QString("%1").arg(sumLastTwo.at(1).toLatin1(),2,16,QLatin1Char('0'));
    return back;
}

