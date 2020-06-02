#include "strategy.h"

BarHistory::BarHistory(QString ticker, int length)
    : QObject(), m_ticker(ticker), m_length(length), m_curI(0)
{
}

int BarHistory::newBar(QDateTime dt, double o, double h, double l, double c, int v)
{
    BarHistoryItem newItem;
    newItem.dt = dt;
    newItem.o = o;
    newItem.h = h;
    newItem.l = l;
    newItem.c = c;
    newItem.v = v;
    if(m_history.length()<m_length)
    {
        m_history.prepend(newItem);
    }
    else
    {
        if(m_curI)
            m_curI--;
        else
            m_curI=m_length-1;
        m_history[m_curI] = newItem;
    }
    int mai;
    //update smas
    for(mai=0;mai<m_smas.count();mai++)
    {
        SMAParams *sma = m_smas.at(mai);
        sma->sum += newItem.getPrice(sma->type);
        if(sma->count < sma->length)
            sma->count++;
        else
            sma->sum -= prevBar(sma->count).getPrice(sma->type);
    }
    //update emas
    for(mai=0;mai<m_emas.count();mai++)
    {
        EMAParams *ema = m_emas.at(mai);
        if(qIsNaN(ema->cvalue))
            ema->cvalue = newItem.getPrice(ema->type);
        else
            ema->cvalue = ema->a * newItem.getPrice(ema->type) + ema->na * ema->cvalue;
    }
    return m_history.length();
}

BarHistoryItem BarHistory::recentBar()
{
    return prevBar(0);
}

BarHistoryItem BarHistory::prevBar(int pos)
{
    if(pos>=m_history.length())
        return BarHistoryItem();
    pos+=m_curI;
    if(pos>=m_history.length())
        pos-=m_history.length();
    return m_history[pos];
}

BarHistoryItem BarHistory::earliestBar()
{
    return prevBar(m_history.length()-1);
}

int BarHistory::addSMA(int len, BarHistoryItem::SourcePrice ptype)
{
    if(len<2)
    {
        qDebug() << "WTF?! Length of SMA < 2";
        return -1;
    }
    if(len>m_length)
    {
        qDebug() << "Length of SMA greater than history deep";
        return -1;
    }
    SMAParams *nsma = new SMAParams();
    nsma->length = len;
    nsma->sum = 0;
    nsma->count = 0;
    nsma->type = ptype;
    if(m_history.length() > 0)
    {
        int i;
        for(i=m_curI, nsma->count=0;nsma->count<=m_history.length() && nsma->count<=len; i++, nsma->count++)
        {
            if(i>m_history.length())
                i=0;
            nsma->sum += m_history[i].getPrice(ptype);
        }
    }
    m_smas.append(nsma);
    return m_smas.count()-1;
}

double BarHistory::getSMAValue(int smaIdx)
{
    if(smaIdx >= m_smas.count())
        return qQNaN();
    SMAParams *sma = m_smas.at(smaIdx);
    if(sma->count < sma->length)
        return qQNaN();
    return (sma->sum / sma->length);
}

int BarHistory::addEMA(int len, BarHistoryItem::SourcePrice ptype)
{
    if(len<2)
    {
        qDebug() << "WTF?! Length of EMA < 2";
        return -1;
    }
    EMAParams *nema = new EMAParams();
    nema->a = 2.0/(1.0 + double(len));
    nema->na = (1.0 - nema->a);
    nema->cvalue = qQNaN();
    nema->type = ptype;
    if(m_history.length() > 0)
    {
        int first = m_history.length()-1;
        first+=m_curI;
        if(first>=m_history.length())
            first-=m_history.length();
        int i;
        for(i=first;i!=m_curI;i--)
        {
            if(i<0)
                i=m_history.length()-1;
            if(qIsNaN(nema->cvalue))
                nema->cvalue = m_history[i].getPrice(ptype);
            else
                nema->cvalue = nema->a * m_history[i].getPrice(ptype) + nema->na * nema->cvalue;
        }
    }
    m_emas.append(nema);
    return m_emas.count()-1;
}

