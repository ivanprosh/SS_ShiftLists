#ifndef LOGGER_H
#define LOGGER_H

#include "sys_messages.h"
#include "typemsg.h"

#include "singleton.h"
//#include <QString>

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
    void setFilter(const severity_level level);

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

#endif // LOGGER_H
