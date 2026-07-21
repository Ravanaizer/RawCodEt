TEMPLATE = app
TARGET = client

QT += widgets

QT += webenginewidgets

QT += core gui network

SOURCES += \
    main.cpp \
    mainwindow.cpp\
    file_tools.cpp\
    console.cpp

HEADERS += \
    mainwindow.h\
    theme.h


INCLUDEPATH += $$PWD/../protocol
LIBS += -L$$PWD/../lib -lprotocol

DESTDIR = $$PWD/../bin
