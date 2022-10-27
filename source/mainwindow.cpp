#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "loginwindow.h"
#include "disksocket.h"

//extern QString sessionID;//当次登录的session
#include<QDirModel>
#include <QTableView>
#include <string>
#include<QMessageBox>   //弹窗功能的实现
#include <iostream>
#include<QFileDialog>
using namespace std;

extern QString sessionID;


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("网盘");

    /*功能按钮的对应操作*/
    connect(ui->pushButton  ,SIGNAL(clicked()),this,SLOT(Func_UploadFile()));
    connect(ui->pushButton_11  ,SIGNAL(clicked()),this,SLOT(Func_UploadDir()));
    connect(ui->pushButton_2,SIGNAL(clicked()),this,SLOT(Func_Download()));
    connect(ui->pushButton_3,SIGNAL(clicked()),this,SLOT(Func_Rename()));
    connect(ui->pushButton_4,SIGNAL(clicked()),this,SLOT(Func_Copy()));
    connect(ui->pushButton_5,SIGNAL(clicked()),this,SLOT(Func_Paste()));
    connect(ui->pushButton_6,SIGNAL(clicked()),this,SLOT(Func_Movefrom()));
    connect(ui->pushButton_7,SIGNAL(clicked()),this,SLOT(Func_Moveto()));
    connect(ui->pushButton_8,SIGNAL(clicked()),this,SLOT(Func_Delete()));
    connect(ui->pushButton_9,SIGNAL(clicked()),this,SLOT(Func_Refresh()));
    connect(ui->pushButton_10,SIGNAL(clicked()),this,SLOT(Func_StoporStart()));


    /*文件列表的一系列操作*/
    model = new QStandardItemModel(ui->tableView);//初始化
    trans_list = new QStandardItemModel(ui->tableView_2);//初始化
    /*设置项*/
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows); //设置选中时为整行选中
    ui->tableView->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置表格的单元为只读属性，即不能编辑
    ui->tableView_2->setSelectionBehavior(QAbstractItemView::SelectRows); //设置选中时为整行选中
    ui->tableView_2->setEditTriggers(QAbstractItemView::NoEditTriggers); //设置表格的单元为只读属性，即不能编辑
    /*单击、双击等事件*/
    connect(ui->tableView,&QTableView::clicked,this,&MainWindow::ClickedTable);
    connect(ui->tableView,&QTableView::doubleClicked,this,&MainWindow::DoubleClickedTable);

    /*设置文件传输表列字段名*/
    trans_list->setRowCount(0);
    trans_list->setColumnCount(6);
    trans_list->setHeaderData(0,Qt::Horizontal, "文件名");
    trans_list->setHeaderData(1,Qt::Horizontal, "传输状态");
    trans_list->setHeaderData(2,Qt::Horizontal, "已下载or上传大小/B");
    trans_list->setHeaderData(3,Qt::Horizontal, "文件总大小/B");
    trans_list->setHeaderData(4,Qt::Horizontal, "网盘路径");
    trans_list->setHeaderData(5,Qt::Horizontal, "本地路径");



}

MainWindow::~MainWindow()
{
    delete ui;
}

/*从loginwindow中接收登录数据，初始化主界面窗口*/
void MainWindow::recv_loginwindow()
{
    trans_num = 0;
    now_path = "/";
    reloadtranslist();
    ui->lineEdit_2->setText(now_path);
    ui->tableView_2->setModel(trans_list);
    get_now_dir();


}

