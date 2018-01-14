#ifndef SQLWORKER_H
#define SQLWORKER_H

#include <QVariantMap>
#include <QSqlDatabase>
#include <QSqlQuery>

class QSqlQuery;
class SQLWorker : public QObject
{
    Q_OBJECT
public:
    enum Error {
       ConnectionError
    };
    SQLWorker(const QVariantMap& connPar, QString queryPattern);
    QStringList supportedParams(){ return m_params.keys();}
    //карта строк подключения
    static const QHash<QString,QString> connString;

    Q_ENUMS(Error)
public slots:
    void connect();
    bool exec(QSqlQuery& query);
    //void onPrepareQuery(const QString& queryPattern);
signals:
    void error(SQLWorker::Error);
    void connected();

private:
    QString m_connString;
    QString m_queryPattern;
    QSqlQuery m_query;
    QVariantMap m_params;
    QSqlDatabase m_database;
};

Q_DECLARE_METATYPE(SQLWorker::Error)

#endif // SQLWORKER_H
