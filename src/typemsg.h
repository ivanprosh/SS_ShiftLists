#ifndef TYPEMSG_H
#define TYPEMSG_H

namespace EventLogScope {
    const QStringList keys {"normal", "notification","warning","error","critical"};
    enum severity_level
    {
        normal,
        notification,
        warning,
        error,
        critical
    };
}


#endif // TYPEMSG_H
