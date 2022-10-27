#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <QStandardItemModel>
#include<QThread>
#include "disksocket.h"
#include <QCloseEvent>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static const int MAX_LEN = 1024;//最大报文头长度，不一定会用到这么多
    const QString IP = "106.12.116.24";
    const int PORT = 3000;
    const QString successlog = "success_trans.log";//记录下载成功的日志，用于秒下
    const QString waitforlog = "waitfor_trans.log";//记录尚未下载完的日志，用于断点续下

    //QTcpSocket* tcpsocket;//从登录窗口继承过来的tcpsocket
    //QString sessionID;//本次登录独一无二的会话id



    QString now_path;//当前位置的路径

    QStandardItemModel* model;//当前路径下文件目录表

    QModelIndex now_index;//当前选中的表项（如果有的话）



    QStandardItemModel* trans_list;//传输列表（实时更新）
    int trans_num;//本次登录的传输数量
    Download_Thread *thread_list[200];//下载线程列表
    Upload_Thread *thread_list2[200];//上传线程列表

    QString copy_name;
    QString move_name;




    /*对应的工具函数*/


    void get_now_dir();//获取当前路径下的详细目录内容并显示

    //void
    void closeEvent(QCloseEvent *event);//覆盖虚函数，关闭时更新下载内容

    void reloadtranslist();//进入主窗口时载入待下载log

    void get_from_dir(QString from_path);//获取from_path下的详细目录内容
    void Func_Download_plus(QString from_path, QString to_path);//下载文件夹用




    void Uploadfile(QString from_path, QString to_path, QString file_name, qint64 file_size, int whe_dir);//上传文件用
    void Uploaddir(QString from_path, QString to_path, QString file_name, qint64 file_size);//上传文件夹用




public slots:
    /*提供给用户的功能选项*/
    void Func_UploadFile();//上传文件
    void Func_UploadDir();//上传文件夹
    void Func_Download();//下载
    void Func_Rename();//重命名
    void Func_Copy();//复制
    void Func_Paste();//粘贴
    void Func_Movefrom();//移动——移出
    void Func_Moveto();//移动——移入
    void Func_Delete();//删除
    void Func_Refresh();//刷新当前目录内容，但是传输不变
    void Func_StoporStart();//刷新当前目录内容，但是传输不变


    /*系统函数*/

    void ClickedTable(const QModelIndex &index);//单击文件列表项

    void DoubleClickedTable(const QModelIndex &index);//双击文件列表项

    void Out_str1(QString str);

    void recv_loginwindow();//初始化，从loginwindow中接收数据

    void Update_trans_table(int col, int row, QString str);//传输过程中更新表

    void Quick_mes(int thread_no);


signals:
    void send_filedata(QString str1, QString str2, int trans_no1, int start_pos1);

    void send_choice(int a, int trans_no);

    void send_filedata2(QString from_path1, QString to_path1, QString file_name1, int trans_no1, qint64 file_size1, int whe_dir1);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
