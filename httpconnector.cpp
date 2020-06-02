#include "httpconnector.h"

OrderData *OrderData::createOrderFromJson(const QJsonObject &ordObj)
{
    OrderData *nord = new OrderData();
    nord->id = ordObj["id"].toInt();
    QString otype = ordObj["type"].toString().toLower();
    if(otype == "market")
        nord->type = OrderData::Market;
    else
    {
        if(otype == "limit")
            nord->type = OrderData::Limit;
        else
        {
            if(otype.startsWith("stop"))
            {
                if(otype.endsWith("limit"))
                    nord->type = OrderData::StopLossLimit;
                else
                    nord->type = OrderData::StopLoss;
            }
            else
            {
                if(ordObj.contains("price"))
                    nord->type = OrderData::TakeProfitLimit;
                else
                    nord->type = OrderData::TakeProfit;
            }
        }
    }
    if(ordObj["side"].toString().toLower()=="buy")
        nord->side = OrderData::Buy;
    else
        nord->side = OrderData::Sell;
    nord->qty = ordObj["qty"].toInt();
    if(ordObj.contains("price"))
        nord->price = ordObj["price"].toDouble();
    else
        nord->price = 0;
    if(ordObj.contains("stopPrice"))
        nord->trigPrice = ordObj["stopPrice"].toDouble();
    else
    {
        if(ordObj.contains("TriggerPrice"))
            nord->trigPrice = ordObj["TriggerPrice"].toDouble();
        else
            nord->trigPrice = 0;
    }
    nord->account = QString();
    nord->portfolio = QString();
    nord->exch = QString();
    nord->symb = ordObj["symbol"].toString();
    QString statStr = ordObj["status"].toString().toLower().trimmed();
    if(statStr == "working")
        nord->status = OrderData::Working;
    else if(statStr == "filled")
        nord->status = OrderData::Filled;
    else if(statStr == "canceled")
        nord->status = OrderData::Canceled;
    else
        nord->status = OrderData::Rejected;
    nord->filledQty = ordObj["filledQty"].toInt();
    return nord;
}

OrderData *OrderData::createMarketOrder(QString acc, QString portf, QString aexch, QString asymb, OrderData::OrderSide aside, int aqty)
{
    OrderData *nord = new OrderData();
    nord->id = 0;
    nord->type = OrderData::Market;
    nord->side = aside;
    nord->qty = aqty;
    nord->price = 0;
    nord->trigPrice = 0;
    nord->account = acc;
    nord->portfolio = portf;
    nord->exch = aexch;
    nord->symb = asymb;
    nord->status = OrderData::Sent;
    nord->filledQty = 0;
    return nord;
}

OrderData *OrderData::createLimitOrder(QString acc, QString portf, QString aexch, QString asymb, OrderData::OrderSide aside, int aqty, double price)
{
    OrderData *nord = new OrderData();
    nord->id = 0;
    nord->type = OrderData::Limit;
    nord->side = aside;
    nord->qty = aqty;
    nord->price = price;
    nord->trigPrice = 0;
    nord->account = acc;
    nord->portfolio = portf;
    nord->exch = aexch;
    nord->symb = asymb;
    nord->status = OrderData::Sent;
    nord->filledQty = 0;
    return nord;
}

OrderData *OrderData::createStopLossOrder(QString acc, QString portf, QString aexch, QString asymb, OrderData::OrderSide aside, int aqty, double tprice)
{
    OrderData *nord = new OrderData();
    nord->id = 0;
    nord->type = OrderData::StopLoss;
    nord->side = aside;
    nord->qty = aqty;
    nord->price = 0;
    nord->trigPrice = tprice;
    nord->account = acc;
    nord->portfolio = portf;
    nord->exch = aexch;
    nord->symb = asymb;
    nord->status = OrderData::Sent;
    nord->filledQty = 0;
    return nord;
}

OrderData::OrderStatus OrderData::stringToStatus(QString sstat)
{
    sstat = sstat.toLower();
    if(sstat == "working")
        return OrderData::Working;
    if(sstat == "filled")
        return OrderData::Filled;
    if(sstat == "canceled")
        return OrderData::Canceled;
    if(sstat == "rejected")
        return OrderData::Rejected;
    //really we can't receive Sent, but let use it as "unknown"
    return OrderData::Sent;
}

