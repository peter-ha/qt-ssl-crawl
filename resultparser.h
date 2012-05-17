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

#ifndef RESULTPARSER_H
#define RESULTPARSER_H

#include <QObject>
#include <QHash>
#include <QSet>
#include "qt-ssl-crawler.h"

class ResultParser : public QObject
{
    Q_OBJECT
public:
    explicit ResultParser(QtSslCrawler *crawler);

signals:
    void parsingDone();

public slots:
    void parseResult(const QUrl &originalUrl, const QUrl &urlWithCertificate,
                     const QSslCertificate &certificate);
    void parseAllResults();

private:
    QtSslCrawler *m_crawler;
    QHash<QString, QSet<QUrl> > m_results;
};

#endif // RESULTPARSER_H
