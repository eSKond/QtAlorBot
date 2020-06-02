#ifndef APPLICATIONCONTROLLER_H
#define APPLICATIONCONTROLLER_H

#include <QObject>
#include "tokenupdater.h"
#include "botconfig.h"
#include "websocketconnector.h"
#include "httpconnector.h"

class Strategy;
class ApplicationController : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationController(QObject *parent = nullptr);
    bool isItTimeForTrade();
    const BotConfig *getConfig(); //yes, it's not best solution, but good enought for example
    HttpConnector *getHttpConnector(){return &httpConn;} //yes, it's also bad decision, but...

private:
    TokenUpdater *tUpdater;
    bool sessionStarted;
    QDateTime startServerTime;
    qint64 offsetFromCurrent;
    bool tokenReceived;
    BotConfig config;
    WebSocketConnector wsConn;
    HttpConnector httpConn;
    Strategy * strat;    

    void startMainLoop();
    QDateTime calcServerTime();
private slots:
    void tokenUpdated();
    void errorOnTokenUpdate();
    void timeUpdated();
    void errorOnTimeUpdate();
    void webSocketReady();

    void serverMessage(QString msg);
signals:

};

#endif // APPLICATIONCONTROLLER_H
