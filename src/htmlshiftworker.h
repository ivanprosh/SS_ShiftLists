#ifndef HTMLSHIFTWORKER_H
#define HTMLSHIFTWORKER_H

#include <QTextDocument>

#include "sys_error.h"

struct HTMLOnePage
{
    QString header;
    QString footer;
};

class HTMLShiftWorker : public QObject
{
    Q_OBJECT

public:
    explicit HTMLShiftWorker(QObject *parent = nullptr);
    template<class T>
    void setValuesMap(T&& map){
        m_valuesMap = map;
    }
    template<class T>
    void setVertHeaderTableTitles(T&& vt){
        qDebug() << "Vert headers: " << vt;
        m_vertheaderTableTitles = vt;
    }
    const QStringList& horHeaderTableTitles() {
        return m_horheaderTableTitles;
    }
    int shiftIndex(){
        return m_curIndexShift;
    }
signals:
    void docFinished();
    void error(SYS::QError);
public slots:
    void setHorHeaderTableTitles(const QStringList& ht){ m_horheaderTableTitles = ht; }
    void setNodeName(const QString& name){m_NodeName = name;}
    void setShiftIndex(int index){m_curIndexShift = index;}
    void setCountTagsOnPage(int count){m_countTagsOnPage = count;}
    void process(QString&);
private:
    void populateDocument(QString&);
    QStringList m_horheaderTableTitles;
    QVector< QPair<QString,QString> > m_vertheaderTableTitles;
    QVector< QVector<float> > m_valuesMap;
    QList<HTMLOnePage> pages;

    QString getHtmlFromFile(const QString& filename);
    QString pageAsHtml(int page);
    QString itemsAsHtmlTable(int page);
    QString m_NodeName;
    //QTextDocument m_doc;

    int m_countTagsOnPage;
    int m_shNumber;
    int m_curIndexShift;
};

#endif // HTMLSHIFTWORKER_H
