#include <QtTest/QtTest>
#include <QDir>

#include "shiftmanager.h"
#include "ss_testcase.h"

class tst_shiftmanager : public TestCase
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void syntaxConfigCheck_data();
    void syntaxConfigCheck();
private:
     QScopedPointer<ShiftManager> manager;
};

void tst_shiftmanager::initTestCase()
{
    manager.reset(new ShiftManager());
}

void tst_shiftmanager::syntaxConfigCheck_data()
{
   QDir dataDir(qApp->applicationDirPath() + "/data/");
   QTest::addColumn<QString>("filename");
   //QTest::addColumn<QString>("result");

   foreach (auto filename, dataDir.entryList()){
        QTest::newRow(filename.toUtf8()) << filename;
   }
}

void tst_shiftmanager::syntaxConfigCheck()
{
    QFETCH(QString, filename);
    QVERIFY_EXCEPTION_THROWN(manager->read(qApp->applicationDirPath() + "/data/" + filename),std::exception);
}

QTEST_MAIN(tst_shiftmanager)
#include "tst_shiftmanager.moc"