HttpConnector::HttpConnector(QString wturl, QString wurl, QObject *parent)
    : QObject(parent), warpTransUrl(wturl), warpUrl(wurl)
{
    m_networkManager = new QNetworkAccessManager(this);
    //orderUpdater.setInterval(100);//100ms
    //orderUpdater.setSingleShot(true);
    //connect(&t_watchdog, SIGNAL(timeout()), m_t_activeReply, SLOT(abort()));
    //t_watchdog.start();
}

QString HttpConnector::newXAlorReqid()
{
    return QUuid::createUuid().toString().mid(20, 11);
}

QString HttpConnector::placeMarketOrderBuy(QString acc, QString portf, QString tradeServerCode, int vol, QString exch, QString ticker)
{
    qDebug() << ">> MARKET BUY";
    QString rurl = QString("%1/%2/v2/client/orders/actions/market").arg(warpTransUrl).arg(tradeServerCode);
    QJsonObject uobj, iobj, robj;
    uobj["Account"] = acc;
    uobj["Portfolio"] = portf;
    iobj["Symbol"] = ticker;
    iobj["Exchange"] = exch;
    robj["Quantity"] = vol;
    robj["Side"] = "buy";
    robj["Instrument"] = iobj;
    robj["User"] = uobj;
    robj["OrderEndUnixTime"] = 0;
    OrderData *order = OrderData::createMarketOrder(acc, portf, exch, ticker, OrderData::Buy, vol);
    QString reqId = httpPOSTJsonObject(rurl, robj);
    m_place_order_requests.insert(reqId, order);
    return reqId;
}

QString HttpConnector::placeMarketOrderSell(QString acc, QString portf, QString tradeServerCode, int vol, QString exch, QString ticker)
{
    qDebug() << ">> MARKET SELL";
    QString rurl = QString("%1/%2/v2/client/orders/actions/market").arg(warpTransUrl).arg(tradeServerCode);
    QJsonObject uobj, iobj, robj;
    uobj["Account"] = acc;
    uobj["Portfolio"] = portf;
    iobj["Symbol"] = ticker;
    iobj["Exchange"] = exch;
    robj["Quantity"] = vol;
    robj["Side"] = "sell";
    robj["Instrument"] = iobj;
    robj["User"] = uobj;
    robj["OrderEndUnixTime"] = 0;
    OrderData *order = OrderData::createMarketOrder(acc, portf, exch, ticker, OrderData::Sell, vol);
    QString reqId = httpPOSTJsonObject(rurl, robj);
    m_place_order_requests.insert(reqId, order);
    return reqId;
}

QString HttpConnector::placeLimitOrderBuy(QString acc, QString portf, QString tradeServerCode, double targBuyPrice, int vol, QString exch, QString ticker)
{
    qDebug() << ">> LIMIT BUY";
    QString rurl = QString("%1/%2/v2/client/orders/actions/limit").arg(warpTransUrl).arg(tradeServerCode);
    QJsonObject uobj, iobj, robj;
    uobj["Account"] = acc;
    uobj["Portfolio"] = portf;
    iobj["Symbol"] = ticker;
    iobj["Exchange"] = exch;
    robj["Quantity"] = vol;
    robj["Side"] = "buy";
    robj["Instrument"] = iobj;
    robj["User"] = uobj;
    robj["OrderEndUnixTime"] = 0;
    robj["Price"] = targBuyPrice;
    OrderData *order = OrderData::createLimitOrder(acc, portf, exch, ticker, OrderData::Buy, vol, targBuyPrice);
    QString reqId = httpPOSTJsonObject(rurl, robj);
    m_place_order_requests.insert(reqId, order);
    return reqId;
}

QString HttpConnector::placeLimitOrderSell(QString acc, QString portf, QString tradeServerCode, double targSellPrice, int vol, QString exch, QString ticker)
{
    qDebug() << ">> LIMIT SELL";
    QString rurl = QString("%1/%2/v2/client/orders/actions/limit").arg(warpTransUrl).arg(tradeServerCode);
    QJsonObject uobj, iobj, robj;
    uobj["Account"] = acc;
    uobj["Portfolio"] = portf;
    iobj["Symbol"] = ticker;
    iobj["Exchange"] = exch;
    robj["Quantity"] = vol;
    robj["Side"] = "sell";
    robj["Instrument"] = iobj;
    robj["User"] = uobj;
    robj["OrderEndUnixTime"] = 0;
    robj["Price"] = targSellPrice;
    OrderData *order = OrderData::createLimitOrder(acc, portf, exch, ticker, OrderData::Sell, vol, targSellPrice);
    QString reqId = httpPOSTJsonObject(rurl, robj);
    m_place_order_requests.insert(reqId, order);
    return reqId;
}

