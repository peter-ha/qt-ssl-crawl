/*************************************************************************************
**
** QtSslCrawl
** Copyright (C) 2012 Peter Hartmann <9qgm-76ea@xemaps.com>
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
*************************************************************************************/

#include "resultparser.h"

#include <QDebug>
#include <QStringList>

ResultParser::ResultParser(QtSslCrawler *crawler) :
    QObject(crawler),
    m_crawler(crawler),
    m_outStream(stdout, QIODevice::WriteOnly)
{
    connect(m_crawler, SIGNAL(crawlResult(QUrl,QUrl,QSslCertificate)),
            this, SLOT(parseResult(QUrl,QUrl,QSslCertificate)));
    connect(m_crawler, SIGNAL(crawlFinished()), this, SLOT(parseAllResults()));
}

void ResultParser::parseResult(const QUrl &originalUrl,
                          const QUrl &urlWithCertificate,
                          const QSslCertificate &certificate) {
    // there could be more than one "O=" field set in the certificate,
    // we need to join all the fields
    QString rootCertOrganizations = certificate.issuerInfo(QSslCertificate::Organization).join(QLatin1String(" / "));
    QPair<QUrl, QString> pair(urlWithCertificate, rootCertOrganizations);
    QSet<QUrl> urlsLinkingToCertificateUrl = m_results.value(pair);
    if (!urlsLinkingToCertificateUrl.contains(originalUrl)) {
        // the index of our results data structure is the URL and certificate,
        // the value is all the URLs that link to the URL containing the certificate
        // (e.g. youtube.com and other sites linking to https://s.ytimg.com)
        urlsLinkingToCertificateUrl.insert(originalUrl);
        m_results.insert(pair, urlsLinkingToCertificateUrl);
    }
}

void ResultParser::parseAllResults()
{
    QHashIterator<QPair<QUrl, QString>, QSet<QUrl> > resultIterator(m_results); // ### make const
    qint32 totalCount = 0;
    while (resultIterator.hasNext()) {
        resultIterator.next();
        QPair<QUrl, QString> pair = resultIterator.key();
        // need to encode the commas in the "Organization" field
        m_outStream << pair.first.toString() << "," << pair.second.replace(',', "\\,");
        QSet<QUrl> urls = resultIterator.value();
        QSetIterator<QUrl> urlIterator(urls);
        while(urlIterator.hasNext()) {
            m_outStream << "," << urlIterator.next().toString();
        }
        totalCount++;
        m_outStream << "\n";
    }
    qWarning() << "in total found" << totalCount << "certificates";
    m_outStream.flush();
    emit parsingDone();
}
