#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGstWebRTCObject.h>
#include <QSignalingServer.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_initPeerConnectionPushButton_clicked();
    void on_createOfferPushButton_clicked();
    void on_createAnswerPushButton_clicked();
    void on_addAnswerPushButton_clicked();

private:
    Ui::MainWindow *ui;

    QGstWebRTCObject* stream;
    QSignalingServer* server;

private slots:
    void OnOffer(QString);
    void OnIceCandidate(QString);
};

#endif // MAINWINDOW_H