void MainWindow::get_now_dir()
{
    //now_path = ui->lineEdit_2->text();
    ui->lineEdit_2->setText(now_path);


    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);

    QString download_path = now_path;
    int path_len = download_path.toStdString().size();
    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "download", 8);

    string str1 = sessionID.toStdString();
    const char* sessionID_ch = str1.c_str();
    memcpy(send_buf1 + 16, sessionID_ch, 32);

    string str2 = to_string(path_len);
    memcpy(send_buf1 + 144, str2.c_str(), str2.size());

    string str3 = download_path.toStdString();
    memcpy(send_buf1 + 160, str3.c_str(), path_len);

    tcpsocket->write(send_buf1, 160+path_len);
    tcpsocket->waitForBytesWritten();

    /*接收下载文件信息*/
    char recv_buf1[160];
    memset(recv_buf1, 0, 160);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 160);
    }

    string sha512 = &(recv_buf1[16]);
    string file_size_str = &(recv_buf1[144]);
    int file_size = atoi(file_size_str.c_str());

    /*文件已被删除*/
    if(&(recv_buf1[16]) == "File not fount")
    {
        now_path = "/";//返回起始路径
        ui->textBrowser->append("该文件夹已被删除!\n");
        tcpsocket->close();
        get_now_dir();//刷新
        return;
    }

    /*进行秒下判断*/
    /*--------------------------*/


    /*--------------------------*/

    /*非秒下——正常下载数据*/
    /*由客户端根据文件大小计算下载方案*/
    int offset = 0;
    int length = 10000;

    QString filename = "nowdir.temp";
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    char *recv_buf2 = new char[16+length];

    /*过程中实时更新下载进度*/
    while(offset < file_size)
    {
        if(offset+length >= file_size)//本次是最后一次，特殊处理
        {
            length = file_size-offset;
        }

        char send_buf2[176];
        memset(send_buf2, 0, 176);
        memcpy(send_buf2, "downdata", 8);
        memcpy(send_buf2 + 16, sha512.c_str(), 128);
        memcpy(send_buf2 + 144, to_string(offset).c_str(), to_string(offset).size());
        memcpy(send_buf2 + 160, to_string(length).c_str(), to_string(length).size());

        tcpsocket->write(send_buf2, 176);
        tcpsocket->waitForBytesWritten();


        memset(recv_buf2, 0, 16+length);
        if(tcpsocket->waitForReadyRead())
        {
            tcpsocket->read(recv_buf2, 16);
        }
        else
        {
            cout<<"network error3"<<endl;
            return ;
        }
        int this_len = 0;
        int time = 0;
        while(this_len<length)
        {
            tcpsocket->waitForReadyRead(50);//50ms的非阻塞时间
            QByteArray array = tcpsocket->readAll();
            int res = array.size();
            memcpy(&(recv_buf2[this_len]), array, res);

            if(res!=0)
            {
                cout<<"res:"<<res<<endl;
                time=0;
            }
            else
            {
                cout<<"network error"<<endl;
                cout<<"time:"<<time<<endl;
                if(time<30)
                    time++;
                else//某次传输中有连续30次失败的传输尝试，报错
                {
                    return ;
                }
            }


            //int res = tcpsocket->read(recv_buf2, length - this_len);
            this_len += res;
        }
        /*一次性写入缓存，减少文件操作的同时便于实际写入数据量与table中的数值相符合，以便恢复使用*/
        out.writeRawData(recv_buf2, length);//直接写入原始数据，防止修改（相当于文件方式的memcpy）
        offset += length;
    }

    delete []recv_buf2;

    char send_buf3[144];
    memset(send_buf3, 0, 144);
    memcpy(send_buf3, "downfnsh", 8);
    memcpy(send_buf3 + 16, sha512.c_str(), 128);
    tcpsocket->write(send_buf3, 144);
    tcpsocket->waitForBytesWritten();

    char *recv_buf3 = new char[144];
    memset(recv_buf3, 0, 144);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf3, 144);
    }
    tcpsocket->close();
    file.close();

    /*设置列字段名*/
    model->setColumnCount(3);
    model->setHeaderData(0,Qt::Horizontal, "文件名");
    model->setHeaderData(1,Qt::Horizontal, "目录 or 文件");
    model->setHeaderData(2,Qt::Horizontal, "文件大小/B");

    QFile file1(filename);
    file1.open(QIODevice::ReadOnly);
    QTextStream in(&file1); //创建输出流
    int i;
    for(i = 0; !in.atEnd(); i++)
    {
        QString line = in.readLine();  //读取一行，解析
        QString unit = "";

        int k = 0;
        for(int j= 0 ;j<line.length();j++)
        {
            if(k==3)
                break;
            if(line[j]==' ')//判断是否是空格，空格直接跳过
            {
                if(k!=1)
                    model->setItem(i, k, new QStandardItem(unit));
                else
                {
                    if(unit == "d")
                        model->setItem(i, k, new QStandardItem("文件夹"));
                    else
                        model->setItem(i, k, new QStandardItem("文件"));
                }
                k++;
                unit="";
                continue;
            }
            else
            {
                unit += line[j];
            }
        }
        model->setItem(i, k, new QStandardItem(unit));
    }
    model->setRowCount(i);
    ui->tableView->setModel(model);

    /*目录全部获取完成，删除临时目录文件*/
    file1.remove();

}

void MainWindow::Out_str1(QString str)
{
    ui->textBrowser->append(str);
}

void MainWindow::Update_trans_table(int col, int row, QString str)
{
    trans_list->setItem(row, col, new QStandardItem(str));
    ui->tableView_2->setModel(trans_list);
}

