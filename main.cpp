#include <QtCore/QCoreApplication>
#include "sslcrawler.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SslCrawler crawler;
    QObject::connect(&crawler, SIGNAL(crawlFinished()), &app, SLOT(quit()));
    crawler.start();
    return app.exec();
}
