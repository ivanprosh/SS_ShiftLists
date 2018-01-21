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
#include <QFinalState>
#include <QSignalTransition>
#include <QTextDocument>
#include <QSqlQuery>
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

using SYS::QError;

namespace {
    const QHash<QString,std::function<void(ShiftManager*)> > DevCreatorMap{
        {"html",&ShiftManager::createDevWorker<HtmlDevPolicy>},
        {"pdf",&ShiftManager::createDevWorker<PdfDevPolicy>},
        {"printer",&ShiftManager::createDevWorker<PrinterDevPolicy>}
    };
    const QHash<QString,std::function<void(ShiftManager*)> > DBAdaptersCreatorMap{
        {"wonderware",&ShiftManager::createDBAdapter<WonderwareDBAdapter>}
    };
    //QStringList availDevices {"file","printer"};
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
    printTimeOffset(0),m_dateTimeformat("yyyy-MM-dd hh:mm:ss.zz"),
    m_needRecreateSQLWorker(false),m_isCriticalErrorSet(false),m_isFinishedCycle(false),
    m_DocWorker(new HTMLShiftWorker),m_sqlQueriesBits(0x00),m_retryAttemptsInterval(1)
{
    initStateMachine();
}

void ShiftManager::onStartState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("start").toUtf8())
    //if critical error set before last cycle or app is not permanent
    if(isCriticalErrorSet() || (isFinishedCycle() && isPermanent))
        emit exit();

    //first or after non critical err
    if(!isFinishedCycle())
        QTimer::singleShot(m_retryAttemptsInterval,this,[this]{this->startStateSuccess();});
    /*
    connect(&m_everyShiftTimer, SIGNAL(timeout()), this, SLOT(start()));
    connect(this,SIGNAL(exit()),qApp, SLOT(quit()));
    QTimer::singleShot(0,this,SLOT(start()));
            */
    setIsFinishedCycle(false);
}

void ShiftManager::onErrorState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("error").toUtf8())
    while(!m_errors.isEmpty()) {
        QString logmsg("%1-%2");
        auto err = m_errors.dequeue();
        logmsg = logmsg.arg(SYS::QEnumToString(err.type));
        logmsg = logmsg.arg(err.what());

        if(err.level > EventLogScope::warning) {
            SYS_LOG_WINDOW(err.level, qPrintable(logmsg), &SYS::warning)
            if(err.level > EventLogScope::error)
                setIsCriticalErrorSet(true);
        } else
            SYS_LOG(EventLogScope::warning, qPrintable(logmsg));

        m_retryAttemptsInterval <<= 1;
        /*
        switch (SYS::toUType(err.type)) {
        case SYS::QError::Type::ConnectionError:
            logmsg = logmsg.arg("ConnectionError");
            //if SQL sender trying to reconnect
            if(SQLWorker* _sender = qobject_cast<SQLWorker*>(sender())) {
                createSQLWorker(true);

                //QTimer::singleShot(retryConnectDelaySecs*1000,_sender,SLOT(connect()));
                //retryConnectDelaySecs <<= 1;
                //if(retryConnectDelaySecs > 3600)
                    emit exit();//retryConnectDelaySecs = 1;
            }
            break;
        default:
            logmsg = logmsg.arg("Unknown type");
            break;
        }
        */
    }
    emit errorAccept();
}
void ShiftManager::onCreateSqlWorkerState()
{
    if(!m_SQLWorkerThr.isNull() && !m_SQLWorkerThr->isFinished()){
        if(needRecreateSQLWorker()) {
            SYS_LOG(EventLogScope::notification, "Recreate new SQL worker instance");
            QMetaObject::invokeMethod(m_SQLWorkerThr.data(), "quit", Qt::QueuedConnection);
            setNeedRecreateSQLWorker(false);

            QEventLoop loop;
            QTimer timeouttimer;
            timeouttimer.setSingleShot(true);
            connect(m_SQLWorkerThr.data(), SIGNAL( finished()), &loop, SLOT(quit()) );
            connect(&timeouttimer, SIGNAL(timeout()), &loop, SLOT(quit()));
            timeouttimer.start(10000);
            loop.exec();

            //if timer not triggered - good, thread are finished,
            //else - emit signal and change state to errState
            if(!timeouttimer.isActive()){
                //thread is not finished
                SYS_LOG(EventLogScope::warning,"Timeout while waiting finished thread.")
                emit createSqlWorkerFailed();
                return;
            }
        } else {
            //test signal
            //emit createSqlWorkerFailed();
            emit createSqlWorkerSuccess();
            return;
        }
    }

    SYS_LOG(EventLogScope::notification, "Create new SQL worker instance");

    SQLWorker* worker = new SQLWorker(configGroups.value("server").toVariantMap());
    m_SQLWorkerThr.reset(new QThread);
    connect( m_SQLWorkerThr.data(), SIGNAL( finished() ), worker, SLOT( deleteLater() ) );
    //connect( m_SQLWorkerThr.data(), &QThread::finished, this, [this](){this->onCreateSqlWorkerState(true);} );
    //connect( m_SQLWorker.data(), SIGNAL( finished() ), m_SQLWorker.data(), SLOT( deleteLater() ) );
    connect( worker, SIGNAL(connected()), this, SIGNAL( connectSqlWorkerSuccess() ) );
    connect( this, &ShiftManager::connectToDB, worker, &SQLWorker::connect );
    //connect( m_SQLWorkerThr.data(), SIGNAL( started() ), worker, SLOT( connect() ));
    connect( worker, SIGNAL( error(SYS::QError) ), this, SLOT( onError(SYS::QError) ));
    connect( this, &ShiftManager::queryExec, worker, &SQLWorker::exec);
    connect( worker, &SQLWorker::queryResult, this, &ShiftManager::onQueryResult);

    worker->moveToThread( m_SQLWorkerThr.data() );
    //m_SQLWorkerThr->start();

    emit createSqlWorkerSuccess();
    /*
    m_SQLWorker.reset(new SQLWorker(configGroups.value("server").toVariantMap()));
    connect(m_SQLWorker.data(),SIGNAL(error(SYS::QError)),
                       this,SLOT(onError(SYS::QError)));
    connect(m_SQLWorker.data(),SIGNAL(connected()),
            this,SLOT(onConnected()));
            */
}

