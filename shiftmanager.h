#ifndef SHIFTMANAGER_H
#define SHIFTMANAGER_H

#include <QObject>
#include <QHash>

class SQLWorker;
class QJsonObject;

class ShiftManager : public QObject
{
    Q_OBJECT
public:
    explicit ShiftManager(QObject *parent = nullptr);
    void read(const QString& filename);
    void read(const QJsonObject&& object);
    void createWorker();
    SQLWorker* getWorker();

    QHash< QString, QVariantMap > options;
signals:

public slots:
private:
    QScopedPointer<SQLWorker> m_worker;
};

#endif // SHIFTMANAGER_H
