#include "logger.h"

#ifndef Q_MOC_RUN
#include <iostream>
#include <fstream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/core/null_deleter.hpp>
#endif

namespace EventLogScope {
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity", severity_level)
BOOST_LOG_ATTRIBUTE_KEYWORD(id, "ID", unsigned int)

// The operator is used for regular stream formatting
// It will convert the enum severity codes into a readable string
std::ostream& operator<< (std::ostream& strm, severity_level level)
{
    static const char* strings[] =
    {
        "normal",
        "notification",
        "warning",
        "error",
        "critical"
    };

    if (static_cast< std::size_t >(level) < sizeof(strings) / sizeof(*strings))
        strm << strings[level];
    else
        strm << static_cast< int >(level);

    return strm;
}

// These two functions will write to the log file when it is opened
// and when it is closed, respectively.
// This could be useful, for example, if you are writing to an XML
// file and want to automatically add the open and close tags
void
write_header(sinks::text_file_backend::stream_type& file)
{
    file << "***** Start *****" << std::endl;
}

void
write_footer(sinks::text_file_backend::stream_type& file)
{
    file << "***** Stop *****" << std::endl;
}

void EventLog::setFilter(const severity_level level)
{
    // Access the core
    //boost::shared_ptr<boost::log::core> core = boost::log::core::get();
    boost::log::core::get()->set_filter(
         expr::attr< severity_level >("Severity").or_default(normal) >= level
    );
}

EventLog::EventLog()
{
    namespace fs = boost::filesystem;

     // Access the core
    boost::shared_ptr<boost::log::core> core = boost::log::core::get();

     // Create a text file sink
    typedef boost::log::sinks::synchronous_sink<sinks::text_file_backend> file_sink;

     // check to see if the destination folder exists
    std::string targetFolder = "./EventLogs";
    fs::path dest(targetFolder);

    // if not, create it
    if (!fs::exists(dest) || !fs::is_directory(dest))
    {
        fs::create_directory(dest);
    }

     // Create the log file name in the form ./logs/Event_date_time_counter.log
    std::string filename = "Event";
    fs::path logPath = fs::absolute(targetFolder + "/");
    logPath = fs::absolute(logPath.string() + filename +  "_%Y%m%d_%H%M%S_%1N.log");

     // force file rotation after file reches 1 mb.
    boost::shared_ptr<file_sink> sink(new file_sink(
        keywords::file_name = logPath.string(),                  // the resulting file name pattern
        keywords::rotation_size =  1 * 1024 * 1024               // rotation size, in characters
        ));

#ifdef DEBUG

    typedef sinks::synchronous_sink< sinks::text_ostream_backend > text_sink;
    shared_ptr< text_sink > tSink(new text_sink);

    text_sink::locked_backend_ptr pBackend = tSink->locked_backend();
    // Next we add streams to which logging records should be output
    shared_ptr< std::ostream > pStream(&std::clog, boost::null_deleter());
    pBackend->add_stream(pStream);

    tSink->set_formatter(expr::format("%2% # %4%")
                 % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
                 % expr::attr< severity_level >("Severity")
                 % expr::format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::reverse, keywords::depth = 2)
                 % expr::smessage);
    core->add_sink(tSink);

#endif
     // Set up where the rotated files will be stored
    sink->locked_backend()->set_file_collector(
        sinks::file::make_collector(
            keywords::target = targetFolder,         // the target directory
            keywords::max_size = 1 * 1024 * 1024    // rotate files after 2 megabytes
        ));

    // Upon restart, scan the directory for files matching the file_name pattern and delete any required files
    sink->locked_backend()->scan_for_files();

     // Enable auto-flushing after each log record written
    sink->locked_backend()->auto_flush(true);

     // add common attributes like date and time.
    boost::log::add_common_attributes();

    // add an ID attribute. This is how the core will know to write only to
    // this log file, as set by the filter
    //auto attr = attrs::constant<unsigned int>(99);
    //m_logger.add_attribute("ID", attr);

     //sink->set_filter( expr::attr<unsigned int>("ID") == 99 );
     //sink->set_filter(
     //     expr::attr< severity_level >("Severity").or_default(normal) >= warning);

     // format of the log records when streamed
     sink->set_formatter(
         expr::format("[%1%] # %2% # %4%")
         % expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
         % expr::attr< severity_level >("Severity")
         % expr::format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::reverse, keywords::depth = 2)
         % expr::smessage);

     // establish the header and footer methods
    sink->locked_backend()->set_open_handler(&write_header);
    sink->locked_backend()->set_close_handler(&write_footer);

     // Add the sink to the core
    core->add_sink(sink);
}


// Actually designate the message to the log
/*
void
EventLog::LogEvent(const severity_level level, const char* message)
{
    BOOST_LOG_SEV(m_logger, level) << message;
}
*/
}

/*
#include <QFile>

void Logger::setFile(const QString &filename, std::function<void(QTextStream&)> headerFillFunc)
{
    if (!filename.isEmpty())
        throw SYS::Error(tr("log file name not specified"));

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        throw SYS::Error(file.errorString());

    //задание шапки с форматированием
    if(!file.isEmpty()){
        //lastFilePos = 0;
        if(file.size()==0){
            logFileStream.setDevice(&logfile);
            headerFillFunc(logFileStream);
        }
        file.close();
    }
    m_file = file;
}

void Logger::addEntry(LoggerEntry&& entry)
{
    if(!m_file.open(QIODevice::ReadWrite | QIODevice::Text))
        return;

}
*/