double BarHistory::getEMAValue(int emaIdx)
{
    if(emaIdx >= m_emas.count())
        return qQNaN();
    EMAParams *ema = m_emas.at(emaIdx);
    return ema->cvalue;
}

Strategy::Strategy(QUuid buid, QUuid puid, ApplicationController *parent)
    : QObject(parent), appController(parent), barUid(buid), posUid(puid),
      hist(nullptr), //initialize on first bar
      cLongMA(qQNaN()), cShortMA(qQNaN()), pLongMA(qQNaN()), pShortMA(qQNaN()),
      longMaIdx(-1), shortMaIdx(-1),
      marketOrder(nullptr), limitOrder(nullptr), slOrder(nullptr),
      oldOrdersCleared(false) //NOTE: we need it for debugging only, don't use it in production
      //oldOrdersCleared(true) //uncomment previous, comment that if you need clear old orders
{
    //uncomment if you need clear old orders
    QString portf = appController->getConfig()->getPortfolio();
    QString exch = appController->getConfig()->getExchange();
    parent->getHttpConnector()->getOrderList(exch, portf, true, true);
}

void Strategy::barReceived(QUuid id, QString ticker, QDateTime dt, double o, double h, double l, double c, int v)
{
    if(id!=barUid)
        return; //ignore
    //we need postpone bar processing until new bar received
    if(recentBarStorage.dt.isNull())
    {
        recentBarStorage.dt=dt;
        recentBarStorage.o=o;
        recentBarStorage.h=h;
        recentBarStorage.l=l;
        recentBarStorage.c=c;
        recentBarStorage.v=v;
        delayedBarReceived(id, ticker, dt, o, h, l, c, v);
        return;
    }
    if(recentBarStorage.dt < dt)
        delayedBarReceived(id, ticker, recentBarStorage.dt,
                           recentBarStorage.o, recentBarStorage.h, recentBarStorage.l, recentBarStorage.c,
                           recentBarStorage.v);
    recentBarStorage.dt=dt;
    recentBarStorage.o=o;
    recentBarStorage.h=h;
    recentBarStorage.l=l;
    recentBarStorage.c=c;
    recentBarStorage.v=v;
}

void Strategy::delayedBarReceived(QUuid id, QString ticker, QDateTime dt, double o, double h, double l, double c, int v)
{
    if(firstBar.isNull())
        firstBar = dt;
    recentBar = dt;
    qInfo() << "Bar(" << id.toString() << "): " << dt.toString("yyyy.MM.dd HH:mm") << "; Volume:" << v;
    if(id==barUid)
    {
        if(!hist)
        {
            hist = new BarHistory(ticker, appController->getConfig()->getLongMALength());
            //you can try SMA or EMA
            //hist->addSMA(appController->getConfig()->getShortMALength(), SMAParams::Close);
            //hist->addSMA(appController->getConfig()->getLongMALength(), SMAParams::Close);
            shortMaIdx = hist->addEMA(appController->getConfig()->getShortMALength(), BarHistoryItem::Close);
            longMaIdx = hist->addEMA(appController->getConfig()->getLongMALength(), BarHistoryItem::Close);
        }
        pLongMA = cLongMA;
        pShortMA = cShortMA;
        hist->newBar(dt, o, h, l, c, v);
        //you can try SMA or EMA
        //cLongMA = hist->getSMAValue(longMaIdx);
        //cShortMA = hist->getSMAValue(shortMaIdx);
        cLongMA = hist->getEMAValue(longMaIdx);
        cShortMA = hist->getEMAValue(shortMaIdx);
        //Do nothing until old orders cleared
        if(oldOrdersCleared)
        {
            //we need skip old bars being sent just after subscription
            QDateTime cdt = QDateTime::currentDateTime();
            int secs = dt.secsTo(cdt);
            if(secs < 60)
            {
                qDebug() << "Bar processing...";
                if(!qIsNaN(pLongMA) && !qIsNaN(pShortMA) && //we need have all data
                        marketOrderId.isEmpty() && !marketOrder && // and we must be in initial state
                        limitOrderId.isEmpty() && !limitOrder &&
                        stopLossOrderId.isEmpty() && !slOrder)
                {
                    QString acc = appController->getConfig()->getAccount();
                    QString portf = appController->getConfig()->getPortfolio();
                    double deltaMacdRub = appController->getConfig()->getDeltaMacdRub();
                    int volume = appController->getConfig()->getQty(); //IMHO "quantity" is more correct word... but... up to you
                    QString exch = appController->getConfig()->getExchange();
                    QString ticker = appController->getConfig()->getTicker();
                    if(cLongMA + deltaMacdRub < cShortMA)
                    {
                        //trendInitiateProcessors.js: processCaseBullTrendInitiate
                        qDebug() << "Индикатор MACD сигнализирует: цена растет, покупаем"; //why MACD? I don't know
                        marketOrderId = appController->getHttpConnector()->placeMarketOrderBuy(acc, portf, "TRADE", volume, exch, ticker);
                    }
                    else if(cLongMA > cShortMA + deltaMacdRub)
                    {
                        //trendInitiateProcessors.js: processCaseBearTrendInitiate
                        qDebug() << "Индикатор MACD сигнализирует: цена падает, продаём";
                        //Really account, portfolio and tradeservercode can be taken from request /client/v1.0/users/{username}/portfolios
                        //...sometime it's very complicated, agree
                        marketOrderId = appController->getHttpConnector()->placeMarketOrderSell(acc, portf, "TRADE", volume, exch, ticker);
                    }
                    //do nothing for rest of cases
                }
            }
        }
    }
}

