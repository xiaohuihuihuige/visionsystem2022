#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>

namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    ~Login();
    QString m_Password;
    QString m_ConfigPath,m_ExePath;
    void writePassword();
private slots:
    void on_pbtnCertain_clicked();

    void on_pbtnEditPassword_clicked();

signals:
   void signal_loginSuccess();
private:
    Ui::Login *ui;
};

#endif // LOGIN_H
