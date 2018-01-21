#include "singleton.h"

/*
 *  реализация 1

QMap<QString, SSingleton*> SSingleton::map;

SSingleton::SSingleton() {
    static bool called = false;
        if (!called) {
//            QObject::connect(qApp, SIGNAL(aboutToQuit()), &cleaner,
//                             SLOT(cleanUp()));
            called = true;
        }
    }


void SSingleton::cleanUp() {
    QMapIterator<QString, SSingleton*> i(SSingleton::map);
    while (i.hasNext()) {
        i.next();
        delete i.value();
    }
    SSingleton::map.clear();
}
*/
/* реализация Loki */
using namespace Loki::Private;

Loki::Private::TrackerArray Loki::Private::pTrackerArray = 0;
unsigned int Loki::Private::elements = 0;

////////////////////////////////////////////////////////////////////////////////
// function AtExitFn
// Ensures proper destruction of objects with longevity
////////////////////////////////////////////////////////////////////////////////

void C_CALLING_CONVENTION_QUALIFIER Loki::Private::AtExitFn()
{
    assert(elements > 0 && pTrackerArray != 0);
    // Pick the element at the top of the stack
    LifetimeTracker* pTop = pTrackerArray[elements - 1];
    // Remove that object off the stack
    // Don't check errors - realloc with less memory
    //     can't fail
    pTrackerArray = static_cast<TrackerArray>(std::realloc(
        pTrackerArray, sizeof(*pTrackerArray) * --elements));
    // Destroy the element
    delete pTop;
}

////////////////////////////////////////////////////////////////////////////////
// Change log:
// June 20, 2001: ported by Nick Thurn to gcc 2.95.3. Kudos, Nick!!!
// January 10, 2002: Fixed bug in call to realloc - credit due to Nigel Gent and
//      Eike Petersen
// May 08, 2002: Refixed bug in call to realloc
////////////////////////////////////////////////////////////////////////////////
