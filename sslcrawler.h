#ifndef SSLCRAWLER_H
#define SSLCRAWLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QUrl>
#include <QSslCertificate>
#include <QSet>
#include <QQueue>
#include <QRunnable>

class SslCrawler : public QObject
{
    Q_OBJECT
public:
    explicit SslCrawler(QObject *parent = 0);

signals:
    void crawlResult(const QUrl &originalUrl, const QUrl &urlWithCertificate,
                     const QSslCertificate &certificate);
    void crawlFinished();

public slots:
    void start();
    void replyMetaDataChanged();
    void replyError(QNetworkReply::NetworkError error);
    void replyFinished();
    void foundUrl(const QUrl &foundUrl, const QUrl &originalUrl);

private:
    void sendMoreRequests();
    void sendRequest(const QNetworkRequest &request);
    void finishRequest(QNetworkReply *reply);
    QNetworkAccessManager *m_manager;
    QSet<QUrl> m_visitedUrls;
    QQueue<QNetworkRequest> m_requestsToSend;
    QSet<QUrl> m_urlsWaitForFinished;
    static int m_concurrentRequests;
};

class UrlFinderRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit UrlFinderRunnable(const QByteArray &data, const QUrl &originalUrl, const QUrl &currentUrl);
    void run();
signals:
    void foundUrl(const QUrl &foundUrl, const QUrl &originalUrl);
private:
    const QByteArray m_data;
    const QUrl m_originalUrl;
    const QUrl m_currentUrl;
    static const QRegExp m_regExp;
};

#endif // SSLCRAWLER_H