/*在这个函数里统一创建并管理线程*/
void MainWindow::Uploadfile(QString from_path, QString to_path, QString file_name, qint64 file_size, int whe_dir)
{
    //新建子线程，后台上传
    Upload_Thread * subThread = new Upload_Thread;
    connect(this, SIGNAL(send_filedata2(QString,QString,QString,int,qint64,int)), subThread, SLOT(get_filedata(QString,QString,QString,int,qint64,int)),Qt::UniqueConnection);
    connect(subThread, SIGNAL(update_trans_table(int,int,QString)), this, SLOT(Update_trans_table(int,int,QString)));
    connect(subThread, SIGNAL(out_str1(QString)), this, SLOT(Out_str1(QString)));
    send_filedata2(from_path, to_path, file_name, trans_num, file_size, whe_dir);
    //多线程环境，为了避免一个信号对预料之外的槽的影响，取消连接
    disconnect(this, SIGNAL(send_filedata2(QString,QString,QString,int,qint64,int)), subThread, SLOT(get_filedata(QString,QString,QString,int,qint64,int)));


    /*初始化下载项*/
    trans_list->setRowCount(trans_num + 1);
    Update_trans_table(0,trans_num,file_name);
    Update_trans_table(1,trans_num,"上传中");
    Update_trans_table(3,trans_num,QString::number(file_size));
    Update_trans_table(4,trans_num,to_path);
    Update_trans_table(5,trans_num,from_path);

    thread_list2[trans_num] = subThread;
    trans_num++;
    subThread->start();
}

//上传文件
void MainWindow::Func_UploadFile()
{
    QString from_path = QDir::toNativeSeparators(QFileDialog::getOpenFileName(this,tr("选择上传文件"),QDir::currentPath()));  //文件路径
    if(from_path == "")
        return;
    from_path = QString::fromLocal8Bit(from_path.toStdString().c_str());
    QFileInfo fileInfo(from_path);
    QString file_name = fileInfo.fileName();
    qint64 file_size = fileInfo.size();

    qDebug()<<from_path;
    QString to_path = now_path;
    if(now_path != "/")
        to_path += "/";
    to_path += file_name;
    Uploadfile(from_path ,to_path, file_name, file_size, 0);
}

/*from:拼接完成  to：拼接*/
void MainWindow::Uploaddir(QString from_path, QString to_path, QString file_name, qint64 file_size)
{
    /*先创建文件夹（不会重复创建）*/
    Uploadfile(from_path, to_path, file_name, file_size, 1);//起始位置先尝试创建文件夹

    /*在本机中递归获取所有文件和文件夹*/
    QDir dir(from_path);
    QFileInfoList filelist = dir.entryInfoList();
    for(int i=0; i<filelist.size(); i++)
    {
        if(filelist[i].fileName() == "." || filelist[i].fileName() == "..")
            continue;
        QString from_path1 = filelist[i].filePath();
        QString to_path1 = to_path + "/" +filelist[i].fileName();
        QString file_name1 = filelist[i].fileName();
        qint64 file_size1 = filelist[i].size();
        if(filelist[i].isDir())
        {
            Uploaddir(from_path1, to_path1, file_name1, file_size1);
        }
        else
        {
            Uploadfile(from_path1 ,to_path1, file_name1, file_size1, 0);
        }
    }
}

//上传文件夹
void MainWindow::Func_UploadDir()
{
    QString from_path=QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this,tr("选择上传文件夹"),QDir::currentPath()));  //文件路径
    if(from_path.isEmpty())
    {
        return;
    }
    QFileInfo fileInfo(from_path);
    QString file_name = fileInfo.fileName();
    qint64 file_size = fileInfo.size();
    QString to_path = now_path;
    if(now_path != "/")
        to_path += "/";
    to_path += file_name;

    Uploaddir(from_path ,to_path, file_name, file_size);
}