QString HttpConnector::placeStopLossOrderBuy(QString acc, QString portf, QString tradeServerCode, double trigBuyPrice, int vol, QString exch, QString ticker)
{
    qDebug() << ">> STOP LOSS BUY";
    QString rurl = QString("%1/%2/v2/client/orders/actions/stoploss").arg(warpTransUrl).arg(tradeServerCode);
    QJsonObject uobj, iobj, robj;
    uobj["Account"] = acc;
    uobj["Portfolio"] = portf;
    iobj["Symbol"] = ticker;
    iobj["Exchange"] = exch;
    robj["Quantity"] = vol;
    robj["Side"] = "buy";
    robj["Instrument"] = iobj;
    robj["User"] = uobj;
    robj["OrderEndUnixTime"] = 0;
    robj["TriggerPrice"] = trigBuyPrice;
    OrderData *order = OrderData::createStopLossOrder(acc, portf, exch, ticker, OrderData::Buy, vol, trigBuyPrice);
    QString reqId = httpPOSTJsonObject(rurl, robj);
    m_place_order_requests.insert(reqId, order);
    return reqId;
}

QString HttpConnector::placeStopLossOrderSell(QString acc, QString portf, QString tradeServerCode, double trigSellPrice, int vol, QString exch, QString ticker)
{
    qDebug() << ">> STOP LOSS SELL";
    QString rurl = QString("%1/%2/v2/client/orders/actions/stoploss").arg(warpTransUrl).arg(tradeServerCode);
    QJsonObject uobj, iobj, robj;
    uobj["Account"] = acc;
    uobj["Portfolio"] = portf;
    iobj["Symbol"] = ticker;
    iobj["Exchange"] = exch;
    robj["Quantity"] = vol;
    robj["Side"] = "sell";
    robj["Instrument"] = iobj;
    robj["User"] = uobj;
    robj["OrderEndUnixTime"] = 0;
    robj["TriggerPrice"] = trigSellPrice;
    OrderData *order = OrderData::createStopLossOrder(acc, portf, exch, ticker, OrderData::Sell, vol, trigSellPrice);
    QString reqId = httpPOSTJsonObject(rurl, robj);
    m_place_order_requests.insert(reqId, order);
    return reqId;
}

void HttpConnector::deleteOrder(QString tradeServerCode, OrderData *order)
{
    QString rurl = QString("%1/%2/v2/client/orders/%3?portfolio=%4&stop=%5").arg(warpTransUrl).arg(tradeServerCode).arg(order->id).
            arg(order->portfolio).arg(order->type==OrderData::StopLoss?"true":"false");
    QString reqid = httpDELETE(rurl);
    qDebug() << "Order(" << order->id << ", " << (order->type==OrderData::StopLoss?"STP":(order->type==OrderData::Limit?"LIM":"MKT")) << ") delete request (" << reqid << "):";
    qDebug().noquote() << rurl;
    m_order_delete_requests.insert(reqid, order->id);
}

void HttpConnector::updateOrderState(OrderData *order)
{
    QString rurl = QString("%1/clients/%2/%3/%5/%4").
            arg(warpUrl).arg(order->exch).arg(order->portfolio).arg(order->id).
            arg(order->type==OrderData::StopLoss?"stoporders":"orders");
    QString reqid = httpGET(rurl);
    qDebug() << "Order(" << order->id << ", " << (order->type==OrderData::StopLoss?"STP":(order->type==OrderData::Limit?"LIM":"MKT")) << ") update request (" << reqid << "):";
    qDebug().noquote() << rurl;
    m_order_state_requests.append(reqid);
}

