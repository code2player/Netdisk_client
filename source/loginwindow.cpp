#include "loginwindow.h"
#include "ui_loginwindow.h"


#include<iostream>
#include<QNetworkProxy>
using namespace std;
#include<QMessageBox>   //弹窗功能的实现

QString sessionID;

loginwindow::loginwindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::loginwindow)
{
    ui->setupUi(this);
    setWindowTitle("登录");
    tcpsocket = new QTcpSocket;

    connect(ui->pushButton,SIGNAL(clicked()),this,SLOT(login_click()));
    connect(ui->pushButton_2,SIGNAL(clicked()),this,SLOT(register_click()));



}

loginwindow::~loginwindow()
{
    delete ui;
}

/*socket连接*/
bool loginwindow::login_to_server()
{
    //return true;
    QString id = ui->lineEdit->text();
    QString password = ui->lineEdit_2->text();

    //QTcpSocket* tcpsocket = new QTcpSocket(this);

    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);
/*
    bool connected = tcpsocket->waitForConnected();
    if(connected == false)
        return false;*/
    //qSleep(1000);
    char send_buf[data_length];
    memset(send_buf, 0, data_length);
    memcpy(send_buf, "login", 5);


    std::string str1 = id.toStdString();
    const char* ch1 = str1.c_str();
    std::string str2 = password.toStdString();
    const char* ch2 = str2.c_str();

    memcpy(send_buf+16, ch1, 63);
    memcpy(send_buf+80, ch2, 63);

    tcpsocket->write(send_buf, data_length);
    tcpsocket->waitForBytesWritten();

    char recv_buf[data_length];
    memset(recv_buf, 0, data_length);

    //QByteArray array = tcpsocket->readAll();

    if(tcpsocket->waitForReadyRead(5000))
    {
        tcpsocket->read(recv_buf, data_length);
    }
    else
    {
        errormessage = "connection error!";
        return false;
    }

    if(strcmp(recv_buf,"login") == 0)
    {
        sessionID = &(recv_buf[16]);
        return true;
    }
    if(strcmp(recv_buf,"message") == 0)
    {
        errormessage = &(recv_buf[16]);
        return false;
    }

    return false;
}

bool loginwindow::register_to_server()
{
    QString id = ui->lineEdit->text();
    QString password = ui->lineEdit_2->text();

    int over = 0;
    int numch = 0;
    int smallch = 0;
    int bigch = 0;
    int otherch = 0;
    for(int i = 0;i < password.size(); i++)
    {
        if(password[i]>='0'&&password[i]<='9')
            numch = 1;
        else if(password[i]>='a'&&password[i]<='z')
            smallch = 1;
        else if(password[i]>='A'&&password[i]<='Z')
            bigch = 1;
        else
            otherch = 1;
    }
    int type_num = numch+smallch+bigch+otherch;
    if(type_num<3 || password.size()<12)
    {
        over = 1;
    }

    if(over == 1)
    {
        errormessage = "密码强度过低，要求长度不小于12位，至少含有数字、大写、小写、其他字符中的三种";
        return false;
    }

    //QTcpSocket* tcpsocket = new QTcpSocket(this);

    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);
/*
    bool connected = tcpsocket->waitForConnected();
    if(connected == false)
        return false;*/
    //qSleep(1000);
    char send_buf[data_length+8];
    memset(send_buf, 0, data_length+8);
    memcpy(send_buf, "register", 8);


    std::string str1 = id.toStdString();
    const char* ch1 = str1.c_str();
    std::string str2 = password.toStdString();
    const char* ch2 = str2.c_str();

    memcpy(send_buf+16, ch1, 63);
    memcpy(send_buf+80, ch2, 63);
    memcpy(send_buf+144, "00000000", 8);

    tcpsocket->write(send_buf, data_length+8);
    tcpsocket->waitForBytesWritten();

    char recv_buf[data_length];
    memset(recv_buf, 0, data_length);

    //QByteArray array = tcpsocket->readAll();

    if(tcpsocket->waitForReadyRead(5000))
    {
        tcpsocket->read(recv_buf, data_length);
    }
    else
    {
        errormessage = "connection error!";
        return false;
    }

    if(strcmp(recv_buf,"register") == 0)
    {
        sessionID = &(recv_buf[16]);
        return true;
    }
    if(strcmp(recv_buf,"message") == 0)
    {
        errormessage = &(recv_buf[16]);
        return false;
    }

    return false;
}

void loginwindow::login_click()
{
    /*上传用户名和密码到服务器，接收服务器返回报文*/
    bool res;
    res = login_to_server();
    if(res == true)//登录成功，跳转
    {
        //cout<<sessionID<<endl;
        //send_mainwindow();
        tcpsocket->close();
        accept();
    }
    else//登录失败，打印提示信息
    {
        tcpsocket->close();
        QMessageBox::critical(nullptr,"错误",errormessage);
    }
}

/*注册*/
void loginwindow::register_click()
{
    /*上传用户名和密码到服务器，接收服务器返回报文*/
    bool res;
    res = register_to_server();
    if(res == true)//注册成功，跳转
    {
        QMessageBox::information(nullptr,"注册","注册成功！");
        tcpsocket->close();
        //accept();
    }
    else//登录失败，打印提示信息
    {
        QMessageBox::critical(nullptr,"错误",errormessage);
        tcpsocket->close();
    }
}
