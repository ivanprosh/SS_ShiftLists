//#include "sys_messages.h"
#include "shiftmanager.h"
#include "logger.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>
#include <QStringList>
#include <QDate>
#include <QThread>
/*
const QHash<QString,QStringList> ShiftManager::scadaQueriesStrings {
    {"wonderware",{
                  //first query - main query
                  "SET QUOTED_IDENTIFIER OFF "
                  "SELECT * FROM OPENQUERY(INSQL, \"SELECT DateTime, %1 FROM WideHistory "
                  "WHERE wwResolution = %2 " //3600000 - 1 hour interval
                  "AND wwQualityRule = \'Extended\' "
                  "AND wwVersion = \'Latest\' "
                  "AND DateTime >= \'%3\' "
                  "AND DateTime <= \'%4\'\")",

                  //second query - description tag query
                  "SELECT Tag.TagName, Tag.Description "
                  "FROM Runtime.dbo.AnalogTag, Runtime.dbo.Tag "
                  "WHERE Tag.TagName IN (\'Reactor_001.ReactLevel\') "
                  "AND Tag.TagName = AnalogTag.TagName"
                  }
    }
};
*/
int ShiftManager::Shift::count = 2;
int ShiftManager::Shift::intervalInMinutes = 60;

namespace {
    const QHash<QString,std::function<void(ShiftManager*)> > DocCreatorMap{
        {"html",&ShiftManager::createDocWorker<HTMLShiftWorker>}
    };
    const QHash<QString,std::function<void(ShiftManager*)> > DBAdaptersCreatorMap{
        {"wonderware",&ShiftManager::createDBAdapter<WonderwareDBAdapter>}
    };
}

QPair<int,int> timeFromString(QString source) {
    QPair<int,int> result{-1,-1};
    //match 8:00, 08:11
    QRegExp rx("(\\d{1,2}):(\\d{1,2})");
    int pos = rx.indexIn(source);
    if(pos>-1){
        result.first = rx.cap(1).toInt();
        result.second = rx.cap(2).toInt();
        qDebug() << Q_FUNC_INFO << " h:" << result.first << " m:" << result.second;
    }
    return result;
}

ShiftManager::ShiftManager(QObject *parent) : QObject(parent),isPermanent(false),
    retryConnectDelaySecs(1), printTimeOffset(0),m_dateTimeformat("yyyy-MM-dd hh:mm:ss.zz")
{
    //query.prepare(queryPattern);
}

void ShiftManager::process()
{
    connect(&m_everyShiftTimer, SIGNAL(timeout()), this, SLOT(work()));
    connect(this,SIGNAL(exit()),qApp, SLOT(quit()));
    QTimer::singleShot(0,this,SLOT(work()));
}

void ShiftManager::read(const QString &filename)
{
    if (filename.isEmpty())
      throw SYS::Error("config file not specified");

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        throw SYS::Error(filename + ":" + file.errorString());

    SYS_LOG(EventLogScope::notification, "Config file opened")

    QByteArray saveData = file.readAll();
    QJsonParseError err;
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData,&err));
    if(loadDoc.isNull())
        throw SYS::Error(err.errorString());
    read(loadDoc.object());
}

bool ShiftManager::readShiftsInfo(const QJsonObject &obj)
{
    auto startFirstShiftTime = timeFromString(obj["start"].toString());
    auto stopFirstShiftTime = timeFromString(obj["stop"].toString());
    auto countShifts = obj["count"].toInt();
    auto intervalShiftsSnapshot = obj["interval"].toInt();

    QTime startT(startFirstShiftTime.first,startFirstShiftTime.second);
    QTime stopT(stopFirstShiftTime.first,stopFirstShiftTime.second);

    m_shifts.clear();

    if(startT.isValid() && stopT.isValid()
            && (countShifts>0) && (countShifts<10)
            && (intervalShiftsSnapshot>0)
            && (intervalShiftsSnapshot<(stopFirstShiftTime.first-startFirstShiftTime.first)*24)
            && Shift::check(startT,stopT)){

        int it = 0;
        QTime startCurrentShift(startT), stopCurrentShift(stopT);
        auto stepTime = startCurrentShift.msecsTo(stopCurrentShift);

        Q_ASSERT(stepTime > 0);

        while(++it <= countShifts){
            Shift sh;
            sh.start = startCurrentShift;
            sh.stop = stopCurrentShift;
            if(startCurrentShift.msecsTo(stopCurrentShift)<0)
                sh.isNightCross = true;
            m_shifts.append(sh);

            //auto startPrevShift = startCurrentShift;
            startCurrentShift = stopCurrentShift;
            stopCurrentShift = stopCurrentShift.addMSecs(stepTime);

            qDebug() << Q_FUNC_INFO << "Step Time: " << stepTime/(1000 * 60)
                                    << "Start: " << sh.start.hour() << ":"<<sh.start.minute()
                                    << "Stop:" << sh.stop.hour() << ":"<<sh.stop.minute();
            if(startT == startCurrentShift)
                break;
        }

        Shift::intervalInMinutes = intervalShiftsSnapshot;
        Shift::count = countShifts;

        //sort in ascending order
        qSort(m_shifts.begin(),m_shifts.end(),[&](const Shift& first,const Shift& second)
            {return first.start.msecsSinceStartOfDay() < second.start.msecsSinceStartOfDay();}
        );
        qDebug() << Q_FUNC_INFO << ", first shift time after qsort:" << m_shifts.first().start.hour();
        return true;
    }
    return false;
}

