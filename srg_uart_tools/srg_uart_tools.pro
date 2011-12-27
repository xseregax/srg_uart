#-------------------------------------------------
#
# Project created by QtCreator 2011-11-03T23:34:07
#
#-------------------------------------------------

QT       += core gui

TARGET = srg_uart_tools
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp\
        qwtchartzoom.cpp

HEADERS  += mainwindow.h\
            qwtchartzoom.h

FORMS    += mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../qextserialport/release/ -lqextserialport
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../qextserialport/debug/ -lqextserialport
else:symbian: LIBS += -lqextserialport
else:unix: LIBS += -L$$OUT_PWD/../qextserialport/build -lqextserialport

INCLUDEPATH += $$PWD/../qextserialport
DEPENDPATH += $$PWD/../qextserialport

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../../usr/lib/release/ -lqwt
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../../usr/lib/debug/ -lqwt
else:symbian: LIBS += -lqwt
else:unix: LIBS += -L$$PWD/../../../../../../usr/lib/ -lqwt

INCLUDEPATH += $$PWD/../../../../../../usr/include/qwt
DEPENDPATH += $$PWD/../../../../../../usr/include/qwt
