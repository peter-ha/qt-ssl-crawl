#include "resultparser.h"

#include <QDebug>

ResultParser::ResultParser(SslCrawler *crawler) :
    QObject(crawler),
    m_crawler(crawler)
{
    connect(m_crawler, SIGNAL(crawlResult(QUrl,QUrl,QSslCertificate)),
            this, SLOT(parseResult(QUrl,QUrl,QSslCertificate)));
    connect(m_crawler, SIGNAL(crawlFinished()), this, SLOT(parseAllResults()));
}

void ResultParser::parseResult(const QUrl &originalUrl,
                          const QUrl &urlWithCertificate,
                          const QSslCertificate &certificate) {
    Q_UNUSED(originalUrl);
    Q_UNUSED(urlWithCertificate);
    QString organization = certificate.issuerInfo(QSslCertificate::Organization);
    qint32 count = m_results.value(organization, 0);
    ++count;
    m_results.insert(organization, count);
}

void ResultParser::parseAllResults()
{
    QHashIterator<QString, qint32> iterator(m_results);
    qint32 totalNumber = 0;
    while (iterator.hasNext()) {
        iterator.next();
        qDebug() << iterator.key() << ": " << iterator.value();
        totalNumber += iterator.value();
    }
    qDebug() << "in total found" << totalNumber << "certificates";
    emit parsingDone();
}
