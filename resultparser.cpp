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
    QString organization = certificate.issuerInfo(QSslCertificate::Organization);
    QSet<QUrl> urlsForOrganization = m_results.value(organization);
    urlsForOrganization.insert(urlWithCertificate); // same URL might be inserted a lot of times
    m_results.insert(organization, urlsForOrganization);
}

void ResultParser::parseAllResults()
{
    QHashIterator<QString, QSet<QUrl> > iterator(m_results);
    qint32 totalCount = 0;
    while (iterator.hasNext()) {
        iterator.next();
        qDebug() << iterator.key() << ": " << iterator.value().size();
        qDebug() << iterator.value().values(); // urls
        totalCount += iterator.value().size();
    }
    qDebug() << "in total found" << totalCount << "certificates";
    emit parsingDone();
}
