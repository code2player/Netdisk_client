#include "mainwindow.h"
#include "loginwindow.h"


#include <QApplication>
#include <QCoreApplication>
#include <QTextCodec>
int main(int argc, char *argv[])
{

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    QApplication a(argc, argv);

    MainWindow w;
    loginwindow login;
    //registerwindow register;
    if (login.exec() == QDialog::Accepted) {	// 如果符合登录条件，login执行后，必返回QDialog::Accepted
        w.recv_loginwindow();
        w.show();	// 主界面显示
        return a.exec();
    }
}
