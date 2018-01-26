//#include "sys_messages.h"
#include "shiftmanager.h"
#include "logger.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDate>
#include <QFinalState>
#include <QSignalTransition>
#include <QTextDocument>
#include <QSqlQuery>

int ShiftManager::Shift::count = 0;
int ShiftManager::Shift::intervalInMinutes = 60;

using SYS::QError;

namespace {
    const QHash<QString,std::function<void(ShiftManager*)> > DevCreatorMap{
        {"html",&ShiftManager::createDevWorker<HtmlDevPolicy>},
        //{"pdf",&ShiftManager::createDevWorker<PdfDevPolicy>},
        {"printer",&ShiftManager::createDevWorker<PrinterDevPolicy>}
    };
    const QHash<QString,std::function<void(ShiftManager*)> > DBAdaptersCreatorMap{
        {"wonderware",&ShiftManager::createDBAdapter<WonderwareDBAdapter>}
    };
    const int optimalColumnCount = 12;
}

QPair<int,int> timeFromString(QString source) {
    QPair<int,int> result{-1,-1};
    //match 8:00, 08:11
    QRegExp rx("(\\d{1,2}):(\\d{1,2})");
    int pos = rx.indexIn(source);
    if(pos>-1){
        result.first = rx.cap(1).toInt();
        result.second = rx.cap(2).toInt();
        //qDebug() << Q_FUNC_INFO << " h:" << result.first << " m:" << result.second;
    }
    return result;
}

ShiftManager::ShiftManager(QObject *parent) : QObject(parent),isPermanent(false),
    printTimeOffset(0),m_dateTimeformat("yyyy-MM-dd hh:mm:ss.zz"),
    m_needRecreateSQLWorker(false),m_isCriticalErrorSet(false),m_isFinishedCycle(false),
    m_sqlQueriesBits(0x00),m_retryAttemptsInterval(1),m_curShiftIndex(0),m_prevShiftIndex(0),
    m_outputPath(QApplication::applicationDirPath())
{
    /*
    connect(&page, &QWebEnginePage::loadFinished,
                   this,
                   [this](bool ok){this->page.printToPdf([=](const QByteArray& data){qDebug() << "data: " << data.isEmpty();}); qDebug() << "Print pdf: " << ok;} );
    */
    createDocWorker<HTMLShiftWorker>();

    initStateMachine();
}

ShiftManager::~ShiftManager()
{
    if(!m_SQLWorkerThr.isNull()){
        m_SQLWorkerThr.data()->quit();
        m_SQLWorkerThr.data()->wait();
    }
}

void ShiftManager::onStartState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("start").toUtf8())

    //if critical error set before last cycle or app is not permanent
    if(isCriticalErrorSet() || (isFinishedCycle() && !isPermanent)){
        emit exit();
        m_everyShiftTimer.stop();
    }

    //first or after non critical err
    if(!isFinishedCycle())
        QTimer::singleShot(m_retryAttemptsInterval,this,[this]{this->startStateSuccess();});

    //timeout of update timer was detected
    if(needUpdate()){
        emit startStateSuccess();
        setNeedUpdate(false);
    }

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
            SYS_LOG_WINDOW(err.level, logmsg.toUtf8(), &SYS::warning)
            if(err.level >= EventLogScope::error)
                setIsCriticalErrorSet(true);
        } else
            SYS_LOG(EventLogScope::warning, logmsg.toUtf8());

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

    m_SQLWorkerThr.reset(new SqlThread(configGroups.value("server").toVariantMap()));

    connect( m_SQLWorkerThr.data(), SIGNAL(connected()), this, SIGNAL( connectSqlWorkerSuccess() ), Qt::QueuedConnection );
    connect( m_SQLWorkerThr.data(), SIGNAL( error(SYS::QError) ), this, SLOT( onError(SYS::QError) ));
    connect( this, &ShiftManager::queryExec, m_SQLWorkerThr.data(), &SqlThread::queryExec);
    connect( m_SQLWorkerThr.data(), &SqlThread::queryResult, this, &ShiftManager::onQueryResult);
    /*
    SQLWorker* worker = new SQLWorker(configGroups.value("server").toVariantMap());
    m_SQLWorkerThr.reset(new QThread);
    worker->moveToThread( m_SQLWorkerThr.data() );
    connect( m_SQLWorkerThr.data(), SIGNAL( finished() ), worker, SLOT( deleteLater() ) );
    //connect( this, SIGNAL( workSqlWorkerSuccess()), m_SQLWorkerThr.data(), SLOT( quit() ) );
    //connect( m_SQLWorker.data(), SIGNAL( finished() ), m_SQLWorker.data(), SLOT( deleteLater() ) );
    connect( worker, SIGNAL(connected()), this, SIGNAL( connectSqlWorkerSuccess() ), Qt::QueuedConnection );
    connect( this, &ShiftManager::connectToDB, worker, &SQLWorker::connect );
    //connect( m_SQLWorkerThr.data(), SIGNAL( started() ), worker, SLOT( connect() ));
    connect( worker, SIGNAL( error(SYS::QError) ), this, SLOT( onError(SYS::QError) ));
    connect( this, &ShiftManager::queryExec, worker, &SQLWorker::exec);
    connect( worker, &SQLWorker::queryResult, this, &ShiftManager::onQueryResult);
    */

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
    QMetaObject::invokeMethod(m_SQLWorkerThr.data(), "connectToDb", Qt::QueuedConnection);
    //emit connectToDB();

}

