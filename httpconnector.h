#ifndef HTTPCONNECTOR_H
#define HTTPCONNECTOR_H

#include <QObject>
#include <QNetworkReply>
#include <QTimer>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>
#include <QList>

struct RequestData
{
    enum HttpRequestType
    {
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_DELETE
    };

    HttpRequestType type;
    QString reqId;
    QNetworkReply *reply;
    QByteArray buf;
    QJsonObject reqObj;
    RequestData(HttpRequestType t):type(t), reply(nullptr){}
};

struct OrderData
{
    enum OrderType
    {
        Market,
        Limit,
        Stop,
        StopLimit,
        StopLoss,
        TakeProfit,
        TakeProfitLimit,
        StopLossLimit
    };
    enum OrderSide
    {
        Buy,
        Sell
    };
    enum OrderStatus
    {
        Sent,
        Working,
        Filled,
        Canceled,
        Rejected
    };

    int id;
    OrderType type;
    OrderSide side;
    int qty;
    double price;
    double trigPrice;
    QString account;
    QString portfolio;
    QString exch;
    QString symb;
    OrderStatus status;
    int filledQty;
    static OrderData *createOrderFromJson(const QJsonObject &ordObj);
    static OrderData *createMarketOrder(QString acc, QString portf, QString aexch, QString asymb, OrderSide aside, int aqty);
    static OrderData *createLimitOrder(QString acc, QString portf, QString aexch, QString asymb, OrderSide aside, int aqty, double price);
    static OrderData *createStopLossOrder(QString acc, QString portf, QString aexch, QString asymb, OrderSide aside, int aqty, double tprice);
    static OrderStatus stringToStatus(QString sstat);
};

class HttpConnector : public QObject
{
    Q_OBJECT
public:
    explicit HttpConnector(QString wturl, QString wurl, QObject *parent = nullptr);

    void setJwtToken(QString jwt){m_jwtToken=jwt;}
    QString getJwtToken(){return m_jwtToken;}

    static QString newXAlorReqid();

    QString placeMarketOrderBuy(QString acc, QString portf, QString tradeServerCode, int vol, QString exch, QString ticker);
    QString placeMarketOrderSell(QString acc, QString portf, QString tradeServerCode, int vol, QString exch, QString ticker);
    QString placeLimitOrderBuy(QString acc, QString portf, QString tradeServerCode, double targBuyPrice, int vol, QString exch, QString ticker);
    QString placeLimitOrderSell(QString acc, QString portf, QString tradeServerCode, double targSellPrice, int vol, QString exch, QString ticker);
    QString placeStopLossOrderBuy(QString acc, QString portf, QString tradeServerCode, double trigBuyPrice, int vol, QString exch, QString ticker);
    QString placeStopLossOrderSell(QString acc, QString portf, QString tradeServerCode, double trigSellPrice, int vol, QString exch, QString ticker);
    void deleteOrder(QString tradeServerCode, OrderData *order);
    void updateOrderState(OrderData *order);

    bool getOrderList(QString exch, QString portf, bool incOrdinaryOrders, bool incStopOrders);
private:
    QString warpTransUrl;
    QString warpUrl;
    QString m_jwtToken;
    QNetworkAccessManager * m_networkManager;
    QList<RequestData *> m_active_requests;
    QMap<QString, OrderData *> m_place_order_requests;
    QStringList m_order_state_requests;
    QMap<QString, int> m_order_delete_requests;
    QString ordinaryOrderListReqId;
    QString stoplossOrderListReqId;

    QMap<int, OrderData *> m_orders;
    QList<OrderData *> m_order_update_que;

    QList<OrderData *> m_order_list;
    //QTimer orderUpdater;

    QString httpPOSTJsonObject(QString url, QJsonObject &obj);
    QString httpGET(QString url);
    QString httpDELETE(QString url);

    void processArrivedJsonObject(QString reqId, QJsonObject &reqObj, const QJsonObject &respObj);
    void processArrivedJsonArray(QString reqId, QJsonObject &reqObj, const QJsonArray &respArr);
    void processArrivedError(int httpCode, QString errmsg, QString reqId, const QJsonObject &reqObj);

    void processPlaceOrderResponse(QString reqId, OrderData *order, const QJsonObject &respObj);
    void processUpdateOrderState(const QJsonObject &respObj);

    RequestData *findRequestByReply(QNetworkReply *r);

    void processOrderListResponse(const QJsonArray &respArr);

private slots:
    void sslErrors(const QList<QSslError> &errors);
    void error(QNetworkReply::NetworkError code);
    void finished();
    void readyRead();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

    void checkOrdersStatus();

signals:
    void serverMessage(QString msg);
    void orderPlaced(QString reqId, OrderData *order);
    void orderPlaceRejected(QString reqId);
    void orderStatusChanged(OrderData *order);
    void orderDeleted(int ordId);
    void orderListReceived(const QList<OrderData *> &olst);
};

#endif // HTTPCONNECTOR_H
