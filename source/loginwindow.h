#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QTcpServer>
#include <QTcpSocket>



namespace Ui {
class loginwindow;
}

class loginwindow : public QDialog
{
    Q_OBJECT

public:
    explicit loginwindow(QWidget *parent = nullptr);
    ~loginwindow();

    //QTcpSocket *tcpsocket;

    static const int data_length = 144;//登录或注册时数据报的长度
    const QString IP = "106.12.116.24";
    const int PORT = 3000;

    QTcpSocket* tcpsocket;
    QString errormessage;
    //QString sessionID;




    bool login_to_server();
    bool register_to_server();

//signals:
    /*登录成功，送以下数据到mainwindow中*/
    void send_mainwindow();



public slots:
    void login_click();//登录
    void register_click();//跳转到注册页面

private:
    Ui::loginwindow *ui;
};

#endif // LOGINWINDOW_H
