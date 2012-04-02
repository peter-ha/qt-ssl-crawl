#include "sslcrawler.h"
#include <QFile>
#include <QUrl>
#include <QDebug>
#include <QNetworkReply>
#include <QSslCertificate>
#include <QSslConfiguration>

SslCrawler::SslCrawler(QObject *parent) :
    QObject(parent),
    m_manager(new QNetworkAccessManager(this))
{
    QFile listFile(QLatin1String(":/list.txt"));
    if (!listFile.open(QIODevice::ReadOnly)) {
        qFatal("could not open resource file ':/list.txt'");
    }
    while (!listFile.atEnd()) {
        QByteArray line = listFile.readLine();
        QByteArray domain = line.right(line.count() - line.indexOf(',') - 1)
                            .prepend("https://");
        QUrl url = QUrl::fromEncoded(domain.trimmed());
        m_urls.append(url);
    }
}

void SslCrawler::start() {
    for (int a = 0; a < m_urls.count(); ++a) {
        QNetworkReply *reply = m_manager->get(QNetworkRequest(m_urls.at(a)));
        connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    }
}

void SslCrawler::replyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    qDebug() << "finished:" << reply->url() << "error?" << reply->errorString();
    QList<QSslCertificate> chain = reply->sslConfiguration().peerCertificateChain();
    if (!chain.empty()) {
        QString organization = chain.last().issuerInfo(QSslCertificate::Organization);
        qDebug() << "organization:" << organization;
    }

}
