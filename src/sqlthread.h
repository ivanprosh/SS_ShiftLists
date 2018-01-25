#ifndef SQLTHREAD_H
#define SQLTHREAD_H

#include <QThread>
#include "sys_error.h"
#include "sqlworker.h"

class SQLWorker;

class SqlThread : public QThread
{
    Q_OBJECT
public:
    SqlThread(QVariantMap _SQLparams);
signals:
    void connectToDb();
    void connected();
    void error(SYS::QError);
    void queryResult(SQLWorker::QueryTypes,QSqlQuery);
    void queryExec(SQLWorker::QueryTypes, QString );
protected:
    void run() override;
private:
    QVariantMap SQLparams;
    SQLWorker* worker;
};

#endif // SQLTHREAD_H
