#include <QtCore/QCoreApplication>
#include "sslcrawler.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    SslCrawler crawler;
    crawler.start();
    return a.exec();
}
