#ifndef HTMLSHIFTWORKER_H
#define HTMLSHIFTWORKER_H

#include <QObject>
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
    template<class T&&>
    void setValuesMap(T&& map){
        m_valuesMap = map;
    }
    template<class T&&>
    void setVertHeaderTableTitles(T&& vt){
        m_vertheaderTableTitles = vt;
    }
signals:
    void docResult(QTextDocument&);
    void error(SYS::QError);
public slots:
    void setHorHeaderTableTitles(const QStringList& ht){ m_horheaderTableTitles = ht; }
    void setCountTagsOnPage(int count){m_countTagsOnPage = count;}
    void process();
private:
    void populateDocument(QTextDocument&);
    QString getHtmlFromFile(const QString& filename);

    QTextDocument m_doc;
    QStringList m_horheaderTableTitles;
    QVector< QPair<QString,QString> > m_vertheaderTableTitles;
    QVector< QVector<float> > m_valuesMap;
    QList<HTMLOnePage> pages;
    int m_countTagsOnPage;
};

#endif // HTMLSHIFTWORKER_H
