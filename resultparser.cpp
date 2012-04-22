#include "resultparser.h"

#include <QDebug>
#include <QStringList>

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
    QStringList organizations = certificate.issuerInfo(QSslCertificate::Organization);
    for (int a = 0; a < organizations.count(); ++a) {
        QSet<QUrl> urlsForOrganization = m_results.value(organizations.at(a));
        urlsForOrganization.insert(urlWithCertificate); // ### same URL might be inserted a lot of times
        m_results.insert(organizations.at(a), urlsForOrganization);
    }
}

void ResultParser::parseAllResults()
{
    QHashIterator<QString, QSet<QUrl> > iterator(m_results);
    qint32 totalCount = 0;
    while (iterator.hasNext()) {
        iterator.next();
        qWarning() << iterator.key() << ": " << iterator.value().size();
        qWarning() << iterator.value().values(); // urls
        totalCount += iterator.value().size();
        qWarning() << "";
    }
    qWarning() << "in total found" << totalCount << "certificates";
    emit parsingDone();
}