//递归下载文件夹
void MainWindow::Func_Download_plus(QString from_path, QString to_path)
{
    QDir dir(to_path);
    if(!dir.exists())//新建文件夹
    {
        dir.mkdir(to_path);
    }


    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);

    QString download_path = from_path;
    int path_len = download_path.toStdString().size();
    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "download", 8);

    string str1 = sessionID.toStdString();
    const char* sessionID_ch = str1.c_str();
    memcpy(send_buf1 + 16, sessionID_ch, 32);

    string str2 = to_string(path_len);
    memcpy(send_buf1 + 144, str2.c_str(), str2.size());

    string str3 = download_path.toStdString();
    memcpy(send_buf1 + 160, str3.c_str(), path_len);

    tcpsocket->write(send_buf1, 160+path_len);
    tcpsocket->waitForBytesWritten();

    /*接收下载文件信息*/
    char recv_buf1[160];
    memset(recv_buf1, 0, 160);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 160);
    }

    string sha512 = &(recv_buf1[16]);
    string file_size_str = &(recv_buf1[144]);
    int file_size = atoi(file_size_str.c_str());

    /*文件已被删除*/
    if(&(recv_buf1[16]) == "File not fount")
    {
        now_path = "/";//返回起始路径
        ui->textBrowser->append("该文件夹已被删除!\n");
        tcpsocket->close();
        get_now_dir();//刷新
        return;
    }

    /*非秒下——正常下载数据*/
    /*由客户端根据文件大小计算下载方案*/
    int offset = 0;
    int length = 10000;

    QString filename = to_path+"/download.temp";
    QFile file(filename);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    char *recv_buf2 = new char[16+length];

    /*过程中实时更新下载进度*/
    while(offset < file_size)
    {
        if(offset+length >= file_size)//本次是最后一次，特殊处理
        {
            length = file_size-offset;
        }

        char send_buf2[176];
        memset(send_buf2, 0, 176);
        memcpy(send_buf2, "downdata", 8);
        memcpy(send_buf2 + 16, sha512.c_str(), 128);
        memcpy(send_buf2 + 144, to_string(offset).c_str(), to_string(offset).size());
        memcpy(send_buf2 + 160, to_string(length).c_str(), to_string(length).size());

        tcpsocket->write(send_buf2, 176);
        tcpsocket->waitForBytesWritten();


        memset(recv_buf2, 0, 16+length);
        if(tcpsocket->waitForReadyRead())
        {
            tcpsocket->read(recv_buf2, 16);
        }
        else
        {
            cout<<"network error3"<<endl;
            return ;
        }
        int this_len = 0;
        int time = 0;
        while(this_len<length)
        {
            tcpsocket->waitForReadyRead(50);//50ms的非阻塞时间
            QByteArray array = tcpsocket->readAll();
            int res = array.size();
            memcpy(&(recv_buf2[this_len]), array, res);

            if(res!=0)
            {
                cout<<"res:"<<res<<endl;
                time=0;
            }
            else
            {
                cout<<"network error"<<endl;
                cout<<"time:"<<time<<endl;
                if(time<30)
                    time++;
                else//某次传输中有连续30次失败的传输尝试，报错
                {
                    return ;
                }
            }


            //int res = tcpsocket->read(recv_buf2, length - this_len);
            this_len += res;
        }
        /*一次性写入缓存，减少文件操作的同时便于实际写入数据量与table中的数值相符合，以便恢复使用*/
        out.writeRawData(recv_buf2, length);//直接写入原始数据，防止修改（相当于文件方式的memcpy）
        offset += length;
    }

    delete []recv_buf2;

    char send_buf3[144];
    memset(send_buf3, 0, 144);
    memcpy(send_buf3, "downfnsh", 8);
    memcpy(send_buf3 + 16, sha512.c_str(), 128);
    tcpsocket->write(send_buf3, 144);
    tcpsocket->waitForBytesWritten();

    char *recv_buf3 = new char[144];
    memset(recv_buf3, 0, 144);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf3, 144);
    }
    tcpsocket->close();
    file.close();

    QFile file1(filename);
    file1.open(QIODevice::ReadOnly);
    QTextStream in(&file1); //创建输出流
    int i;
    for(i = 0; !in.atEnd(); i++)
    {
        QString line = in.readLine();  //读取一行，解析
        QString unit = "";
        QString downfile_name;
        QString downfile_type;
        QString downfile_size;
        if(i < 2)
        {
            continue;
        }


        int k = 0;
        for(int j= 0 ;j<line.length();j++)
        {
            if(k == 3)
                break;
            if(line[j]==' ')//判断是否是空格，空格直接跳过
            {
                if(k==0)
                {
                    downfile_name = unit;
                }
                else if(k==1)
                {
                    downfile_type = unit;
                }
                else if(k==2)
                {
                    downfile_size = unit;
                }
                else
                {

                }

                k++;
                unit="";
                continue;
            }
            else
            {
                unit += line[j];
            }
        }
        if(k==2)
        {
            downfile_size = unit;
        }

        if(downfile_type == "d")//下载文件夹，递归
        {
            QString from_path1 = from_path + "/" + downfile_name;
            QString to_path1 = to_path + "/" + downfile_name;
            Func_Download_plus(from_path1, to_path1);
        }
        else//下载文件
        {
            qDebug()<<"ready:"<<downfile_name;
            QString from_path1 = from_path + "/" + downfile_name;
            QString to_path1 = to_path + "/" + downfile_name;
            //新建子进程，后台下载
            Download_Thread * subThread = new Download_Thread;
            connect(subThread, SIGNAL(out_str1(QString)), this, SLOT(Out_str1(QString)));
            connect(subThread, SIGNAL(update_trans_table(int,int,QString)), this, SLOT(Update_trans_table(int,int,QString)));
            connect(this, SIGNAL(send_choice(int,int)), subThread, SLOT(get_choice(int,int)));
            connect(subThread, SIGNAL(quick_mes(int)), this, SLOT(Quick_mes(int)));
            connect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)),Qt::UniqueConnection);
            send_filedata(from_path1, to_path1, trans_num, 0);//直接点下载按钮，从零开始下载

            //多线程环境，为了避免一个信号对预料之外的槽的影响，取消连接
            disconnect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)));
            /*初始化下载项*/
            trans_list->setRowCount(trans_num + 1);
            Update_trans_table(0,trans_num,downfile_name);
            Update_trans_table(1,trans_num,"传输中");
            Update_trans_table(3,trans_num,downfile_size);
            Update_trans_table(4,trans_num,from_path1);
            Update_trans_table(5,trans_num,to_path1);


            thread_list[trans_num] = subThread;
            trans_num++;
            qDebug()<<"start:"<<downfile_name;
            subThread->start();
        }






    }

    /*目录全部获取完成，删除临时目录文件*/
    file1.remove();


}


