#include "logger.h"
#include "htmlshiftworker.h"

HTMLShiftWorker::HTMLShiftWorker(QObject *parent) : QObject(parent)
{
    _logger::Instance().LogEvent(EventLogScope::notification, "Create new HTML Docworker instance");
}
