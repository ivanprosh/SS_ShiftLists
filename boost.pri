BOOST_ROOT = d:/WORK/_QT/_Libs/_boost/boost_1_66_0
BOOST_LIBS_PATH = d:/WORK/_QT/_Libs/_boost/boost_1_66_0/stage/lib

INCLUDEPATH += $${BOOST_ROOT}
DEPENDPATH += $${BOOST_LIBS_PATH}

CONFIG(debug, debug|release){
    BOOST_LIB_FLAGS = -gd
}

LIBS += -L$${BOOST_LIBS_PATH} -llibboost_log-vc140-mt$${BOOST_LIB_FLAGS}-x32-1_66
