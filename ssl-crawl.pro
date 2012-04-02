#-------------------------------------------------
#
# Project created by QtCreator 2012-04-01T21:29:28
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = ssl-crawl
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    sslcrawler.cpp

HEADERS += \
    sslcrawler.h

OTHER_FILES += \
    list.txt

RESOURCES += \
    list.qrc
