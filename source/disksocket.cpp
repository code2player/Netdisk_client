#include "disksocket.h"
#include<QDirModel>
#include<QThread>
#include <QTableView>
#include <string>
#include <iostream>
#include<QMessageBox>   //弹窗功能的实现
#include<QTest>
#include <QCoreApplication>
#include <QTextCodec>
using namespace std;

extern QString sessionID;


disksocket::disksocket(QObject *parent) : QObject(parent)
{
    tcpsocket=new QTcpSocket(this);
}


int disksocket::connect_establish()
{
    tcpsocket->connectToHost("127.0.0.1",3000);
    char ch[111];
    tcpsocket->write(ch,11);
    //connect(tcpsocket,SIGNAL(connected()),this,SLOT(connected_SLOT()));
    //printf("打开客户端 ");
}



void Download_Thread::get_filedata(QString from_path1, QString to_path1, int trans_no1, int start_pos1)
{
    from_path = from_path1;
    to_path = to_path1;
    trans_no = trans_no1;
    start_pos = start_pos1;
}

/*做一个发送序号的验证*/
void Download_Thread::get_choice(int a, int trans_no1)
{
    if(trans_no == trans_no1)
        mes_choice = a;
}

int Download_Thread::cmd_check(QString cmd_str)
{
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec* gbk = QTextCodec::codecForName("gbk");

    QString strUnicode= utf8->toUnicode(cmd_str.toLocal8Bit().data());
    //2. unicode -> gbk, 获得QByteArray
    QByteArray gb_bytes= gbk->fromUnicode(strUnicode);
    char *cmd =  gb_bytes.data(); //获取其char *



    /*char cmd[1024];
    memset(cmd, 0, 1024);
    strcpy(cmd, cmd_str.toStdString().c_str());*/
    char MsgBuff[1024];
    int MsgLen=1020;
    FILE * fp;

    int ret = -1;
    if (cmd == NULL)
    {
        return -2;
    }
    if ((fp = _popen(cmd, "r")) == NULL)
    {
        return -2;
    }
    else
    {
        memset(MsgBuff, 0, MsgLen);

        //读取命令执行过程中的输出
        fgets(MsgBuff, MsgLen, fp);
        fgets(MsgBuff, MsgLen, fp);
        MsgBuff[128] = '\0';

        QString this_sha512 = MsgBuff;
        if(this_sha512 == SHA512_code)
        {
            ret = 0;
            qDebug()<<cmd_str;
        }

        while (fgets(MsgBuff, MsgLen, fp) != NULL)
        {
            //printf("MsgBuff: %s\n", MsgBuff);
        }

        //关闭执行的进程
        if(_pclose(fp) == -1)
        {
            return -3;
        }
    }
    return ret;
}

int Download_Thread::check_download()
{
    QString cmd_head = "certutil -hashfile ";
    QString cmd_tail = " SHA512";


    int pos = max(to_path.lastIndexOf("/") , to_path.lastIndexOf("\\"));
    QString dir_path = to_path.mid(0 , pos);
    QString file_name = to_path.mid(pos + 1);
    cmd_head += dir_path;
    cmd_head += "/";


    /*先检查本文件夹的同名文件*/
    QFileInfo fileInfo(to_path);


    if(fileInfo.isFile()==false)
    {
        return 0;//正常下载
    }
    else
    {
        QString cmd = cmd_head + fileInfo.fileName() + cmd_tail;
        int ret = cmd_check(cmd);
        if(ret == 0)//文件相同,秒下
        {
            qDebug()<<"cd:"<<cmd;
            return 1;//秒下
        }
        else//同名不同文件，询问是否覆盖
        {
            quick_mes(trans_no);
            while(mes_choice == 0)//循环等待选择结果
            {
                QTest::qSleep(100);
            }
            if(mes_choice == 1)
            {
                /*删除原文件*/
                QFile file(to_path);
                file.open(QIODevice::WriteOnly|QIODevice::Append);
                file.remove();
                return 0;
            }
            else
            {
                return -1;//取消下载
            }
        }
    }
    return 0;


    /*再检查本文件夹下的所有文件*/
    /*QDir dir(dir_path);

    dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);
    dir.setSorting(QDir::Size | QDir::Reversed);

    QFileInfoList list = dir.entryInfoList();
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        QString cmd = cmd_head + fileInfo.fileName() + cmd_tail;
        if(fileInfo.fileName() == file_name)//秒下
        {
            return 1;
        }
        else//非秒下
        {
            continue;
        }


        int ret = cmd_check(cmd);
        if(ret == 0)//存在文件
        {
            if(fileInfo.fileName() == file_name)//秒下
            {
                return 1;
            }
            else//非秒下
            {

                return -1;
            }

        }
        else//非秒下，正常下载
        {
            return -1;
        }
    }*/

}


