/*************************************************************************************
**
** QtSslCrawl
** Copyright (C) 2012 Peter Hartmann <9qgm-76ea@xemaps.com>
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
**
*************************************************************************************/

#include <QtCore/QCoreApplication>
#include "qt-ssl-crawler.h"
#include "resultparser.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int from = 0, to = 0;
    if (app.arguments().count() >= 3) {
        from = app.arguments().at(1).toInt();
        to = app.arguments().at(2).toInt();
    }
    QtSslCrawler crawler(0, from, to);
    ResultParser parser(&crawler);
    QObject::connect(&parser, SIGNAL(parsingDone()), &app, SLOT(quit()));
    crawler.start();
    return app.exec();
}