void Strategy::positionUpdated(QUuid id, QString symb, double avgP, int qty, int open, int plannedBuy, int plannedSell, int unrealisedPl)
{
    //we need show positions somewhere
    Q_UNUSED(id);
    Q_UNUSED(symb);
    Q_UNUSED(avgP);
    Q_UNUSED(qty);
    Q_UNUSED(open);
    Q_UNUSED(plannedBuy);
    Q_UNUSED(plannedSell);
    Q_UNUSED(unrealisedPl);
}

void Strategy::orderPlaceRejected(QString reqId)
{
    if(reqId == marketOrderId)
    {
        //Couldn't place market order - stop all and wait next events
        marketOrderId = QString();
    }
    else
    {
        if(reqId == limitOrderId)
        {
            qDebug() << "Couldn't place limit order";
            limitOrderId = QString();
        }
        else
        {
            if(reqId == stopLossOrderId)
            {
                qDebug() << "Couldn't place stop loss order";
                stopLossOrderId = QString();
            }
            else
            {
                qDebug() << "Hey, WTF?!";
            }
        }
    }
}

void Strategy::orderPlaced(QString reqId, OrderData *order)
{
    qDebug() << "Order(" << order->id << ", " << (order->type==OrderData::StopLoss?"STP":(order->type==OrderData::Limit?"LIM":"MKT")) << ") placed";
    if(reqId == marketOrderId)
    {
        marketOrderId = QString(); //we don't need it more
        if(order->status != OrderData::Canceled && order->status != OrderData::Rejected)
        {
            marketOrder = order;
            if(order->status == OrderData::Filled)
                orderStatusChanged(order);
        }
    }
    else
    {
        if(reqId == limitOrderId)
        {
            limitOrderId = QString();
            if(order->status == OrderData::Canceled || order->status == OrderData::Rejected)
            {
                //i don't know how to handle it, may be you know?
                qDebug() << "Лимитная заявка на " <<
                            (order->side==OrderData::Buy?"покупку":"продажу") <<
                            " отвергнута, теперь робот не " <<
                            (order->side==OrderData::Buy?"продаст купленую":"купит проданную") <<
                            " акцию";
                //if stop loss alreade placed...
                if(slOrder)
                {
                    appController->getHttpConnector()->deleteOrder("TRADE", slOrder);
                    slOrder = nullptr;
                }
                return;
            }
            limitOrder = order;
            if(order->status == OrderData::Filled)
                orderStatusChanged(order);
        }
        else
        {
            if(reqId == stopLossOrderId)
            {
                if(order->status == OrderData::Canceled || order->status == OrderData::Rejected)
                {
                    //i don't know how to handle it, may be you know?
                    qDebug() << "Стоплосс заявка отвергнута, теперь робот не " <<
                                (order->side==OrderData::Buy?"продаст купленую":"купит проданную") <<
                                " акцию";
                    //if limit order alreade placed...
                    if(limitOrder)
                    {
                        appController->getHttpConnector()->deleteOrder("TRADE", limitOrder);
                        limitOrder = nullptr;
                    }
                    return;
                }
                slOrder = order;
                if(order->status == OrderData::Filled)
                    orderStatusChanged(order);
            }
            else
            {
                qDebug() << "Hey, WTF?! (yes, again, yes, no fantasy)";
            }
        }
    }
}

