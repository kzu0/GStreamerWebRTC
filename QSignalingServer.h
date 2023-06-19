#ifndef QSIGNALINGSERVER_H
#define QSIGNALINGSERVER_H
//-------------------------------------------------------------------------------------------------------------------------------------------
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QHostAddress>
#include <QTimer>
#include "json-c/json.h"
//-------------------------------------------------------------------------------------------------------------------------------------------
class QSignalingServer : public QWebSocketServer {

    Q_OBJECT

public:
    explicit QSignalingServer(QString serverName = "Signaling Server", QWebSocketServer::SslMode secureMode = QWebSocketServer::NonSecureMode, QObject *parent = nullptr);
    ~QSignalingServer();

private:
    QList<QWebSocket*> sockets;

public slots:
    //
    // Server
    //
    void onNewConnection();
    void onAcceptError(QAbstractSocket::SocketError socketError);

    //
    // Socket
    //
    void onStateChanged(QAbstractSocket::SocketState socketState);
    void onBinaryMessageReceived(const QByteArray &message);
    void onTextMessageReceived(const QString &message);
    void onError(QAbstractSocket::SocketError error);

private slots:
    void OnOffer(QString);
    void OnIceCandidate(QString);

    void OnOfferReady(QString);

signals:
    void onOfferRequest();
    void onAnswer(QString);

    void onUnconnect();
};

#endif // QSIGNALINGSERVER_H
