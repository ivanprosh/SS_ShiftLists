QT = core testlib
QT -= gui

CONFIG -= app_bundle
CONFIG += testcase no_testcase_installs

INCLUDEPATH += $${PWD}/common \
               $${PWD}/../src \

HEADERS += $${PWD}/common/ss_testcase.h

