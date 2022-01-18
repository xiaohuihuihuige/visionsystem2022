#include "mcnet.h"

MCNet::MCNet(QObject *parent) : QObject (parent)
{
    tcpClient=new QTcpSocket(this);
    tcpClient->abort();
    QObject::connect(tcpClient,&QTcpSocket::readyRead,this,&MCNet::slot_readyRead);
    //设置tcp错误报警
    typedef void (QAbstractSocket::*QAbstractSocketErrorSignal)(QAbstractSocket::SocketError);
    QObject::connect(tcpClient, static_cast<QAbstractSocketErrorSignal>(&QTcpSocket::error), this, &MCNet::slot_readError);

    _p_read_write_timer=new QTimer(this);
    connect(_p_read_write_timer, &QTimer::timeout, this, &MCNet::slot_read_write);

    _p_write_timer=new QTimer(this);
    connect(_p_write_timer, &QTimer::timeout, this, &MCNet::slot_timeto_write);
}
MCNet::~MCNet()
{
    disconnectMCNet();
    if(tcpClient!=nullptr)
    {
        delete tcpClient;
        tcpClient=nullptr;
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

void MCNet::initPar(QString comname, QString ipPort,int serverID,int readspace, bool readsignal,QMap<QString,int> addrInfo)
{
    _isAnswer=false;
    _name=comname;
    _ipPort=ipPort;
    _serverID=serverID;
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

int8_t MCNet::initMCNet()
{
    if(tcpClient==nullptr)
        return 1;
    QUrl urlIPPort=QUrl::fromUserInput(_ipPort);
    tcpClient->connectToHost(urlIPPort.host(),urlIPPort.port());
    if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
    {
        return 0;
    }
    return 1;
}

bool MCNet::disconnectMCNet()
{
    _isConnected=false;
    tcpClient->disconnectFromHost();
    if (tcpClient->state() == QAbstractSocket::UnconnectedState || tcpClient->waitForDisconnected(1000))  //已断开连接则进入if{}
    {
        return true;
    }
    return false;
}
QString MCNet::addrNumStr(int startAddr,int num)
{
    QString sendstr="";
    QByteArray array0;

    //起始地址
    array0=QString("%1").arg(startAddr,6,16,QLatin1Char('0')).toLatin1();
    sendstr+=array0.mid(4,2);
    sendstr+=array0.mid(2,2);
    sendstr+=array0.mid(0,2);
    sendstr+="A8";
    //数据长度
    array0=QString("%1").arg(num,4,16,QLatin1Char('0')).toUpper().toLatin1();
    sendstr+=array0.mid(2,2);
    sendstr+=array0.mid(0,2);
    return sendstr;
}
//发送帧内容为：50 00 00 FF FF 03 00 0E 00 10 00 01 14 00 00 58 1B 00 A8 01 00 0C 00
//响应帧内容为：D0 00 00 FF FF 03 00 02 00 00 00
bool MCNet::writeRegister(int startAddr,QList<int> dataWrite)
{
    //写D地址
    QString sendstr="500000FFFF0300";
    QByteArray array0;
    array0=QString("%1").arg(dataWrite.size()*2+12,4,16,QLatin1Char('0')).toUpper().toLatin1();
    sendstr+=array0.mid(2,2);
    sendstr+=array0.mid(0,2);
    sendstr+="100001140000";
    sendstr+=addrNumStr(startAddr,dataWrite.size());
    //数据
    for (int i=0;i<dataWrite.size();i++)
    {
        QString ss=QString("%1").arg(dataWrite[i],4,16,QLatin1Char('0')).toUpper();
        sendstr+=ss.mid(2,2);
        sendstr+=ss.mid(0,2);
    }
    sendHex(sendstr);
    return true;
}
//发送帧内容为：50 00 00 FF FF 03 00 0C 00 10 00 01 04 00 00 58 1B 00 A8 05 00
//响应帧内容为：D0 00 00 FF FF 03 00 0C 00 00 00 0C 00 00 00 00 00 00 00 00 00
bool MCNet::readRegister(int addr,int num)
{
    //读D地址
    QString sendstr="500000FFFF0300";
    sendstr+="0C001000";
    sendstr+="01040000";
    sendstr+=addrNumStr(addr,num);
    sendHex(sendstr);
    return true;
}

void MCNet::slot_readyRead()
{
    QByteArray readByte;
    readByte = tcpClient->readAll();
    if(!readByte.isEmpty())
    {
        if(readByte.size()<12)return;
        _isAnswer=true;
        emit signal_com_state(_name,_isAnswer);
        if(_isWrite)
        {
            _isWrite=false;
            return;
        }
        QByteArray buf=readByte.mid(11,readByte.size()-11);
        QList<int> data=transToIntList(buf);
        QList<int> compData=compareReaddata(data,_read_data);
        _read_data=data;
        emit signal_readFinish(compData,_name);
    }
    readByte.clear();
}

void MCNet::slot_write(QString comname,int startaddr, QList<int> writedata)
{
    if(_name!=comname)return;
    for (int i=startaddr;i<startaddr+writedata.size();i++)
    {
        _write_data[i]=writedata[i-startaddr];
    }
    _is_write_data_change=true;
}

void MCNet::slot_read(QString comname)
{
    if(_name!=comname)return;
    readRegister(_read_startaddr,_read_size);
}

void MCNet::slot_read_write()
{
    if(!_isAnswer)
    {
        disconnectMCNet();
        initMCNet();
        readRegister(0,1);
        _isAnswer=true;
        return;
    }
    if(_is_read_TriggerSignal)
    {
        readRegister(_read_startaddr,_read_size);
    }
    _p_write_timer->start(_read_space/2);
}

void MCNet::slot_timeto_write()
{
    //if(_is_write_data_change)
    {
        _isWrite=true;
        writeRegister(_write_startaddr,_write_data);
        _is_write_data_change=false;
        zeroQList(&_write_data,_write_size);
    }
    _p_write_timer->stop();
}
void MCNet::slot_com_start()
{
    _p_read_write_timer->start(_read_space);
}

void MCNet::slot_com_stop()
{
    _p_read_write_timer->stop();

}
QList<int> MCNet::transToIntList(QByteArray buf)
{
    QList<int> intdata=QList<int>();
    for (int i=0;i<buf.size()/2;i++)
    {
        QByteArray array;
        array.resize(2);
        array[1]=buf[i*2];
        array[0]=buf[i*2+1];
        bool ok;
        int data=array.toHex().toInt(&ok,16);
        intdata<<data;
    }
    return intdata;
}
//tcp发送十六进制
void MCNet::sendHex(QString sendText)
{
    _isAnswer=false;
    emit signal_com_state(_name,_isAnswer);
    QByteArray sendByte=QByteArray::fromHex(sendText.toStdString().data());
    tcpClient->write(sendByte);
}
void MCNet::zeroQList(QList<int> *list, int size)
{
    (*list).clear();
    for (int i=0;i<size;i++)
    {
        (*list)<<0;
    }
}

QList<int> MCNet::compareReaddata(QList<int> data1, QList<int> data2)
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
//接收tcp错误并发送信号
void MCNet::slot_readError(QTcpSocket::SocketError)
{
    _isConnected=false;
    //emit signal_com_state(_name,false);
}
