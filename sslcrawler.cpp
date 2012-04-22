#include "sslcrawler.h"
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <QNetworkReply>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QCoreApplication>
#include <QStringList>

SslCrawler::SslCrawler(QObject *parent) :
    QObject(parent),
    m_manager(new QNetworkAccessManager(this))
//    m_pendingRequests(0)
{
    // ### do we even need a resource file?
    QFile listFile(QLatin1String(":/list.txt"));
    if (!listFile.open(QIODevice::ReadOnly)) {
        qFatal("could not open resource file ':/list.txt'");
    }
    while (!listFile.atEnd()) {
        QByteArray line = listFile.readLine();
        QByteArray domain = line.right(line.count() - line.indexOf(',') - 1).prepend("http://www.");
        QUrl url = QUrl::fromEncoded(domain.trimmed());
        m_urls.append(url);
    }
}

void SslCrawler::start() {
    for (int a = 0; a < m_urls.count(); ++a) {
        QUrl url = m_urls.at(a);
        url.setScheme(QLatin1String("https"));
        QNetworkRequest request(url);
        // setting the attribute to trace the originating URL,
        // because we might try different URLs or get redirects
        request.setAttribute(QNetworkRequest::User, QVariant(m_urls.at(a)));
        startRequest(request);
    }
}

void SslCrawler::startRequest(const QNetworkRequest &request) {

    if (!m_visitedUrls.contains(request.url())) {
        QNetworkReply *reply = m_manager->get(request);
        reply->ignoreSslErrors(); // we don't care, we just want the certificate
        connect(reply, SIGNAL(metaDataChanged()), this, SLOT(replyMetaDataChanged()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(replyError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
//        ++m_pendingRequests;

        m_pendingUrls.insert(request.url()); // temp
    } else {
        qDebug() << "visited" << request.url() << "already.";
    }
}

void SslCrawler::finishRequest(QNetworkReply *reply) {

    reply->disconnect(SIGNAL(metaDataChanged()));
    reply->disconnect(SIGNAL(error(QNetworkReply::NetworkError)));
    reply->disconnect(SIGNAL(finished()));
    reply->close();
    reply->abort();
    reply->deleteLater();
    m_visitedUrls.insert(reply->url());
//    --m_pendingRequests;

    m_pendingUrls.remove(reply->request().url()); // temp
    qDebug() << "finishRequest pending requests:" << m_pendingUrls.count();

//    if (m_pendingRequests < 30) {
//        qDebug() << "pending urls:" << m_pendingUrls;
//    }
    if (m_pendingUrls.count() == 0) {
        emit crawlFinished();
    }
}

void SslCrawler::replyMetaDataChanged() {

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl currentUrl = reply->url();
    QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();

    qDebug() << "replyMetaDataChanged" << currentUrl << "original url:" << originalUrl;

    if (reply->error() == QNetworkReply::NoError) {
        if (currentUrl.scheme() == QLatin1String("https")) {
            // success, https://[domain] exists and serves meaningful content
            QList<QSslCertificate> chain = reply->sslConfiguration().peerCertificateChain();
            if (!chain.empty()) {
                QStringList organizations = chain.last().issuerInfo(QSslCertificate::Organization);
                emit crawlResult(originalUrl, currentUrl, chain.last());
                qDebug() << "found ssl cert for" << originalUrl << "at" <<
                        currentUrl << "organizations:" << organizations;
            } else {
                qWarning() << "weird: no errors but certificate chain is empty for " << reply->url();
            }

        } else if (currentUrl.scheme() == QLatin1String("http")) {
            // check for redirections, we might end up at an SSL site
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 300 && statusCode < 400) {
                QByteArray locationHeader = reply->header(QNetworkRequest::LocationHeader).toByteArray();
                if (locationHeader.isEmpty()) // this seems to be a bug in QtNetwork
                    locationHeader = reply->rawHeader("Location");
                QUrl newUrl = QUrl::fromEncoded(locationHeader);
                if (!newUrl.isEmpty()) {
                    qDebug() << "found redirect header at" << currentUrl << "to" << newUrl;
                    QNetworkRequest request(newUrl);
                    request.setAttribute(QNetworkRequest::User, originalUrl);
                    startRequest(request);
                }
            } else {
                qDebug() << "meta data changed for" << currentUrl << "do nothing I guess, wait for finished";
            }
        } else {
            qWarning() << "scheme for" << currentUrl << "is neither https nor http";
        }

    } else { // there was an error

        qWarning() << "THIS SHOULD NOT BE REACHED";
        qDebug() << "error with" << currentUrl << reply->errorString();
        // 2nd try: if https://[domain] does not work, fetch
        // http://[domain] and parse the HTML for https:// URLs
        if (originalUrl.host() == currentUrl.host()) {
            QNetworkRequest newRequest(originalUrl); // ### probably we can just copy it
            newRequest.setAttribute(QNetworkRequest::User, originalUrl);
            qDebug() << "starting new request for" << originalUrl;
            startRequest(newRequest);
            finishRequest(reply);
        } else {
            qWarning() << "could not fetch" << currentUrl;
        }
    }
//    --m_pendingRequests;
//    qDebug() << "pending requests:" << m_pendingRequests;
//    if (m_pendingRequests == 0) {
//        emit crawlFinished();
//    }
}

void SslCrawler::replyError(QNetworkReply::NetworkError error) {

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl currentUrl = reply->url();
    QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();

    qDebug() << "replyError" << currentUrl << reply->errorString() << "original url:" << originalUrl;
    // 2nd try: if https://[domain] does not work, fetch
    // http://[domain] and parse the HTML for https:// URLs

    // ### check which error we got

    if (originalUrl.host() == currentUrl.host() && currentUrl.scheme() == QLatin1String("https")) {
        QNetworkRequest newRequest(originalUrl); // ### probably we can just copy it
        newRequest.setAttribute(QNetworkRequest::User, originalUrl);
        qDebug() << "starting new request" << currentUrl << "original url:" << originalUrl;
        startRequest(newRequest);
        if (m_pendingUrls.count() == 0) {
            emit crawlFinished();
        }
    } else {
        qWarning() << "could not fetch" << currentUrl << "original url:" << originalUrl;
    }
    finishRequest(reply);
}

void SslCrawler::replyFinished() {

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl currentUrl = reply->url();
    QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "reply finished:" << currentUrl << "original url:" << originalUrl << ", now grep for urls";
        QByteArray replyData = reply->readAll();
        // just grep for https:// URLs
        QRegExp regExp("(https://[a-z0-9.@:]+)", Qt::CaseInsensitive);
        int pos = 0;
        while ((pos = regExp.indexIn(replyData, pos)) != -1) {
            QUrl foundUrl(regExp.cap(1));
            if (foundUrl.isValid()
                && foundUrl.host().contains('.') // filter out 'https://ssl'
                && foundUrl.host() != originalUrl.host()
                && foundUrl != currentUrl) { // prevent endless loops
                qDebug() << "found valid url at" << foundUrl << "for" << originalUrl;
                QNetworkRequest request(foundUrl);
                // ### request copy constructor should copy attributes
                request.setAttribute(QNetworkRequest::User, originalUrl);
                startRequest(request);
                break; // ### remove?
            }
            pos += regExp.matchedLength();
        }
    } else {
        qWarning() << "got error while parsing" << currentUrl << "for" << originalUrl << reply->errorString();
    }
    finishRequest(reply);
}
