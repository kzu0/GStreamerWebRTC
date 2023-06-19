#include "mainwindow.h"
#include <QApplication>

// file:///home/moscara/Scrivania/WEBRTC/HTML/index.html

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
