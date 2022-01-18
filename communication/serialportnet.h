#ifndef SERIALPORTNET_H
#define SERIALPORTNET_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include<QTimer>

class SerialPortNet : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortNet(QObject *parent = nullptr);
    ~SerialPortNet();
    QTimer *timer_rev;
    QSerialPort *serialPort;//创建串口变量


    static QStringList getAvailableSerial();//获取当前检测到的串口
    int8_t initSerialPort(QString portName,QString checkBit,int baudRate, int dataBit,int stopBit);
    bool disconnectSerial();
    void sendStr(QString sendText);
    void sendHex(QString sendText);
    void sendByte(QByteArray sendByte);
signals:
    void signal_readByteArryFinish(QByteArray readstr);
    void signal_errOccurred();
private slots:
    void slot_readError(QSerialPort::SerialPortError);
    void slot_readyRead();
    void slot_startTimer();


};

#endif // SERIALPORTNET_H