//下载
void MainWindow::Func_Download()
{
    int row1 = ui->tableView->currentIndex().row();
    if(row1 < 0)
        return;
    QString file_name = model->index(row1 , 0).data().toString();

    if(file_name=="." || file_name=="..")
    {
        return;
    }
    QString from_path = now_path + "/" + file_name;

    QString to_path=QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this,tr("文件保存路径选择"),QDir::currentPath()));  //文件路径
    if(to_path.isEmpty())
    {
        return;
    }

    to_path+="/";
    to_path+=file_name;

    if(model->index(row1 , 1).data().toString() == "文件夹")//特殊处理
    {
        //return;
        Func_Download_plus(from_path, to_path);
        return ;
    }



    //新建子进程，后台下载
    Download_Thread * subThread = new Download_Thread;
    connect(subThread, SIGNAL(out_str1(QString)), this, SLOT(Out_str1(QString)));
    connect(subThread, SIGNAL(update_trans_table(int,int,QString)), this, SLOT(Update_trans_table(int,int,QString)));
    connect(this, SIGNAL(send_choice(int,int)), subThread, SLOT(get_choice(int,int)));
    connect(subThread, SIGNAL(quick_mes(int)), this, SLOT(Quick_mes(int)));
    connect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)),Qt::UniqueConnection);
    send_filedata(from_path, to_path, trans_num, 0);//直接点下载按钮，从零开始下载
    //多线程环境，为了避免一个信号对预料之外的槽的影响，取消连接
    disconnect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)));

    /*初始化下载项*/
    trans_list->setRowCount(trans_num + 1);
    Update_trans_table(0,trans_num,file_name);
    Update_trans_table(1,trans_num,"传输中");
    Update_trans_table(3,trans_num,model->index(row1 , 2).data().toString());
    Update_trans_table(4,trans_num,from_path);
    Update_trans_table(5,trans_num,to_path);

    thread_list[trans_num] = subThread;
    trans_num++;
    subThread->start();




}

//重命名
void MainWindow::Func_Rename()
{
    int row1 = ui->tableView->currentIndex().row();
    if(row1 < 0)
        return;
    QString file_name = model->index(row1 , 0).data().toString();

    if(file_name=="." || file_name=="..")
    {
        return;
    }
    QString from_path = now_path + "/" + file_name;

    QString new_name = ui->lineEdit->text();
    if(new_name.isEmpty())
    {
        return;
    }

    QString to_path = now_path + "/" + new_name;

    int from_len = from_path.toStdString().size();
    int to_len = to_path.toStdString().size();

    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);

    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "move", 4);

    string str1 = sessionID.toStdString();
    const char* sessionID_ch = str1.c_str();
    memcpy(send_buf1 + 16, sessionID_ch, 32);

    string str2 = to_string(from_len);
    memcpy(send_buf1 + 144, str2.c_str(), str2.size());

    string str3 = from_path.toStdString();
    memcpy(send_buf1 + 160, str3.c_str(), from_len);

    string str4 = to_string(to_len);
    memcpy(send_buf1 + 160 + from_len, str4.c_str(), str4.size());

    string str5 = to_path.toStdString();
    memcpy(send_buf1 + 176 + from_len, str5.c_str(), to_len);


    tcpsocket->write(send_buf1, (176 + from_len + to_len));
    tcpsocket->waitForBytesWritten();

    /*接收下载文件信息*/
    char recv_buf1[32];
    memset(recv_buf1, 0, 32);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 160);
    }
    QString result = &(recv_buf1[16]);
    if(result == "success")
    {
        ui->textBrowser->append("重命名成功！");
    }
    else
        ui->textBrowser->append("重命名失败！");
}

//复制
void MainWindow::Func_Copy()
{
    int row1 = ui->tableView->currentIndex().row();
    if(row1 < 0)
        return;
    QString file_name = model->index(row1 , 0).data().toString();

    if(file_name=="." || file_name=="..")
    {
        return;
    }
    QString from_path = now_path + "/" + file_name;
    copy_name = file_name;

    ui->lineEdit_3->setText(from_path);
}