bool HttpConnector::getOrderList(QString exch, QString portf, bool incOrdinaryOrders, bool incStopOrders)
{
    if(!ordinaryOrderListReqId.isEmpty() || !stoplossOrderListReqId.isEmpty())
        return false;
    while(!m_order_list.isEmpty())
    {
        delete m_order_list.takeLast();
    }
    if(incOrdinaryOrders)
    {
        QString ourl = QString("%1/clients/%2/%3/orders").arg(warpUrl).arg(exch).arg(portf);
        ordinaryOrderListReqId = httpGET(ourl);
    }
    if(incStopOrders)
    {
        QString surl = QString("%1/clients/%2/%3/stoporders").arg(warpUrl).arg(exch).arg(portf);
        stoplossOrderListReqId = httpGET(surl);
    }
    return true;
}

QString HttpConnector::httpPOSTJsonObject(QString url, QJsonObject &obj)
{
    RequestData *rdata = new RequestData(RequestData::HTTP_POST);
    QNetworkRequest request;
    QNetworkReply *reply;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "QtAlorBot");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_jwtToken).toLatin1());
    QString reqid = newXAlorReqid();
    request.setRawHeader("X-ALOR-REQID", reqid.toLatin1());
    request.setUrl(url);
    QJsonDocument doc(obj);
    reply = m_networkManager->post(request, doc.toJson());
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    //let not use watchdog here
    rdata->reqId = reqid;
    rdata->reply = reply;
    rdata->reqObj = obj;
    m_active_requests.append(rdata);
    return reqid;
}

QString HttpConnector::httpGET(QString url)
{
    RequestData *rdata = new RequestData(RequestData::HTTP_GET);
    QNetworkRequest request;
    QNetworkReply *reply;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "QtAlorBot");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_jwtToken).toLatin1());
    QString reqid = newXAlorReqid(); //we don't need it here, but to make all requests processing similar...
    //request.setRawHeader("X-ALOR-REQID", reqid);
    request.setUrl(url);
    //QJsonDocument doc(obj);
    reply = m_networkManager->get(request);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    //let not use watchdog here
    rdata->reqId = reqid;
    rdata->reply = reply;
    rdata->reqObj = QJsonObject();//obj;
    m_active_requests.append(rdata);
    return reqid;
}

QString HttpConnector::httpDELETE(QString url)
{
    RequestData *rdata = new RequestData(RequestData::HTTP_DELETE);
    QNetworkRequest request;
    QNetworkReply *reply;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "QtAlorBot");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_jwtToken).toLatin1());
    QString reqid = newXAlorReqid(); //we don't need it here, but to make all requests processing similar...
    request.setRawHeader("X-ALOR-REQID", reqid.toLatin1());
    request.setUrl(url);
    //QJsonDocument doc(obj);
    reply = m_networkManager->deleteResource(request);
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    //let not use watchdog here
    rdata->reqId = reqid;
    rdata->reply = reply;
    rdata->reqObj = QJsonObject();//obj;
    m_active_requests.append(rdata);
    return reqid;
}

void HttpConnector::processArrivedJsonObject(QString reqId, QJsonObject &reqObj, const QJsonObject &respObj)
{
    Q_UNUSED(reqObj);

    if(m_place_order_requests.contains(reqId))
    {
        OrderData *order = m_place_order_requests.take(reqId);
        processPlaceOrderResponse(reqId, order, respObj);
    }
    else
    {
        if(m_order_state_requests.contains(reqId))
        {
            m_order_state_requests.removeAll(reqId);
            processUpdateOrderState(respObj);
        }
        else
        {
            if(m_order_delete_requests.contains(reqId))
            {
                int delOrdId = m_order_delete_requests.value(reqId);
                emit orderDeleted(delOrdId);
            }
        }
    }
}

void HttpConnector::processArrivedJsonArray(QString reqId, QJsonObject &reqObj, const QJsonArray &respArr)
{
    Q_UNUSED(reqObj);

    if(m_order_state_requests.contains(reqId))
    {
        m_order_state_requests.removeAll(reqId);
        int i;
        for(i=0;i<respArr.count();i++)
            processUpdateOrderState(respArr.at(i).toObject());
    }
    else
    {
        if(reqId == ordinaryOrderListReqId)
        {
            ordinaryOrderListReqId.clear();
            processOrderListResponse(respArr);
        }
        if(reqId == stoplossOrderListReqId)
        {
            stoplossOrderListReqId.clear();
            processOrderListResponse(respArr);
        }
    }
}

