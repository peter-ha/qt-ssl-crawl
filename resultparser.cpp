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
