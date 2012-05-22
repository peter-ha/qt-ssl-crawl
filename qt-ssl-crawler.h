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

#ifndef QTSSLCRAWLER_H
#define QTSSLCRAWLER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QList>
#include <QUrl>
#include <QSslCertificate>
#include <QSet>
#include <QQueue>
#include <QRunnable>

class QtSslCrawler : public QObject
{
    Q_OBJECT
public:
    explicit QtSslCrawler(QObject *parent = 0, int from = 0, int to = 0);

signals:
    void crawlResult(const QUrl &originalUrl, const QUrl &urlWithCertificate,
                     const QList<QSslCertificate> &certificateChain);
    void crawlFinished();

public:
    void setCrawlFrom(int from) { m_crawlFrom = from; }
    void setCrawlTo(int to) { m_crawlTo = to; }
public slots:
    void start();
    void replyMetaDataChanged();
    void replyError(QNetworkReply::NetworkError error);
    void replyFinished();
    void foundUrl(const QUrl &foundUrl, const QUrl &originalUrl);

private:
    Q_INVOKABLE void checkForSendingMoreRequests();
    void queueRequestIfNew(const QNetworkRequest &request);
    void sendRequest(const QNetworkRequest &request);
    void finishRequest(QNetworkReply *reply);
    QNetworkAccessManager *m_manager;
    QSet<QUrl> m_visitedUrls;
    QQueue<QNetworkRequest> m_requestsToSend;
    QSet<QUrl> m_urlsWaitForFinished;
    int m_crawlFrom;
    int m_crawlTo;
    static int m_concurrentRequests;
};

class UrlFinderRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit UrlFinderRunnable(const QByteArray &data, const QUrl &originalUrl, const QUrl &currentUrl);
    void run();
signals:
    void foundUrl(const QUrl &foundUrl, const QUrl &originalUrl);
private:
    const QByteArray m_data;
    const QUrl m_originalUrl;
    const QUrl m_currentUrl;
    const QRegExp m_regExp;
};

#endif // QTSSLCRAWLER_H