void HttpConnector::processArrivedError(int httpCode, QString errmsg, QString reqId, const QJsonObject &reqObj)
{
    Q_UNUSED(reqObj);
    qDebug() << "HTTP error " << httpCode << " on request " << reqId;
    if(m_place_order_requests.contains(reqId))
    {
        delete m_place_order_requests.take(reqId);
        qDebug() << "Couldn't place order because: " << errmsg;
        emit orderPlaceRejected(reqId);
    }
}

void HttpConnector::processPlaceOrderResponse(QString reqId, OrderData *order, const QJsonObject &respObj)
{
    QString msg = respObj["message"].toString();
    order->id = respObj["orderNumber"].toInt();
    m_orders.insert(order->id, order);
    emit serverMessage(msg);
    emit orderPlaced(reqId, order);
    updateOrderState(order);
}

void HttpConnector::processUpdateOrderState(const QJsonObject &respObj)
{
    int oid = respObj["id"].toInt();
    if(m_orders.contains(oid))
    {
        OrderData *order = m_orders.value(oid);
        OrderData::OrderStatus nstat = OrderData::stringToStatus(respObj["status"].toString());
        if(order->status != nstat)
            order->status = nstat;
        order->price = respObj["price"].toDouble();
        order->qty = respObj["qty"].toInt();
        order->filledQty = respObj["filledQty"].toInt();
        if(nstat == OrderData::Working) //Rest of cases doesn't require checking more
        {
            //check later again
            m_order_update_que.append(order);
            QTimer::singleShot(100, this, SLOT(checkOrdersStatus()));
        }
        emit orderStatusChanged(order);
    }
}

RequestData * HttpConnector::findRequestByReply(QNetworkReply *r)
{
    int i;
    for(i=0;i<m_active_requests.count();i++)
    {
        RequestData *res = m_active_requests.at(i);
        if(res->reply == r)
            return res;
    }
    return nullptr;
}

void HttpConnector::processOrderListResponse(const QJsonArray &respArr)
{
    int i;
    for(i=0;i<respArr.count();i++)
    {
        QJsonObject oobj = respArr.at(i).toObject();
        OrderData *order = OrderData::createOrderFromJson(oobj);
        m_order_list.append(order);
    }
    if(ordinaryOrderListReqId.isEmpty() && stoplossOrderListReqId.isEmpty())
        emit orderListReceived(m_order_list);
}

void HttpConnector::sslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
}

void HttpConnector::error(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
}

void HttpConnector::finished()
{
    QNetworkReply *sndr = qobject_cast<QNetworkReply *>(sender());
    RequestData * rdata = findRequestByReply(sndr);
    if(rdata)
    {
        m_active_requests.removeAll(rdata);
        rdata->buf.append(rdata->reply->readAll());
        QJsonDocument jdoc=QJsonDocument::fromJson(rdata->buf);
        if(sndr->error() == QNetworkReply::NoError)
        {
            if(jdoc.isArray())
                processArrivedJsonArray(rdata->reqId, rdata->reqObj, jdoc.array());
            else
            {
                processArrivedJsonObject(rdata->reqId, rdata->reqObj, jdoc.object());
            }
        }
        else
        {
            QString msg;
            int httpCode = sndr->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if(httpCode == 400)
                msg = jdoc["message"].toString();
            else
                msg = QString("HTTP Error %1").arg(httpCode);
            if(httpCode)
                processArrivedError(httpCode, msg, rdata->reqId, rdata->reqObj);
            else
            {
                //I'm not sure why, but it's possible to get empty response on orders list request, so...
                QJsonArray emptyList;
                if(rdata->reqId == ordinaryOrderListReqId)
                {
                    ordinaryOrderListReqId.clear();
                    processOrderListResponse(emptyList);
                } else
                if(rdata->reqId == stoplossOrderListReqId)
                {
                    stoplossOrderListReqId.clear();
                    processOrderListResponse(emptyList);
                }
            }
        }
        rdata->reply->deleteLater();
        delete rdata;
    }
}

void HttpConnector::readyRead()
{
    RequestData * rdata = findRequestByReply(qobject_cast<QNetworkReply *>(sender()));
    if(rdata)
        rdata->buf.append(rdata->reply->readAll());
}

void HttpConnector::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesReceived);
    Q_UNUSED(bytesTotal);
}

void HttpConnector::checkOrdersStatus()
{
    if(!m_order_update_que.isEmpty())
    {
        OrderData *order = m_order_update_que.takeFirst();
        if(order->status != OrderData::Filled)
            updateOrderState(order);
    }
}
