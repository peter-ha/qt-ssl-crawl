#ifndef SSLCRAWLER_H
#define SSLCRAWLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QUrl>
#include <QSslCertificate>

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
    void handleReply();

private:
    void startRequest(const QNetworkRequest &request);
    QNetworkAccessManager *m_manager;
    QList<QUrl> m_urls;
    qint64 m_pendingRequests;
};

#endif // SSLCRAWLER_H
