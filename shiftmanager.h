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

class ShiftManager : public QObject
{
    Q_OBJECT
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
    //main function - initialize work timer
    void process();
    //
    void read(const QString& filename);
    void read(const QJsonObject&& object);
    bool readShiftsInfo(const QJsonObject& obj);

    //factory functions
    SQLWorker* getSQLWorker();
    void createSQLWorker();
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
signals:
    void exit();
public slots:
    void onError(SYS::QError);
    void onConnected();
    void work();

private:
    QStringList m_tagsNamesList;
    QList<Shift> m_shifts;
    QVector< QPair<QString,QString> > m_tagsNameDescription;
    //strategies
    QScopedPointer<SQLWorker> m_SQLWorker;
    QScopedPointer<HTMLShiftWorker> m_DocWorker;
    QScopedPointer<DBAdapter> m_DBAdapter;
    //**********
    QTimer m_everyShiftTimer;
    int retryConnectDelaySecs;
};

template <class TWorker>
void ShiftManager::createDocWorker()
{
    m_DocWorker.reset(new TWorker);
}
template <class TAdapter>
void ShiftManager::createDBAdapter()
{
    m_DBAdapter.reset(new TAdapter);
}

Q_DECLARE_METATYPE(ShiftManager::Shift)

#endif // SHIFTMANAGER_H
