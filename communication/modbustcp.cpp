#include "modbustcp.h"


ModbusTcp::ModbusTcp(QObject *parent) : QObject(parent)
{
    modbusDevice = new QModbusTcpClient(this);
    connect(modbusDevice, &QModbusClient::errorOccurred,this ,&ModbusTcp::slot_errorOccurred);
    connect(modbusDevice, &QModbusClient::stateChanged,this,&ModbusTcp::slot_stateChange);

    _p_read_write_timer=new QTimer(this);
    connect(_p_read_write_timer, &QTimer::timeout, this, &ModbusTcp::slot_read_write);

    _p_write_timer=new QTimer(this);
    connect(_p_write_timer, &QTimer::timeout, this, &ModbusTcp::slot_timeto_write);
}
ModbusTcp::~ModbusTcp()
{
    disconnectModbustcp();
    if(modbusDevice!=nullptr)
    {
        delete modbusDevice;
        modbusDevice=nullptr;/////
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
void ModbusTcp::initPar(QString comname, QString ipPort,int server,int readspace, bool readsignal,QMap<QString,int> addrInfo)
{
    _name=comname;
    _ipport=ipPort;
    _serverAddress=server;
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

//初始化modbustcp
int8_t ModbusTcp::initModbusTcp()
{
    QString errInfoString=QString("");
    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        const QUrl url = QUrl::fromUserInput(_ipport);
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkPortParameter, url.port());
        modbusDevice->setConnectionParameter(QModbusDevice::NetworkAddressParameter, url.host());
        modbusDevice->setTimeout(100);
        if (!modbusDevice->connectDevice())
        {
            errInfoString += QString("Connect failed: ") + modbusDevice->errorString();
            return 2;
        }
    }
    else
    {
        modbusDevice->disconnectDevice();
    }
    return 0;
}
//断开modbustcp
void ModbusTcp::disconnectModbustcp()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
}

//接收plc发送的消息
void ModbusTcp::readReady()
{
    QModbusDataUnit readData=QModbusDataUnit();
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
    {
        disconnectModbustcp();
        return;
    }
    if (reply->error() == QModbusDevice::NoError)
    {
        //有应答，
        _isAnswer=true;
        emit signal_com_state(_name,_isAnswer);
        //解析收到的数据
        readData = reply->result();
        //判断是否为写数据的应答，是的话，return
        if(_isWrite)
        {
            _isWrite=false;
            return;
        }

        QList<int> datalist=QList<int>();
        for (uint i = 0; i < readData.valueCount(); i++)
        {
            datalist<<readData.value(i);
        }
        QList<int> compData=compareReaddata(datalist,_read_data);
        //QList<int> compData=datalist;
        _read_data=datalist;
        emit signal_readFinish(compData,_name);

    }
    else
    {
        QModbusDevice::Error err=reply->error() ;
        disconnectModbustcp();
    }
    reply->deleteLater();
}

