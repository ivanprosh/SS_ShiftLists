#ifndef MSG_H
#define MSG_H

#include "sys_error.h"

#include <QLoggingCategory>
#include <QApplication>
#include <QByteArray>
#include <QList>
#include <QMessageBox>
#include <QSet>
#include <QScopedPointer>
#include <QString>
#include <QVector>

#ifdef Q_CC_MSVC
#include <math.h>
#else
#include <cmath>
#endif

// For weird Microsoft compilers, e.g. MSVC9
#ifndef M_PI
#define M_PI 3.14159265358
#endif

Q_DECLARE_LOGGING_CATEGORY(lgSysStream)

namespace SYS {

/* возвращает приведенное к родному типу значение класса enum'a */
template<typename E>
constexpr typename std::underlying_type<E>::type
toUType(E enumerator) noexcept{
    return static_cast<typename std::underlying_type<E>::type>(enumerator);
}

void information(QWidget *parent, const QString &title,
        const QString &text, const QString &detailedText=QString());
void warning(QWidget *parent, const QString &title,
        const QString &text, const QString &detailedText=QString());
bool question(QWidget *parent, const QString &title,
        const QString &text, const QString &detailedText=QString(),
        const QString &yesText=QObject::tr("&Yes"),
        const QString &noText=QObject::tr("&No"));
bool okToDelete(QWidget *parent, const QString &title,
        const QString &text, const QString &detailedText=QString());

template<typename T>
bool okToClearData(bool (T::*saveData)(), T *parent,
        const QString &title, const QString &text,
        const QString &detailedText=QString())
{
    Q_ASSERT(saveData && parent);
    QScopedPointer<QMessageBox> messageBox(new QMessageBox(parent));
    messageBox->setWindowModality(Qt::WindowModal);
    messageBox->setIcon(QMessageBox::Question);
    messageBox->setWindowTitle(QString("%1 - %2")
            .arg(QApplication::applicationName()).arg(title));
    messageBox->setText(text);
    if (!detailedText.isEmpty())
        messageBox->setInformativeText(detailedText);
    messageBox->addButton(QMessageBox::Save);
    messageBox->addButton(QMessageBox::Discard);
    messageBox->addButton(QMessageBox::Cancel);
    messageBox->setDefaultButton(QMessageBox::Save);
    messageBox->exec();
    if (messageBox->clickedButton() ==
        messageBox->button(QMessageBox::Cancel))
        return false;
    if (messageBox->clickedButton() ==
        messageBox->button(QMessageBox::Save))
        return (parent->*saveData)();
    return true;
}

const int MSecPerSecond = 1000;

void hoursMinutesSecondsForMSec(const int msec, int *hours,
                                int *minutes, int *seconds);

inline qreal radiansFromDegrees(qreal degrees)
    { return degrees * M_PI / 180.0; }
inline qreal degreesFromRadians(qreal radians)
    { return radians * 180.0 / M_PI; }

QString applicationPathOf(const QString &path=QString());
QString filenameFilter(const QString &name,
                       const QList<QByteArray> formats);
QString filenameFilter(const QString &name,
                       const QStringList &mimeTypes);
QSet<QString> suffixesForMimeTypes(const QStringList &mimeTypes);

QVector<int> chunkSizes(const int size, const int chunkCount);
}
#endif // MSG_H
