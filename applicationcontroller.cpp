#include "applicationcontroller.h"
#include <QCoreApplication>
#include <QTimer>
#include "strategy.h"

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent), sessionStarted(false), tokenReceived(false),
      config(QString()), //config loading from file is not implemented, so empty file name
      httpConn("https://apidev.alor.ru/warptrans", "https://apidev.alor.ru/md")
{
    tUpdater = new TokenUpdater("https://apidev.alor.ru/md/time",
                                "https://oauthdev.alor.ru/refresh?token=%1",
                                config.getRefreshToken());
    connect(tUpdater, SIGNAL(tokenUpdated()), this, SLOT(tokenUpdated()));
    connect(tUpdater, SIGNAL(errorOnTokenUpdate()), this, SLOT(errorOnTokenUpdate()));
    connect(tUpdater, SIGNAL(timeUpdated()), this, SLOT(timeUpdated()));
    connect(tUpdater, SIGNAL(errorOnTimeUpdate()), this, SLOT(errorOnTimeUpdate()));
    tUpdater->startTimeUpdate();
    connect(&wsConn, SIGNAL(webSocketReady()), this, SLOT(webSocketReady()));

    connect(&httpConn, SIGNAL(serverMessage(QString)), this, SLOT(serverMessage(QString)));
}

bool ApplicationController::isItTimeForTrade()
{
    QDateTime ctm = calcServerTime();
    if(ctm.date().dayOfWeek()>5)
        return false;
    QString hhmm = ctm.toString("HH:mm");
    if(hhmm<config.getHhmmStart() || hhmm>config.getHhmmEnd())
        return false;
    return true;
}

const BotConfig *ApplicationController::getConfig()
{
    return &config;
}

void ApplicationController::startMainLoop()
{
    QUuid guidBars = wsConn.barsGetAndSubscribe(config.getTicker(), 60, startServerTime.addDays(-1).toUTC(), config.getExchange());
    QUuid guidPositions = wsConn.positionsGetAndSubscribe(config.getPortfolio(), config.getExchange());
    strat = new Strategy(guidBars, guidPositions, this);
    connect(&wsConn, SIGNAL(barReceived(QUuid,QString,QDateTime,double,double,double,double,int)),
            strat, SLOT(barReceived(QUuid,QString,QDateTime,double,double,double,double,int)));
    connect(&wsConn, SIGNAL(positionUpdated(QUuid,QString,double,int,int,int,int,int)),
            strat, SLOT(positionUpdated(QUuid,QString,double,int,int,int,int,int)));

    connect(&httpConn, SIGNAL(orderPlaced(QString,OrderData*)), strat, SLOT(orderPlaced(QString,OrderData*)));
    connect(&httpConn, SIGNAL(orderPlaceRejected(QString)), strat, SLOT(orderPlaceRejected(QString)));
    connect(&httpConn, SIGNAL(orderStatusChanged(OrderData*)), strat, SLOT(orderStatusChanged(OrderData*)));

    connect(&httpConn, SIGNAL(orderListReceived(QList<OrderData*>)), strat, SLOT(orderListReceived(QList<OrderData*>)));
}

QDateTime ApplicationController::calcServerTime()
{
    return QDateTime::currentDateTime().addSecs(-offsetFromCurrent);
}

void ApplicationController::tokenUpdated()
{
    tokenReceived=true;
    wsConn.wsConnect("wss://apidev.alor.ru/ws", tUpdater->jwtToken());
    httpConn.setJwtToken(tUpdater->jwtToken());
}

void ApplicationController::errorOnTokenUpdate()
{
    qDebug() << "Couldn't update token";
    sessionStarted=false;
    tokenReceived=false;
    QTimer::singleShot(1000*60*15, tUpdater, SLOT(startTimeUpdate()));
}

void ApplicationController::timeUpdated()
{
    startServerTime = tUpdater->getServerTime().toLocalTime();
    offsetFromCurrent = startServerTime.secsTo(QDateTime::currentDateTime());
    if(isItTimeForTrade())
    {
        sessionStarted=true;
        tokenReceived=false;
        tUpdater->startTokenUpdate();
    }
    else
    {
        sessionStarted=false;
        tokenReceived=false;
        qDebug() << "It's not time to trade, my friend. Be patient, please!";
        QTimer::singleShot(1000*60*15, tUpdater, SLOT(startTimeUpdate()));
    }
}

void ApplicationController::errorOnTimeUpdate()
{
    if(!sessionStarted)
    {
        qDebug() << "Couldn't connect to server. Let wait 15 min more";
        QTimer::singleShot(1000*60*15, tUpdater, SLOT(startTimeUpdate()));
    }
    else
    {
        //do nothing for a while
    }
}

void ApplicationController::webSocketReady()
{
    startMainLoop();
}

void ApplicationController::serverMessage(QString msg)
{
    qDebug().noquote() << msg; //for a while
}