void ShiftManager::read(const QJsonObject &&object)
{
    foreach (auto key, object.keys()) {
        if(key=="tags") {
            m_tagsNamesList.clear();
            auto tagList = object.value(key).toArray();
            while(!tagList.isEmpty()){
                m_tagsNamesList << QString("%1").arg(tagList.takeAt(0).toString());
            }
            continue;
        }
        configGroups[key] = object.value(key).toObject();
    }
    //parse "output" group
    QJsonObject outputObj = configGroups["output"];
    if(!outputObj.isEmpty()){
        QJsonValue formatVal = outputObj["format"];
        qDebug() << formatVal.toString();
        if(DocCreatorMap.keys().contains(formatVal.toString()))
            DocCreatorMap.value(formatVal.toString())(this);
    }
    //parse "options" group
    QJsonObject optionsObj = configGroups["options"];
    if(!optionsObj.isEmpty()){
        QJsonObject shiftObj = optionsObj["shift"].toObject();
        if(!readShiftsInfo(shiftObj)){
            throw SYS::Error("Shift group of options don't correct.");
        }
        //print time offset
        QJsonValue printObjVal = optionsObj["printTimeOffset"];
        if(!printObjVal.isUndefined()){
            printTimeOffset = printObjVal.toInt();
            //printTimeOffset =
        }

        //scada value
        QJsonValue scadaObjVal = optionsObj["scada"];
        if(!scadaObjVal.isUndefined()){
            if(DBAdaptersCreatorMap.keys().contains(scadaObjVal.toString())){
                DBAdaptersCreatorMap.value(scadaObjVal.toString())(this);
                //mainQueryStringPattern = scadaQueriesStrings.value(scadaObjVal.toString()).first();
                //descrTagQueryStringPattern = scadaQueriesStrings.value(scadaObjVal.toString()).value(1);
            } else {
                throw SYS::Error(
                         QString("Cannot recognize scada. Check options:scada in config. Supported scada's: %1").
                             arg(DBAdaptersCreatorMap.keys().join(",")).toUtf8());
            }
        }
        //dateTimeformat
        QJsonValue dateTimeFormatObjVal = optionsObj["specDateTimeFormat"];
        if(!dateTimeFormatObjVal.isUndefined()){
            m_dateTimeformat = dateTimeFormatObjVal.toString();
        }
    }
    if(m_tagsNamesList.isEmpty())
        throw SYS::Error("Tags names not recognized, check config file");

    SYS_LOG(EventLogScope::notification, "Config file parsed");
}

void ShiftManager::createSQLWorker()
{
    SYS_LOG(EventLogScope::notification, "Create new SQL worker instance");

    SQLWorker* worker = new SQLWorker(configGroups.value("server").toVariantMap());
    QThread* thread = new QThread;
    connect( thread, SIGNAL( finished() ), worker, SLOT( deleteLater() ) );
    connect( thread, SIGNAL( finished() ), thread, SLOT( deleteLater() ) );
    //connect( task, SIGNAL( ready( int, int, QImage ) ), SLOT( onPartReady( int, int, QImage ) ) );
    connect( worker, SIGNAL(connected()), thread, SLOT( quit() ) );
    connect( thread, SIGNAL( started() ), worker, SLOT( connect() ));
    connect( worker, SIGNAL( error(SYS::QError) ), this, SLOT( onError(SYS::QError) ));

    worker->moveToThread( thread );
    thread->start();
    /*
    m_SQLWorker.reset(new SQLWorker(configGroups.value("server").toVariantMap()));
    connect(m_SQLWorker.data(),SIGNAL(error(SYS::QError)),
                       this,SLOT(onError(SYS::QError)));
    connect(m_SQLWorker.data(),SIGNAL(connected()),
            this,SLOT(onConnected()));
            */
}

