#ifndef MODBUSRTU_H
#define MODBUSRTU_H

#include <QObject>
#include <QModbusDataUnit>
#include <QtSerialBus/qtserialbusglobal.h>
#include <QSerialPort>
#include<QVariant>
#include<QModbusDevice>
#include<QModbusClient>
#include<QTimer>
#include <QModbusRtuSerialMaster>


class QModbusClient;
class QModbusReply;

class ModbusRTU : public QObject
{
    Q_OBJECT
public:
    explicit ModbusRTU(QObject *parent = nullptr);
~ModbusRTU();

    QModbusClient *modbusDevice;
    int _serverAddress;
    QString _name="";
    QString _portName;
    QString _parity;
    int _baudRate;
    int _databit;
    int _stopbit;

    QMap<QString,int> _parity_int;

    QTimer *_p_read_write_timer=nullptr;//循环读计时器
    QTimer *_p_write_timer=nullptr;//读后写计时器计时器

    int _read_startaddr,_read_size;//读取起始位置，读取长度
    QList<int> _read_data;//上次读取数据，

    int _write_startaddr,_write_size;//写入起始位置
    QList<int> _write_data;//本次写入得数据

    double _read_space=100;//读取间隔
    bool _is_read_TriggerSignal;//是否读取PLC信号
    bool _is_write_data_change;//写入数据变化

    bool _isConnected=false;
    bool _isAnswer=true;
    bool _isWrite=false;


    void initPar(QString comname, QString portName, QString parity ,int baud,int databits,int stopbit,
                 int server, int readspace, bool readsignal,QMap<QString,int> addrInfo);

    int8_t initModbusRTU();
    //QModbusDataUnit readRequest(int readType,int startAddr,int readCount);
    int writeRequest(QModbusDataUnit writeUnit);
    void disconnectModbusRTU();
    int writeRequest(int startAddr, QList<int> writeData);
    int readRequest(int startAddr, int readCount);
    QModbusReply *sendRequest(int sendtype, QModbusDataUnit dataUnit);
    void zeroQList(QList<int> *list, int size);
    QList<int> compareReaddata(QList<int> data1, QList<int> data2);
signals:
    void signal_readFinish(QList<int> dataList,QString conname);
    void signal_errStringSend(QString errstr);
    void signal_modbusCurState(QModbusClient::State state);
    void signal_com_state(QString conname,bool state);

public slots:
    void slot_readReady();
    void slot_errorOccurred(QModbusDevice::Error err);
    void slot_stateChange(QModbusClient::State state);
    void slot_write(QString comname,int startaddr, QList<int> writedata);
    void slot_read(QString comname);
    void slot_read_write();
    void slot_timeto_write();
    void slot_com_start();
    void slot_com_stop();

};

#endif // MODBUSRTU_H
