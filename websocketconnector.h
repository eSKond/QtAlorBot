#ifndef WEBSOCKETCONNECTOR_H
#define WEBSOCKETCONNECTOR_H

#include <QObject>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

class WebSocketConnector : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketConnector(QObject *parent = nullptr);

    bool isWsConnected();
    bool wsConnect(QString surl, QString jwt);
    void wsDisconnect();

    QUuid barsGetAndSubscribe(QString ticker, int timeFrame, QDateTime startFromUTC, QString exch);
    QUuid positionsGetAndSubscribe(QString portfolio, QString exch);
private:
    QString m_jwtToken;
    QWebSocket *m_ws;
    //We can implement frame by frame, see commented code to know how to do that, but it's enought just wait full message
    //QByteArray framecollection;
    //QString stringframecollection;
    QMap<QUuid, QString> barSubscriptions;
    QMap<QUuid, QString> barSubscriptionRequests;
    QList<QUuid> positionSubscriptions;
    QList<QUuid> positionSubscriptionRequests;

    void processArrivedJson(QJsonDocument &jsonDoc);
    bool sendJsonObject(QJsonObject &obj);
public slots:
    void wsConnected();
    void wsDisconnected();
    //void wsBinaryFrameReceived(const QByteArray &frame, bool isLastFrame);
    void wsBinaryMessageReceived(const QByteArray &message);
    //void wsTextFrameReceived(const QString &frame, bool isLastFrame);
    void wsTextMessageReceived(const QString &message);
    void wsError(QAbstractSocket::SocketError error);
    void wsSslErrors(const QList<QSslError> &errors);

signals:
    void webSocketClosed();
    void webSocketReady();
    void barReceived(QUuid id, QString ticker, QDateTime dt, double o, double h, double l, double c, int v);
    void positionUpdated(QUuid id, QString symb, double avgP, int qty, int open, int plannedBuy, int plannedSell, int unrealisedPl);
};

#endif // WEBSOCKETCONNECTOR_H
