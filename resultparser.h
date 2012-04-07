#ifndef RESULTPARSER_H
#define RESULTPARSER_H

#include <QObject>
#include <QHash>
#include <QSet>
#include "sslcrawler.h"

class ResultParser : public QObject
{
    Q_OBJECT
public:
    explicit ResultParser(SslCrawler *crawler);

signals:
    void parsingDone();

public slots:
    void parseResult(const QUrl &originalUrl, const QUrl &urlWithCertificate,
                     const QSslCertificate &certificate);
    void parseAllResults();

private:
    SslCrawler *m_crawler;
    QHash<QString, QSet<QUrl> > m_results;
};

#endif // RESULTPARSER_H
