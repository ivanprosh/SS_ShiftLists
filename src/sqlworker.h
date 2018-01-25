#ifndef SQLWORKER_H
#define SQLWORKER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include "sys_error.h"

class SQLWorker : public QObject
{
    Q_OBJECT
public:
    enum class QueryTypes {
       TagDescription, TagValues
    };
    Q_ENUMS(QueryTypes)

    SQLWorker(const QVariantMap& connPar);
    ~SQLWorker();
    QStringList supportedParams(){ return m_params.keys();}

    //карта строк подключения
    static const QHash<QString,QString> connString;

public slots:
    void connect();
    bool exec(SQLWorker::QueryTypes,QString);
    //void onPrepareQuery(const QString& queryPattern);
signals:
    void error(SYS::QError);
    void queryResult(SQLWorker::QueryTypes,QSqlQuery);
    void connected();

private:
    QString m_connString;
    QString m_queryPattern;
    QVariantMap m_params;
    QSqlDatabase m_database;
};

Q_DECLARE_METATYPE(SQLWorker::QueryTypes)
Q_DECLARE_METATYPE(QSqlQuery)

#endif // SQLWORKER_H
