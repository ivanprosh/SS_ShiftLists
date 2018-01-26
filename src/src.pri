PRECOMPILED_HEADER = $${PWD}/prh.h

CONFIG += precompile_header

SOURCES += \
    $${PWD}/shiftmanager.cpp \
    $${PWD}/sqlworker.cpp \
    $${PWD}/sys_messages.cpp \
    $${PWD}/logger.cpp \
    $${PWD}/singleton.cpp \
    $${PWD}/htmlshiftworker.cpp \
    $${PWD}/dbadapter.cpp \
    $$PWD/sqlthread.cpp

HEADERS += \
    $${PWD}/shiftmanager.h \
    $${PWD}/sqlworker.h \
    $${PWD}/sys_messages.h \
    $${PWD}/logger.h \
    $${PWD}/singleton.h \
    $${PWD}/sys_error.h \
    $${PWD}/htmlshiftworker.h \
    $${PWD}/dbadapter.h \
    $${PWD}/typemsg.h \
    $${PWD}/devpolicies.h \
    $$PWD/prh.h \
    $$PWD/sqlthread.h

#message($${PWD})
