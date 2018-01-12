#ifndef SYS_ERROR_H
#define SYS_ERROR_H

#include <exception>
#include <QString>

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

}

#endif // SYS_ERROR_H
