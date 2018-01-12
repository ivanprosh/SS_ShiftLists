//#include "sys_messages.h"
#include "shiftmanager.h"
#include "sqlworker.h"
#include "logger.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonDocument>

ShiftManager::ShiftManager(QObject *parent) : QObject(parent)
{
    //_logger::Instance().LogEvent(EventLogScope::notification, "Hello");
}

void ShiftManager::read(const QString &filename)
{
    if (filename.isEmpty())
        throw SYS::Error("config file not specified");

    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly))
        throw SYS::Error(file.errorString());

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
        options[key] = object.value(key).toObject().toVariantMap();
    }
    _logger::Instance().LogEvent(EventLogScope::notification, "Config file parsed");
}

void ShiftManager::createWorker()
{
    _logger::Instance().LogEvent(EventLogScope::notification, "Create new worker instance");
    m_worker.reset(new SQLWorker(options.value("server")));
}

SQLWorker* ShiftManager::getWorker()
{
    return m_worker.data();
}
