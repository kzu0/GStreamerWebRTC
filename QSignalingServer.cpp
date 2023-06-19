#include "QSignalingServer.h"
#include <QDebug>
//-------------------------------------------------------------------------------------------------------------------------------------------
QSignalingServer::QSignalingServer(QString serverName, QWebSocketServer::SslMode secureMode, QObject *parent) : QWebSocketServer(serverName, secureMode, parent) {

    bool ret = listen(QHostAddress::Any, 8080);

    qDebug() << "Signaling Server is Listening:" << ret;

    connect(this, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
    connect(this, SIGNAL(acceptError(QAbstractSocket::SocketError)), this, SLOT(onAcceptError(QAbstractSocket::SocketError)));
}
//-------------------------------------------------------------------------------------------------------------------------------------------
QSignalingServer::~QSignalingServer() {

    close();
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::onNewConnection() {

    QWebSocket* socket = nextPendingConnection();

    connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(onStateChanged(QAbstractSocket::SocketState)));
    connect(socket, SIGNAL(binaryMessageReceived(const QByteArray)), this, SLOT(onBinaryMessageReceived(const QByteArray)));
    connect(socket, SIGNAL(textMessageReceived(const QString)), this, SLOT(onTextMessageReceived(const QString)));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    sockets.push_back(socket);

    QHostAddress address = socket->peerAddress();
    quint16 port = socket->peerPort();

    qDebug() << "New Connection fom address:" << address << "port:" << port;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::onAcceptError(QAbstractSocket::SocketError socketError) {

    qDebug() << socketError;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::onStateChanged(QAbstractSocket::SocketState socketState) {

    QWebSocket* socket = static_cast<QWebSocket*>(QObject::sender());

    switch(socketState) {
    case QAbstractSocket::UnconnectedState:
        sockets.removeOne(socket);
        emit onUnconnect();
        qDebug() << "QAbstractSocket::UnconnectedState";
        break;
    case QAbstractSocket::HostLookupState:
        qDebug() << "QAbstractSocket::HostLookupState";
        break;
    case QAbstractSocket::ConnectingState:
        qDebug() << "QAbstractSocket::ConnectingState";
        break;
    case QAbstractSocket::ConnectedState:
        qDebug() << "QAbstractSocket::ConnectedState";
        break;
    case QAbstractSocket::BoundState:
        qDebug() << "QAbstractSocket::BoundState";
        break;
    case QAbstractSocket::ListeningState:
        qDebug() << "QAbstractSocket::ListeningState";
        break;
    case QAbstractSocket::ClosingState:
        qDebug() << "QAbstractSocket::ClosingState";
        break;
    }

    Q_UNUSED(socket);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::onBinaryMessageReceived(const QByteArray &message) {

    qDebug() << "onBinaryMessageReceived";
    qDebug() << message;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::onTextMessageReceived(const QString &message) {

    QWebSocket* socket = static_cast<QWebSocket*>(QObject::sender());

    qDebug() << "onTextMessageReceived";
    qDebug() << message;

    if (message == "HELLO") {
        socket->sendTextMessage("HELLO");
        return;
    }

    if (message == "READY") {
        emit onOfferRequest();
        return;
    }

    //
    // JSON messages
    //
    json_object *json_msg = json_tokener_parse(message.toUtf8().data());

    json_object* json_sdp;
    json_object* json_type;

    if (json_object_object_get_ex(json_msg, "sdp", &json_sdp) and json_object_object_get_ex(json_msg, "type", &json_type)) {
        const char *type = json_object_get_string(json_type);
        if (strcmp(type, "answer") == 0) {
            emit onAnswer(message);
        }
    }

    json_object_put(json_msg);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::onError(QAbstractSocket::SocketError error) {

    qDebug() << "onError";
    qDebug() << error;
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::OnOffer(QString offer) {

    //qDebug() << offer;
    //sockets.at(0)->sendTextMessage(offer);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::OnIceCandidate(QString candidate) {

    //qDebug() << candidate;
    //sockets.at(0)->sendTextMessage(candidate);
}
//-------------------------------------------------------------------------------------------------------------------------------------------
void QSignalingServer::OnOfferReady(QString offer) {

    if (sockets.isEmpty()) {
        return;
    }

    sockets.at(0)->sendTextMessage(offer);
}



