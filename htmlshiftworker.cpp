#include "logger.h"
#include "htmlshiftworker.h"

#include <QTextDocument>

HTMLShiftWorker::HTMLShiftWorker(QObject *parent) : QObject(parent), m_countTagsOnPage(10)
{
    _logger::Instance().LogEvent(EventLogScope::notification, "Create new HTML Docworker instance");
}

void HTMLShiftWorker::process()
{
    if(m_horheaderTableTitles.isEmpty() || m_vertheaderTableTitles.isEmpty() || m_valuesMap.isEmpty()){
        emit error(SYS::QError(EventLogScope::error,
                               SYS::QError::Type::DataSourceError,
                               QString("HTML Doc worker hasn't enough data for analizing")));
        return;
    }
    m_doc.clear();
    pages.clear();
    for(int i = 0; i < m_valuesMap.size(); i+=m_countTagsOnPage)
        pages << HTMLOnePage();
    populateDocument(m_doc);
}
QString HTMLShiftWorker::getHtmlFromFile(const QString& filename){
    QFile fileHtml(filename);
    QString result;
    if(!fileHtml.open(QIODevice::ReadOnly | QIODevice::Text)){
        emit error(SYS::QError(EventLogScope::error, SYS::QError::Type::DataSourceError,
                               QString("Cann't open html file for footer/header part. Error: %1").arg(fileHtml.errorString())));
        return QString();
    }
    QTextStream stream(&fileHtml);
    result = stream.readAll();
    fileHtml.close();
    return result
}
void HTMLShiftWorker::populateDocument(QTextDocument &doc)
{
    QString html("<html>\n<body>\n");
    html += getHtmlFromFile(QApplication::applicationDirPath() + "/header.html");

    for (int page = 0; page < pages.count(); ++page) {
        html += pageAsHtml(page, false);
        if (page + 1 < pages.count())
            html += "<br style='page-break-after:always;'/>\n";
    }

    html += getHtmlFromFile(QApplication::applicationDirPath() + "/footer.html");
    html += "</body>\n</html>\n";
    doc->setHtml(html);
}
