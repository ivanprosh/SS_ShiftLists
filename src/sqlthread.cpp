#include "sqlthread.h"

SqlThread::SqlThread(QVariantMap _SQLparams):SQLparams(_SQLparams)
{

}

void SqlThread::run()
{
    worker = new SQLWorker(SQLparams);

    connect( this, SIGNAL( finished() ), worker, SLOT( deleteLater() ) );
    connect( worker, SIGNAL(connected()), this, SIGNAL( connected() ) );
    connect( this, SIGNAL(connectToDb()), worker, SLOT( connect() ) );
    //connect( this, &SqlThread::connectToDb, [](){qDebug() << "connect";} );
    connect( worker, SIGNAL( error(SYS::QError) ), this, SIGNAL( error(SYS::QError) ));
    connect( this, &SqlThread::queryExec, worker, &SQLWorker::exec);
    connect( worker, &SQLWorker::queryResult, this, &SqlThread::queryResult);

    emit connectToDb();
    exec();
}
