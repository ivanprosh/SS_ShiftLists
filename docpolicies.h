#ifndef DOCPOLICIES_H
#define DOCPOLICIES_H

#include <QTextDocument>

class HtmlDocPolicy {
public:
    void operator()(QTextDocument& doc){
        ;
    }
};

class PdfDocPolicy {
public:
    void operator()(QTextDocument& doc){
        ;
    }
};

#endif // OUTPOLICIES_H
