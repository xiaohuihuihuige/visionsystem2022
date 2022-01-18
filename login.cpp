#include "login.h"
#include "ui_login.h"
#include"QMessageBox"
#include"mainwindow.h"
#include"general/configfileoperate.h"

Login::Login(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Login)
{
    ui->setupUi(this);
    setWindowFlag(Qt::Window);
    setWindowModality(Qt::WindowModal);
    m_ExePath=qApp->applicationDirPath();
    m_ConfigPath=m_ExePath+"\\Config.ini";
    ConfigFileOperate tconfig(m_ConfigPath);
    m_Password=tconfig.readKeyValue(QString("Password"));
}

Login::~Login()
{
    delete ui;
}

void Login::on_pbtnCertain_clicked()
{
    QString str=ui->ledtPassword->text();
    if(str==m_Password)
    {
        emit signal_loginSuccess();
        close();
    }
    else
    {
        QMessageBox::warning(this,QString::fromLocal8Bit("warning"),QString::fromLocal8Bit("Error Password"));
        return;
    }

}


void Login::on_pbtnEditPassword_clicked()
{

     QString str=ui->ledtPassword->text();
     if(str.indexOf('#')==-1)
     {
         QMessageBox::warning(this,QString::fromLocal8Bit("警告"),
                              QString::fromLocal8Bit("输入错误"));
         return;
     }
     QStringList list=str.split('#');
     if(m_Password!=list[0])
     {
         QMessageBox::warning(this,QString::fromLocal8Bit("警告"),
                              QString::fromLocal8Bit("输入原密码错误"));
         return;
     }
     m_Password=list[1];
     writePassword();
}
void Login::writePassword()
{
    ConfigFileOperate tconfig(m_ConfigPath);
    tconfig.setKeyValue(QString("Password"),m_Password);
}