//读plc命令发送
int ModbusTcp::readRequest(int startAddr,int readCount)
{
    QString errInfoString = QString("");
    const auto type =static_cast<QModbusDataUnit::RegisterType> (QModbusDataUnit::RegisterType::HoldingRegisters);
    int startAddress = startAddr;
    int numberOfEntries = readCount;
    QModbusDataUnit dataUnit= QModbusDataUnit(type, startAddress, numberOfEntries);
    if (auto *reply =sendRequest(2,dataUnit) )
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusTcp::readReady);
        else
            delete reply; // broadcast replies return immediately
    }
    else
    {
        errInfoString += QString("Read error: ") + modbusDevice->errorString();
        return 1;
    }
    return  0;
}
//sendtype=1,发送写入请求，=2读取请求
QModbusReply *ModbusTcp::sendRequest(int sendtype,QModbusDataUnit dataUnit)
{
    _isAnswer=false;
    emit signal_com_state(_name,_isAnswer);
    QModbusReply *reply;
    switch (sendtype)
    {
    case 1:
        reply=modbusDevice->sendWriteRequest(dataUnit, _serverAddress);
        break;
    case 2:
        reply=modbusDevice->sendReadRequest(dataUnit, _serverAddress);
        break;
    default:
        break;
    }
    return reply;

}
//数组置零
void ModbusTcp::zeroQList(QList<int> *list, int size)
{
    (*list).clear();
    for (int i=0;i<size;i++)
    {
        (*list)<<0;
    }
}
//对比两次读写的数据是否相同
QList<int> ModbusTcp::compareReaddata(QList<int> data1, QList<int> data2)
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
//写plc命令发送
int ModbusTcp::writeRequest(int startAddr,QList<int> writeData)
{
    const auto type =static_cast<QModbusDataUnit::RegisterType> (QModbusDataUnit::RegisterType::HoldingRegisters);
    int startAddress = startAddr;
    quint16 numberOfEntries = quint16(writeData.size());
    QModbusDataUnit dataUnit= QModbusDataUnit(type, startAddress, numberOfEntries);
    for (int i=0;i<writeData.size();i++)
    {
        dataUnit.setValue(i,quint16(writeData[i]));
    }
    QString errInfoString = QString("");
    if (auto *reply =sendRequest(1,dataUnit))
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusTcp::readReady);
        else
            delete reply; // broadcast replies return immediately
        /*if (!reply->isFinished())
        {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() != QModbusDevice::NoError)
                {
                    QString errInfoString = QString("Write response error: %1 (code: 0x%2)").
                            arg(reply->errorString()).arg(reply->error());
                }
                reply->deleteLater();
            });
            return 1;
        }
        else
        {
            reply->deleteLater();
            return 0;
        }*/
        return 0;
    }
    else
    {
        errInfoString += QString("Write error: ") + modbusDevice->errorString();
        return 2;
    }
}

void ModbusTcp::slot_errorOccurred(QModbusDevice::Error err)
{
    //异常发生时发送异常
    //QString errModbusString=modbusDevice->errorString();
    _isConnected=false;
    //emit signal_com_state(_name,false);
    //emit signal_errStringSend(errModbusString);
}


void ModbusTcp::slot_stateChange(QModbusClient::State state)
{
    //状态改变时发送状态
    QModbusClient::State modbusState=modbusDevice->state();
    if(modbusState==QModbusClient::ConnectedState)
    {
        _isConnected=true;
    }
    else
    {
        _isConnected=false;
    }
}
//写入数据变化
void ModbusTcp::slot_write(QString comname,int startaddr, QList<int> writedata)
{
    if(_name!=comname)return;
    for (int i=startaddr;i<startaddr+writedata.size();i++)
    {
        _write_data[i]=writedata[i-startaddr];
    }
    _is_write_data_change=true;
}

//读取plc数据
void ModbusTcp::slot_read(QString comname)
{
    if(_name!=comname)return;
    readRequest(_read_startaddr,_read_size);
}

//定时器触发，间隔读写
void ModbusTcp::slot_read_write()
{
    if(!_isAnswer)
    {
        disconnectModbustcp();
        initModbusTcp();
        _isAnswer=true;
        return;
    }
    if(_is_read_TriggerSignal)
    {
        readRequest(_read_startaddr,_read_size);
    }
    _p_write_timer->start(_read_space/2);
}
//读完数据后，延时0.5倍写入
void ModbusTcp::slot_timeto_write()
{
    //if(_is_write_data_change)
    {
        writeRequest(_write_startaddr,_write_data);
        _isWrite=true;
        _is_write_data_change=false;
        zeroQList(&_write_data,_write_size);
    }
    _p_write_timer->stop();
}
//槽启动读写定时器
void ModbusTcp::slot_com_start()
{
    _p_read_write_timer->start(_read_space);
}
//槽停止读写定时器
void ModbusTcp::slot_com_stop()
{
    _p_read_write_timer->stop();
}












