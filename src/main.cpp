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

class ShiftManager;

void parseCommandLine(QCommandLineParser &parser, ShiftManager& controlClass){
    parser.setApplicationDescription("SS_Shiftlists application command options helper");
    parser.addHelpOption();
    parser.addVersionOption();
    //parser.addPositionalArgument("source", QCoreApplication::translate("main", "Source file to copy."));
    //parser.addPositionalArgument("destination", QCoreApplication::translate("main", "Destination directory."));

    parser.addOptions({
              {"p",
                  QCoreApplication::translate("main", "Run in background permanent and print shifts\n"
                                                      "every time then shift finished with offset from config file.\n")},
              // An option with a value
              {{"f","filepath"},
                  QCoreApplication::translate("main", "<path> to config file. Config file options help see in manual"),
                  QCoreApplication::translate("main", "path")},
              {{"o","outputpath"},
                  QCoreApplication::translate("main", "<path> to output dir.\n"
                                                      "Without this flag, output dir path= output.path options value from config file."),
                  QCoreApplication::translate("main", "path")},
              // An option with a value
              {{"t", "time-for-execute"},
                  QCoreApplication::translate("main", "<Date Time> then app printing shifts.\n"
                                                      "Format - yyyy-MM-dd hh:mm. (2018-01-12 18:00)\n"
                                                      "Must be used then you want to print out current shift.\n"
                                                      "Example: you want to print shift data 2018-01-12 08:00 - 20:00.\n"
                                                      "You may execute program with -t \"2018-01-12 20:01\" \n"
                                                      ),
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

      if (parser.isSet("o")){
          _logger::Instance().LogEvent(EventLogScope::notification,
                                       QString("Read output file path from command args: %1").arg(parser.value("o")).toUtf8());
          controlClass.m_outputPath = parser.value("o");
      }

      if (parser.isSet("p")){
          _logger::Instance().LogEvent(EventLogScope::notification,
                                       "Set permanent execute mode");
          controlClass.isPermanent = true;
      }

      if (parser.isSet("t")){
          _logger::Instance().LogEvent(EventLogScope::notification,
                                        QString("Set specific execute time: %1").arg(parser.value("t")).toUtf8());
          QDateTime dt = QDateTime::fromString(parser.value("t"),"yyyy-MM-dd hh:mm");
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
    QApplication::setApplicationName("Shift lists print fo 3rd party scada projects");
    QApplication::setApplicationVersion("1.0");

    //register type for use in slot-signal model
    qRegisterMetaType<QError>();
    qRegisterMetaType<SQLWorker::QueryTypes>();
    qRegisterMetaType<QSqlQuery>();

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

    qDebug() << "Main thr: " << qApp->thread()->currentThreadId();
    //Init timers and other connections
    shiftManager.start();

    return a.exec();
}
