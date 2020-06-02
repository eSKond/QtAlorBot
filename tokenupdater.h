#ifndef TOKENUPDATER_H
#define TOKENUPDATER_H

#include <QObject>
#include <QNetworkReply>
#include <QTimer>
#include <QDateTime>

//время:    https://apidev.alor.ru/md/time
//токен:    https://oauthdev.alor.ru/refresh?token=${track.refreshToken}
//запросы:  https://apidev.alor.ru/warptrans

class TokenUpdater : public QObject
{
    Q_OBJECT
public:
    explicit TokenUpdater(QString tupdUrl, QString refreshUrlPatt, QString rToken, QObject *parent = nullptr);
    ~TokenUpdater();

    const QString jwtToken(){return m_jwtToken;}
    const QDateTime getServerTime(){return m_serverTime;}

private:
    QDateTime m_serverTime;
    QString m_timeUpdateUrl;
    QString m_urlPattern;
    QString m_refreshToken;
    QString m_jwtToken;
    QNetworkReply *m_activeReply;
    QNetworkAccessManager * m_networkManager;
    QTimer watchdog;
    QByteArray m_buf;
    //same for time updater
    QNetworkReply *m_t_activeReply;
    QTimer t_watchdog;
    QByteArray m_t_buf;

private slots:
    void sslErrors(const QList<QSslError> &errors);
    void error(QNetworkReply::NetworkError code);
    void finished();
    void readyRead();
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);

public slots:
    void startTimeUpdate();
    void startTokenUpdate();

signals:
    void tokenUpdated();
    void errorOnTokenUpdate();
    void timeUpdated();
    void errorOnTimeUpdate();
};

#endif // TOKENUPDATER_H
