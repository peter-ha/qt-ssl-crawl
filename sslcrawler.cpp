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
        // because we might try different URLs or get redirects
        request.setAttribute(QNetworkRequest::User, QVariant(m_urls.at(a)));
        startRequest(request);
    }
}

void SslCrawler::startRequest(const QNetworkRequest &request) {
    QNetworkReply *reply = m_manager->get(request);
    connect(reply, SIGNAL(metaDataChanged()), this, SLOT(handleReply()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleReply()));
    ++m_pendingRequests;
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
            // we don't want to get an 'operation cancelled error' later
            reply->disconnect(SIGNAL(error(QNetworkReply::NetworkError)));
            reply->abort(); // no need to load the body

        } else if (currentUrl.scheme() == QLatin1String("http")) {
            // check for redirections, we might end up at a SSL site
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 300 && statusCode < 400) {
                QByteArray locationHeader = reply->header(QNetworkRequest::LocationHeader).toByteArray();
                if (locationHeader.isEmpty()) // this seems to be a bug in QtNetwork
                    locationHeader = reply->rawHeader("Location");
                QUrl newUrl = QUrl::fromEncoded(locationHeader);
                if (!newUrl.isEmpty()) {
                    QNetworkRequest request(newUrl);
                    // passing on original URL
                    request.setAttribute(QNetworkRequest::User, reply->request().attribute(QNetworkRequest::User));
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
        QUrl newUrl;
        QString currentHost = currentUrl.host();

        if (currentHost == originalUrl.host()) {

            // 2nd try: https://secure.[originalDomain]
            newUrl = currentUrl;
            currentHost.prepend("secure.");
            newUrl.setHost(currentHost);
        } else if(currentHost == originalUrl.host().prepend("secure.")) {

            // 3rd try: https://login.[originalDomain]
            newUrl = currentUrl;
            currentHost.replace(QRegExp(QLatin1String("^secure\\.")), "login.");
            newUrl.setHost(currentHost);
        } else if (currentUrl.scheme() == QLatin1String("https")) {
            // 4th try: if trying https://*.[originalDomain] failed, just try
            // http://[originalDomain] and see whether we end up with a
            // redirect to a SSL site
            newUrl = originalUrl;
            newUrl.setScheme("http");
        } else {
            qDebug() << "error for" << originalUrl;
        }
        if (!newUrl.isEmpty()) {
            // try the new URL
            QNetworkRequest request(newUrl);
            qDebug() << "trying" << newUrl;
            request.setAttribute(QNetworkRequest::User, reply->request().attribute(QNetworkRequest::User));
            startRequest(request);
        }
    }
    --m_pendingRequests;
    qDebug() << "pending requests:" << m_pendingRequests;
    if (m_pendingRequests == 0) {
        emit crawlFinished();
    }
}
