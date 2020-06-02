#include "botconfig.h"
#include <QDebug>
#include <QDateTime>
#include <QTimeZone>

BotConfig::BotConfig(QString fname, QObject *parent)
    : QObject(parent),
      brokerComissionPercents(0.05), // комиссия, отчисляемая в пользу брокера, проценты
      readyToLoosePercents(0.35), // степень риска - на столько процентов уменьшится баланс после закрытия позиции ()
      toWinPercents(0.01), // на столько процентов увеличится баланс после закрытия позиции, рекомендуется 0.07
      qty(1), // объём открываемой позиции
      longMALength(15), //5 минутная задержка создаётся сервисом, из-за чего при запросе минутных свечей за последние 15 минут
      shortMALength(7), //приходит только 10 наиболее старых свечей за запрошенные 15 минут
      serviceTimeDelaySeconds(300), // компенсирует 5 новейших свечей задержанных сервером, установить 0 если сервер не задерживает свечи
      exponentialDamper(0.02), //параметр b. EMA среднее берется с весом exp (-b*t). оптимально в диапазоне от 0.01 до 0.5
      hhmmStart(10, 42), // не торговать до(в пределах текущих суток) этого времени
      hhmmEnd(18, 28), // не торговать после(в пределах текущих суток) этого времени
      deltaMacdRub(0.002), // чем больше это значение, тем меньше шанс получить ложный сигнал от MACD (и тем больше задержка сигнала MACD)
      //waitForWebSocketTries(20),
      waitMarketFillMax(3), // количество повторов запросов в ожидании исполнения (переход из статуса working в статус filled) рыночной заявки
      //mainLoopDelay(1025)//9116//4173 // задержка в главном цикле, миллисекунды
      //refreshToken("c7356e00-7f16-42c9-905e-23d499abf97d"),
      refreshToken("e6ef7fd6-b13a-48a1-9330-fab173d8a4cc"),
      ticker("sber"),
      //exchange("GAME"),
      exchange("MOEX"),
      //portfolio("D39004"),
      portfolio("D00008"),
      //account("L01-00000F00")
      account("L01-00000F00")
{
    QDateTime locdt = QDateTime::currentDateTime();
    QDateTime mosdt = locdt.toTimeZone(QTimeZone("Europe/Moscow"));
    tzOffset = locdt.time().hour()-mosdt.time().hour(); //yes, it's not best method
    loadConfig(fname);
}

void BotConfig::loadConfig(QString fname)
{
    Q_UNUSED(fname);
    //do nothing here, just use hardcoded config for a while
}

void BotConfig::saveConfig(QString fname)
{
    Q_UNUSED(fname);
    //do nothing here, just use hardcoded config for a while
}

QString BotConfig::getHhmmStart() const
{
    //we need consider any timezone, so need fix from Moscow tz to local
    QTime offsStart = hhmmStart.addSecs(getTzOffset()*3600);
    QString res = offsStart.toString("HH:mm");
    return res;
}

QString BotConfig::getHhmmEnd() const
{
    QString res = hhmmEnd.addSecs(getTzOffset()*3600).toString("HH:mm");
    return res;
}

double BotConfig::calculateBuyPricesForLimitOrder(double orderMarketPrice) const
{
    double res = orderMarketPrice * (100 - brokerComissionPercents - toWinPercents) / (100 + brokerComissionPercents);
    qDebug() << "Продали за " << orderMarketPrice << " выкупаем за " << res;
    return res;
}

double BotConfig::calculateBuyPricesForStopOrder(double orderMarketPrice) const
{
    double res = orderMarketPrice * (100 - brokerComissionPercents + readyToLoosePercents) / (100 + brokerComissionPercents);
    qDebug() << "Продали за " << orderMarketPrice << " выкупаем за " << res << " при росте цены. SL";
    return res;
}

double BotConfig::calculateSellPricesForLimitOrder(double orderMarketPrice) const
{
    double res = orderMarketPrice * (100 + brokerComissionPercents + toWinPercents) / (100 - brokerComissionPercents);
    qDebug() << "Купили за " << orderMarketPrice << " продаем за " << res;
    return res;
}

double BotConfig::calculateSellPricesForStopOrder(double orderMarketPrice) const
{
    double res = orderMarketPrice * (100 + brokerComissionPercents - readyToLoosePercents) / (100 - brokerComissionPercents);
    qDebug() << "Купили за " << orderMarketPrice << " продаем за " << res << " при падении цены. SL";
    return res;
}

int BotConfig::getTzOffset() const
{
    return tzOffset;
}
