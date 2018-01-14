//#include "sys_messages.h"
#include "shiftmanager.h"
#include "logger.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QTimer>

const QString queryPattern(
        "SET QUOTED_IDENTIFIER OFF "
        "SELECT * FROM OPENQUERY(INSQL, \"SELECT DateTime, %1 FROM WideHistory WHERE wwResolution = 60000\")"
        );

ShiftManager::ShiftManager(QObject *parent) : QObject(parent),isPermanent(false),
    m_shift(Shift()), retryConnectDelaySecs(1)
{
    //query.prepare(queryPattern);
}

void ShiftManager::read(const QString &filename)
{
    if (filename.isEmpty())
      throw SYS::Error("config file not specified");

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        throw SYS::Error(filename + ":" + file.errorString());

    _logger::Instance().LogEvent(EventLogScope::notification, "Config file opened");

    QByteArray saveData = file.readAll();
    QJsonParseError err;
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData,&err));
    if(loadDoc.isNull())
        throw SYS::Error(err.errorString());
    read(loadDoc.object());
}

void ShiftManager::read(const QJsonObject &&object)
{
    foreach (auto key, object.keys()) {
        if(key=="tags") {
            m_tagsNamesList.clear();
            auto tagList = object.value(key).toArray();
            while(!tagList.isEmpty()){
                m_tagsNamesList << QString("[%1]").arg(tagList.takeAt(0).toString());
            }
            continue;
        }
        configGroups[key] = object.value(key).toObject();
    }
    //parse "options" group
    QJsonObject optionsObj = configGroups["options"];
    if(!optionsObj.isEmpty()){
        QJsonObject shiftObj = optionsObj["shift"].toObject();
        if(!m_shift.read(shiftObj))
            _logger::Instance().LogEvent(EventLogScope::warning,
                     "Shift group of options don't correct. App using default values.");
    }
    if(m_tagsNamesList.isEmpty())
        throw SYS::Error("Tags names not recognized, check config file");

    _logger::Instance().LogEvent(EventLogScope::notification, "Config file parsed");
}

void ShiftManager::createWorker()
{
    _logger::Instance().LogEvent(EventLogScope::notification, "Create new worker instance");
    m_worker.reset(new SQLWorker(configGroups.value("server").toVariantMap(),queryPattern));
    connect(m_worker.data(),SIGNAL(error(SQLWorker::Error)),
                       this,SLOT(onError(SQLWorker::Error)));
    connect(m_worker.data(),SIGNAL(connected()),
            this,SLOT(onConnected()));
}

void ShiftManager::prepareQuery()
{

}

SQLWorker* ShiftManager::getWorker()
{
    return m_worker.data();
}

void ShiftManager::onError(SQLWorker::Error err)
{
    qDebug() << Q_FUNC_INFO << " Err: " << err;
    QString logmsg("%1:%2");
    logmsg = logmsg.arg("SQL error: ");

    switch (err) {
    case SQLWorker::ConnectionError:
        logmsg = logmsg.arg("ConnectionError");
        //trying to reconnect
        QTimer::singleShot(retryConnectDelaySecs*1000,sender(),SLOT(connect()));
        retryConnectDelaySecs <<= 1;
        if(retryConnectDelaySecs > 3600)
            retryConnectDelaySecs = 1;
        break;
    default:
        logmsg = logmsg.arg("Unknown type");
        break;
    }
    _logger::Instance().LogEvent(EventLogScope::warning, qPrintable(logmsg));
}

void ShiftManager::onConnected() {
    retryConnectDelaySecs=1;
    _logger::Instance().LogEvent(EventLogScope::notification, "Connect to db success");
    //prepare query
    prepareQuery();
}

bool ShiftManager::Shift::read(const QJsonObject &obj){
    auto startTime = timeFromString(obj["start"].toString());
    auto stopTime = timeFromString(obj["stop"].toString());
    auto _isSingle = obj["isSingle"].toBool();
    QTime startT(startTime.first,startTime.second);
    QTime stopT(stopTime.first,stopTime.second);
    if(startT.isValid() && stopT.isValid()){
        start = startT;
        stop = stopT;
        isSingle = _isSingle;
        return true;
    }
    return false;
}
