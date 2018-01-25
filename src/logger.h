#ifndef LOGGER_H
#define LOGGER_H

#include "sys_messages.h"
#include "typemsg.h"

#ifndef Q_MOC_RUN
#include "singleton.h"
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
#include <QString>

using boost::shared_ptr;

//class ShiftManager;
namespace EventLogScope {

namespace logging = boost::log;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace keywords = boost::log::keywords;
/*
enum severity_level
{
    normal,
    notification,
    warning,
    error,
    critical
};
*/
class EventLog
{
public:
    using windowHndl = void(*)(QWidget*, const QString&, const QString&, const QString&);
    friend struct Loki::CreateUsingNew<EventLog>;
    inline void LogEvent(const severity_level level, const char* message){
        BOOST_LOG_SEV(m_logger, level) << message;
    }
    inline void LogEvent(const severity_level level, const char* message, windowHndl window){
        BOOST_LOG_SEV(m_logger, level) << message;
        window(nullptr,"Shift print Application",message,QString());
    }

protected:
    EventLog();
private:
    src::severity_logger<severity_level> m_logger;
};
}

typedef Loki::SingletonHolder<EventLogScope::EventLog> _logger;

#define SYS_LOG(S,MSG) _logger::Instance().LogEvent(S,MSG);
//#define SYS_LOG(S,MSG)
#define SYS_LOG_WINDOW(S,MSG,WND_TYPE) _logger::Instance().LogEvent(S,MSG,WND_TYPE);

/*
#include "singleton.h"

#include <functional>
#include <QTextStream>;

//typedef Loki::SingletonHolder<Logger> _logger;

void logHeaderFill(QTextStream& stream) noexcept{
    if(stream.device() == nullptr)
        return;
    stream.setFieldAlignment(QTextStream::AlignCenter);
    stream.setFieldWidth(20);
    stream << QString("Время");
    stream.setFieldWidth(10);
    stream << QString("Тип");
    stream.setFieldWidth(30);
    stream << QString("Идентификатор");
    stream.setFieldWidth(80);
    stream << QString("Текст сообщения");
}

class QFile;
struct LoggerEntry {

};

class Logger
{
public:
    //friend class _logger;

    void setFile(const QString&filename, std::function<void(QTextStream&)> headerFillFunc);
    auto addEntry(LoggerEntry&&) -> void;

protected:
    Logger(){}
    Logger(const Logger& ) = delete;
    Logger& operator=(const Logger& ) = delete;
    ~Logger(){}
private:
    QTextStream logFileStream;
    QFile m_file;
};
*/
#endif // LOGGER_H
