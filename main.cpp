#include <QtCore/QCoreApplication>
#include "sslcrawler.h"
#include "resultparser.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    SslCrawler crawler;
    ResultParser parser(&crawler);
    QObject::connect(&parser, SIGNAL(parsingDone()), &app, SLOT(quit()));
    crawler.start();
    return app.exec();
}
