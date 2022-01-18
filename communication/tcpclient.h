#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>

#include <QTcpSocket>
#include <QUrl>
#include<QByteArray>
#include<QTimer>

class TcpClient : public QObject
{
    Q_OBJECT
public:
    explicit TcpClient(QObject *parent = nullptr);

    ~TcpClient();

    QString m_name="";
    QTcpSocket * tcpClient;
    QString strIPPort;
    QTimer *_p_reconnect_timer;

    int8_t initTcpClient(QString ipport,QString comname="");
    bool disconnectTcpClient();
    void sendStr(QString sendText);
    void sendHex(QString sendText);
    void sendByte(QByteArray sendByte);

signals:
    void signal_readStrFinish(QString readstr,QString conname);
    void signal_readHexFinish(QByteArray readstr,QString conname);
    void signal_state(bool state);

public slots:
    void slot_readError(QAbstractSocket::SocketError);
    void slot_readyRead();
};

#endif // TCPCLIENT_H
