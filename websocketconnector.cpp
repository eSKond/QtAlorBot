#include "websocketconnector.h"

WebSocketConnector::WebSocketConnector(QObject *parent)
    : QObject(parent), m_ws(nullptr)
{

}

bool WebSocketConnector::isWsConnected()
{
    if(m_ws && m_ws->state()==QAbstractSocket::ConnectedState)
        return true;
    return false;
}

bool WebSocketConnector::wsConnect(QString surl, QString jwt)
{
    if(m_ws)
        return false;
    m_jwtToken = jwt;
    m_ws=new QWebSocket();
    connect(m_ws, &QWebSocket::connected, this, &WebSocketConnector::wsConnected);
    connect(m_ws, &QWebSocket::disconnected, this, &WebSocketConnector::wsDisconnected);
    //connect(m_ws, &QWebSocket::binaryFrameReceived, this, &WebSocketConnector::wsBinaryFrameReceived);
    connect(m_ws, &QWebSocket::binaryMessageReceived, this, &WebSocketConnector::wsBinaryMessageReceived);
    //connect(m_ws, &QWebSocket::textFrameReceived, this, &WebSocketConnector::wsTextFrameReceived);
    connect(m_ws, &QWebSocket::textMessageReceived, this, &WebSocketConnector::wsTextMessageReceived);
    connect(m_ws, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(wsError(QAbstractSocket::SocketError)));
    connect(m_ws, &QWebSocket::sslErrors, this, &WebSocketConnector::wsSslErrors);
    //Если нужно подправить заголовки, то лучше использовать QNetworkRequest
    //QNetworkRequest request(QUrl(surl));
    //m_ws->open(request);
    //Но в нашем случае достаточно url
    m_ws->open(QUrl(surl));
    return true;
}

void WebSocketConnector::wsDisconnect()
{
    if(m_ws)
    {
        m_ws->disconnect();
        m_ws->deleteLater();
        m_ws=nullptr;
    }
}

QUuid WebSocketConnector::barsGetAndSubscribe(QString ticker, int timeFrame, QDateTime startFromUTC, QString exch)
{
    QUuid uiid = QUuid::createUuid();
    QString reqUiid = uiid.toString();
    reqUiid = reqUiid.mid(1, reqUiid.length()-2);
    QJsonObject robj;
    robj["opcode"] = "BarsGetAndSubscribe";
    robj["code"] = ticker.toUpper();
    robj["tf"] = timeFrame;
    robj["from"] = startFromUTC.toSecsSinceEpoch();
    robj["exchange"] = exch.toUpper();
    robj["format"] = "TV";
    robj["delayed"] = false;
    robj["guid"] = reqUiid;
    sendJsonObject(robj);
    barSubscriptionRequests.insert(uiid, ticker);
    return uiid;
}

QUuid WebSocketConnector::positionsGetAndSubscribe(QString portfolio, QString exch)
{
    QUuid uiid = QUuid::createUuid();
    QString reqUiid = uiid.toString();
    reqUiid = reqUiid.mid(1, reqUiid.length()-2);
    QJsonObject robj;
    robj["opcode"] = "PositionsGetAndSubscribe";
    robj["portfolio"] = portfolio.toUpper();
    robj["exchange"] = exch.toUpper();
    robj["format"] = "TV";
    robj["guid"] = reqUiid;
    sendJsonObject(robj);
    positionSubscriptionRequests.append(uiid);
    return uiid;
}

