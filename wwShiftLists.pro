#using boost log dynamic library
include(boost.pri)

QT -= gui
QT += core sql

CONFIG += c++11 console
CONFIG -= app_bundle

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    win32:BUILD_TYPE = debug
} else {
    win32:BUILD_TYPE = release
}

deployment.path = $$OUT_PWD/$${BUILD_TYPE}
deployment.files += config.json
INSTALLS += deployment

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += main.cpp \
    shiftmanager.cpp \
    sqlworker.cpp \
    #sys_messages.cpp \
    logger.cpp \
    singleton.cpp

HEADERS += \
    shiftmanager.h \
    sqlworker.h \
    #sys_messages.h \
    logger.h \
    singleton.h \
    sys_error.h

#message($$DEFINES)