void Download_Thread::run()
{
    mes_choice = 0;
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
    if(tcpsocket->waitForReadyRead(5000))
    {
        int rres = tcpsocket->read(recv_buf1, 160);
        qDebug()<<"rres:"<<rres;
    }

    string sha512 = &(recv_buf1[16]);
    string file_size_str = &(recv_buf1[144]);
    int file_size = atoi(file_size_str.c_str());
    char chx1[129];
    memcpy(chx1,&(recv_buf1[16]),128);
    chx1[128]='\0';
    SHA512_code = chx1;


    /*文件已被删除*/
    if(&(recv_buf1[16]) == "File not fount")
    {
        out_str1("该文件已被删除!");
        tcpsocket->close();
        return;
    }
    /*进行秒下判断*/
    /*--------------------------*/
    if(start_pos == 0)//从零开始传输，才需要判断秒下，端点续传不判断
    {
        qDebug()<<"judge1:"<<to_path;
        int quick_download = check_download();
        qDebug()<<"judge2:"<<to_path;
        if(quick_download > 0)
        {
            qDebug()<<to_path;
            qDebug()<<trans_no;
            update_trans_table(1, trans_no, "秒下");
            out_str1("秒下成功！");
            return;
        }
        else if(quick_download == 0)//正常下载
        {

        }
        else
        {
            update_trans_table(1, trans_no, "取消下载");
            //out_str1("秒下成功！");
            return ;
        }
    }


    /*--------------------------*/

    /*非秒下——正常下载数据*/
    /*由客户端根据文件大小计算下载方案*/
    int offset = start_pos;
    int length = 60000;

    QString filename = to_path;
    QFile file(filename);
    file.open(QIODevice::WriteOnly|QIODevice::Append);
    QDataStream out(&file);

    char *recv_buf2 = new char[16+length];

    /*过程中实时更新下载进度*/
    update_trans_table(2, trans_no, QString::number(offset));
    while(offset < file_size)
    {
        pause.lock();
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
        if(tcpsocket->waitForBytesWritten(5000))
        {

        }
        else
        {
            cout<<"network error2"<<endl;
            out_str1("网络连接断开，请重新检查网络连接2！");
            update_trans_table(1, trans_no, "断开连接");
            //exit(-1);
            return ;
        }



        memset(recv_buf2, 0, 16+length);
        if(tcpsocket->waitForReadyRead())
        {
            tcpsocket->read(recv_buf2, 16);
        }
        else
        {
            cout<<"network error3"<<endl;
            out_str1("网络连接断开，请重新检查网络连接3！");
            update_trans_table(1, trans_no, "断开连接");
            //exit(-1);
            return ;
        }
        int this_len = 0;
        int time = 0;
        while(this_len<length)
        {
            /*if(tcpsocket->bytesAvailable()<=0)
                exit(-1);
            if(tcpsocket->bytesAvailable()>0)
                cout<<"bytes:"<<tcpsocket->bytesAvailable()<<endl;*/


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
                    out_str1("网络连接断开，请重新检查网络连接！");
                    update_trans_table(1, trans_no, "断开连接");
                    //QMessageBox::critical(nullptr,"错误","网络连接断开，请重新检查网络连接");
                    //exit(-1);
                    return ;
                    /*while(tcpsocket->bytesAvailable()<=0)//循环检测，30s一次
                    {
                        QTest::qSleep(3000);
                    }*/
                }
            }


            //int res = tcpsocket->read(recv_buf2, length - this_len);
            this_len += res;
        }
        /*一次性写入缓存，减少文件操作的同时便于实际写入数据量与table中的数值相符合，以便恢复使用*/
        out.writeRawData(recv_buf2, length);//直接写入原始数据，防止修改（相当于文件方式的memcpy）
        offset += length;
        update_trans_table(2, trans_no, QString::number(offset));//信号放在循环外面，不然会消耗很多资源




        /*if(tcpsocket->waitForReadyRead())
        {
            this_len = tcpsocket->read(recv_buf2, 16+length);
        }
        else
        {
            out_str1("????????");
            exit(-1);
        }
        out.writeRawData(&(recv_buf2[16]), this_len);//直接写入原始数据，防止修改（相当于文件方式的memcpy）

        offset += this_len;
        update_trans_table(2, trans_no, QString::number(offset));*/
        pause.unlock();
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


    /*下载完成*/
    update_trans_table(1, trans_no, "下载完成");
    out_str1("下载完成！！！");
    file.close();
    tcpsocket->close();
    //ui->textBrowser->append("下载完成！！！");
}

