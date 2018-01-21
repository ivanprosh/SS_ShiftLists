include($${PWD}/../tests.pri)
include($${PWD}/../../SS_ShiftLists.pri)
include($${PWD}/../../src/src.pri)

TARGET = tst_shiftmanager
SOURCES += tst_shiftmanager.cpp \

DISTFILES += $$files($${PWD}/data/*.json)

deployment.path = $$OUT_PWD/$${BUILD_TYPE}/data/
deployment.files += $${DISTFILES}

INSTALLS += deployment