//粘贴
void MainWindow::Func_Paste()
{
    QString from_path = ui->lineEdit_3->text();
    if(from_path.isEmpty())
    {
        return;
    }
    QString to_path = now_path + "/" + copy_name;

    int from_len = from_path.toStdString().size();
    int to_len = to_path.toStdString().size();

    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);

    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "copy", 4);

    string str1 = sessionID.toStdString();
    const char* sessionID_ch = str1.c_str();
    memcpy(send_buf1 + 16, sessionID_ch, 32);

    string str2 = to_string(from_len);
    memcpy(send_buf1 + 144, str2.c_str(), str2.size());

    string str3 = from_path.toStdString();
    memcpy(send_buf1 + 160, str3.c_str(), from_len);

    string str4 = to_string(to_len);
    memcpy(send_buf1 + 160 + from_len, str4.c_str(), str4.size());

    string str5 = to_path.toStdString();
    memcpy(send_buf1 + 176 + from_len, str5.c_str(), to_len);


    int write_num = tcpsocket->write(send_buf1, (176 + from_len + to_len));
    tcpsocket->waitForBytesWritten();

    qDebug()<<"write_num:"<<write_num<<endl;

    /*接收下载文件信息*/
    char recv_buf1[32];
    memset(recv_buf1, 0, 32);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 160);
    }
    QString result = &(recv_buf1[16]);
    if(result == "success")
    {
        ui->textBrowser->append("复制成功！");
    }
    else
        ui->textBrowser->append("复制失败！");

}

//移动——移出
void MainWindow::Func_Movefrom()
{
    int row1 = ui->tableView->currentIndex().row();
    if(row1 < 0)
        return;
    QString file_name = model->index(row1 , 0).data().toString();

    if(file_name=="." || file_name=="..")
    {
        return;
    }
    QString from_path = now_path + "/" + file_name;
    move_name = file_name;

    ui->lineEdit_4->setText(from_path);

}

//移动——移入
void MainWindow::Func_Moveto()
{
    QString from_path = ui->lineEdit_4->text();
    if(from_path.isEmpty())
    {
        return;
    }
    QString to_path = now_path + "/" + move_name;

    int from_len = from_path.toStdString().size();
    int to_len = to_path.toStdString().size();

    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);

    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "move", 4);

    string str1 = sessionID.toStdString();
    const char* sessionID_ch = str1.c_str();
    memcpy(send_buf1 + 16, sessionID_ch, 32);

    string str2 = to_string(from_len);
    memcpy(send_buf1 + 144, str2.c_str(), str2.size());

    string str3 = from_path.toStdString();
    memcpy(send_buf1 + 160, str3.c_str(), from_len);

    string str4 = to_string(to_len);
    memcpy(send_buf1 + 160 + from_len, str4.c_str(), str4.size());

    string str5 = to_path.toStdString();
    memcpy(send_buf1 + 176 + from_len, str5.c_str(), to_len);


    tcpsocket->write(send_buf1, (176 + from_len + to_len));
    tcpsocket->waitForBytesWritten();

    /*接收下载文件信息*/
    char recv_buf1[32];
    memset(recv_buf1, 0, 32);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 160);
    }
    QString result = &(recv_buf1[16]);
    if(result == "success")
    {
        ui->textBrowser->append("移动成功！");
    }
    else
        ui->textBrowser->append("移动失败！");
}

//删除
void MainWindow::Func_Delete()
{
    int row1 = ui->tableView->currentIndex().row();
    if(row1 < 0)
        return;
    QString file_name = model->index(row1 , 0).data().toString();

    if(file_name=="." || file_name=="..")
    {
        return;
    }
    QString from_path = now_path + "/" + file_name;

    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);
    //tcpsocket->setReadBufferSize(100000);

    QString download_path = from_path;
    int path_len = download_path.toStdString().size();
    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "delete", 6);

    string str1 = sessionID.toStdString();
    const char* sessionID_ch = str1.c_str();
    memcpy(send_buf1 + 16, sessionID_ch, 32);

    string str2 = to_string(path_len);
    memcpy(send_buf1 + 144, str2.c_str(), str2.size());

    string str3 = download_path.toStdString();
    memcpy(send_buf1 + 160, str3.c_str(), path_len);

    tcpsocket->write(send_buf1, 160+path_len);
    tcpsocket->waitForBytesWritten();

    /*接收下载文件信息*/
    char recv_buf1[32];
    memset(recv_buf1, 0, 32);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 160);
    }
    QString result = &(recv_buf1[16]);
    if(result == "success")
    {
        ui->textBrowser->append("删除成功！");
    }
    else
        ui->textBrowser->append("删除失败！");
}

//刷新当前目录内容，但是传输不变
void MainWindow::Func_Refresh()
{
    get_now_dir();
}