void Strategy::orderStatusChanged(OrderData *order)
{
    if(order == marketOrder)
    {
        if(order->status == OrderData::Filled)
        {
            //place take profit and stop order at once
            if(order->side == OrderData::Buy)
            {
                double targetSellPrice = appController->getConfig()->calculateSellPricesForLimitOrder(order->price);
                double targetTriggerSellPrice = appController->getConfig()->calculateSellPricesForStopOrder(order->price);
                limitOrderId =
                        appController->getHttpConnector()->placeLimitOrderSell(order->account, order->portfolio, "TRADE",
                                                                               targetSellPrice, order->qty,
                                                                               order->exch, order->symb);
                stopLossOrderId =
                        appController->getHttpConnector()->placeStopLossOrderSell(order->account, order->portfolio, "TRADE",
                                                                                  targetTriggerSellPrice, order->qty,
                                                                                  order->exch, order->symb);
            }
            else
            {
                double targetBuyPrice = appController->getConfig()->calculateBuyPricesForLimitOrder(order->price);
                double targetTriggerBuyPrice = appController->getConfig()->calculateBuyPricesForStopOrder(order->price);
                limitOrderId =
                        appController->getHttpConnector()->placeLimitOrderBuy(order->account, order->portfolio, "TRADE",
                                                                              targetBuyPrice, order->qty,
                                                                              order->exch, order->symb);
                stopLossOrderId =
                        appController->getHttpConnector()->placeStopLossOrderBuy(order->account, order->portfolio, "TRADE",
                                                                                 targetTriggerBuyPrice, order->qty,
                                                                                 order->exch, order->symb);
            }
        }
        else
        {
            //It's strange. Just do nothing
        }
    }
    else
    {
        if(order == limitOrder)
        {
            //on any status change (excluding working) we need remove other side order - stop loss
            if(order->status != OrderData::Working)
            {
                //if stop loss alreade placed...
                if(slOrder)
                {
                    appController->getHttpConnector()->deleteOrder("TRADE", slOrder);
                    slOrder = nullptr;
                }
                limitOrder = nullptr;
            }
            //we don't calc pl here, we just can loot at positions
        }
        else
        {
            if(order == slOrder)
            {
                //on any status change (excluding working) we need remove other side order - limit order
                if(order->status != OrderData::Working)
                {
                    //if limit order alreade placed...
                    if(limitOrder)
                    {
                        appController->getHttpConnector()->deleteOrder("TRADE", limitOrder);
                        limitOrder = nullptr;
                    }
                    slOrder = nullptr;
                }
                //we don't calc pl here, we just can loot at positions
            }
        }
    }
}

void Strategy::orderListReceived(const QList<OrderData *> &olst)
{
    int i;
    QString portf = appController->getConfig()->getPortfolio();
    QString exch = appController->getConfig()->getExchange();
    QString acc = appController->getConfig()->getAccount();
    for(i=0;i<olst.count();i++)
    {
        OrderData *order = olst.at(i);
        order->portfolio = portf;
        order->exch = exch;
        order->account = acc;
        if(order->status == OrderData::Working)
        {
            appController->getHttpConnector()->deleteOrder("TRADE", order);
        }
    }
    oldOrdersCleared = true;
    qDebug() << "We are ready...";
}

double BarHistoryItem::getPrice(BarHistoryItem::SourcePrice ptype)
{
    if(ptype == BarHistoryItem::Open)
        return o;
    if(ptype == BarHistoryItem::High)
        return h;
    if(ptype == BarHistoryItem::Low)
        return l;
    return c;
}
