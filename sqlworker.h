#ifndef SQLWORKER_H
#define SQLWORKER_H

#include <QVariantMap>
#include <QSqlDatabase>

class SQLWorker
{
public:
    SQLWorker(const QVariantMap& connPar);
    QStringList supportedParams(){ return m_params.keys();}
    //карта строк подключения
    static const QHash<QString,QString> connString;

public slots:
    void connect();
    bool exec(const QString& query);
private:
    QString m_connString;
    QVariantMap m_params;
    QSqlDatabase m_database;
};

#endif // SQLWORKER_H
