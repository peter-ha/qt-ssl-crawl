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
    connect(m_crawler, SIGNAL(crawlResult(QUrl,QUrl,QList<QSslCertificate>)),
            this, SLOT(parseResult(QUrl,QUrl,QList<QSslCertificate>)));
    connect(m_crawler, SIGNAL(crawlFinished()), this, SLOT(parseAllResults()));
    m_outStream << "URL containing certificate;site certificate country;root cert organization;" <<
            "root certificate country;linking URLs\n";
}

void ResultParser::parseResult(const QUrl &originalUrl,
                          const QUrl &urlWithCertificate,
                          const QList<QSslCertificate> &certificateChain) {
    Result currentResult = m_results.value(urlWithCertificate);

    if (currentResult.sitesContainingLink.empty()) { // first time we encounter this site
        QSslCertificate lastCertInChain = certificateChain.last();
        currentResult.siteCertCountry = certificateChain.first().subjectInfo(QSslCertificate::CountryName).join(" / ");
        currentResult.rootCertCountry = lastCertInChain.issuerInfo(QSslCertificate::CountryName).join(" / ");
        currentResult.rootCertOrganization =
                lastCertInChain.issuerInfo(QSslCertificate::Organization).join(" / ");
    }
    if (!currentResult.sitesContainingLink.contains(originalUrl)) {
        currentResult.sitesContainingLink.insert(originalUrl);
        m_results.insert(urlWithCertificate, currentResult);
    }
}

void ResultParser::parseAllResults()
{
    QHashIterator<QUrl, Result> resultIterator(m_results);
    qint32 totalCount = 0;
    while (resultIterator.hasNext()) {
        resultIterator.next();
        QUrl urlWithCertificate = resultIterator.key();
        Result currentResult = resultIterator.value();
        m_outStream << urlWithCertificate.toString() << ";"
                << currentResult.siteCertCountry << ";"
                << currentResult.rootCertOrganization << ";"
                << currentResult.rootCertCountry;
        QSet<QUrl> urls = currentResult.sitesContainingLink;
        QSetIterator<QUrl> urlIterator(urls);
        while(urlIterator.hasNext()) {
            m_outStream << ";" << urlIterator.next().toString();
        }
        totalCount++;
        m_outStream << "\n";
    }
    qWarning() << "in total found" << totalCount << "certificates";
    m_outStream.flush();
    emit parsingDone();
}
