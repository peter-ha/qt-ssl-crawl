#include "sslcrawler.h"
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <QNetworkReply>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QCoreApplication>

SslCrawler::SslCrawler(QObject *parent) :
    QObject(parent),
    m_manager(new QNetworkAccessManager(this)),
    m_pendingRequests(0)
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
        // setting the attribute to check the originating URL,
        // because we might try different URLs or get redirects
        request.setAttribute(QNetworkRequest::User, QVariant(m_urls.at(a)));
        startRequest(request);
    }
}

QNetworkReply *SslCrawler::startRequest(const QNetworkRequest &request) {

    QNetworkReply *reply = m_manager->get(request);
    reply->ignoreSslErrors(); // we don't care, we just want the certificate
    connect(reply, SIGNAL(metaDataChanged()), this, SLOT(handleReply()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleReply()));
    ++m_pendingRequests;
    return reply;
}

void SslCrawler::handleReply() {

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl currentUrl = reply->url();
    QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();

    if (reply->error() == QNetworkReply::NoError) {
        if (currentUrl.scheme() == QLatin1String("https")) {
            // success, now parse the certificate
            QList<QSslCertificate> chain = reply->sslConfiguration().peerCertificateChain();
            if (!chain.empty()) {
                QString organization = chain.last().issuerInfo(QSslCertificate::Organization);
                // original url might be different from url that is reachable over SSL,
                // e.g. http://mail.ru leads to https://auth.mail.ru
                emit crawlResult(originalUrl, currentUrl, chain.last());
                qDebug() << originalUrl << currentUrl << "organization:" << organization;
            } else {
                qWarning() << "weird: no errors but certificate chain is empty for "
                        << reply->url();
            }
            reply->disconnect(SIGNAL(error(QNetworkReply::NetworkError)));
            reply->abort(); // no need to load the body
            reply->deleteLater();

        } else if (currentUrl.scheme() == QLatin1String("http")) {
            // check for redirections, we might end up at an SSL site
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 300 && statusCode < 400) {
                QByteArray locationHeader = reply->header(QNetworkRequest::LocationHeader).toByteArray();
                if (locationHeader.isEmpty()) // this seems to be a bug in QtNetwork
                    locationHeader = reply->rawHeader("Location");
                QUrl newUrl = QUrl::fromEncoded(locationHeader);
                if (!newUrl.isEmpty()) {
                    QNetworkRequest request(newUrl);
                    request.setAttribute(QNetworkRequest::User, originalUrl);
                    startRequest(request);
                }
            } else {
                qDebug() << "giving up on" << currentUrl;
            }
        } else {
            qWarning() << "scheme for" << currentUrl << "is neither https nor http";
        }

    } else { // there was an error

        qDebug() << "error with" << currentUrl << reply->errorString();
        // 2nd try: if https://[domain] does not work, fetch
        // http://[domain] and parse the HTML for https:// URLs
        if (originalUrl.host() == currentUrl.host()) {
            QNetworkRequest request(originalUrl); // ### probably we can just copy it
            request.setAttribute(QNetworkRequest::User, originalUrl);
            QNetworkReply *reply = startRequest(request);
            connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
            reply->disconnect(SIGNAL(metaDataChanged())); // not interesting any more, wait for finished
            reply->disconnect(SIGNAL(error(QNetworkReply::NetworkError))); // we handle that in finished now
        } else {
            qWarning() << "could not fetch" << currentUrl;
        }
    }
    --m_pendingRequests;
    qDebug() << "pending requests:" << m_pendingRequests;
    if (m_pendingRequests == 0) {
        emit crawlFinished();
    }
}

void SslCrawler::replyFinished() {

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl currentUrl = reply->url();
    QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();
    if (reply->error() == QNetworkReply::NoError) {
        qDebug() << "reply finished:" << currentUrl << ", now grep for urls";
        QByteArray replyData = reply->readAll();
        // just grep for https:// URLs
        QRegExp regExp("(https://[a-z0-9.@:]+)", Qt::CaseInsensitive);
        int pos = 0;
        while ((pos = regExp.indexIn(replyData, pos)) != -1) {
            QUrl foundUrl(regExp.cap(1));
            // the '.' makes sure we do not parse the same url all over again
            if (foundUrl.isValid() && foundUrl.host().endsWith(originalUrl.host().prepend('.'))) {
                qDebug() << "found valid url at" << foundUrl << "for" << originalUrl;
                QNetworkRequest request(foundUrl);
                // ### request copy constructor should copy attributes
                request.setAttribute(QNetworkRequest::User, originalUrl);
                startRequest(request);
                break;
            }
            pos += regExp.matchedLength();
        }
    } else {
        qWarning() << "got error while parsing" << currentUrl << "for" << originalUrl;
    }
    reply->deleteLater();
    --m_pendingRequests;
    qDebug() << "pending requests:" << m_pendingRequests;
    if (m_pendingRequests == 0) {
        emit crawlFinished();
    }
}
