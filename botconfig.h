#ifndef BOTCONFIG_H
#define BOTCONFIG_H

#include <QObject>
#include <QTime>

/*
Access Token: eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiJQMDUzNDU0IiwiZW50IjoiY2xpZW50IiwiZWluIjoiMDMyNDUiLCJjbGllbnRJZCI6IjgxMTYxIiwiYXpwIjoiMWQ0MDRiZmM3MDU1NGU2M2E2YzEiLCJhZ3JlZW1lbnRzIjoiMDQyMTkgNTM0NTQiLCJwb3J0Zm9saW9zIjoiNzUwMDEwMyA3NTAwMDA4IEQ0MjE5IEQwMDAwOCBHMDE5MDQgRzAwMDA4Iiwic2NvcGUiOiJPcmRlcnNSZWFkIFRyYWRlcyBQZXJzb25hbCBTdGF0cyIsImV4cCI6MTU4Njk0MzM4MywiaWF0IjoxNTg2OTQxNTgzLCJpc3MiOiJBbG9yLklkZW50aXR5IiwiYXVkIjoiQ2xpZW50IFdBUlAgV2FycEFUQ29ubmVjdG9yIn0.PmV97742MfvX5Yggc8eikzAb-4Qw3IdNwXmXFnM3ipleWWk2sUSlUZx_pYxx_fo1maOLCDMY92ykGHl_GL8yxw
Refresh Token: c7356e00-7f16-42c9-905e-23d499abf97d
 */
class BotConfig : public QObject
{
    Q_OBJECT
public:
    explicit BotConfig(QString fname, QObject *parent = nullptr);

    void loadConfig(QString fname);
    void saveConfig(QString fname);

    //getters
    double getBrokerComissionPercents() const{return brokerComissionPercents;}    // комиссия, отчисляемая в пользу брокера, проценты
    double getReadyToLoosePercents() const{return readyToLoosePercents;}          // степень риска - на столько процентов уменьшится баланс после закрытия позиции ()
    double getToWinPercents() const{return toWinPercents;}                        // на столько процентов увеличится баланс после закрытия позиции, рекомендуется 0.07
    int getQty() const{return qty;}                                               // объём открываемой позиции
    int getLongMALength() const{return longMALength;}                             //5 минутная задержка создаётся сервисом, из-за чего при запросе минутных свечей за последние 15 минут
    int getShortMALength() const{return shortMALength;}                           //приходит только 10 наиболее старых свечей за запрошенные 15 минут
    int getServiceTimeDelaySeconds() const{return serviceTimeDelaySeconds;}       // компенсирует 5 новейших свечей задержанных сервером, установить 0 если сервер не задерживает свечи
    double getExponentialDamper() const{return exponentialDamper;}                //параметр b. EMA среднее берется с весом exp (-b*t). оптимально в диапазоне от 0.01 до 0.5
    QString getHhmmStart() const;                                                 // не торговать до(в пределах текущих суток) этого времени
    QString getHhmmEnd() const;                                                   // не торговать после(в пределах текущих суток) этого времени
    int getTzOffset() const;
    double getDeltaMacdRub() const{return deltaMacdRub;}                          // чем больше это значение, тем меньше шанс получить ложный сигнал от MACD (и тем больше задержка сигнала MACD)
    int getWaitMarketFillMax() const{return waitMarketFillMax;}                   // количество повторов запросов в ожидании исполнения (переход из статуса working в статус filled) рыночной заявки
    QString getRefreshToken() const{return refreshToken;}
    QString getTicker() const{return ticker;}
    QString getExchange() const{return exchange;}
    QString getPortfolio() const{return portfolio;}
    QString getAccount() const{return account;}

    double calculateBuyPricesForLimitOrder(double orderMarketPrice) const;
    double calculateBuyPricesForStopOrder(double orderMarketPrice) const;
    double calculateSellPricesForLimitOrder(double orderMarketPrice) const;
    double calculateSellPricesForStopOrder(double orderMarketPrice) const;
private:
    double brokerComissionPercents; // комиссия, отчисляемая в пользу брокера, проценты
    double readyToLoosePercents;    // степень риска - на столько процентов уменьшится баланс после закрытия позиции ()
    double toWinPercents;           // на столько процентов увеличится баланс после закрытия позиции, рекомендуется 0.07
    int qty;                        // объём открываемой позиции
    int longMALength;               //5 минутная задержка создаётся сервисом, из-за чего при запросе минутных свечей за последние 15 минут
    int shortMALength;              //приходит только 10 наиболее старых свечей за запрошенные 15 минут
    int serviceTimeDelaySeconds;    // компенсирует 5 новейших свечей задержанных сервером, установить 0 если сервер не задерживает свечи
    double exponentialDamper;       //параметр b. EMA среднее берется с весом exp (-b*t). оптимально в диапазоне от 0.01 до 0.5
    QTime hhmmStart;              // не торговать до(в пределах текущих суток) этого времени
    QTime hhmmEnd;                // не торговать после(в пределах текущих суток) этого времени
    double deltaMacdRub;            // чем больше это значение, тем меньше шанс получить ложный сигнал от MACD (и тем больше задержка сигнала MACD)
    //int waitForWebSocketTries;      //не уверене, нужно ли нам это здесь
    int waitMarketFillMax;          // количество повторов запросов в ожидании исполнения (переход из статуса working в статус filled) рыночной заявки
    //mainLoopDelay: 1025//9116//4173 // задержка в главном цикле, миллисекунды
    QString refreshToken;
    QString ticker;
    QString exchange;
    QString portfolio;
    QString account;

    int tzOffset;

signals:

};

#endif // BOTCONFIG_H
