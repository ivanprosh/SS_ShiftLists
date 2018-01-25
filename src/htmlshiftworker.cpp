#include "logger.h"
#include "htmlshiftworker.h"

#include <QTextDocument>

HTMLShiftWorker::HTMLShiftWorker(QObject *parent) : QObject(parent), m_countTagsOnPage(10), m_shNumber(1)
{
    _logger::Instance().LogEvent(EventLogScope::notification, "Create new HTML Docworker instance");
}

void HTMLShiftWorker::process(QString &docString)
{
    if(m_horheaderTableTitles.isEmpty() || m_vertheaderTableTitles.isEmpty()
            || m_valuesMap.isEmpty() || m_valuesMap.at(0).isEmpty() ||
            m_horheaderTableTitles.size() != m_valuesMap.at(0).size() ||
            m_vertheaderTableTitles.size() != m_valuesMap.size()){
        qDebug() << m_valuesMap.size() << " " << m_valuesMap.at(0).size() << " " << m_vertheaderTableTitles.size() << " "
                 << m_horheaderTableTitles.size();
        emit error(SYS::QError(EventLogScope::error,
                               SYS::QError::Type::DataSourceError,
                               QString("HTML Doc worker hasn't enough data for analizing or data incorrect")));
        return;
    }
    docString.clear();
    pages.clear();
    for(int i = 0; i < m_valuesMap.size(); i+=m_countTagsOnPage)
        pages << HTMLOnePage();
    populateDocument(docString);
}
QString HTMLShiftWorker::getHtmlFromFile(const QString& filename){
    QFile fileHtml(filename);
    QString result;
    if(!fileHtml.open(QIODevice::ReadOnly | QIODevice::Text)){
        emit error(SYS::QError(EventLogScope::error, SYS::QError::Type::ExternalError,
                               QString("Cann't open html file for footer/header part. Error: %1").arg(fileHtml.errorString())));
        return QString();
    }
    QTextStream stream(&fileHtml);
    result = stream.readAll();
    fileHtml.close();
    return result;
}

QString HTMLShiftWorker::itemsAsHtmlTable(int page)
{
    QString html("<table border='1' cellpadding='10' width='100%' bordercolor = '#787878' "
                 "style = 'border-collapse: collapse'>");
    /*  horizontal headers filling */
    /*  first rect in left top corner is grey */
    html += "<tr bgcolor = '#d1d1e0'>\n"
            "<td></td>";
    for (int hColumn = 0; hColumn < m_horheaderTableTitles.size(); ++hColumn){
        html += QString("<td align='center'><b>%1</b></td>\n").
                arg(m_horheaderTableTitles.at(hColumn));
    }
    /*  main table part */
    for (int row = 0; row < m_countTagsOnPage; ++row) {
        //if (i % m_valuesMap.at(0).size() == 0)
        int indexInMap = row + m_countTagsOnPage*page;
        if(indexInMap == m_valuesMap.size())
            break;

        html += "<tr>\n";
        html += QString("<td align='center'>%1</td>\n").arg(m_vertheaderTableTitles.at(indexInMap).second);
        for(int column = 0; column < m_valuesMap.at(indexInMap).size(); ++column){
            html += QString("<td align='center'>%1</td>\n")
                            .arg(m_valuesMap.at(indexInMap).at(column));
        }
        //if (i % 2 != 0)
        html += "</tr>\n";
    }
    html += "</table>\n";
    return html;
}

QString HTMLShiftWorker::pageAsHtml(int page)
{
    //const HTMLOnePage &thePage = pages.at(page);

    QString html;
    QString headerString(getHtmlFromFile(QApplication::applicationDirPath() + "/pageheader.html"));

    html += headerString.arg(m_NodeName)
                        .arg(m_curIndexShift)
                        .arg(m_horheaderTableTitles.first())
                        .arg(m_horheaderTableTitles.last());
    //html += QString("<h1 align='center'>%1</h1>\n")
    //                .arg(thePage.title.toHtmlEscaped());
    html += "<p>";
    html += itemsAsHtmlTable(page);
    html += "</p>\n";
    //html += QString("<p style='font-size:15pt;font-family:times'>"
    //                "%1</p><hr>\n").arg(thePage.descriptionHtml);

    return html;
}
void HTMLShiftWorker::populateDocument(QString &docString)
{
    QString html("<html>\n");

    html += getHtmlFromFile(QApplication::applicationDirPath() + "/header.html");
    html += "<body style='font-family: \"Tahoma\"'>\n";

    for (int page = 0; page < pages.count(); ++page) {
        html += "<div class=\"page\">";
        html += pageAsHtml(page);
        if (page + 1 < pages.count())
            html += "<br style='page-break-after:always;'/>\n";
        else
            html += getHtmlFromFile(QApplication::applicationDirPath() + "/footer.html");
        html += "</div>";
    }

    html += "</body>\n</html>\n";

    docString = html;
    emit docFinished();
}
