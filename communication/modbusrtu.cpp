#include "modbusrtu.h"

ModbusRTU::ModbusRTU(QObject *parent) : QObject(parent)
{
    modbusDevice=nullptr;
    _parity_int.clear();
    _parity_int["No"]=0;
    _parity_int["Even"]=2;
    _parity_int["Odd"]=3;
    modbusDevice = new QModbusRtuSerialMaster(this);
    connect(modbusDevice, &QModbusClient::errorOccurred,this ,&ModbusRTU::slot_errorOccurred);
    connect(modbusDevice, &QModbusClient::stateChanged,this,&ModbusRTU::slot_stateChange);

    _p_read_write_timer=new QTimer(this);
    connect(_p_read_write_timer, &QTimer::timeout, this, &ModbusRTU::slot_read_write);

    _p_write_timer=new QTimer(this);
    connect(_p_write_timer, &QTimer::timeout, this, &ModbusRTU::slot_timeto_write);

}
ModbusRTU::~ModbusRTU()
{
    disconnectModbusRTU();
    if(modbusDevice!=nullptr)
    {
        delete modbusDevice;
        modbusDevice=nullptr;
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

void ModbusRTU::initPar(QString comname, QString portName, QString parity ,int baud,int databits,int stopbit,
                        int server, int readspace, bool readsignal,QMap<QString,int> addrInfo)
{

    _name=comname;
    _portName=portName;
    _parity=parity;
    _baudRate=baud;
    _databit=databits;
    _stopbit=stopbit;
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
//初始化modbusrtu
int8_t ModbusRTU::initModbusRTU()
{
    QString errInfoString=QString("");
    if (!modbusDevice)
    {
        errInfoString += QString("Could not create Modbus master.");
        return 1;
    }
    if (modbusDevice->state() != QModbusDevice::ConnectedState)
    {
        modbusDevice->setConnectionParameter(QModbusDevice::SerialPortNameParameter,
            _portName);

        modbusDevice->setConnectionParameter(QModbusDevice::SerialParityParameter,
           _parity_int[_parity]);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,
            _baudRate);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,
            _databit);
        modbusDevice->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,
            _stopbit);
        modbusDevice->setTimeout(500);
        modbusDevice->setNumberOfRetries(2);

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

//断开modbusrtu
void ModbusRTU::disconnectModbusRTU()
{
    if (modbusDevice)
        modbusDevice->disconnectDevice();
}

//接收modbusrtu发送的内容
void ModbusRTU::slot_readReady()
{
    QModbusDataUnit readData=QModbusDataUnit();
    auto reply = qobject_cast<QModbusReply *>(sender());
    if (!reply)
    {
        disconnectModbusRTU();
        return;
    }
    if (reply->error() == QModbusDevice::NoError)
    {
        _isAnswer=true;
        emit signal_com_state(_name,_isAnswer);
        if(_isWrite)
        {
            _isWrite=false;
            return;
        }
        readData = reply->result();
        QList<int> datalist=QList<int>();
        for (uint i = 0; i < readData.valueCount(); i++)
        {
            int data=readData.value(i);
            datalist<<data;
        }
        QList<int> compData=compareReaddata(datalist,_read_data);
        _read_data=datalist;
        emit signal_readFinish(compData,_name);
    }
    else
    {
        disconnectModbusRTU();
    }
    reply->deleteLater();

}

//读PLC字
int ModbusRTU::readRequest(int startAddr,int readCount)
{
    QString errInfoString = QString("");
    const auto type =static_cast<QModbusDataUnit::RegisterType> (QModbusDataUnit::RegisterType::HoldingRegisters);
    int startAddress = startAddr;
    int numberOfEntries = readCount;
    QModbusDataUnit dataUnit= QModbusDataUnit(type, startAddress, numberOfEntries);
    if (auto *reply = sendRequest(2,dataUnit))
    {
        if (!reply->isFinished())
            connect(reply, &QModbusReply::finished, this, &ModbusRTU::slot_readReady);
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
//写PLC字
int ModbusRTU::writeRequest(QModbusDataUnit writeUnit)
{
    QString errInfoString = QString("");
    if (auto *reply = sendRequest(1,writeUnit))
    {
        if (!reply->isFinished())
        {
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &ModbusRTU::slot_readReady);
            else
                delete reply;
            // broadcast replies return immediately
            /*connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::NoError)
                {
                    _isAnswer=true;
                }
                else
                {
                    QString errInfoString = QString("Write response error: %1 (code: 0x%2)").
                            arg(reply->errorString()).arg(reply->error());
                }
                reply->deleteLater();
            });*/
            return 1;
        }
        else
        {
            reply->deleteLater();
            return 0;
        }
    }
    else
    {
        errInfoString += QString("Write error: ") + modbusDevice->errorString();
        return 2;
    }
}
//写PLC字
int ModbusRTU::writeRequest(int startAddr,QList<int> writeData)
{

    const auto type =static_cast<QModbusDataUnit::RegisterType> (QModbusDataUnit::RegisterType::HoldingRegisters);
    int startAddress = startAddr;
    int numberOfEntries = writeData.size();
    QModbusDataUnit dataUnit= QModbusDataUnit(type, startAddress, numberOfEntries);
    for (int i=0;i<writeData.size();i++)
    {
        dataUnit.setValue(i,writeData[i]);
    }
    QString errInfoString = QString("");
    if (auto *reply = sendRequest(1,dataUnit))
    {
        if (!reply->isFinished())
        {
            if (!reply->isFinished())
                connect(reply, &QModbusReply::finished, this, &ModbusRTU::slot_readReady);
            else
                delete reply;
            /*connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() != QModbusDevice::NoError)
                {
                    QString errInfoString = QString("Write response error: %1 (code: 0x%2)").
                            arg(reply->errorString()).arg(reply->error());
                }
                reply->deleteLater();
            });*/

            return 1;
        }
        else
        {
            reply->deleteLater();
            return 0;
        }
    }
    else
    {
        errInfoString += QString("Write error: ") + modbusDevice->errorString();
        return 2;
    }
}

//sendtype=1,发送写入请求，=2读取请求
QModbusReply *ModbusRTU::sendRequest(int sendtype,QModbusDataUnit dataUnit)
{
    _isAnswer=false;
    emit signal_com_state(_name,_isAnswer);
    QModbusReply *reply;
    switch (sendtype)
    {
    case 1:
        _isWrite=true;
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

void ModbusRTU::zeroQList(QList<int> *list, int size)
{
    (*list).clear();
    for (int i=0;i<size;i++)
    {
        (*list)<<0;
    }
}

QList<int> ModbusRTU::compareReaddata(QList<int> data1, QList<int> data2)
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

void ModbusRTU::slot_errorOccurred(QModbusDevice::Error err)
{  
    //异常发生时发送异常
    //QString errModbusString=modbusDevice->errorString();
    _isConnected=false;
    //emit signal_com_state(_name,_isConnected);
    //emit signal_errStringSend(errModbusString);
}


void ModbusRTU::slot_stateChange(QModbusClient::State state)
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
    //emit signal_com_state(_name,_isConnected);

    //emit signal_modbusCurState(modbusState);
}

void ModbusRTU::slot_write(QString comname,int startaddr, QList<int> writedata)
{
    if(_name!=comname)return;
    for (int i=startaddr;i<startaddr+writedata.size();i++)
    {
        _write_data[i]=writedata[i-startaddr];
    }
    _is_write_data_change=true;
}

void ModbusRTU::slot_read(QString comname)
{
    if(_name!=comname)return;
    readRequest(_read_startaddr,_read_size);
}

void ModbusRTU::slot_read_write()
{
    if(!_isAnswer)
    {
        disconnectModbusRTU();
        initModbusRTU();
        readRequest(0,1);
        return;
    }
    if(_is_read_TriggerSignal)
    {
        readRequest(_read_startaddr,_read_size);
    }
    _p_write_timer->start(_read_space/2);
}
void ModbusRTU::slot_timeto_write()
{
    //if(_is_write_data_change)
    {
        _isWrite=true;
        writeRequest(_write_startaddr,_write_data);
        _is_write_data_change=false;
        zeroQList(&_write_data,_write_size);
    }
    _p_write_timer->stop();
}
void ModbusRTU::slot_com_start()
{
    _p_read_write_timer->start(_read_space);
}

void ModbusRTU::slot_com_stop()
{
    _p_read_write_timer->stop();

}

