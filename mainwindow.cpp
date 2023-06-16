#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {

    ui->setupUi(this);

    stream = new QGstWebRTCObject(this);
    connect(stream, SIGNAL(onOffer(QString)), this, SLOT(OnOffer(QString)));
    connect(stream, SIGNAL(onIceCandidate(QString)), this, SLOT(OnIceCandidate(QString)));

    server = new QSignalingServer("Signaling Server", QWebSocketServer::NonSecureMode, this);
    connect(server, SIGNAL(onOfferRequest()), stream, SLOT(OnOfferRequest()));
    connect(server, SIGNAL(onAnswer(QString)), stream, SLOT(OnAnswer(QString)));
    connect(stream, SIGNAL(onOfferReady(QString)), server, SLOT(OnOfferReady(QString)));
}

MainWindow::~MainWindow() {

    delete ui;
}

void MainWindow::on_initPeerConnectionPushButton_clicked() {

    stream->InstantiatePipeline();
}

void MainWindow::on_createOfferPushButton_clicked() {

    QString offer = stream->ComposeOffer();
    ui->offerSdpLineEdit->setText(offer);
}

void MainWindow::on_createAnswerPushButton_clicked() {

    //...
}

void MainWindow::on_addAnswerPushButton_clicked() {

    QString answer = ui->answerSdpLineEdit->text();
    stream->SetRemoteDescription(answer);
}

void MainWindow::OnOffer(QString offer) {

    //...
}

void MainWindow::OnIceCandidate(QString candidate) {

    QString str = ui->iceCandidatesLineEdit->text();
    str.append(candidate);
    ui->iceCandidatesLineEdit->setText(str);
}