//暂停的重启，重启的暂停
//通过互斥量对线程进行操作
void MainWindow::Func_StoporStart()
{
    int row1 = ui->tableView_2->currentIndex().row();
    if(row1 < 0)
        return;
    QString state = trans_list->index(row1 , 1).data().toString();
    if(state == "暂停")
    {
        thread_list[row1]->pause.unlock();
        Update_trans_table(1,row1,"传输中");
        return;
    }
    if(state == "传输中")
    {
        thread_list[row1]->pause.lock();
        Update_trans_table(1,row1,"暂停");
        return;
    }
    if(state == "断开连接")//由于网络因素断开连接，创建新进程
    {

        //新建子进程，后台下载
        Download_Thread * subThread = new Download_Thread;
        connect(subThread, SIGNAL(out_str1(QString)), this, SLOT(Out_str1(QString)));
        connect(subThread, SIGNAL(update_trans_table(int,int,QString)), this, SLOT(Update_trans_table(int,int,QString)));

        connect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)),Qt::UniqueConnection);
        connect(this, SIGNAL(send_choice(int,int)), subThread, SLOT(get_choice(int,int)));
        connect(subThread, SIGNAL(quick_mes(int)), this, SLOT(Quick_mes(int)));

        QString from_path = trans_list->index(row1 , 4).data().toString();
        QString to_path = trans_list->index(row1 , 5).data().toString();
        int new_offset = trans_list->index(row1 , 2).data().toString().toInt();
        send_filedata(from_path, to_path, row1, new_offset);//直接点下载按钮，从零开始下载
        //多线程环境，为了避免一个信号对预料之外的槽的影响，取消连接
        disconnect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)));

        /*初始化下载项*/
        thread_list[row1] = subThread;//放入列表中
        Update_trans_table(1,row1,"传输中");
        subThread->start();
        return;
    }

    /*以下是上传内容*/
    if(state == "暂停上传")
    {
        thread_list[row1]->pause.unlock();
        Update_trans_table(1,row1,"上传中");
        return;
    }
    if(state == "上传中")
    {
        thread_list[row1]->pause.lock();
        Update_trans_table(1,row1,"暂停上传");
        return;
    }

    if(state == "上传断连")//由于网络因素断开连接，创建新进程
    {
        QString from_path = trans_list->index(row1 , 5).data().toString();
        QString to_path = trans_list->index(row1 , 4).data().toString();
        QString file_name = trans_list->index(row1 , 0).data().toString();
        qint64 file_size = trans_list->index(row1 , 3).data().toString().toLongLong();
        int whe_dir = 0;


        //新建子线程，后台上传
        Upload_Thread * subThread = new Upload_Thread;
        connect(this, SIGNAL(send_filedata2(QString,QString,QString,int,qint64,int)), subThread, SLOT(get_filedata(QString,QString,QString,int,qint64,int)),Qt::UniqueConnection);
        connect(subThread, SIGNAL(update_trans_table(int,int,QString)), this, SLOT(Update_trans_table(int,int,QString)));
        connect(subThread, SIGNAL(out_str1(QString)), this, SLOT(Out_str1(QString)));
        send_filedata2(from_path, to_path, file_name, row1, file_size, whe_dir);
        //多线程环境，为了避免一个信号对预料之外的槽的影响，取消连接
        disconnect(this, SIGNAL(send_filedata2(QString,QString,QString,int,qint64,int)), subThread, SLOT(get_filedata(QString,QString,QString,int,qint64,int)));


        /*初始化下载项*/

        Update_trans_table(0,row1,file_name);
        Update_trans_table(1,row1,"上传中");
        Update_trans_table(3,row1,QString::number(file_size));
        Update_trans_table(4,row1,to_path);
        Update_trans_table(5,row1,from_path);

        thread_list2[row1] = subThread;

        subThread->start();
        return;
    }




}

void MainWindow::Quick_mes(int thread_no)
{
    if( QMessageBox::question(this,tr("相同文件"),
                                 tr("检测到同名的文件，是否覆盖？"),
                                  QMessageBox::Yes, QMessageBox::No )
                       == QMessageBox::Yes)
    {
        send_choice(1,thread_no);
    }
    else
      send_choice(2,thread_no);
    disconnect(this, SIGNAL(send_choice(int,int)), thread_list[thread_no], SLOT(get_choice(int,int)));


}

