#ifndef SERIALRELAY_H
#define SERIALRELAY_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include<QTimer>
#include<QMap>

class SerialRelay : public QObject
{
    Q_OBJECT
public:
    explicit SerialRelay(QObject *parent = nullptr);
    ~SerialRelay();
    QSerialPort *serialPort=nullptr;//创建串口变量

    QString _name="";
    QString _portName;
    QString _parity;
    int _baudRate;
    int _databit;
    int _stopbit;

    QTimer *_p_timer_rev=nullptr;//串口数据延时读计时器
    QTimer *_p_read_write_timer=nullptr;//循环读计时器

    int _write_startaddr,_write_size;//写入起始位置
    QList<int> _write_data;//本次写入得数据

    bool _is_read_TriggerSignal=false;//是否循环写入
    int _read_space=100;//读取间隔
    bool _isConnected=false;

    //初始化参数
    void initPar(QString comname, QString portName, QString parity ,int baud,int databits,int stopbit,int readsignal,int readspace,QMap<QString,int> addrInfo);
    //初始化通信
    int8_t initSerialRelay();
    //断开链接
    bool disconnectSerialRelay();
    //发送数据
    void sendHex(QString sendText);
    //计算校验和
    QString computeSum(QString linkstring);
    void writeRegister(QList<int> writedata);
    void zeroQList(QList<int> *list, int size);
signals:
    void signal_com_state(QString conname,bool state);
public slots:
    void slot_startTimer();
    void slot_readyRead();
    void slot_readError();
    void slot_write(QString comname,int startaddr, QList<int> writedata);
    void slot_read_write();
    void slot_com_start();
    void slot_com_stop();


};

#endif // SERIALRELAY_H
