#ifndef SYS_ERROR_H
#define SYS_ERROR_H

#include <exception>
#include <QString>
#include <QMetaType>

#include "typemsg.h"

namespace SYS {

class Error : public std::exception
{
public:
    explicit Error(const QString& message) noexcept
        : charArray(message.toUtf8()) {}
    ~Error() noexcept {}

    const char *what() const noexcept {
        return charArray.data();
    }

private:
    QByteArray charArray;
};

//using for signal-slot system
class QError
{
public:
    enum class Type {
        ConnectionError,
        ConfigurationError,
        InternalError,
        ExternalError,
        ExchangeError,
        DataSourceError
    };

    EventLogScope::severity_level level;
    Type type;

    QError(){}
    explicit QError(EventLogScope::severity_level lvl,
                    Type tpy,
                    const QString& message) noexcept
        :level(lvl),type(tpy),charArray(message.toUtf8()){}

    const char *what() const noexcept {
        return charArray.data();
    }
    void clear() {
        charArray.clear();
    }
    /*
    QError& operator=(const QError& right) {
        //проверка на самоприсваивание
        if (this == &right) {
            return *this;
        }
        type = right.type;
        level = right.level;
        charArray = right.charArray;
        return *this;
    }
    */
private:
    QByteArray charArray;
};

}

Q_DECLARE_METATYPE(SYS::QError)

#endif // SYS_ERROR_H
