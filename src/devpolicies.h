#ifndef DEVPOLICIES_H
#define DEVPOLICIES_H

#include <QTextDocument>
#include <QFile>
#include <QPrinter>
#include <QPrinterInfo>

#include "sys_error.h"
//#include "logger.h"

class DevPolicy {
public:
    virtual ~DevPolicy(){}
    virtual bool operator()(QString filename,
                            QTextDocument* doc) = 0;
    SYS::QError lastError;
};

class HtmlDevPolicy : public DevPolicy {
public:
    virtual bool operator()(QString filename,
                            QTextDocument* doc) override{
        //Q_ASSERT(filename.endsWith(".htm") || filename.endsWith(".html"));
        //lastError = SYS::QError()
        QFile file(filename.append(".html"));
        if (!file.open(QIODevice::WriteOnly|QIODevice::Text)) {
            lastError = SYS::QError(EventLogScope::error,
                                    SYS::QError::Type::ExternalError,
                                    QString("Failed to open file %1:%2")
                                    .arg(filename).arg(file.errorString()));
            return false;
        }
        QTextStream out(&file);
        out.setCodec("utf-8");
        out << doc->toHtml("utf-8");
        file.close();
        return true;
    }
};
class PdfDevPolicy : public DevPolicy {
public:
    virtual bool operator()(QString filename,
                            QTextDocument* doc) override{
        QPrinter printer(QPrinter::HighResolution);
        printer.setPaperSize(QPrinter::A4);
        printer.setOrientation(QPrinter::Landscape);
        filename.append(".pdf");
        printer.setOutputFileName(filename);
        printer.setOutputFormat(QPrinter::PdfFormat);
        doc->print(&printer);
        return true;
    }
};

class PrinterDevPolicy : public DevPolicy {
public:
    virtual bool operator()(QString filename,
                            QTextDocument* doc) override{
        Q_UNUSED(filename)
        QPrinter printer(QPrinterInfo::defaultPrinter(),
                         QPrinter::HighResolution);
        if(!printer.isValid()){
            lastError = SYS::QError(EventLogScope::error,
                                    SYS::QError::Type::ExternalError,
                                    QString("Failed to use default printer"));
            return false;
        }
        printer.setPaperSize(QPrinter::A4);
        printer.setOrientation(QPrinter::Landscape);
        doc->print(&printer);
        return true;
    }
};

#endif // OUTPOLICIES_H
