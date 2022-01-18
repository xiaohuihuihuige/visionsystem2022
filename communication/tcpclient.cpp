#include "tcpclient.h"


TcpClient::TcpClient(QObject *parent) : QObject(parent),
    tcpClient(nullptr)
{
    tcpClient=new QTcpSocket(this);
    tcpClient->abort();
    QObject::connect(tcpClient,&QTcpSocket::readyRead,this,&TcpClient::slot_readyRead);
    //设置tcp错误报警
    typedef void (QAbstractSocket::*QAbstractSocketErrorSignal)(QAbstractSocket::SocketError);
    QObject::connect(tcpClient, static_cast<QAbstractSocketErrorSignal>(&QTcpSocket::error), this, &TcpClient::slot_readError);
}
TcpClient::~TcpClient()
{
    if(tcpClient->state()==QAbstractSocket::ConnectedState)
    {
        disconnectTcpClient();
    }
}
//初始化tcp通信
int8_t TcpClient::initTcpClient(QString ipport,QString comname)
{
    m_name=comname;
    strIPPort=ipport;
    if(tcpClient==nullptr)
        return 1;
    QUrl urlIPPort=QUrl::fromUserInput(strIPPort);
    tcpClient->connectToHost(urlIPPort.host(),urlIPPort.port());
    if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
    {
        return 0;
    }
    return 1;
}
//断开tcp通信
bool TcpClient::disconnectTcpClient()
{
    tcpClient->disconnectFromHost();
    if (tcpClient->state() == QAbstractSocket::UnconnectedState || tcpClient->waitForDisconnected(1000))  //已断开连接则进入if{}
    {
        return true;
    }
    return false;
}
//接收tcp消息
void TcpClient::slot_readyRead()
{
    QByteArray byteAll;
    byteAll=tcpClient->readAll();
    QByteArray readByte=byteAll.toHex();
    emit signal_readHexFinish(readByte,m_name);
    QString readString= byteAll;
    emit signal_readStrFinish(readString,m_name);
}
//tcp发送字符
void TcpClient::sendStr(QString sendText)
{
    tcpClient->write(sendText.toLatin1()); //qt5去除了.toAscii()
}
//tcp发送十六进制
void TcpClient::sendHex(QString sendText)
{
    QByteArray sendByte;
    sendByte.fromHex(sendText.toLocal8Bit());
    tcpClient->write(sendByte);
}
//tcp发送字节
void TcpClient::sendByte(QByteArray sendByte)
{
    tcpClient->write(sendByte);
}

//接收tcp错误并发送信号
void TcpClient::slot_readError(QTcpSocket::SocketError)
{
    emit signal_state(false);
    _p_reconnect_timer=new QTimer();
    _p_reconnect_timer->start(300);
    connect(_p_reconnect_timer,&QTimer::timeout,[=]()
    {
        tcpClient->disconnectFromHost();
        int ret=this->initTcpClient(this->strIPPort,this->m_name);
        if(ret==0)
        {
            emit signal_state(true);
            this->_p_reconnect_timer->stop();
            delete this->_p_reconnect_timer;
            this->_p_reconnect_timer=nullptr;
        }
    });
}
