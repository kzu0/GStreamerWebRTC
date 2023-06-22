#include "mainwindow.h"
#include <QApplication>
#include <QObject>
#include <QGstWebRTCObject.h>
#include <QSignalingServer.h>

// file:///home/moscara/Scrivania/WEBRTC/HTML/index.html

int main(int argc, char *argv[]) {

		QCoreApplication a(argc, argv);

		QGstWebRTCObject* stream = new QGstWebRTCObject(nullptr);

		QSignalingServer* server = new QSignalingServer("Signaling Server", QWebSocketServer::NonSecureMode, nullptr);
		QObject::connect(server, SIGNAL(onOfferRequest()), stream, SLOT(OnOfferRequest()));
		QObject::connect(server, SIGNAL(onUnconnect()), stream, SLOT(OnUnconnect()));
		QObject::connect(server, SIGNAL(onAnswer(QString)), stream, SLOT(OnAnswer(QString)));
		QObject::connect(stream, SIGNAL(onOfferReady(QString)), server, SLOT(OnOfferReady(QString)));

    return a.exec();
}
