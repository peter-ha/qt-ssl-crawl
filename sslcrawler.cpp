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
    QFile listFile(QLatin1String(":/list.txt"));
    if (!listFile.open(QIODevice::ReadOnly)) {
        qFatal("could not open resource file ':/list.txt'");
    }
    while (!listFile.atEnd()) {
        QByteArray line = listFile.readLine();
        // 1st try: just prepend 'https://' to the URL and see whether it works
        QByteArray domain = line.right(line.count() - line.indexOf(',') - 1).prepend("http://");
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
        // because we might get redirects
        request.setAttribute(QNetworkRequest::User, QVariant(m_urls.at(a)));
        startRequest(request);
    }
}

void SslCrawler::startRequest(const QNetworkRequest &request) {
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    ++m_pendingRequests;
}

void SslCrawler::replyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QUrl url = reply->url();
    if (reply->error() == QNetworkReply::NoError) {
        if (url.scheme() == QLatin1String("https")) {
            QList<QSslCertificate> chain = reply->sslConfiguration().peerCertificateChain();
            if (!chain.empty()) {
                QString organization = chain.last().issuerInfo(QSslCertificate::Organization);
                QUrl originalUrl = reply->request().attribute(QNetworkRequest::User).toUrl();
                qDebug() << originalUrl << "organization:" << organization;
            } else {
                qWarning() << "weird: no errors but certificate chain is empty for "
                        << reply->url();
            }
        } else if (url.scheme() == QLatin1String("http")) {
            // check for redirections, we might end up at a SSL site
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 300 && statusCode < 400) {
                QByteArray locationHeader = reply->header(QNetworkRequest::LocationHeader).toByteArray();
                QUrl newUrl = QUrl::fromEncoded(locationHeader);
                QNetworkRequest request(newUrl);
                // passing on original URL
                request.setAttribute(QNetworkRequest::User, reply->request().attribute(QNetworkRequest::User));
            } else {
                qDebug() << "what now with" << reply->url() << "?";
            }
        }
    } else { // there was an error
        // if trying https://[originalDomain] failed, just try http://[originalDomain]
        // and see whether we end up with a redirect to a SSL site
        if (url.scheme() == QLatin1String("https")) {
            QUrl newUrl(url);
            newUrl.setScheme("http");
            QNetworkRequest request(newUrl);
            request.setAttribute(QNetworkRequest::User, reply->request().attribute(QNetworkRequest::User));
            startRequest(request);
        } else {
            qDebug() << "error parsing" << reply->url() << reply->errorString();
        }
    }
    --m_pendingRequests;
    if (m_pendingRequests == 0) {
        QCoreApplication::exit(0);
    }
}
