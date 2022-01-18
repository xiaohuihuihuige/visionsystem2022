#ifndef TAIDAPLC_H
#define TAIDAPLC_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include<QTimer>
#include<QMap>
class Taidaplc : public QObject
{
    Q_OBJECT
public:
    explicit Taidaplc(QObject *parent = nullptr);


    ~Taidaplc();
    QSerialPort *serialPort=nullptr;//创建串口变量
    QString _name="";
    QString _portName;
    QString _parity;
    int _baudRate;
    int _databit;
    int _stopbit;

    QTimer *_p_read_write_timer=nullptr;//循环读计时器
    QTimer *_p_write_timer=nullptr;//读后写计时器计时器
    QTimer *_p_timer_rev=nullptr;

    int _read_startaddr,_read_size;//读取起始位置，读取长度
    QList<int> _read_data;//上次读取数据，

    int _write_startaddr,_write_size;//写入起始位置
    QList<int> _write_data;//本次写入得数据

    double _read_space=100;//读取间隔
    bool _is_read_TriggerSignal;//是否读取PLC信号
    bool _is_write_data_change;//写入数据是否变化

    bool _isConnected=false;
    bool _isAnswer=true;
    bool _isWrite=false;


    //初始化参数
    void initPar(QString comname,QString portName, QString parity, int baud,
                      int databits, int stopbit,int readspace,bool readsignal,QMap<QString,int> addrInfo);
    int8_t initTaidaplc();
    bool disconnectTaidaplc();
    void sendStr(QString sendText);

    QString computeFCS(QString linkstring);

    void writeWord(int startAddr, QList<int> dataWrite);
    void readWord(int startAddr, int count);
    QList<int> transToIntList(QByteArray buf);
    void zeroQList(QList<int> *list, int size);
    QList<int> compareReaddata(QList<int> data1, QList<int> data2);
signals:
    void signal_readFinish(QList<int> readdata,QString comname);
    void signal_com_state(QString conname,bool state);
public slots:
    void slot_startTimer();
    void slot_readyRead();
    void slot_readError();
    void slot_write(QString comname,int startaddr, QList<int> writedata);
    void slot_read(QString comname);
    void slot_read_write();
    void slot_timeto_write();

    void slot_com_start();
    void slot_com_stop();
};

#endif // TAIDAPLC_H
