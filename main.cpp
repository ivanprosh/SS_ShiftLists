#include "shiftmanager.h"
//#include "sys_messages.h"
#include "sqlworker.h"
#include "logger.h"
#include "sys_error.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QTextCodec>


const QString defaultconfigFileName("config.json");


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

/*
const QString testQuery3(
        "SET QUOTED_IDENTIFIER OFF "
        "DECLARE @SelectString VARCHAR(5000) "
        "SET @SelectString = "
        "\'select * FROM OPENQUERY(INSQL, \"SELECT DateTime, [Reactor_001.ReactLevel] FROM WideHistory WHERE wwResolution = 3600000\")\' "
        "EXEC(@SelectString) "
        );
*/
class ShiftManager;

void parseCommandLine(QCommandLineParser &parser, ShiftManager& controlClass){
    parser.setApplicationDescription("Shift lists app helper");
    parser.addHelpOption();
    parser.addVersionOption();
    //parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
    //parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));

    parser.addOptions({
              {"p",
                  QCoreApplication::translate("main", "Run in background permanent")},
              // An option with a value
              {{"f","filepath"},
                  QCoreApplication::translate("main", "<path> to config file"),
                  QCoreApplication::translate("main", "path")},
              // An option with a value
              {{"t", "time-for-execute"},
                  QCoreApplication::translate("main", "<Date Time> then app scanning shifts.\n"
                                                      " Format - yyyy-MM-dd hh:mm:ss. (2018-01-12 18:00:23)"),
                  QCoreApplication::translate("main", "date time")},
          });

      // Process the actual command line arguments given by the user
      if(!parser.parse(QCoreApplication::arguments()))
          throw SYS::Error(parser.errorText());

      if (parser.isSet("version")){
          parser.showVersion();
          //return;
      }
      if (parser.isSet("help")){
          parser.showHelp();
          //return;
      }

      if (parser.isSet("f")){
          _logger::Instance().LogEvent(EventLogScope::notification,
                                       QString("Read file path from command args: %1").arg(parser.value("f")).toUtf8());
          controlClass.read(parser.value("f"));
      } else {
          _logger::Instance().LogEvent(EventLogScope::notification,
                                       QString("Read file path from application work dir: %1").arg(parser.value("f")).toUtf8());
          controlClass.read(QGuiApplication::applicationDirPath() + "/" + defaultconfigFileName);
      }

      if (parser.isSet("p")){
          _logger::Instance().LogEvent(EventLogScope::notification,
                                       "Set permanent execute mode");
          controlClass.isPermanent = true;
      }

      if (parser.isSet("t")){
          _logger::Instance().LogEvent(EventLogScope::notification,
                                        QString("Set specific execute time: %1").arg(parser.value("t")).toUtf8());
          QDateTime dt = QDateTime::fromString(parser.value("t"));
          if(!dt.isValid())
              throw SYS::Error("Format DateTime (-t) not recognized, see --help");
          controlClass.setSpecDateTime(dt);
      }

      const QStringList args = parser.positionalArguments();

      qDebug() << args;
      // source is args.at(0), destination is args.at(1)
/*
      bool showProgress = parser.isSet(showProgressOption);
      bool force = parser.isSet(forceOption);
      QString targetDir = parser.value(targetDirectoryOption);
*/

}

using namespace SYS;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("Shift lists print fo Wonderware projects");
    QApplication::setApplicationVersion("1.0");

    //register type for use in slot-signal model
    qRegisterMetaType<QError>();

    QCommandLineParser parser;
    ShiftManager shiftManager;

    try {
        //parse commandLine par
        parseCommandLine(parser,shiftManager);

        //create SQL worker and connect to database
        //shiftManager.createSQLWorker();

    }
    catch (std::exception &error) {
        //SYS::warning(this, tr("Error"), tr("Failed to load %1: %2")
        //             .arg(configFile).arg(QString::fromUtf8(error.what())));
        _logger::Instance().LogEvent(EventLogScope::critical, error.what());
        return 0;
    }

    //Init timers and other connections
    shiftManager.start();

    //return 0;
    return a.exec();
}
