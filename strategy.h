#ifndef STRATEGY_H
#define STRATEGY_H

#include <QObject>
#include "applicationcontroller.h"
#include "httpconnector.h"

struct BarHistoryItem
{
    enum SourcePrice
    {
        Open,
        High,
        Low,
        Close
    };
    QDateTime dt;
    double o;
    double h;
    double l;
    double c;
    int v;
    BarHistoryItem():o(0),h(0),l(0),c(0),v(0){}
    double getPrice(SourcePrice ptype = Close);
};

struct SMAParams
{
    int length;
    double sum;
    int count;
    BarHistoryItem::SourcePrice type;
};

struct EMAParams
{
    double a;
    double na;
    double cvalue;
    BarHistoryItem::SourcePrice type;
};

class BarHistory : public QObject
{
public:
    explicit BarHistory(QString ticker, int length);
    int newBar(QDateTime dt, double o, double h, double l, double c, int v);
    BarHistoryItem recentBar();
    BarHistoryItem prevBar(int pos);
    BarHistoryItem earliestBar();
    int addSMA(int len, BarHistoryItem::SourcePrice ptype);
    double getSMAValue(int smaIdx);
    int addEMA(int len, BarHistoryItem::SourcePrice ptype);
    double getEMAValue(int emaIdx);
private:
    QString m_ticker;
    int m_length;
    int m_curI;
    QVector<BarHistoryItem> m_history;
    QList<SMAParams *> m_smas;
    QList<EMAParams *> m_emas;
};

class Strategy : public QObject
{
    Q_OBJECT
public:
    explicit Strategy(QUuid buid, QUuid puid, ApplicationController *parent);

private:
    ApplicationController *appController;
    QUuid barUid;
    QUuid posUid;
    QString ticker;
    BarHistory *hist;
    //BarHistory *shortHist;
    double cLongMA;
    double cShortMA;
    double pLongMA;
    double pShortMA;
    int longMaIdx;
    int shortMaIdx;
    QString marketOrderId;
    OrderData *marketOrder;
    QString limitOrderId;
    OrderData *limitOrder;
    QString stopLossOrderId;
    OrderData *slOrder;
    bool oldOrdersCleared;
    BarHistoryItem recentBarStorage;

    QDateTime firstBar;
    QDateTime recentBar;

public slots:
    void barReceived(QUuid id, QString ticker, QDateTime dt, double o, double h, double l, double c, int v);
    void delayedBarReceived(QUuid id, QString ticker, QDateTime dt, double o, double h, double l, double c, int v);
    void positionUpdated(QUuid id, QString symb, double avgP, int qty, int open, int plannedBuy, int plannedSell, int unrealisedPl);

    void orderPlaceRejected(QString reqId);
    void orderPlaced(QString reqId, OrderData *order);
    void orderStatusChanged(OrderData *order);

    void orderListReceived(const QList<OrderData *> &olst);
signals:

};

#endif // STRATEGY_H
