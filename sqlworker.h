#ifndef SQLWORKER_H
#define SQLWORKER_H

#include <QVariantMap>
#include <QSqlDatabase>
#include "sys_error.h"

class SQLWorker : public QObject
{
    Q_OBJECT
public:
    enum class Error {
       ConnectionError
    };
    Q_ENUMS(Error)

    SQLWorker(const QVariantMap& connPar);
    QStringList supportedParams(){ return m_params.keys();}

    //карта строк подключения
    static const QHash<QString,QString> connString;

public slots:
    void connect();
    bool exec(QString& query);
    //void onPrepareQuery(const QString& queryPattern);
signals:
    void error(SYS::QError);
    void connected();

private:
    QString m_connString;
    QString m_queryPattern;
    QVariantMap m_params;
    QSqlDatabase m_database;
};

Q_DECLARE_METATYPE(SQLWorker::Error)

#endif // SQLWORKER_H
