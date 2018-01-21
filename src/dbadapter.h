#ifndef DBADAPTER_H
#define DBADAPTER_H

//#include "sys_error.h"
#include "sys_messages.h"

#include <QStringList>
#include <QSqlQuery>
#include <QVector>
#include <QTime>
#include <QVariantMap>
#include <utility>

class DBAdapter
{
public:
    enum class QueryTypes {TagDescriptionList,TagValuesList};

    static const QHash<DBAdapter::QueryTypes,QStringList> keys;
    static QStringList keysList(QueryTypes type) {
        return DBAdapter::keys.value(type);
    }

    DBAdapter(QHash<QueryTypes,QString>&& m_q):
        queries(std::move(m_q)){}
    virtual ~DBAdapter(){}

    virtual QVector< QPair<QString,QString> > getTagDescr(QSqlQuery&& query) = 0;
    virtual QVector<QVector<float> > getTagValuesMap(QSqlQuery&& query, QStringList& horizontalHeader) = 0;
    virtual QString prepareQuery(QueryTypes type, QVariantMap&& params) = 0;

protected:
    const QHash<QueryTypes,QString> queries;

};
inline uint qHash(DBAdapter::QueryTypes key, uint seed) {
   return ::qHash(SYS::toUType(key), seed);
}

class WonderwareDBAdapter : public QObject, public DBAdapter {
    Q_OBJECT
public:
    WonderwareDBAdapter();
    virtual ~WonderwareDBAdapter(){}
    virtual QVector< QPair<QString,QString> > getTagDescr(QSqlQuery&& query) override;
    virtual QVector<QVector<float> > getTagValuesMap(QSqlQuery&& query, QStringList &horizontalHeader) override;
    virtual QString prepareQuery(QueryTypes type, QVariantMap&& params) override;

    QString prepareTagValuesQuery(QVariantMap&& params);
    QString prepareTagDescrQuery(QVariantMap&& params);
signals:
    void error(SYS::QError err);
};

#endif // DBADAPTER_H
