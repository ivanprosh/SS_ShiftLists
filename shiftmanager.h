#ifndef SHIFTMANAGER_H
#define SHIFTMANAGER_H

#include "sqlworker.h"

#include <QSqlQuery>
#include <QVariantMap>
#include <QObject>
#include <QHash>
#include <QTime>
#include <QJsonObject>

QPair<int,int> timeFromString(QString source) {
    QPair<int,int> result{-1,-1};
    //match 8:00, 08:11
    QRegExp rx("(\\d{1,2}):(\\d{1,2})");
    int pos = rx.indexIn(source);
    if(pos>-1){
        result.first = rx.cap(1).toInt();
        result.second = rx.cap(2).toInt();
    }
    return result;
}

class ShiftManager : public QObject
{
    Q_OBJECT
public:
    struct Shift {
        QTime start;
        QTime stop;
        int isSingle;
        Shift():start(8,0),stop(20,0),isSingle(0){}
        bool read(const QJsonObject& obj);
    };

    explicit ShiftManager(QObject *parent = nullptr);
    void read(const QString& filename);
    void read(const QJsonObject&& object);
    void createWorker();
    void prepareQuery();
    SQLWorker* getWorker();

    bool isPermanent;
    QHash< QString, QJsonObject > configGroups;
    QSqlQuery query;

signals:

public slots:
    void onError(SQLWorker::Error);
    void onConnected();

private:
    QStringList m_tagsNamesList;
    Shift m_shift;
    QScopedPointer<SQLWorker> m_worker;
    int retryConnectDelaySecs;
};

#endif // SHIFTMANAGER_H
