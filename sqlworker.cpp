#include "sqlworker.h"
#include "logger.h"
#include "docpolicies.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QHash>
#include <QString>
#include <QTimer>
//#include <boost/log/common.hpp>
//#include <boost/log/sources/logger.hpp>
//namespace src = boost::log::sources;
//extern src::logger lg;

const QHash<QString,QString> SQLWorker::connString {
    {"MSSQL","DRIVER={SQL Server};SERVER=%1;DATABASE=%2;UID=%3;PWD=%4;Trusted_Connection=no; WSID=."}
};

SQLWorker::SQLWorker(const QVariantMap &connPar): QObject(),
    m_params{{"type",""},
            {"driver",""},
            {"address",""},
            {"user",""},
            {"password",""},
            {"db",""},
            {"provider",""}} {

    foreach (auto key, connPar.keys()) {
        if(!supportedParams().contains(key))
            throw SYS::Error("Unsupported params. Check config file SQL settings section");
        m_params[key] = connPar.value(key);
    }
    if(!m_params.value("type").toString().isEmpty() &&
            !m_params.value("driver").toString().isEmpty()){
        m_database = QSqlDatabase::addDatabase(m_params.value("driver").toString());
        m_connString = connString.value(m_params.value("type").toString()).
                             arg(m_params.value("address").toString()). //SERVER
                             arg(m_params.value("db").toString()). //database
                             arg(m_params.value("user").toString()). //user
                             arg(m_params.value("password").toString()); //pass

        //execute slot after construct object complete and event loop is started
        QTimer::singleShot(0,this,SLOT(connect()));
        // And output...
        //BOOST_LOG(lg) << "Hello, World!";
    }
}

void SQLWorker::connect()
{
    if(m_connString.isEmpty() || m_database.isOpen())
        return;

    SYS_LOG(EventLogScope::notification,
            QString("Atempt to connect to SQL Server, connection string: %1").arg(m_connString).toUtf8());

    m_database.setDatabaseName(m_connString);
    if(!m_database.open()){
        //exception in slot - not worked, only write in log
        //_logger::Instance().LogEvent(EventLogScope::error,
        //               QString("SQL error: %1").arg(m_database.lastError().text()).toUtf8());

        emit error(SYS::QError(EventLogScope::error,
                               SYS::QError::Type::ConnectionError,
                               QString("SQL error: %1").arg(m_database.lastError().text())));
        return;
    }
    //m_query = QSqlQuery(m_database);
    emit connected();
}

bool SQLWorker::exec(QString &queryString)
{
    QSqlQuery query;

    _logger::Instance().LogEvent(EventLogScope::notification,
                                 QString("Sending query: %1").arg(queryString).toUtf8());

    if(query.exec(queryString)){

        if(query.next())
            qDebug() << query.value(0);
        _logger::Instance().LogEvent(EventLogScope::notification, "Query exec success.");
        return true;
    }

    //exception in slot - not worked, only write in log
   _logger::Instance().LogEvent(EventLogScope::warning, query.lastError().text().toUtf8());

   return false;
}
/*
void SQLWorker::onPrepareQuery(const QString &queryPattern)
{
    m_query.prepare(queryPattern);
    //m_query.bindValue()
}
*/
