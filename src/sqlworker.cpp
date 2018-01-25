#include "sqlworker.h"
#include "logger.h"
#include "devpolicies.h"

#include <QSqlError>
#include <QSqlRecord>
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
        m_database = QSqlDatabase::database(m_params.value("driver").toString());
        if(!m_database.isValid())
            m_database = QSqlDatabase::addDatabase(m_params.value("driver").toString());
        m_connString = connString.value(m_params.value("type").toString()).
                             arg(m_params.value("address").toString()). //SERVER
                             arg(m_params.value("db").toString()). //database
                             arg(m_params.value("user").toString()). //user
                             arg(m_params.value("password").toString()); //pass

        //execute slot after construct object complete and event loop is started
        // QTimer::singleShot(0,this,SLOT(connect()));
        // And output...
        //BOOST_LOG(lg) << "Hello, World!";
    }
}

SQLWorker::~SQLWorker()
{
    SYS_LOG(EventLogScope::notification,"SQL worker dsrctr");
    m_database.close();
}

void SQLWorker::connect()
{
     qDebug() << "SQLWorker thr: " << QThread::currentThreadId();

    if(m_connString.isEmpty())
        return;

    if(m_database.isOpen()){
        emit connected();
        return;
    }

    SYS_LOG(EventLogScope::notification,
            QString("Atempt to connect to SQL Server, connection string: %1").arg(m_connString).toUtf8());

    m_database.setDatabaseName(m_connString);
    if(!m_database.open()){
        emit error(SYS::QError(EventLogScope::error,
                               SYS::QError::Type::ConnectionError,
                               QString("SQL error string: %1").arg(m_database.lastError().text())));
        return;
    }

    emit connected();
}

bool SQLWorker::exec(QueryTypes id, QString queryString)
{
    QSqlQuery query;

    SYS_LOG(EventLogScope::notification,
            QString("Sending query: %1").arg(queryString).toUtf8());

    if(query.exec(queryString)){
        SYS_LOG(EventLogScope::notification, "Query exec success.");
        emit queryResult(id,query);
        return true;
    }

   //exception in slot - not worked, only write in log
   emit error(SYS::QError(EventLogScope::error,
                          SYS::QError::Type::ExchangeError,
                          QString("SQL error: %1").arg(query.lastError().text())));

   return false;
}
/*
void SQLWorker::onPrepareQuery(const QString &queryPattern)
{
    m_query.prepare(queryPattern);
    //m_query.bindValue()
}
*/
