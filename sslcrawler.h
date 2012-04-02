#ifndef SSLCRAWLER_H
#define SSLCRAWLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QList>
#include <QUrl>

class SslCrawler : public QObject
{
    Q_OBJECT
public:
    explicit SslCrawler(QObject *parent = 0);

signals:

public slots:
    void start();
    void replyFinished();

private:
    void startRequest(const QNetworkRequest &request);
    QNetworkAccessManager *m_manager;
    QList<QUrl> m_urls;
    qint64 m_pendingRequests;
};

#endif // SSLCRAWLER_H
