#include "sslcrawler.h"
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <QNetworkReply>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QCoreApplication>
#include <QStringList>

int SslCrawler::m_concurrentRequests = 200;

SslCrawler::SslCrawler(QObject *parent) :
    QObject(parent),
    m_manager(new QNetworkAccessManager(this))
{
    // ### do we even need a resource file?
    QFile listFile(QLatin1String(":/list.txt"));
    if (!listFile.open(QIODevice::ReadOnly)) {
        qFatal("could not open resource file ':/list.txt'");
    }
    while (!listFile.atEnd()) {
        QByteArray line = listFile.readLine();
        QByteArray domain = line.right(line.count() - line.indexOf(',') - 1).prepend("https://www.");
        QUrl url = QUrl::fromEncoded(domain.trimmed());
        QNetworkRequest request(url);
        // setting the attribute to trace the originating URL,
        // because we might try different URLs or get redirects
        request.setAttribute(QNetworkRequest::User, url);
        m_requestsToSend.enqueue(request);
    }
}

void SslCrawler::start() {
    sendMoreRequests();
}

void SslCrawler::sendMoreRequests() {

    while (m_urlsWaitForFinished.count() < m_concurrentRequests
           && m_requestsToSend.count() > 0) {
        QNetworkRequest request = m_requestsToSend.dequeue();
        sendRequest(request);
    }
}

void SslCrawler::sendRequest(const QNetworkRequest &request) {

    if (!m_visitedUrls.contains(request.url())) {
        QNetworkReply *reply = m_manager->get(request);
        reply->ignoreSslErrors(); // we don't care, we just want the certificate
        connect(reply, SIGNAL(metaDataChanged()), this, SLOT(replyMetaDataChanged()));
        connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
                this, SLOT(replyError(QNetworkReply::NetworkError)));
        connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));

        m_urlsWaitForFinished.insert(request.url());
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
    m_urlsWaitForFinished.remove(reply->request().url());
    qDebug() << "finishRequest pending requests:" << m_requestsToSend.count() + m_urlsWaitForFinished.count();
    sendMoreRequests();
    if (m_urlsWaitForFinished.count() == 0) {
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
                    sendRequest(request);
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
            sendRequest(newRequest);
            finishRequest(reply);
        } else {
            qWarning() << "could not fetch" << currentUrl;
        }
    }
}

void SslCrawler::replyError(QNetworkReply::NetworkError error) {

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl currentUrl = reply->url();
    QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();

    qDebug() << "replyError" << error << currentUrl << reply->errorString() << "original url:" << originalUrl;
    // 2nd try: if https://[domain] does not work, fetch
    // http://[domain] and parse the HTML for https:// URLs

    // ### check which error we got

    // our blind check for https://[domain] was not succesful, try http://[domain] now
    if (originalUrl.host() == currentUrl.host() && currentUrl.scheme() == QLatin1String("https")) {
        QUrl newUrl = currentUrl;
        newUrl.setScheme(QLatin1String("http"));
        QNetworkRequest newRequest(newUrl); // ### probably we can just copy it
        newRequest.setAttribute(QNetworkRequest::User, newUrl);
        qDebug() << "starting new request" << newUrl << "original url:" << newUrl;
        sendRequest(newRequest);
//        if (m_pendingUrls.count() == 0) {
//            emit crawlFinished();
//        }
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
                sendRequest(request);
                break; // ### remove?
            }
            pos += regExp.matchedLength();
        }
    } else {
        qWarning() << "got error while parsing" << currentUrl << "for" << originalUrl << reply->errorString();
    }
    finishRequest(reply);
}
