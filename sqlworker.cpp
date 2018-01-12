#include "sqlworker.h"
#include "sys_error.h"
#include "logger.h"

#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QHash>

//#include <boost/log/common.hpp>
//#include <boost/log/sources/logger.hpp>
//namespace src = boost::log::sources;

//extern src::logger lg;

const QHash<QString,QString> SQLWorker::connString {
    {"MSSQL","DRIVER={SQL Server};SERVER=%1;DATABASE=%2;UID=%3;PWD=%4;Trusted_Connection=no; WSID=."}
};

SQLWorker::SQLWorker(const QVariantMap &connPar):
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
        connect();
        // And output...
        //BOOST_LOG(lg) << "Hello, World!";
    }
}

void SQLWorker::connect()
{
    if(m_connString.isEmpty() || m_database.isOpen())
        return;
    m_database.setDatabaseName(m_connString);
    if(!m_database.open()){
        qDebug() << m_database.lastError() << " Connection string: " << m_connString;
        //exception in slot - not worked, only write in log
        //throw::SYS::Error(m_database.lastError() + ". \nConnection string is " + m_connString);
    }
}

bool SQLWorker::exec(const QString &query)
{
    QSqlQuery queryClass;

    _logger::Instance().LogEvent(EventLogScope::notification, "Sending query");

    if(queryClass.exec(query)){
        QSqlRecord rec = queryClass.record();
        qDebug() << rec.count();
        while(queryClass.next()){
            //qDebug() << queryClass.value(rec.indexOf("DateTime")).toString();
        }
        return true;
    }

    //exception in slot - not worked, only write in log
   _logger::Instance().LogEvent(EventLogScope::warning, queryClass.lastError());

    return false;
}
