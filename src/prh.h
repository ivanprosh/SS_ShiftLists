#ifndef PRH_H
#define PRH_H

#include <QObject>
#include <QVector>
#include <QVariant>
#include <QVariantMap>
#include <QHash>
#include <QTimer>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QScopedPointer>
#include <QDebug>

#ifndef Q_MOC_RUN

#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/filesystem.hpp>
#endif

#endif // PRH_H