/*QString UTF82GBK(const QString &str)
{
    QTextCodec *gbk = QTextCodec::codecForName("GB18030");
    return gbk->toUnicode(str.toLocal8Bit());
}

QString GBK2UTF8(const QString &str)
{
    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    return utf8->toUnicode(str.toUtf8());
}*/


/*计算本地文件的sha512，run()开头执行一次即可*/
int Upload_Thread::get_sha512()
{
    QString cmd_head = "certutil -hashfile ";
    QString cmd_tail = " SHA512";
    QString cmd_str = cmd_head + from_path + cmd_tail;

    //cmd_str = GBK2UTF8(cmd_str);

    QTextCodec *utf8 = QTextCodec::codecForName("UTF-8");
    QTextCodec* gbk = QTextCodec::codecForName("gbk");

    QString strUnicode= utf8->toUnicode(cmd_str.toLocal8Bit().data());
    //2. unicode -> gbk, 获得QByteArray
    QByteArray gb_bytes= gbk->fromUnicode(strUnicode);
    char *cmd =  gb_bytes.data(); //获取其char *

    char MsgBuff[1024];
    int MsgLen=1020;
    FILE * fp;

    int ret = -1;
    if (cmd == NULL)
    {
        return -2;
    }
    if ((fp = _popen(cmd, "r")) == NULL)
    {
        return -2;
    }
    else
    {
        memset(MsgBuff, 0, MsgLen);

        //读取命令执行过程中的输出
        fgets(MsgBuff, MsgLen, fp);
        fgets(MsgBuff, MsgLen, fp);
        MsgBuff[128] = '\0';

        SHA512_code = MsgBuff;
        ret = 0;


        while (fgets(MsgBuff, MsgLen, fp) != NULL)
        {
            //printf("MsgBuff: %s\n", MsgBuff);
        }

        //关闭执行的进程
        if(_pclose(fp) == -1)
        {
            return -3;
        }
    }
    return ret;
}

void Upload_Thread::get_filedata(QString from_path1, QString to_path1, QString file_name1, int trans_no1, qint64 file_size1, int whe_dir1)
{
    from_path = from_path1;
    to_path = to_path1;
    file_name = file_name1;
    trans_no = trans_no1;
    file_size = file_size1;
    whe_dir = whe_dir1;
}