QString ShiftManager::prepareMainQuery(QDateTime& requestedDateTime)
{
    int CurshiftIndex = getShiftIndexOnReqTime(requestedDateTime.time());
    auto& prevShift = (CurshiftIndex==0) ? m_shifts.last() : m_shifts.at(CurshiftIndex-1);

    return m_DBAdapter->prepareQuery(DBAdapter::QueryTypes::TagValuesList,
                                            QVariantMap {{"taglist",QVariant::fromValue(m_tagsNamesList)},
                                                         {"shift",QVariant::fromValue(prevShift)},
                                                         {"datetime",QVariant::fromValue(requestedDateTime)},
                                                         {"datetimeformat",QVariant::fromValue(m_dateTimeformat)}});
}

QString ShiftManager::prepareTagDescriptionQuery()
{
    //QString queryString = descrTagQueryStringPattern;
    return m_DBAdapter->prepareQuery(DBAdapter::QueryTypes::TagDescriptionList,
                                            QVariantMap {{"taglist",QVariant::fromValue(m_tagsNamesList)}});

}

QList<QString> ShiftManager::supportedOutputFormats()
{
    return DocCreatorMap.keys();
}

int ShiftManager::getShiftIndexOnReqTime(QTime &reqTime)
{
    auto it = std::find_if(m_shifts.begin(),m_shifts.end(),
                 [&](const Shift& sh){return sh.isMyTime(reqTime);});
    if(it != m_shifts.end())
        return m_shifts.indexOf(*it);
    return -1;
}

SQLWorker* ShiftManager::getSQLWorker()
{
    return m_SQLWorker.data();
}

void ShiftManager::onError(SYS::QError err)
{
    qDebug() << Q_FUNC_INFO << " Err: " << SYS::toUType(err.type);
    QString logmsg("%1:%2");
    //logmsg = logmsg.arg("SQL error: ");

    switch (SYS::toUType(err.type)) {
    case SYS::QError::Type::ConnectionError:
        logmsg = logmsg.arg("ConnectionError");
        //if SQL sender trying to reconnect
        if(SQLWorker* _sender = qobject_cast<SQLWorker*>(sender())) {
            QTimer::singleShot(retryConnectDelaySecs*1000,_sender,SLOT(connect()));
            retryConnectDelaySecs <<= 1;
            if(retryConnectDelaySecs > 3600)
                emit exit();//retryConnectDelaySecs = 1;
        }
        break;
    default:
        logmsg = logmsg.arg("Unknown type");
        break;
    }
    if(err.level > EventLogScope::warning) {
        SYS_LOG_WINDOW(err.level, qPrintable(logmsg), &SYS::warning)
        emit exit();
    } else
        SYS_LOG(EventLogScope::warning, qPrintable(logmsg));
}

void ShiftManager::onConnected() {
    retryConnectDelaySecs=1;
    SYS_LOG(EventLogScope::notification, "Connect to db success");

    if(m_tagsNameDescription.isEmpty())
        ;//m_tagsNameDescription = m_SQLWorker.data()->exec(prepareTagDescriptionQuery());
    //prepare query
    //prepareQuery(QDateTime::currentDateTime());
}

void ShiftManager::work()
{
    if(m_tagsNameDescription.isEmpty())
        ;//m_tagsNameDescription = m_DBAdapter.data()->
        //        getTagDescr(m_SQLWorker.data()->exec(prepareTagDescriptionQuery()));
    if(m_SQLWorker.isNull())
        createSQLWorker();

    if(!isPermanent)
        ;//emit exit();
}

bool ShiftManager::Shift::check(QTime& start,QTime& stop){
    auto diffInMinutes = start.msecsTo(stop)/(1000*60);
    if(diffInMinutes < 1){
        SYS_LOG(EventLogScope::warning, "Finished time of shift less then start or diff between them less 1 minute.")
        //if stop time less then start
        //diffInMinutes = 24*60 - stop.msecsTo(start)/(1000*60);
        return false;
    }
    if((24*60 % diffInMinutes) != 0){
        SYS_LOG(EventLogScope::warning, "Counts of shifts not filling all day.")
        return false;
    }
    return true;
}
