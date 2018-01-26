include(boost.pri)

QT += core sql widgets printsupport
#QT += webenginewidgets

*msvc*{
    QMAKE_CXXFLAGS += -MP
}

CONFIG += c++14 console
CONFIG -= app_bundle

CONFIG(debug, debug|release){
    DEFINES += DEBUG
    win32:BUILD_TYPE = debug
} else {
    DEFINES += QT_NO_DEBUG_OUTPUT
    win32:BUILD_TYPE = release
}

export(BUILD_TYPE)
