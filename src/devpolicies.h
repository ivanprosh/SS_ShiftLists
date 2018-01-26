#ifndef DEVPOLICIES_H
#define DEVPOLICIES_H

#include <QPrinter>
#include <QPrinterInfo>

/*
#include <QTextDocument>
#include <QFile>
#include <QTimer>
#include <QDebug>
#include <QEventLoop>
#include <QApplication>
#include <QThread>
*/
#include "sys_error.h"

class DevPolicy : public QObject
{
    Q_OBJECT

public:
    virtual ~DevPolicy(){}
    virtual void process(QString filename,
                            const QString& doc) = 0;
    SYS::QError lastError;

signals:
    void error(SYS::QError);
    void success();
};

class HtmlDevPolicy : public DevPolicy
{
     Q_OBJECT

public:
    virtual void process(QString filename,
                            const QString& doc) override{
        //Q_ASSERT(filename.endsWith(".htm") || filename.endsWith(".html"));
        //lastError = SYS::QError()
        QFile file(filename.append(".html"));
        if (!file.open(QIODevice::WriteOnly|QIODevice::Text)) {
            lastError = SYS::QError(EventLogScope::error,
                                    SYS::QError::Type::ExternalError,
                                    QString("Failed to open file %1:%2")
                                    .arg(filename).arg(file.errorString()));
            emit error(lastError);
            return;
        }
        QTextStream out(&file);
        out.setCodec("utf-8");
        out << doc;//doc->toHtml("utf-8");
        file.close();
        emit success();
    }
};


class PrinterDevPolicy : public DevPolicy
{
    Q_OBJECT

public:
    virtual void process(QString filename,
                            const QString& doc) override{
        Q_UNUSED(doc)
        Q_UNUSED(filename)

        QPrinter printer(QPrinterInfo::defaultPrinter(),
                         QPrinter::HighResolution);
        if(!printer.isValid()){
            lastError = SYS::QError(EventLogScope::error,
                                    SYS::QError::Type::ExternalError,
                                    QString("Failed to use default printer"));
            return;
        }
        printer.setPaperSize(QPrinter::A4);
        printer.setOrientation(QPrinter::Landscape);

        emit success();
    }
};

class PdfDevPolicy : public DevPolicy
{
    Q_OBJECT

public:
    PdfDevPolicy() : DevPolicy(), printer(QPrinter::HighResolution) {
    }
    virtual void process(QString filename,
                            const QString& doc) override{
        Q_UNUSED(doc)

        //connect(&page, &QWebEnginePage::loadFinished, this, &PdfDevPolicy::onLoad );
        lastError.clear();

        filename.append(".pdf");

/*
 *        not success, webpage don't load, but load before separated thread created (it's my opinion)
 *        This method start from UI thread and it's not the point
 */
//        page.setHtml(doc);
//        QEventLoop loop;
//        QTimer timeouttimer;
//        timeouttimer.setSingleShot(true);

//        connect(&page, &QWebEnginePage::loadFinished, &loop, &QEventLoop::quit );
//        connect(&timeouttimer, &QTimer::timeout, &loop, &QEventLoop::quit);
//        timeouttimer.start(10000);
//        loop.exec();

//        if(!timeouttimer.isActive()){
//            emit error(SYS::QError(EventLogScope::error,
//                                   SYS::QError::Type::ExternalError,
//                                   QString("Timeout waiting for load page")));
//            return;
//        }

//        if(!lastError.isEmpty()){
//            emit error(lastError);
//            return;
//        }

    }
public slots:
    void onLoad(bool ok) {
        if(!ok){
            lastError = SYS::QError(EventLogScope::error,
                                    SYS::QError::Type::ExternalError,
                                    QString("Failed to load html page"));
            emit error(lastError);
            return;
        }
        //m_manager->page.printToPdf(qApp->applicationDirPath() + "/test.pdf");
        emit success();
    }
private:
    //QWebEnginePage& page;
    QPrinter printer;
};

#endif // OUTPOLICIES_H
