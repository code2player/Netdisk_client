#ifndef DISKSOCKET_H
#define DISKSOCKET_H

#include <QObject>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include<QNetworkProxy>
#include<QMutex>

class disksocket : public QObject
{
    Q_OBJECT
public:
    explicit disksocket(QObject *parent = nullptr);
    QTcpSocket *tcpsocket;

    int connect_establish();


signals:

public slots:
};



//对于需要后台进行的操作（如下载），设置多线程来保证前台用户的操作
class Download_Thread:public QThread
{
    Q_OBJECT
public:
    static const int MAX_LEN = 1024;//最大报文头长度，不一定会用到这么多
    const QString IP = "106.12.116.24";
    const int PORT = 3000;

    QString from_path;
    QString to_path;
    int trans_no;
    int start_pos;
    QString SHA512_code;
    //int whe_quick;//是否需要进行秒下检测（初次）

    QMutex pause;//互斥量，暂停/重启使用

    int check_download();//检查，计算hash，用于秒下检测
    int cmd_check(QString cmd_str);//执行cmd查找的文件sha512与本文件进行比较，看是否相同
    int mes_choice;


    void run();

signals:
    void out_str1(QString str);
    void update_trans_table(int col, int row, QString str);//传输过程中更新表

    void quick_mes(int thread_no);



public slots:
    void get_filedata(QString from_path1, QString to_path1, int trans_no1, int start_pos1);//原路径（server），目的路径（local）,传输序号, 开始下载位置（断点续传）

    void get_choice(int a, int trans_no1);
};





class Upload_Thread:public QThread
{
    Q_OBJECT
public:
    static const int MAX_LEN = 1024;//最大报文头长度，不一定会用到这么多
    const QString IP = "106.12.116.24";
    const int PORT = 3000;

    QString from_path;//本机path
    QString to_path;//server路径
    QString file_name;
    int trans_no;//线程序号
    qint64 file_size;
    int whe_dir;//0为文件，1为文件夹

    qint64 rate_progress;

    //int start_pos;//起始的传输位置，不需要，由server端分配对应的传输位置
    QString SHA512_code;//本机计算出
    QMutex pause;//互斥量，暂停/重启使用


    int get_sha512();//计算sha512

    void run();






signals:
    void out_str1(QString str);
    void update_trans_table(int col, int row, QString str);//传输过程中更新表



public slots:
    void get_filedata(QString from_path1, QString to_path1, QString file_name1, int trans_no1, qint64 file_size1, int whe_dir1);

};









#endif // DISKSOCKET_H