void ShiftManager::onConnectSqlWorkerState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("connect").toUtf8())
    if(m_SQLWorkerThr.isNull())
            onError(SYS::QError(EventLogScope::warning,
                                QError::Type::InternalError, "Thread must exist in connection state"));
    if(!m_SQLWorkerThr->isRunning())
        m_SQLWorkerThr->start();
    emit connectToDB();
}

void ShiftManager::onWorkSqlWorkerState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("sql work").toUtf8())
    if(!(sqlQueriesBits() & SYS::toUType(QueriesResult::TagDescriptionSuccess)))
        emit queryExec(SQLWorker::QueryTypes::TagDescription, prepareTagDescriptionQuery());
    else if(!(sqlQueriesBits() & SYS::toUType(QueriesResult::TagValuesSuccess)))
        emit queryExec(SQLWorker::QueryTypes::TagValues,
                   prepareMainQuery(specDateTime().isValid() ? specDateTime() : QDateTime::currentDateTime()));
}

void ShiftManager::onWorkDocWorkerState()
{
    if(m_DocWorker.isNull()) {
        onError(SYS::QError(EventLogScope::warning,
                            QError::Type::InternalError, "Doc creator must be valid in current state"));
        return;
    }
    m_DocWorker->process();
}

void ShiftManager::onWorkDevWorkerState()
{
    if(m_DevWorker.isNull()){
        onError(SYS::QError(EventLogScope::warning,
                            QError::Type::InternalError, "Device creator must be valid in current state"));
        return;
    }
    if((*m_DevWorker)(QString("%1/testResultFile").arg(QApplication::applicationDirPath()),m_lastDoc))
        emit workDevWorkerSuccess();
    else
        onError(m_DevWorker->lastError);
}