void Upload_Thread::run()
{
    rate_progress = 0;
    if(whe_dir == 0)
    {
        int sha_ret = get_sha512();
        if(sha_ret!=0)
        {
            out_str1("sha512 error!");
            return ;
        }
    }
    else
    {
        char ch[129];
        memset(ch, 'g', 128);
        ch[128] = '\0';
        SHA512_code = ch;
    }

    QTcpSocket* tcpsocket = new QTcpSocket;
    tcpsocket->setProxy(QNetworkProxy::NoProxy);
    tcpsocket->connectToHost(IP, PORT, QAbstractSocket::ReadWrite, QAbstractSocket::IPv4Protocol);

    QString upload_path = to_path;
    int path_len = upload_path.toStdString().size();
    qDebug()<<"path_len:"<<path_len<<endl;
    /*路径在上文已经获取*/
    /*开始下载*/

    /*发送下载请求*/
    char send_buf1[MAX_LEN];
    memset(send_buf1, 0, MAX_LEN);
    memcpy(send_buf1, "upload", 6);

    string str1 = sessionID.toStdString();
    memcpy(send_buf1 + 16, str1.c_str(), 32);

    string str2 = SHA512_code.toStdString();
    memcpy(send_buf1 + 144, str2.c_str(), 128);


    QString sizex = tr("%1").arg(file_size);
    string str3 = sizex.toStdString();
    memcpy(send_buf1 + 272, str3.c_str(), str3.size());

    string str4 = to_string(path_len);
    memcpy(send_buf1 + 288, str4.c_str(), str4.size());

    string str5 = upload_path.toStdString();
    memcpy(send_buf1 + 304, str5.c_str(), path_len);

    tcpsocket->write(send_buf1, 304 + path_len);
    tcpsocket->waitForBytesWritten();

    /*接收下载文件信息*/
    char recv_buf1[32];
    memset(recv_buf1, 0, 32);
    if(tcpsocket->waitForReadyRead())
    {
        tcpsocket->read(recv_buf1, 32);
    }

    QString recv1 = &(recv_buf1[16]);
    if(recv1 == "done")//秒传
    {
        update_trans_table(1, trans_no, "秒传");
        out_str1("秒传成功！");
        tcpsocket->close();
        return;
    }

    if(whe_dir == 1)//若为文件夹，至此退出
    {
        update_trans_table(1, trans_no, "已创建");
        tcpsocket->close();
        return;
    }

    /*正式开始数据传输*/
    QFile file(from_path);
    if(!file.open(QIODevice::ReadOnly))
    {
        return ;
        qDebug()<<"文件打开出现问题"<<endl;
    }
    QDataStream in(&file);

    char *send_buf3 = new char[1024*1024*2];

    while(1)
    {
        pause.lock();
        /*upready*/
        char send_buf2[144];
        memset(send_buf2, 0, 144);
        memcpy(send_buf2, "upready", 7);

        string str2_1 = SHA512_code.toStdString();
        memcpy(send_buf2 + 16, str2_1.c_str(), 128);

        tcpsocket->write(send_buf2, 144);
        tcpsocket->waitForBytesWritten();

        char recv_buf2[48];
        memset(recv_buf2, 0, 48);
        if(tcpsocket->waitForReadyRead(5000))
        {
            tcpsocket->read(recv_buf2, 48);
        }
        else//断开连接，5s检测
        {
            update_trans_table(1, trans_no, "上传断连");
            qDebug()<<"断开连接1"<<endl;
            return ;
        }

        QString offset_str = &(recv_buf2[16]);
        QString length_str = &(recv_buf2[32]);

        qint64 offset = offset_str.toLongLong();
        int length = length_str.toInt();

        memset(send_buf3, 0, 1024*1024*2);
        memcpy(send_buf3, "updata", 6);

        string str3_2 = SHA512_code.toStdString();
        memcpy(send_buf3 + 16, str3_2.c_str(), 128);

        QString offsetx = tr("%1").arg(offset);
        string str3_3 = offsetx.toStdString();
        memcpy(send_buf3 + 144, str3_3.c_str(), str3_3.size());

        string str3_4 = to_string(length);
        memcpy(send_buf3 + 160, str3_4.c_str(), str3_4.size());

        in.device()->seek(offset);//调整指针位置
        char *chbufx = &(send_buf3[176]);
        in.readRawData(send_buf3 + 176, length);//写入sendbuf中
        file.flush();


        int this_len = 0;//当前已经write的字节数
        while(this_len < (length + 176))
        {
            int wrlen = 176 + length - this_len;
            int res = tcpsocket->write(send_buf3, wrlen);
            //tcpsocket->flush();

            if(tcpsocket->waitForBytesWritten() == false)
            {
                qDebug()<<"waitForBytesWritten!!!!"<<endl;
            }

            if(res == 0)
            {
                qDebug()<<"res!!!!"<<endl;
            }
            qDebug()<<"write res: "<<res;
            qDebug()<<"write wrlen: "<<wrlen;
            qDebug()<<"write offset: "<<offset;
            qDebug()<<"write length: "<<length;
            qDebug()<<"write this_len: "<<this_len;
            qDebug()<<endl;
            this_len += res;
            /*if(tcpsocket->waitForBytesWritten(5000))
            {

            }
            else//
            {
                qDebug()<<"断开连接2"<<endl;
                return ;
            }*/
            pause.unlock();
        }




        char recv_buf3[MAX_LEN];
        memset(recv_buf3, 0, MAX_LEN);
        if(tcpsocket->waitForReadyRead(1000000))
        {
            QByteArray array = tcpsocket->readAll();
            int res = array.size();
            memcpy(recv_buf3, array, res);

            qDebug()<<recv_buf3<<endl;
            qDebug()<<&(recv_buf3[16])<<endl;
        }
        else
        {
            update_trans_table(1, trans_no, "上传断连");
            qDebug()<<"断开连接3"<<endl;
            return ;
        }

        char testbug[200];
        memcpy(testbug, send_buf3, 200);

        rate_progress = max(rate_progress, (offset + length));
        update_trans_table(2, trans_no, QString::number(rate_progress));//信号放在循环外面，不然会消耗很多资源

        char recv3_ch[17];
        memset(recv3_ch, 0, 17);
        memcpy(recv3_ch, &(recv_buf3[16]), 16);

        QString recv3 = recv3_ch;
        if(recv3 == "done")//传输完成
        {
            update_trans_table(1, trans_no, "上传成功");
            update_trans_table(2, trans_no, QString::number(file_size));
            out_str1("上传成功！");
            return;
        }




    }
}

