void WebSocketConnector::processArrivedJson(QJsonDocument &jsonDoc)
{
    //QString arrivedText = QString::fromLocal8Bit(jsonDoc.toJson());
    //qDebug() << "Arrived:";
    //qDebug().noquote() << QString("\n%1\n").arg(arrivedText);
    if(jsonDoc.isObject())
    {
        QJsonObject robj = jsonDoc.object();
        if(robj.contains("requestGuid"))
        {
            QUuid rguid = QUuid::fromString(robj["requestGuid"].toString());
            if(barSubscriptionRequests.contains(rguid))
            {
                QString ticker = barSubscriptionRequests.take(rguid);
                if(robj.contains("httpCode") && robj["httpCode"].toInt()==200
                        //&& robj.contains("message") && robj["message"]=="Handled successfully" //it's not best way to do things
                        )
                    barSubscriptions.insert(rguid, ticker);
            }
            else
            {
                if(positionSubscriptionRequests.contains(rguid))
                {
                    positionSubscriptionRequests.removeAll(rguid);
                    if(robj.contains("httpCode") && robj["httpCode"].toInt()==200
                            //&& robj.contains("message") && robj["message"]=="Handled successfully" //it's not best way to do things
                            )
                        positionSubscriptions.append(rguid);
                }
            }
        }
        else
        {
            if(robj.contains("guid"))
            {
                QUuid rguid = QUuid::fromString(robj["guid"].toString());
                if(barSubscriptionRequests.contains(rguid)) //subscription answer didn't arrive but data is going
                {
                    QString ticker = barSubscriptionRequests.take(rguid);
                    barSubscriptions.insert(rguid, ticker);
                }
                if(positionSubscriptionRequests.contains(rguid)) //subscription answer didn't arrive but data is going
                {
                    positionSubscriptionRequests.removeAll(rguid);
                    positionSubscriptions.append(rguid);
                }
                if(barSubscriptions.contains(rguid) && robj.contains("data"))
                {
                    QJsonObject dobj = robj["data"].toObject();
                    double dbltm = dobj["time"].toDouble();
                    QDateTime dtm = QDateTime::fromMSecsSinceEpoch((qlonglong)dbltm);
                    double o, h, l, c;
                    int v;
                    o = dobj["open"].toDouble();
                    h = dobj["high"].toDouble();
                    l = dobj["low"].toDouble();
                    c = dobj["close"].toDouble();
                    v = dobj["volume"].toInt();
                    emit barReceived(rguid, barSubscriptions.value(rguid), dtm, o, h, l, c, v);
                }
                else
                {
                    if(positionSubscriptions.contains(rguid) && robj.contains("data"))
                    {
                        QJsonObject dobj = robj["data"].toObject();
                        QString symb;
                        double avgp, unrealPL;
                        int qty, open, plannedBuy, plannedSell;
                        symb = dobj["symbol"].toString();
                        avgp = dobj["avgPrice"].toDouble();
                        qty = dobj["qty"].toInt();
                        open = dobj["open"].toInt();
                        plannedBuy = dobj["plannedBuy"].toInt();
                        plannedSell = dobj["plannedSell"].toInt();
                        unrealPL = dobj["unrealisedPl"].toDouble();
                        emit positionUpdated(rguid, symb, avgp, qty, open, plannedBuy, plannedSell, unrealPL);
                    }
                }
            }
        }
    }
}

bool WebSocketConnector::sendJsonObject(QJsonObject &obj)
{
    obj["token"] = m_jwtToken;
    QJsonDocument doc(obj);
    if(!m_ws)
        return false;
    QString sendText = QString::fromLocal8Bit(doc.toJson());
    qDebug() << "Sending:";
    qDebug().noquote() << QString("\n%1\n").arg(sendText);
    m_ws->sendTextMessage(sendText);
    return true;
}

void WebSocketConnector::wsConnected()
{
    emit webSocketReady();
}

void WebSocketConnector::wsDisconnected()
{
    emit webSocketClosed();
}

//void WebSocketConnector::wsBinaryFrameReceived(const QByteArray &frame, bool isLastFrame)
//{
//    framecollection.append(frame);
//    if(isLastFrame)
//    {
//        wsBinaryMessageReceived(framecollection);
//        framecollection.clear();
//    }
//}

void WebSocketConnector::wsBinaryMessageReceived(const QByteArray &message)
{
    QJsonDocument jsonDoc=QJsonDocument::fromJson(message);
    processArrivedJson(jsonDoc);
}

//void WebSocketConnector::wsTextFrameReceived(const QString &frame, bool isLastFrame)
//{
//    stringframecollection.append(frame);
//    if(isLastFrame)
//    {
//        wsTextMessageReceived(stringframecollection);
//        stringframecollection.clear();
//    }
//}

void WebSocketConnector::wsTextMessageReceived(const QString &message)
{
    QJsonDocument jsonDoc=QJsonDocument::fromJson(message.toUtf8());
    processArrivedJson(jsonDoc);
}

void WebSocketConnector::wsError(QAbstractSocket::SocketError error)
{
    qDebug() << "Hey, bro, there is some error!" << (int)error;
}

void WebSocketConnector::wsSslErrors(const QList<QSslError> &errors)
{
    qDebug() << "ssl errors (" << errors.count() << ")";
}