void ShiftManager::initStateMachine()
{
    //глобальные свойства
    QState* startState = new QState(&stateMachine);
    QState* createSqlWorkerState = new QState(&stateMachine);
    QState* processState = new QState(&stateMachine);
    QState* errorState = new QState(&stateMachine);
    QFinalState* finalState = new QFinalState(&stateMachine);

    //подсвойства
    QState* connectSqlWorkerState = new QState(processState);
    QState* workSqlWorkerState = new QState(processState);
    QState* workDocWorkerState = new QState(processState);
    QState* workDevWorkerState = new QState(processState);
    QFinalState* processFinalState = new QFinalState(processState);

    //transitions
    startState->addTransition(this, SIGNAL(exit()),finalState);
    startState->addTransition(this, SIGNAL(startStateSuccess()),createSqlWorkerState);

    //process
    QSignalTransition *finishedCycleTrans =
            processState->addTransition(processState,SIGNAL(finished()), startState);
    processState->addTransition(this, SIGNAL(errorChange()),errorState);

    //create SQL Worker state
    QSignalTransition *errFromSqlCreateStateTrans =
            createSqlWorkerState->addTransition(this, SIGNAL(createSqlWorkerFailed()), errorState);
    createSqlWorkerState->addTransition(this,SIGNAL(createSqlWorkerSuccess()), processState);
    createSqlWorkerState->assignProperty(this,"isCriticalErrorSet",false);
    //sub proccess
    connectSqlWorkerState->addTransition(this,SIGNAL(connectSqlWorkerSuccess()),workSqlWorkerState);
    workSqlWorkerState->addTransition(this,SIGNAL(workSqlWorkerSuccess()),workDocWorkerState);
    workDocWorkerState->addTransition(this,SIGNAL(workDocWorkerSuccess()),workDevWorkerState);
    workDevWorkerState->addTransition(this,SIGNAL(workDevWorkerSuccess()),processFinalState);
    //error state
    errorState->addTransition(this, SIGNAL(errorAccept()),startState);

    processState->setInitialState(connectSqlWorkerState);
    stateMachine.setInitialState(startState);
    stateMachine.setGlobalRestorePolicy(QStateMachine::DontRestoreProperties);

    connect(&stateMachine, SIGNAL(finished()), qApp,  SLOT(quit()));
    connect(startState, SIGNAL(entered()), this, SLOT(onStartState()));
    connect(createSqlWorkerState, SIGNAL(entered()), this, SLOT(onCreateSqlWorkerState()));
    connect(connectSqlWorkerState, SIGNAL(entered()), this, SLOT(onConnectSqlWorkerState()));
    connect(workSqlWorkerState, SIGNAL(entered()), this, SLOT(onWorkSqlWorkerState()));
    connect(workDocWorkerState, SIGNAL(entered()), this, SLOT(onWorkDocWorkerState()));
    connect(workDevWorkerState, SIGNAL(entered()), this, SLOT(onWorkDevWorkerState()));
    connect(errorState, SIGNAL(entered()), this, SLOT(onErrorState()));

    //on transition set need recreate flag
    connect(errFromSqlCreateStateTrans, &QSignalTransition::triggered,
            this, [this](){this->setNeedRecreateSQLWorker(true);});
    //on transition set need set finishcycle property and reset spec datetime property
    connect(finishedCycleTrans, &QSignalTransition::triggered,
            this, [this](){ setIsFinishedCycle(true);
                            setSpecDateTime(QDateTime());
                            setSqlQueriesBits(0x00);
                            setRetryAttemptsInterval(1);});
    //connect(processState, SIGNAL(finished()), this, SLOT(onStartState()));
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
        std::sort(m_shifts.begin(),m_shifts.end(),[&](const Shift& first,const Shift& second)
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
        if(DevCreatorMap.keys().contains(formatVal.toString())){
            DevCreatorMap.value(formatVal.toString())(this);
        }
        QJsonValue countTagsOnPage = outputObj["countTagsOnPage"];
        if(!countTagsOnPage.isUndefined())
            m_DocWorker->setCountTagsOnPage(countTagsOnPage.toInt());
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

QString ShiftManager::prepareMainQuery(QDateTime requestedDateTime)
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
    return m_DBAdapter->prepareQuery(DBAdapter::QueryTypes::TagDescriptionList,
                                            QVariantMap {{"taglist",QVariant::fromValue(m_tagsNamesList)}});

}

QList<QString> ShiftManager::supportedOutputFormats()
{
    return DevCreatorMap.keys();
}

void ShiftManager::onError(SYS::QError err)
{
    qDebug() << Q_FUNC_INFO << " Err: " << SYS::toUType(err.type);
    m_errors.enqueue(err);
    emit errorChange();
}

void ShiftManager::onQueryResult(SQLWorker::QueryTypes id, QSqlQuery query)
{
    if(!query.isValid()) {
        onError(SYS::QError(EventLogScope::warning,
                            QError::Type::InternalError, "Query must be valid in current state"));
        return;
    }
    if(!m_DocWorker.isNull()) {
        onError(SYS::QError(EventLogScope::warning,
                            QError::Type::InternalError, "Doc creator must be valid in current state"));
        return;
    }
    QStringList headerTitles;

    switch (SYS::toUType(id)) {
    case SYS::toUType(SQLWorker::QueryTypes::TagDescription):
        SYS_LOG(EventLogScope::notification,"Tag description query get by Shift Manager")
        m_DocWorker->setVertHeaderTableTitles(m_DBAdapter->getTagDescr(std::move(query)));
        setSqlQueriesBits(sqlQueriesBits() | SYS::toUType(QueriesResult::TagDescriptionSuccess));
        break;

    case SYS::toUType(SQLWorker::QueryTypes::TagValues):
        SYS_LOG(EventLogScope::notification,"Tag values query get by Shift Manager")
        m_DocWorker->setValuesMap(m_DBAdapter->getTagValuesMap(std::move(query),headerTitles));
        m_DocWorker->setHorHeaderTableTitles(headerTitles);
        setSqlQueriesBits(sqlQueriesBits() | SYS::toUType(QueriesResult::TagValuesSuccess));
        break;
    default:
        break;
    }

    //0000 0011 - both queries exec
    if(sqlQueriesBits() & (SYS::toUType(QueriesResult::TagDescriptionSuccess) | SYS::toUType(QueriesResult::TagValuesSuccess)))
        emit workSqlWorkerSuccess();
    else
        onWorkSqlWorkerState();
}

void ShiftManager::onDocResult(QTextDocument* doc)
{
    if(doc->isEmpty()){
        onError(SYS::QError(EventLogScope::error,
                            QError::Type::InternalError,
                            "Document must be filling in current state"));
        return;
    }
    m_lastDoc = doc;
    emit workDocWorkerSuccess();
}

void ShiftManager::start()
{
    stateMachine.start();
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
