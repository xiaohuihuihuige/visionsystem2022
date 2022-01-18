#ifndef MCNET_H
#define MCNET_H

#include <QObject>

#include <QTcpSocket>
#include <QUrl>
#include<QByteArray>
#include<QTimer>

class MCNet : public QObject
{
    Q_OBJECT
public:
    explicit MCNet(QObject *parent = nullptr);
    ~MCNet();

    QTcpSocket *tcpClient;

    QString _name="";
    QString _ipPort;
    int _serverID=0;

    QTimer *_p_read_write_timer=nullptr;//循环读计时器
    QTimer *_p_write_timer=nullptr;//读后写计时器计时器

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



    void initPar(QString comname,QString ipport,int serverID, int readspace, bool readsignal,QMap<QString,int> addrInfo);
    int8_t initMCNet();
    bool disconnectMCNet();

    QList<int> replyToListInt(QByteArray readByte);
    bool writeRegister(int startAddr, QList<int> dataWrite);
    bool readRegister(int addr, int num);
    QList<int> transToIntList(QByteArray buf);
    void zeroQList(QList<int> *list, int size);
    QList<int> compareReaddata(QList<int> data1, QList<int> data2);

    QString addrNumStr(int startAddr, int num);
    void sendHex(QString sendText);
signals:
    void signal_readFinish(QList<int> readdata,QString conname);
    void signal_com_state(QString conname,bool state);

public slots:
    void slot_readError(QAbstractSocket::SocketError);
    void slot_readyRead();
    void slot_write(QString comname,int startaddr, QList<int> writedata);
    void slot_read(QString comname);
    void slot_read_write();
    void slot_timeto_write();
    void slot_com_start();
    void slot_com_stop();

};

#endif // MCNET_H
