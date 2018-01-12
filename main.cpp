#include "shiftmanager.h"
//#include "sys_messages.h"
#include "sqlworker.h"
#include "logger.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTextCodec>

const QString queryPattern(
        "SET QUOTED_IDENTIFIER OFF "
        "SELECT * FROM OPENQUERY(INSQL, \"SELECT DateTime, %1 FROM WideHistory WHERE wwResolution = 60000\")"
        );

const QString testQuery(
        "SET QUOTED_IDENTIFIER OFF "
        "DECLARE @OFFSET VARCHAR(10) "
        "DECLARE @StartDate VARCHAR(100) "
        "DECLARE @SelectString VARCHAR(5000) "
        "SET @StartDate = \'\"\"9/1/2018 10:00:00:00\"\"\' "
        "SET @OFFSET = -5 "
        "SET @SelectString ="
        "\'select * FROM OPENQUERY(INSQL, \"SELECT DateTime, [Reactor_001.ReactLevel], [Reactor_001.ReactTemp] "
        " FROM WideHistory "
        " WHERE wwRetrievalMode = \'\'Cyclic\'\' "
        " AND wwResolution = 3600000 "
        " AND wwQualityRule = \'\'Extended\'\' "
        " AND wwVersion = \'\'Latest\'\' "
        " AND DateTime >= DateAdd(HH,\' + @OFFSET + \',\' + @StartDate + \') "
        " AND DateTime <= GetDate() "
        " \")\' "
        "EXEC(@SelectString) "
        );

const QString testQuery2(
        "SET QUOTED_IDENTIFIER OFF "
        "SELECT * FROM OPENQUERY(INSQL, \"SELECT DateTime, [%1.ReactTemp] FROM WideHistory WHERE wwResolution = 60000\")"
        );
/*
const QString testQuery3(
        "SET QUOTED_IDENTIFIER OFF "
        "DECLARE @SelectString VARCHAR(5000) "
        "SET @SelectString = "
        "\'select * FROM OPENQUERY(INSQL, \"SELECT DateTime, [Reactor_001.ReactLevel] FROM WideHistory WHERE wwResolution = 3600000\")\' "
        "EXEC(@SelectString) "
        );
*/

// boost::log advanced usage example
void initLog(){

}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    ShiftManager shiftManager;

    //auto eventLog = new EventLogScope::EventLog();

    /* end boost::log init part */

    //QTextCodec::setCodecForLocale( QTextCodec::codecForName( "UTF-8" ) );

    const QString configFile(QCoreApplication::applicationDirPath() + "/config.json");

    try {
        //qDebug() << configFile;
        shiftManager.read(configFile);
        shiftManager.createWorker();
        if(!shiftManager.getWorker()->exec(testQuery2.arg("Reactor_001"))){
           //qDebug() << " query failed: " << testQuery2;
        }
    }
    catch (SYS::Error &error) {
        //SYS::warning(this, tr("Error"), tr("Failed to load %1: %2")
        //             .arg(configFile).arg(QString::fromUtf8(error.what())));
        _logger::Instance().LogEvent(EventLogScope::critical, error.what());
        return 0;
    }

    return a.exec();
}
