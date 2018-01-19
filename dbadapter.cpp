#include "dbadapter.h"
#include "logger.h"
#include "shiftmanager.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>


const QHash<DBAdapter::QueryTypes,QStringList> DBAdapter::keys{
    {DBAdapter::QueryTypes::TagDescriptionList,{"taglist"}},
    {DBAdapter::QueryTypes::TagValuesList,{"taglist","shift","datetime","datetimeformat"}}
};

WonderwareDBAdapter::WonderwareDBAdapter():DBAdapter(
    {{QueryTypes::TagDescriptionList,"SELECT Tag = Tag.TagName, Description = Tag.Description, Unit "
                                             "FROM Runtime.dbo.AnalogTag, Runtime.dbo.Tag, Runtime.dbo.EngineeringUnit "
                                             "WHERE Tag.TagName IN (\'%1\') "
                                             "AND Tag.TagName = AnalogTag.TagName "
                                             "AND AnalogTag.EUKey = EngineeringUnit.EUKey "},
    {QueryTypes::TagValuesList,"SET QUOTED_IDENTIFIER OFF "
                                        "SELECT * FROM OPENQUERY(INSQL, \"SELECT DateTime, %1 FROM WideHistory "
                                        "WHERE wwResolution = %2 " //3600000 - 1 hour interval
                                        "AND wwQualityRule = \'Extended\' "
                                        "AND wwVersion = \'Latest\' "
                                        "AND DateTime >= \'%3\' "
                                        "AND DateTime <= \'%4\'\")"}}
                                                     )
{
}

QVector< QPair<QString,QString> > WonderwareDBAdapter::getTagDescr(QSqlQuery &&query)
{
    if(!query.isValid()){
        SYS_LOG_WINDOW(EventLogScope::warning,QObject::tr("Tag Description query is not valid").toUtf8(),&SYS::warning)
        return QVector< QPair<QString,QString> >();
    }

    QSqlRecord rec = query.record();
    QVector< QPair<QString,QString> > result;

    while(query.next()) {
        QString descr = query.value(rec.indexOf("Description")).toString();
        QString unit = query.value(rec.indexOf("Unit")).toString();

        result.append(qMakePair(query.value(rec.indexOf("Tag")).toString(),
                                unit.isEmpty() ? descr : descr.append(QString(" (%1)").arg(unit))));
    }
    return result;
}

QVector<QVector<float> > WonderwareDBAdapter::getTagValuesMap(QSqlQuery &&query, QStringList& horizontalHeader)
{
    if(!query.isValid())
        return QVector<QVector<float> >();
        //SYS_LOG_WINDOW(EventLogScope::warning,qPrintable(QObject::tr("Tag values query is not valid")),&SYS::warning)
    QSqlRecord rec = query.record();
    QVector<QVector<float> > resultMap;
    horizontalHeader.clear();
    int row = 0;
    while (query.next()){
        //0 - DateTime column
        horizontalHeader.append(query.value(rec.indexOf("DateTime")).toDateTime().toString("hh:mm"));
        for(int column=1; column<rec.count(); ++column){
            resultMap[row][column-1] = query.value(column).toFloat();
        }
        ++row;
    }
    return resultMap;
}

QString WonderwareDBAdapter::prepareQuery(DBAdapter::QueryTypes type, QVariantMap&& params)
{
    switch (type) {
    case SYS::toUType(QueryTypes::TagDescriptionList):
        return prepareTagDescrQuery(std::forward<QVariantMap>(params));
        break;
    case SYS::toUType(QueryTypes::TagValuesList):
        return prepareTagValuesQuery(std::forward<QVariantMap>(params));
        break;
    default:
        Q_ASSERT(false);
        break;
    }
    return QString();
}

//params must contain "start","stop","interval","taglist"
QString WonderwareDBAdapter::prepareTagValuesQuery(QVariantMap &&params)
{
    foreach (auto key, keysList(QueryTypes::TagValuesList)) {
        if(!params.contains(key)){
            //SYS_LOG_WINDOW(EventLogScope::critical,
            //               QString("Prepare main query failed.Key %1 is not find in request params map").arg(key).toUtf8(),
            //               &SYS::warning);
            emit error(SYS::QError(EventLogScope::critical,
                       SYS::QError::Type::ConfigurationError,
                       QString("Prepare main query failed.Key %1 is not find in request params map").arg(key)));
            return QString();
        }
    }

    QString queryString = queries.value(QueryTypes::TagValuesList);

    QString tagString("[" + params["taglist"].toStringList().join("],[") + "]");

    auto requestedDateTime = params["datetime"].toDateTime();
    auto requestedShift = params["shift"].value<ShiftManager::Shift>();

    //query start on last day because shift is Nightcrossed
    QDateTime dateStart = requestedDateTime.addDays((-1)*requestedShift.isNightCross);
    //query stop time depend on difference date between request time and stop last shift time
    QDateTime dateStop = requestedDateTime.addDays((-1)*(requestedShift.stop.msecsTo(requestedDateTime.time()) < 0));

    dateStart.setTime(requestedShift.start);
    dateStop.setTime(requestedShift.stop);

    queryString = queryString.arg(tagString);
    queryString = queryString.arg(QString::number(ShiftManager::Shift::intervalInMinutes*60*1000));
    queryString = queryString.arg(dateStart.toString(params["datetimeformat"].toString()));
    queryString = queryString.arg(dateStop.toString(params["datetimeformat"].toString()));

    return queryString;
    /*
    queryString = queryString.arg(m_tagsNamesList.join("],["));

    int indexShift = 0;

    foreach (auto shift, m_shifts) {
        auto requestedTimeMSecs = requestedDateTime.time().msecsSinceStartOfDay();
        auto startShiftMSecs = shift.start.msecsSinceStartOfDay();
        auto stopShiftMSecs = shift.stop.msecsSinceStartOfDay();
        if((stopShiftMSecs-startShiftMSecs) > 0 && (requestedTimeMSecs > startShiftMSecs) && (requestedTimeMSecs < stopShiftMSecs) ||
            (stopShiftMSecs-startShiftMSecs) < 0 && (requestedTimeMSecs < stopShiftMSecs))
        {
            indexShift = m_shifts.indexOf(shift);
            break;
        }
    }

    const auto& prevShift = (indexShift==0) ? m_shifts.last() : m_shifts.at(indexShift-1);

    QDateTime dateStart = requestedDateTime.addDays((-1)*prevShift.isNightCross);
    auto dateStop = requestedDateTime.addDays((-1)*(prevShift.stop.msecsTo(requestedDateTime.time()) < 0));
    dateStart.setTime(prevShift.start);
    dateStop.setTime(prevShift.stop);

    queryString = queryString.arg(QString::number(Shift::intervalInMinutes*60*1000));
    queryString = queryString.arg(dateStart.toString(m_dateTimeformat));
    queryString = queryString.arg(dateStop.toString(m_dateTimeformat));
    */

    //return queryString;
}

QString WonderwareDBAdapter::prepareTagDescrQuery(QVariantMap &&params)
{
    foreach (auto key, keysList(QueryTypes::TagDescriptionList)) {
        if(!params.contains(key)){
            SYS_LOG_WINDOW(EventLogScope::critical,
                           QString("Prepare main query failed.Key %1 is not find in request params map").arg(key).toUtf8(),
                           &SYS::warning);
            return QString();
        }
    }
    QString queryString = queries.value(QueryTypes::TagDescriptionList);

    queryString.arg(params["taglist"].toStringList().join("','"));

    return queryString;
}