void MainWindow::reloadtranslist()
{
    QFile file(waitforlog);
    file.open(QIODevice::ReadWrite);
    QTextStream in(&file); //创建输出流
    int i;
    for(i = 0; !in.atEnd(); i++)
    {
        QString line = in.readLine();  //读取一行，解析
        QString unit = "";

        int k = 0;
        for(int j= 0 ;j<line.length();j++)
        {
            if(k==6)
                break;


            if(line[j]==' ')//判断是否是空格，空格直接跳过
            {
                trans_list->setItem(i, k, new QStandardItem(unit));

                if(k==1)
                {
                    if(unit=="1")
                        trans_list->setItem(i, k, new QStandardItem("断开连接"));
                    else
                        trans_list->setItem(i, k, new QStandardItem("上传断连"));
                }

                k++;
                unit="";
                continue;
            }
            else
            {
                unit += line[j];
            }
        }

        //新建子进程，后台下载
        /*Download_Thread * subThread = new Download_Thread;
        connect(subThread, SIGNAL(out_str1(QString)), this, SLOT(Out_str1(QString)));
        connect(subThread, SIGNAL(update_trans_table(int,int,QString)), this, SLOT(Update_trans_table(int,int,QString)));
        connect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)),Qt::UniqueConnection);

        connect(this, SIGNAL(send_choice(int,int)), subThread, SLOT(get_choice(int,int)));
        connect(subThread, SIGNAL(quick_mes(int)), this, SLOT(Quick_mes(int)));

        QString from_path = trans_list->index(trans_num , 4).data().toString();
        QString to_path = trans_list->index(trans_num , 5).data().toString();
        int new_offset = trans_list->index(trans_num , 2).data().toString().toInt();
        subThread->pause.lock();
        send_filedata(from_path, to_path, trans_num, new_offset);//直接点下载按钮，从零开始下载
        //多线程环境，为了避免一个信号对预料之外的槽的影响，取消连接
        disconnect(this, SIGNAL(send_filedata(QString,QString,int,int)), subThread, SLOT(get_filedata(QString,QString,int,int)));*/

        /*初始化下载项*/
        /*trans_list->setRowCount(trans_num + 1);
        thread_list[trans_num] = subThread;
        trans_num++;
        subThread->start();*/
    }
    if(trans_num!=0)
        ui->textBrowser->append("检测到未完成的传输任务，已加载！");
    trans_list->setRowCount(i);
    ui->tableView_2->setModel(trans_list);

    /*目录全部获取完成，删除临时目录文件*/
    file.remove();
}

/*侦听到窗口关闭事件，重写虚函数保存当前的传输列表*/
void MainWindow::closeEvent ( QCloseEvent * event )
{
    /*if( QMessageBox::question(this,
                             tr("Quit"),
                             tr("Are you sure to quit this application?"),
                              QMessageBox::Yes, QMessageBox::No )
                   == QMessageBox::Yes){
        event->accept();//不会将事件传递给组件的父组件

        qDebug()<<"ok";
    }
    else
      event->ignore();*/
    QFile file(waitforlog);
    file.open(QIODevice::ReadWrite);
    QTextStream out(&file);

    int i;
    for(int i=0; i<trans_num; i++)
    {
        QString state = trans_list->index(i , 1).data().toString();
        if(state == "传输中"||state == "断开连接"||state == "暂停")
        {
            out<<trans_list->index(i , 0).data().toString()<<" ";//name
            out<<"1 ";
            out<<trans_list->index(i , 2).data().toString()<<" ";//已下载
            out<<trans_list->index(i , 3).data().toString()<<" ";//总大小
            out<<trans_list->index(i , 4).data().toString()<<" ";//from_path
            out<<trans_list->index(i , 5).data().toString()<<" ";//to_path
            out<<endl;
        }
        if(state == "上传中"||state == "暂停上传"||state == "上传断连")
        {
            out<<trans_list->index(i , 0).data().toString()<<" ";//name
            out<<"2 ";
            out<<trans_list->index(i , 2).data().toString()<<" ";//已下载
            out<<trans_list->index(i , 3).data().toString()<<" ";//总大小
            out<<trans_list->index(i , 4).data().toString()<<" ";//from_path
            out<<trans_list->index(i , 5).data().toString()<<" ";//to_path
            out<<endl;
        }
    }

    file.close();
}



void MainWindow::ClickedTable(const QModelIndex &index)
{
    //QStandardItemModel* model = new QStandardItemModel(this);

    //model->index(index.row() , 0).data().toString();
    //index.row();
    //获取被点击的文件的信息并显示
   // QString strShow = m_model->isDir(index)? "selected is dir: ":"selected is file: ";
    //strShow.append(m_model->fileName(index));
    //ui->textBrowser->append(model->index(index.row() , 0).data().toString());
}


void MainWindow::DoubleClickedTable(const QModelIndex &index)
{
    if(model->index(index.row() , 1).data().toString() != "文件夹")
    {
        return;
    }

    if(model->index(index.row() , 0).data().toString() == ".")
    {
        return;
    }

    if(model->index(index.row() , 0).data().toString() == "..")
    {
        if(now_path == "/")//已经是根目录
            return;
        int pos = now_path.lastIndexOf("/");
        QString new_path = now_path.mid(0, pos);
        if(new_path == "")
            new_path = "/";
        now_path = new_path;
        get_now_dir();
        return ;
    }

    //QString new_path = now_path + "/" + model->index(index.row() , 0).data().toString();
    //now_path = new_path;
    if(now_path != "/")
        now_path += "/";
    now_path += model->index(index.row() , 0).data().toString();

    get_now_dir();
}



