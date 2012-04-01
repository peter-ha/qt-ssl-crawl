#ifndef SSLCRAWLER_H
#define SSLCRAWLER_H

#include <QObject>
#include <QNetworkAccessManager>

class SslCrawler : public QObject
{
    Q_OBJECT
public:
    explicit SslCrawler(QObject *parent = 0);

signals:

public slots:
    void replyFinished();

private:
    QNetworkAccessManager *m_manager;
};

#endif // SSLCRAWLER_H
