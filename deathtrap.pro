QT -= gui
QT += sql websockets testlib

CONFIG += c++latest console
CONFIG -= app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += src/ \
               test/

SOURCES += \
        src/wsclient.cpp \
        src/wsserver.cpp \
        src/database.cpp \
        test/main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    src/database.h \
    src/wsserver.h \
    src/wsclient.h \
    src/wsmessage.h \
    test/test.h
