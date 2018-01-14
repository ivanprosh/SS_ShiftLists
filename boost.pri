win32 {
    BOOST_INCLUDE_ROOT = d:/WORK/_QT/_Libs/_boost/boost_1_66_0
    BOOST_LIBS_PATH = d:/WORK/_QT/_Libs/_boost/boost_1_66_0/stage/lib
}
unix:!macx:{
    BOOST_INCLUDE_ROOT = /usr/local/include
    BOOST_LIBS_PATH = /usr/local/lib
}

INCLUDEPATH += $${BOOST_INCLUDE_ROOT}
DEPENDPATH += $${BOOST_LIBS_PATH}

CONFIG(debug, debug|release){
    BOOST_LIB_FLAGS = -gd
}

LIBS += -L$${BOOST_LIBS_PATH}

win32 {
    LIBS += -llibboost_log-vc140-mt$${BOOST_LIB_FLAGS}-x32-1_66
}
unix:!macx:{
    #define for shared linking boost::log
    DEFINES += BOOST_LOG_DYN_LINK
    #my unix build boost contains only release versions of libs
    LIBS += -lboost_log \
            -lboost_filesystem \
            -lboost_date_time \
            -lboost_system \
            -lboost_thread
    #LIBS += $${BOOST_LIBS_PATH}/libboost_filesystem.a
}