void ShiftManager::onWorkSqlWorkerState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("sql work").toUtf8())
    if(!(sqlQueriesBits() & SYS::toUType(QueriesResult::TagDescriptionSuccess)))
        emit queryExec(SQLWorker::QueryTypes::TagDescription, prepareTagDescriptionQuery());
    else if(!(sqlQueriesBits() & SYS::toUType(QueriesResult::TagValuesSuccess)))
        emit queryExec(SQLWorker::QueryTypes::TagValues,
                   prepareMainQuery(specDateTime().isValid() ? specDateTime() : QDateTime::currentDateTime()));
    //page.setHtml("<html><body><h2 align='center'>Сменная ведомость</h2><body/></html>");
}

void ShiftManager::onWorkDocWorkerState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("create doc").toUtf8())
    if(m_DocWorker.isNull()) {
        onError(SYS::QError(EventLogScope::error,
                            QError::Type::InternalError, "Doc creator must be valid in current state"));
        return;
    }

    m_DocWorker->setShiftIndex(m_prevShiftIndex);
    m_DocWorker->process(m_lastDoc);
}

void ShiftManager::onWorkDevWorkerState()
{
    SYS_LOG(EventLogScope::notification,QString("In %1 state").arg("save/print doc").toUtf8())
    if(m_DevWorker.isNull()){
        onError(SYS::QError(EventLogScope::warning,
                            QError::Type::InternalError, "Device creator must be valid in current state"));
        return;
    }
    auto dateTime = specDateTime().isValid() ? specDateTime() : QDateTime::currentDateTime();
    if(m_shifts.value(m_prevShiftIndex).isNightCross ||
            m_shifts.value(m_prevShiftIndex).start.msecsTo(dateTime.time()) < 0)
         dateTime = dateTime.addDays(-1);

    m_DevWorker->process(QString("%1/%2_SL_%3_%4_%5").arg(m_outputPath)
                         .arg(m_Node)
                         .arg(dateTime.date().toString("yyyy-MM-dd"))
                         .arg(QString(m_DocWorker->horHeaderTableTitles().first()).remove(":"))
                         .arg(QString(m_DocWorker->horHeaderTableTitles().last()).remove(":")),
                        m_lastDoc);


    //emit workDevWorkerSuccess();
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
    //auto countShifts = obj["count"].toInt();
    auto intervalShiftsSnapshot = obj["interval"].toInt();

    QTime startT(startFirstShiftTime.first,startFirstShiftTime.second);
    QTime stopT(stopFirstShiftTime.first,stopFirstShiftTime.second);

    m_shifts.clear();

    if(startT.isValid() && stopT.isValid()
            //&& (countShifts>0) && (countShifts<10)
            && (intervalShiftsSnapshot>0)
            //interval snapshot must be less then size of shift time
            && (intervalShiftsSnapshot*60*1000 < startT.msecsTo(stopT))
            //and common columns must be less then optimal count
            && (intervalShiftsSnapshot*60*1000*optimalColumnCount >= startT.msecsTo(stopT))
            && (Shift::count = Shift::check(startT,stopT))){

        int it = 0;
        QTime startCurrentShift(startT), stopCurrentShift(stopT);
        auto stepTime = startCurrentShift.msecsTo(stopCurrentShift);

        Q_ASSERT(stepTime > 0);

        while(++it <= Shift::count){
            Shift sh;
            sh.start = startCurrentShift;
            sh.stop = stopCurrentShift;
            if(startCurrentShift.msecsTo(stopCurrentShift)<0)
                sh.isNightCross = true;
            m_shifts.append(sh);

            //auto startPrevShift = startCurrentShift;
            startCurrentShift = stopCurrentShift;
            stopCurrentShift = stopCurrentShift.addMSecs(stepTime);

            qDebug() << "Shift №" << it << "Step Time: " << stepTime/(1000 * 60)
                                        << "Start: " << sh.start.hour() << ":"<<sh.start.minute()
                                        << "Stop:" << sh.stop.hour() << ":"<<sh.stop.minute();
            if(startT == startCurrentShift)
                break;
        }

        Shift::intervalInMinutes = intervalShiftsSnapshot;

        //sort in ascending order
        std::sort(m_shifts.begin(),m_shifts.end(),[&](const Shift& first,const Shift& second)
            {return first.start.msecsSinceStartOfDay() < second.start.msecsSinceStartOfDay();}
        );
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
        QJsonValue pathVal = outputObj["path"];
        if(!pathVal.isUndefined() && !pathVal.toString().isEmpty() )
            m_outputPath = pathVal.toString();
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
    //node
    QJsonValue NodeObjVal = object["node"];
    if(!NodeObjVal.isUndefined()){
        m_Node = NodeObjVal.toString();
        m_DocWorker->setNodeName(m_Node);
    }
    if(m_tagsNamesList.isEmpty())
        throw SYS::Error("Tags names not recognized, check config file");

    SYS_LOG(EventLogScope::notification, "Config file parsed");
}

QString ShiftManager::prepareMainQuery(QDateTime requestedDateTime)
{
    m_curShiftIndex = getShiftIndexOnReqTime(requestedDateTime.time());
    auto& prevShift = (m_curShiftIndex==0) ? m_shifts.last() : m_shifts.at(m_curShiftIndex-1);
    m_prevShiftIndex = m_shifts.indexOf(prevShift);

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
    if(!query.isActive()) {
        onError(SYS::QError(EventLogScope::error,
                            QError::Type::InternalError, "Query must be active in current state"));
        return;
    }
    if(m_DocWorker.isNull()) {
        onError(SYS::QError(EventLogScope::error,
                            QError::Type::InternalError, "Doc creator must be valid in current state"));
        return;
    }
    QStringList headerTitles;

    switch (SYS::toUType(id)) {
    case SYS::toUType(SQLWorker::QueryTypes::TagDescription):
        SYS_LOG(EventLogScope::notification,"Tag description query get by Shift Manager")
        m_DocWorker->setVertHeaderTableTitles(m_DBAdapter->getTagDescr(std::move(query), m_tagsNamesList));
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
    if((sqlQueriesBits() & (SYS::toUType(QueriesResult::TagDescriptionSuccess)))
            && (sqlQueriesBits() & SYS::toUType(QueriesResult::TagValuesSuccess))){
        emit workSqlWorkerSuccess();
    } else
        onWorkSqlWorkerState();
}

void ShiftManager::onDocResult()
{
    if(m_lastDoc.isEmpty()){
        onError(SYS::QError(EventLogScope::error,
                            QError::Type::InternalError,
                            "Document must be filling in current state"));
        return;
    }
    emit workDocWorkerSuccess();
}

void ShiftManager::start()
{
    //update timer set to diff between shifts + printTimeOffset interval
    auto& shift = m_shifts.first();
    m_everyShiftTimer.setInterval(shift.start.msecsTo(shift.stop)
                                  + printTimeOffset*60*1000);
    connect(&m_everyShiftTimer,&QTimer::timeout,this,
            [=](){this->setNeedUpdate(true);});
    m_everyShiftTimer.start();
    stateMachine.start();
}

int ShiftManager::Shift::check(QTime& start,QTime& stop){
    auto diffInMinutes = start.msecsTo(stop)/(1000*60);
    //int count = 0;
    if(diffInMinutes < 1){
        SYS_LOG(EventLogScope::warning, "Finished time of shift less then start or diff between them less then 1 minute.")
        //if stop time less then start
        //diffInMinutes = 24*60 - stop.msecsTo(start)/(1000*60);
        return 0;
    }
    if((24*60 % diffInMinutes) != 0){
        SYS_LOG(EventLogScope::warning, "Counts of shifts not filling all day.")
        return 0;
    } else {
        return ((24*60) / diffInMinutes);
    }
}
