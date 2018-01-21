include(boost.pri)

QT += core sql widgets printsupport

CONFIG += c++11 console
CONFIG -= app_bundle

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    win32:BUILD_TYPE = debug
} else {
    win32:BUILD_TYPE = release
}

export(BUILD_TYPE)
