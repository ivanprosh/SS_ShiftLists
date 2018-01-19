#ifndef SHIFTMANAGER_H
#define SHIFTMANAGER_H

#include "sqlworker.h"
#include "htmlshiftworker.h"
#include "dbadapter.h"

#include <QSqlQuery>
#include <QVariantMap>
#include <QObject>
#include <QHash>
#include <QVector>
#include <QTime>
#include <QJsonObject>
#include <QTimer>
#include <QThread>
#include <QScopedPointerDeleteLater>
#include <QStateMachine>
#include <QQueue>
#include <QTextDocument>

#include <QDebug>

class ShiftManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool needRecreateSQLWorker READ needRecreateSQLWorker WRITE setNeedRecreateSQLWorker)
    Q_PROPERTY(bool isCriticalErrorSet READ isCriticalErrorSet WRITE setIsCriticalErrorSet)
    Q_PROPERTY(bool isFinishedCycle READ isFinishedCycle WRITE setIsFinishedCycle)
    Q_PROPERTY(QDateTime specDateTime READ specDateTime WRITE setSpecDateTime)
    Q_PROPERTY(quint8 docCreatorCnstrQueriesBits READ docCreatorCnstrQueriesBits WRITE setDocCreatorCnstrQueriesBits)
public:
    struct Shift {
        static int count;
        static int intervalInMinutes;
        static bool check(QTime& start, QTime& stop);
        Shift():start(8,0),stop(20,0),isNightCross(0){}

        bool isMyTime(const QTime& time) const noexcept{
            //time of start is greater, example: start - 23:00, time - 00:05
            if(start.msecsTo(time) < 0)
                return time.msecsSinceStartOfDay() < stop.msecsSinceStartOfDay();
            //time of start is less, example: start - 23:00, time - 23:05
            //shift is nightcross, example: start - 23:00, stop - 2:00
            return isNightCross ? true : time.msecsSinceStartOfDay() < stop.msecsSinceStartOfDay();
        }
        QTime start;
        QTime stop;
        bool isNightCross;
        bool operator ==(const Shift& rhs){
            return start==rhs.start && stop==rhs.stop;}
    };

    explicit ShiftManager(QObject *parent = nullptr);

    //
    void read(const QString& filename);
    void read(const QJsonObject&& object);
    bool readShiftsInfo(const QJsonObject& obj);

    //factory functions
    //SQLWorker* getSQLWorker();

    template <class TWorker>
    void createDocWorker();
    template <class TAdapter>
    void createDBAdapter();
    //*****************
    //SQL func
    QString prepareMainQuery(QDateTime &stop);
    QString prepareTagDescriptionQuery();

    QList<QString> supportedOutputFormats();
    //Shift handl
    int getShiftIndexOnReqTime(QTime &reqTime);

    bool isPermanent;
    int printTimeOffset;
    QString m_dateTimeformat;
    QHash< QString, QJsonObject > configGroups;
    bool needRecreateSQLWorker() const    {
        return m_needRecreateSQLWorker;
    }
    bool isCriticalErrorSet() const {
        return m_isCriticalErrorSet;
    }

    bool isFinishedCycle() const
    {
        return m_isFinishedCycle;
    }

    QDateTime specDateTime() const
    {
        return m_specDateTime;
    }

    quint8 docCreatorCnstrQueriesBits() const
    {
        return m_docCreatorCnstrQueriesBits;
    }

signals:
    void exit();
    void errorChange();
    void errorAccept();
    void createSqlWorkerSuccess();
    void createSqlWorkerFailed();
    void connectSqlWorkerSuccess();
    void workSqlWorkerSuccess();
    void workDocWorkerSuccess();
    void startStateSuccess();
    void queryExec(SQLWorker::QueryTypes, QString&);
public slots:
    //main state - initialize work timer
    void onStartState();
    void onErrorState();
    void onCreateSqlWorkerState();
    void onConnectSqlWorkerState();
    void onWorkSqlWorkerState();
    void onWorkDocWorkerState();

    void onError(SYS::QError);
    void onQueryResult(SQLWorker::QueryTypes,QSqlQuery);
    void onDocResult(QTextDocument&);
    void start();

    //properties
    void setNeedRecreateSQLWorker(bool needRecreateSQLWorker)    {
        m_needRecreateSQLWorker = needRecreateSQLWorker;
    }

    void setIsCriticalErrorSet(bool isCriticalErrorSet)  {
        qDebug() << Q_FUNC_INFO << " val:" << isCriticalErrorSet;
        m_isCriticalErrorSet = isCriticalErrorSet;
    }

    void setIsFinishedCycle(bool isFinishedCycle)   {
        qDebug() << Q_FUNC_INFO << " val:" << isFinishedCycle;
        m_isFinishedCycle = isFinishedCycle;
    }

    void setSpecDateTime(QDateTime specDateTime)
    {
        m_specDateTime = specDateTime;
    }

    void setDocCreatorCnstrQueriesBits(quint8 docCreatorCnstrQueriesBits)
    {
        m_docCreatorCnstrQueriesBits = docCreatorCnstrQueriesBits;
    }

private:
    void initStateMachine();

    QStringList m_tagsNamesList;
    QList<Shift> m_shifts;
    QQueue<SYS::QError> m_errors;
    QVector< QPair<QString,QString> > m_tagsNameDescription;
    QStateMachine stateMachine;
    //strategies
    QScopedPointer<QThread, QScopedPointerDeleteLater > m_SQLWorkerThr;
    QScopedPointer<HTMLShiftWorker> m_DocWorker;
    QScopedPointer<DBAdapter> m_DBAdapter;
    //**********
    QTimer m_everyShiftTimer;
    int retryConnectDelaySecs;
    bool m_needRecreateSQLWorker;
    bool m_isCriticalErrorSet;
    bool m_isFinishedCycle;
    QDateTime m_specDateTime;
    quint8 m_docCreatorCnstrQueriesBits;
};

template <class TWorker>
void ShiftManager::createDocWorker()
{
    m_DocWorker.reset(new TWorker);
    connect( m_DocWorker.data(), SIGNAL( error(SYS::QError) ), this, SLOT( onError(SYS::QError) ));
    connect( m_DocWorker.data(), SIGNAL( docResult(QTextDocument&)), this, SLOT( onDocResult(QTextDocument&)));
}
template <class TAdapter>
void ShiftManager::createDBAdapter()
{
    m_DBAdapter.reset(new TAdapter);
}

Q_DECLARE_METATYPE(ShiftManager::Shift)

#endif // SHIFTMANAGER_H
