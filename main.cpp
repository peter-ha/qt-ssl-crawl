#include <QtCore/QCoreApplication>
#include "sslcrawler.h"
#include "resultparser.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int from = 0, to = 0;
    if (app.arguments().count() >= 3) {
        from = app.arguments().at(1).toInt();
        to = app.arguments().at(2).toInt();
    }
    SslCrawler crawler(0, from, to);
    ResultParser parser(&crawler);
    QObject::connect(&parser, SIGNAL(parsingDone()), &app, SLOT(quit()));
    crawler.start();
    return app.exec();
}
