#include "tokenupdater.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

TokenUpdater::TokenUpdater(QString tupdUrl, QString refreshUrlPatt, QString rToken, QObject *parent)
    : QObject(parent), m_timeUpdateUrl(tupdUrl), m_urlPattern(refreshUrlPatt), m_refreshToken(rToken),
      m_activeReply(nullptr), m_t_activeReply(nullptr)
{
    m_networkManager = new QNetworkAccessManager(this);
}

TokenUpdater::~TokenUpdater()
{
    if(m_activeReply)
        delete m_activeReply;
    if(m_networkManager)
        delete m_networkManager;
}

void TokenUpdater::startTimeUpdate()
{
    if(m_t_activeReply)
        return;
    m_t_buf.clear();
    QNetworkRequest request;
    request.setRawHeader("User-Agent", "QtAlorBot");
    request.setRawHeader("Connection", "close");
    request.setUrl(m_timeUpdateUrl);
    m_t_activeReply = m_networkManager->get(request);
    connect(m_t_activeReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    connect(m_t_activeReply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(m_t_activeReply, SIGNAL(finished()), this, SLOT(finished()));
    connect(m_t_activeReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(m_t_activeReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    t_watchdog.setInterval(60000*5);//5min
    t_watchdog.setSingleShot(true);
    connect(&t_watchdog, SIGNAL(timeout()), m_t_activeReply, SLOT(abort()));
    t_watchdog.start();
}

void TokenUpdater::startTokenUpdate()
{
    if(m_activeReply)
        return;
    m_buf.clear();
    QNetworkRequest request;
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("User-Agent", "QtAlorBot");
    request.setRawHeader("Connection", "close");
    request.setUrl(m_urlPattern.arg(m_refreshToken));
    m_activeReply = m_networkManager->post(request, QByteArray());
    connect(m_activeReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
    connect(m_activeReply, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(m_activeReply, SIGNAL(finished()), this, SLOT(finished()));
    connect(m_activeReply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(m_activeReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(downloadProgress(qint64,qint64)));
    watchdog.setInterval(60000*5);//5min
    watchdog.setSingleShot(true);
    connect(&watchdog, SIGNAL(timeout()), m_activeReply, SLOT(abort()));
    watchdog.start();
}

void TokenUpdater::sslErrors(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);
    if(qobject_cast<QNetworkReply *>(sender()) == m_activeReply)
        watchdog.stop();
    else
        t_watchdog.stop();
}

void TokenUpdater::error(QNetworkReply::NetworkError code)
{
    Q_UNUSED(code);
    if(qobject_cast<QNetworkReply *>(sender()) == m_activeReply)
        watchdog.stop();
    else
        t_watchdog.stop();
}

void TokenUpdater::finished()
{
    if(qobject_cast<QNetworkReply *>(sender()) == m_activeReply)
    {
        watchdog.stop();
        if(!m_activeReply->error())
        {
            m_buf.append(m_activeReply->readAll());
            QJsonDocument jdoc=QJsonDocument::fromJson(m_buf);
            m_jwtToken = jdoc.object().value("AccessToken").toString();
            qDebug() << "Token updated: " << m_jwtToken.right(8);
            emit tokenUpdated();
        }
        else
            emit errorOnTokenUpdate();
        m_activeReply->deleteLater();
    }
    else
    {
        t_watchdog.stop();
        if(!m_t_activeReply->error())
        {
            m_t_buf.append(m_t_activeReply->readAll());
            qint64 v = QString::fromLatin1(m_t_buf).toLongLong();
            m_serverTime = QDateTime::fromSecsSinceEpoch(v, Qt::UTC);
            qDebug() << "Server time: " << m_serverTime.toTimeZone(QTimeZone("Europe/Moscow")).toString("dd.MM.yyyy HH:mm:ss");
            emit timeUpdated();
        }
        else
            emit errorOnTimeUpdate();
        m_t_activeReply->deleteLater();
    }
}

void TokenUpdater::readyRead()
{
    if(qobject_cast<QNetworkReply *>(sender()) == m_activeReply)
        m_buf.append(m_activeReply->readAll());
    else
        m_t_buf.append(m_t_activeReply->readAll());
}

void TokenUpdater::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_UNUSED(bytesReceived);
    Q_UNUSED(bytesTotal);
    //restart timer to continue download
    if(qobject_cast<QNetworkReply *>(sender()) == m_activeReply)
        watchdog.start();
    else
        t_watchdog.start();
}
