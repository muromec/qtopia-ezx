/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qobject.h"
#include "qobject_p.h"

#include "qabstracteventdispatcher.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"
#include "qvariant.h"
#include "qmetaobject.h"
#include <qregexp.h>
#include <qthread.h>
#include <private/qthread_p.h>
#include <qdebug.h>
#include <qhash.h>
#include <qpair.h>
#include <qvarlengtharray.h>
#include <qset.h>
#include <qsemaphore.h>

#include <new>

#include <ctype.h>
#include <limits.h>

static int DIRECT_CONNECTION_ONLY = 0;

Q_GLOBAL_STATIC(QReadWriteLock, qt_object_read_write_lock)
QReadWriteLock *QObjectPrivate::readWriteLock() { return qt_object_read_write_lock(); }

static int *queuedConnectionTypes(const QList<QByteArray> &typeNames)
{
    int *types = static_cast<int *>(qMalloc((typeNames.count() + 1) * sizeof(int)));
    for (int i = 0; i < typeNames.count(); ++i) {
        const QByteArray typeName = typeNames.at(i);
        if (typeName.endsWith('*'))
            types[i] = QMetaType::VoidStar;
        else
            types[i] = QMetaType::type(typeName);

        if (!types[i]) {
            qWarning("QObject::connect: Cannot queue arguments of type '%s'\n"
                     "(Make sure '%s' is registered using qRegisterMetaType().)",
                     typeName.constData(), typeName.constData());
            qFree(types);
            return 0;
        }
    }
    types[typeNames.count()] = 0;

    return types;
}

struct QConnection {
    QObject *sender;
    int signal;
    QObject *receiver;
    int method;
    uint refCount:29;
    uint type:3; // 0 == auto, 1 == direct, 2 == queued, 4 == blocking
    int *types;
};
Q_DECLARE_TYPEINFO(QConnection, Q_MOVABLE_TYPE);

class QConnectionList
{
public:
    QReadWriteLock lock;

    typedef QMultiHash<const QObject *, int> Hash;
    Hash sendersHash, receiversHash;
    QList<int> unusedConnections;
    typedef QList<QConnection> List;
    List connections;

    void remove(QObject *object);

    void addConnection(QObject *sender, int signal,
                       QObject *receiver, int method,
                       int type = 0, int *types = 0);
    bool removeConnection(QObject *sender, int signal,
                          QObject *receiver, int method);
};

Q_GLOBAL_STATIC(QConnectionList, connectionList)

/*! \internal

    Removes \a object from the connection list completely, i.e. all
    connections containing \a object are removed.
*/
void QConnectionList::remove(QObject *object)
{
    for (int i = 0; i < 2; ++i) {
        Hash &hash1 = i == 0 ? sendersHash : receiversHash;
        Hash &hash2 = i == 0 ? receiversHash : sendersHash;

        Hash::iterator it = hash1.find(object);
        const Hash::iterator end = hash1.end();
        while (it != end && it.key() == object) {
            const int at = it.value();
            QConnection &c = connections[at];
            if (c.sender) {
                if (c.types && c.types != &DIRECT_CONNECTION_ONLY) {
                    qFree(c.types);
                    c.types = 0;
                }
                it = hash1.erase(it);

                const QObject * const partner = i == 0 ? c.receiver : c.sender;
                Hash::iterator x = hash2.find(partner);
                const Hash::iterator xend = hash2.end();
                while (x != xend && x.key() == partner) {
                    if (x.value() == at) {
                        x = hash2.erase(x);
                        break;
                    } else {
                        ++x;
                    }
                }

                uint refCount = c.refCount;
                memset(&c, 0, sizeof(c));
                c.refCount = refCount;
                Q_ASSERT(!unusedConnections.contains(at));
                unusedConnections.prepend(at);
            } else {
                ++it;
            }
        }
    }
}

/*! \internal
    Adds the specified connection.
*/
void QConnectionList::addConnection(QObject *sender, int signal,
                                    QObject *receiver, int method,
                                    int type, int *types)
{
    QConnection c = { sender, signal, receiver, method, 0, 0, types };
    c.type = type; // don't warn on VC++6
    int at = -1;
    for (int i = 0; i < unusedConnections.size(); ++i) {
        if (!connections.at(unusedConnections.at(i)).refCount) {
            // reuse an unused connection
            at = unusedConnections.takeAt(i);
            connections[at] = c;
            break;
        }
    }
    if (at == -1) {
        // append new connection
        at = connections.size();
        connections << c;
    }
    sendersHash.insert(sender, at);
    receiversHash.insert(receiver, at);
}

/*! \internal

    Removes the specified connection.  See QObject::disconnect() for
    more information about valid arguments.
 */
bool QConnectionList::removeConnection(QObject *sender, int signal,
                                       QObject *receiver, int method)
{
    bool success = false;
    Hash::iterator it = sendersHash.find(sender);
    while (it != sendersHash.end() && it.key() == sender) {
        const int at = it.value();
        QConnection &c = connections[at];
        if (c.receiver
            && (signal < 0 || signal == c.signal)
            && (receiver == 0
                || (c.receiver == receiver && (method < 0 || method == c.method)))) {
            if (c.types) {
                if (c.types != &DIRECT_CONNECTION_ONLY)
                    qFree(c.types);
                c.types = 0;
            }
            it = sendersHash.erase(it);

            Hash::iterator x = receiversHash.find(c.receiver);
            const Hash::iterator xend = receiversHash.end();
            while (x != xend && x.key() == c.receiver) {
                if (x.value() == at) {
                    x = receiversHash.erase(x);
                    break;
                } else {
                    ++x;
                }
            }

            uint refCount = c.refCount;
            memset(&c, 0, sizeof(c));
            c.refCount = refCount;
            unusedConnections << at;
            success = true;
        } else {
            ++it;
        }
    }
    return success;
}

/*
  QObjectSet sets the minimum capacity to 4099 (the first prime number
  after 4096), so that we can speed up QObject destruction.
 */
class QObjectSet : public QSet<QObject *>
{
public:
    QObjectSet()
    { reserve(4099); }
};

Q_GLOBAL_STATIC(QObjectSet, allObjects)

extern "C" Q_CORE_EXPORT void qt_addObject(QObject *object)
{
    QWriteLocker locker(QObjectPrivate::readWriteLock());
    QObjectSet *set = allObjects();
    if (set)
        set->insert(object);
}

extern "C" Q_CORE_EXPORT void qt_removeObject(QObject *object)
{
    QObjectSet *set = allObjects();
    if (set)
        set->remove(object);
}

#ifdef Q_CC_MSVC
#pragma warning(push)
#pragma warning(disable:4190)
#endif

extern "C" Q_CORE_EXPORT QList<QObject *> qt_allTopLevelWidgets()
{
    QList<QObject *> list;

    QReadLocker locker(QObjectPrivate::readWriteLock());
    const QObjectSet *set = allObjects();
    if (!set)
        return list;

    for (QSet<QObject *>::const_iterator it = set->constBegin(); it != set->constEnd(); ++it) {
        if ((*it)->isWidgetType() && !(*it)->parent())
            list << *it;
    }

    return list;
}

#ifdef Q_CC_MSVC
#pragma warning(pop)
#endif

bool QObjectPrivate::isValidObject(QObject *object)
{
    QObjectSet *set = allObjects();
    return set ? set->contains(object) : false;
}

QObjectPrivate::QObjectPrivate(int version)
    : threadData(0), currentSender(0), currentSenderSignalIdStart(-1), currentSenderSignalIdEnd(-1)
{
    if (version != QObjectPrivateVersion)
        qFatal("Cannot mix incompatible Qt libraries");

    // QObjectData initialization
    q_ptr = 0;
    parent = 0;                                 // no parent yet. It is set by setParent()
    isWidget = false;                           // assume not a widget object
    pendTimer = false;                          // no timers yet
    blockSig = false;                           // not blocking signals
    wasDeleted = false;                         // double-delete catcher
    sendChildEvents = true;                     // if we should send ChildInsert and ChildRemove events to parent
    receiveChildEvents = true;
    postedEvents = 0;
    extraData = 0;
    connectedSignals = 0;
}

QObjectPrivate::~QObjectPrivate()
{
#ifndef QT_NO_USERDATA
    if (extraData)
        qDeleteAll(extraData->userData);
    delete extraData;
#endif
}

#ifdef QT3_SUPPORT
void QObjectPrivate::sendPendingChildInsertedEvents()
{
    Q_Q(QObject);
    for (int i = 0; i < pendingChildInsertedEvents.size(); ++i) {
        QObject *c = pendingChildInsertedEvents.at(i);
        if (!c)
            continue;
        QChildEvent childEvent(QEvent::ChildInserted, c);
        QCoreApplication::sendEvent(q, &childEvent);
    }
    pendingChildInsertedEvents.clear();
}

void QObjectPrivate::removePendingChildInsertedEvents(QObject *child)
{
    if (!child) {
        pendingChildInsertedEvents.clear();
        return;
    }

    // the QObject destructor calls QObject::removeChild, which calls
    // QCoreApplication::sendEvent() directly.  this can happen while the event
    // loop is in the middle of posting events, and when we get here, we may
    // not have any more posted events for this object.

    // if this is a child remove event and the child insert hasn't
    // been dispatched yet, kill that insert
    for (int i = 0; i < pendingChildInsertedEvents.size(); ++i) {
        QObject *&c = pendingChildInsertedEvents[i];
        if (c == child)
            c = 0;
    }
}
#endif

bool QObjectPrivate::isSender(const QObject *receiver, const char *signal) const
{
    Q_Q(const QObject);
    int signal_index = q->metaObject()->indexOfSignal(signal);
    if (signal_index < 0)
        return false;
    QConnectionList *list = ::connectionList();
    QReadLocker locker(&list->lock);
    QConnectionList::Hash::const_iterator it = list->sendersHash.constFind(q);
    while (it != list->sendersHash.constEnd() && it.key() == q) {
        const QConnection &c = list->connections.at(it.value());
        if (c.signal == signal_index && c.receiver == receiver)
            return true;
        ++it;
    }
    return false;
}

QObjectList QObjectPrivate::receiverList(const char *signal) const
{
    Q_Q(const QObject);
    QObjectList receivers;
    int signal_index = q->metaObject()->indexOfSignal(signal);
    if (signal_index < 0)
        return receivers;
    QConnectionList *list = ::connectionList();
    QReadLocker locker(&list->lock);
    QConnectionList::Hash::const_iterator it = list->sendersHash.constFind(q);
    while (it != list->sendersHash.constEnd() && it.key() == q) {
        const QConnection &c = list->connections.at(it.value());
        if (c.signal == signal_index)
            receivers << c.receiver;
        ++it;
    }
    return receivers;
}

QObjectList QObjectPrivate::senderList() const
{
    Q_Q(const QObject);
    QObjectList senders;
    QConnectionList *list = ::connectionList();
    QReadLocker locker(&list->lock);
    QConnectionList::Hash::const_iterator it = list->receiversHash.constFind(q);
    while (it != list->receiversHash.constEnd() && it.key() == q) {
        const QConnection &c = list->connections.at(it.value());
        senders << c.sender;
        ++it;
    }
    return senders;
}

typedef QMultiHash<QObject *, QObject **> GuardHash;
Q_GLOBAL_STATIC(GuardHash, guardHash)
Q_GLOBAL_STATIC(QReadWriteLock, guardHashLock)

/*!\internal
 */
void QMetaObject::addGuard(QObject **ptr)
{
    if (!*ptr)
        return;
    GuardHash *hash = guardHash();
    if (!hash) {
        *ptr = 0;
        return;
    }
    QWriteLocker locker(guardHashLock());
    hash->insert(*ptr, ptr);
}

/*!\internal
 */
void QMetaObject::removeGuard(QObject **ptr)
{
    if (!*ptr)
        return;
    GuardHash *hash = guardHash();
    if (!hash)
        return;
    QWriteLocker locker(guardHashLock());
    GuardHash::iterator it = hash->find(*ptr);
    const GuardHash::iterator end = hash->end();
    for (; it.key() == *ptr && it != end; ++it) {
        if (it.value() == ptr) {
            (void) hash->erase(it);
            break;
        }
    }
}

/*!\internal
 */
void QMetaObject::changeGuard(QObject **ptr, QObject *o)
{
    GuardHash *hash = guardHash();
    if (!hash) {
        *ptr = 0;
        return;
    }
    QWriteLocker locker(guardHashLock());
    if (*ptr) {
        GuardHash::iterator it = hash->find(*ptr);
        const GuardHash::iterator end = hash->end();
        for (; it.key() == *ptr && it != end; ++it) {
            if (it.value() == ptr) {
                (void) hash->erase(it);
                break;
            }
        }
    }
    *ptr = o;
    if (*ptr)
        hash->insert(*ptr, ptr);
}

/*! \internal
 */
void QObjectPrivate::clearGuards(QObject *object)
{
    GuardHash *hash = ::guardHash();
    if (hash) {
        QWriteLocker locker(guardHashLock());
        GuardHash::iterator it = hash->find(object);
        const GuardHash::iterator end = hash->end();
        while (it.key() == object && it != end) {
            *it.value() = 0;
            it = hash->erase(it);
        }
    }
}

/*! \internal
 */
QMetaCallEvent::QMetaCallEvent(int id, const QObject *sender,
                               int nargs, int *types, void **args,
                               QSemaphore *semaphore)
    :QEvent(MetaCall), id_(id), sender_(sender), idFrom_(-1), idTo_(-1),
     nargs_(nargs), types_(types), args_(args), semaphore_(semaphore)
{ }

/*! \internal
 */
QMetaCallEvent::QMetaCallEvent(int id, const QObject *sender, int idFrom, int idTo,
                               int nargs, int *types, void **args, QSemaphore *semaphore)
    : QEvent(MetaCall), id_(id), sender_(sender), idFrom_(idFrom), idTo_(idTo),
      nargs_(nargs), types_(types), args_(args), semaphore_(semaphore)
{ }

/*! \internal
 */
QMetaCallEvent::~QMetaCallEvent()
{
    for (int i = 0; i < nargs_; ++i) {
        if (types_[i] && args_[i])
            QMetaType::destroy(types_[i], args_[i]);
    }
    if (types_) qFree(types_);
    if (args_) qFree(args_);
#ifndef QT_NO_THREAD
    if (semaphore_)
        semaphore_->release();
#endif
}

/*! \internal
 */
int QMetaCallEvent::placeMetaCall(QObject *object)
{
    return object->qt_metacall(QMetaObject::InvokeMetaMethod, id_, args_);
}

/*!
    \class QObject
    \brief The QObject class is the base class of all Qt objects.

    \ingroup objectmodel
    \mainclass
    \reentrant

    QObject is the heart of the \l{Qt object model}. The central
    feature in this model is a very powerful mechanism for seamless
    object communication called \l{signals and slots}. You can
    connect a signal to a slot with connect() and destroy the
    connection with disconnect(). To avoid never ending notification
    loops you can temporarily block signals with blockSignals(). The
    protected functions connectNotify() and disconnectNotify() make
    it possible to track connections.

    QObjects organize themselves in object trees. When you create a
    QObject with another object as parent, the object will
    automatically add itself to the parent's children() list. The
    parent takes ownership of the object i.e. it will automatically
    delete its children in its destructor. You can look for an object
    by name and optionally type using findChild() or findChildren().

    Every object has an objectName() and its class name can be found
    via the corresponding metaObject() (see QMetaObject::className()).
    You can determine whether the object's class inherits another
    class in the QObject inheritance hierarchy by using the
    inherits() function.

    When an object is deleted, it emits a destroyed() signal. You can
    catch this signal to avoid dangling references to QObjects.

    QObjects can receive events through event() and filter the events
    of other objects. See installEventFilter() and eventFilter() for
    details. A convenience handler, childEvent(), can be reimplemented
    to catch child events.

    Events are delivered in the thread in which the object was
    created; see \l{Thread Support in Qt} and thread() for details.
    Note that event processing is not done at all for QObjects with no
    thread affinity (thread() returns zero). Use the moveToThread()
    function to change the thread affinity for an object and its
    children (the object cannot be moved if it has a parent).

    Last but not least, QObject provides the basic timer support in
    Qt; see QTimer for high-level support for timers.

    Notice that the Q_OBJECT macro is mandatory for any object that
    implements signals, slots or properties. You also need to run the
    \l{moc}{Meta Object Compiler} on the source file. We strongly
    recommend the use of this macro in all subclasses of QObject
    regardless of whether or not they actually use signals, slots and
    properties, since failure to do so may lead certain functions to
    exhibit strange behavior.

    All Qt widgets inherit QObject. The convenience function
    isWidgetType() returns whether an object is actually a widget. It
    is much faster than
    \l{qobject_cast()}{qobject_cast}<QWidget *>(\e{obj}) or
    \e{obj}->\l{inherits()}{inherits}("QWidget").

    Some QObject functions, e.g. children(), return a QObjectList.
    QObjectList is a typedef for QList<QObject *>.

    \section1 Auto-Connection

    Qt's meta-object system provides a mechanism to automatically connect
    signals and slots between QObject subclasses and their children. As long
    as objects are defined with suitable object names, and slots follow a
    simple naming convention, this connection can be performed at run-time
    by the QMetaObject::connectSlotsByName() function.

    \l uic generates code that invokes this function to enable auto-connection
    to be performed between widgets on forms created with \QD. More information
    about using auto-connection with \QD is given in the
    \l{Using a Component in Your Application}{Using a
    Component in Your Application} section of the \QD manual.

    \section1 Dynamic Properties

    From Qt 4.2, dynamic properties can be added to and removed from QObject
    instances at run-time. Dynamic properties do not need to be declared at
    compile-time, yet they provide the same advantages as static properties
    and are manipulated using the same API - using property() to read them
    and setProperty() to write them.

    From Qt 4.3, dynamic properties are supported by
    \l{Qt Designer's Widget Editing Mode#The Property Editor}{Qt Designer},
    and both standard Qt widgets and user-created forms can be given dynamic
    properties.

    \sa QMetaObject, QPointer, QObjectCleanupHandler,
        {Object Trees and Object Ownership}
*/

/*!
    \relates QObject

    Returns a pointer to the object named \a name that inherits \a
    type and with a given \a parent.

    Returns 0 if there is no such child.

    \code
        QLineEdit *lineEdit = static_cast<QLineEdit *>(
                qt_find_obj_child(myWidget, "QLineEdit", "my line edit"));
        if (lineEdit)
            lineEdit->setText("Default");
    \endcode
*/

void *qt_find_obj_child(QObject *parent, const char *type, const QString &name)
{
    QObjectList list = parent->children();
    if (list.size() == 0) return 0;
    for (int i = 0; i < list.size(); ++i) {
        QObject *obj = list.at(i);
        if (name == obj->objectName() && obj->inherits(type))
            return obj;
    }
    return 0;
}


/*****************************************************************************
  QObject member functions
 *****************************************************************************/

// check the constructor's parent thread argument
static bool check_parent_thread(QObject *parent,
                                QThreadData *parentThreadData,
                                QThreadData *currentThreadData)
{
    if (parent && parentThreadData != currentThreadData) {
        QThread *parentThread = parentThreadData->thread;
        QThread *currentThread = currentThreadData->thread;
        qWarning("QObject: Cannot create children for a parent that is in a different thread.\n"
                 "(Parent is %s(%p), parent's thread is %s(%p), current thread is %s(%p)",
                 parent->metaObject()->className(),
                 parent,
                 parentThread ? parentThread->metaObject()->className() : "QThread",
                 parentThread,
                 currentThread ? currentThread->metaObject()->className() : "QThread",
                 currentThread);
        return false;
    }
    return true;
}

/*!
    Constructs an object with parent object \a parent.

    The parent of an object may be viewed as the object's owner. For
    instance, a \l{QDialog}{dialog box} is the parent of the \gui OK
    and \gui Cancel buttons it contains.

    The destructor of a parent object destroys all child objects.

    Setting \a parent to 0 constructs an object with no parent. If the
    object is a widget, it will become a top-level window.

    \sa parent(), findChild(), findChildren()
*/

QObject::QObject(QObject *parent)
    : d_ptr(new QObjectPrivate)
{
    Q_D(QObject);
    ::qt_addObject(d_ptr->q_ptr = this);
    d->threadData = QThreadData::current();
    d->threadData->ref();
    if (!check_parent_thread(parent, parent ? parent->d_func()->threadData : 0, d->threadData))
        parent = 0;
    setParent(parent);
}

#ifdef QT3_SUPPORT
/*!
    \overload
    \obsolete

    Creates a new QObject with the given \a parent and object \a name.
 */
QObject::QObject(QObject *parent, const char *name)
    : d_ptr(new QObjectPrivate)
{
    Q_D(QObject);
    ::qt_addObject(d_ptr->q_ptr = this);
    d->threadData = QThreadData::current();
    d->threadData->ref();
    if (!check_parent_thread(parent, parent ? parent->d_func()->threadData : 0, d->threadData))
        parent = 0;
    setParent(parent);
    setObjectName(QString::fromAscii(name));
}
#endif

/*! \internal
 */
QObject::QObject(QObjectPrivate &dd, QObject *parent)
    : d_ptr(&dd)
{
    Q_D(QObject);
    ::qt_addObject(d_ptr->q_ptr = this);
    d->threadData = QThreadData::current();
    d->threadData->ref();
    if (!check_parent_thread(parent, parent ? parent->d_func()->threadData : 0, d->threadData))
        parent = 0;
    if (d->isWidget) {
        if (parent) {
            d->parent = parent;
            d->parent->d_func()->children.append(this);
        }
        // no events sent here, this is done at the end of the QWidget constructor
    } else {
        setParent(parent);
    }
}

/*!
    Destroys the object, deleting all its child objects.

    All signals to and from the object are automatically disconnected, and
    any pending posted events for the object are removed from the event
    queue. However, it is often safer to use deleteLater() rather than
    deleting a QObject subclass directly.

    \warning All child objects are deleted. If any of these objects
    are on the stack or global, sooner or later your program will
    crash. We do not recommend holding pointers to child objects from
    outside the parent. If you still do, the destroyed() signal gives
    you an opportunity to detect when an object is destroyed.

    \warning Deleting a QObject while pending events are waiting to
    be delivered can cause a crash. You must not delete the QObject
    directly if it exists in a different thread than the one currently
    executing. Use deleteLater() instead, which will cause the event
    loop to delete the object after all pending events have been
    delivered to it.

    \sa deleteLater()
*/

QObject::~QObject()
{
    Q_D(QObject);
    if (d->wasDeleted) {
#if defined(QT_DEBUG)
        qWarning("QObject: Double deletion detected");
#endif
        return;
    }
    d->wasDeleted = true;

    d->blockSig = 0; // unblock signals so we always emit destroyed()

    if (!d->isWidget) {
        // set all QPointers for this object to zero - note that
        // ~QWidget() does this for us, so we don't have to do it twice
        QObjectPrivate::clearGuards(this);
     }

    emit destroyed(this);

    QConnectionList *list = ::connectionList();
    if (list) {
        QWriteLocker locker(&list->lock);
        list->remove(this);
    }

    if (d->pendTimer) {
        // unregister pending timers
        if (d->threadData->eventDispatcher)
            d->threadData->eventDispatcher->unregisterTimers(this);
    }

#ifdef QT3_SUPPORT
    d->pendingChildInsertedEvents.clear();
#endif

    d->eventFilters.clear();

    if (!d->children.isEmpty())
        d->deleteChildren();

    {
        QWriteLocker locker(QObjectPrivate::readWriteLock());
        ::qt_removeObject(this);

        /*
          theoretically, we cannot check d->postedEvents without
          holding the postEventList.mutex for the object's thread,
          but since we hold the QObjectPrivate::readWriteLock(),
          nothing can go into QCoreApplication::postEvent(), which
          effectively means no one can post new events, which is what
          we are trying to prevent. this means we can safely check
          d->postedEvents, since we are fairly sure it will not
          change (it could, but only by decreasing, i.e. removing
          posted events from a differebnt thread)
        */
        if (d->postedEvents > 0)
            QCoreApplication::removePostedEvents(this);
    }

    if (d->parent)        // remove it from parent object
        d->setParent_helper(0);

    d->threadData->deref();

    delete d;
    d_ptr = 0;
}


/*!
    \fn QMetaObject *QObject::metaObject() const

    Returns a pointer to the meta-object of this object.

    A meta-object contains information about a class that inherits
    QObject, e.g. class name, superclass name, properties, signals and
    slots. Every class that contains the Q_OBJECT macro will also have
    a meta-object.

    The meta-object information is required by the signal/slot
    connection mechanism and the property system. The inherits()
    function also makes use of the meta-object.

    If you have no pointer to an actual object instance but still
    want to access the meta-object of a class, you can use \l
    staticMetaObject.

    Example:

    \code
        QObject *obj = new QPushButton;
        obj->metaObject()->className();             // returns "QPushButton"

        QPushButton::staticMetaObject.className();  // returns "QPushButton"
    \endcode

    \sa staticMetaObject
*/

/*!
    \variable QObject::staticMetaObject

    This variable stores the meta-object for the class.

    A meta-object contains information about a class that inherits
    QObject, e.g. class name, superclass name, properties, signals and
    slots. Every class that contains the Q_OBJECT macro will also have
    a meta-object.

    The meta-object information is required by the signal/slot
    connection mechanism and the property system. The inherits()
    function also makes use of the meta-object.

    If you have a pointer to an object, you can use metaObject() to
    retrieve the meta-object associated with that object.

    Example:

    \code
        QPushButton::staticMetaObject.className();  // returns "QPushButton"

        QObject *obj = new QPushButton;
        obj->metaObject()->className();             // returns "QPushButton"
    \endcode

    \sa metaObject()
*/

/*! \fn T *qobject_cast<T *>(QObject *object)
    \relates QObject

    Returns the given \a object cast to type T if the object is of type
    T (or of a subclass); otherwise returns 0.

    The class T must inherit (directly or indirectly) QObject and be
    declared with the \l Q_OBJECT macro.

    A class is considered to inherit itself.

    Example:

    \code
        QObject *obj = new QTimer;          // QTimer inherits QObject

        QTimer *timer = qobject_cast<QTimer *>(obj);
        // timer == (QObject *)obj

        QAbstractButton *button = qobject_cast<QAbstractButton *>(obj);
        // button == 0
    \endcode

    The qobject_cast() function behaves similarly to the standard C++
    \c dynamic_cast(), with the advantages that it doesn't require
    RTTI support and it works across dynamic library boundaries.

    qobject_cast() can also be used in conjunction with interfaces;
    see the \l{tools/plugandpaint}{Plug & Paint} example for details.

    \warning If T isn't declared with the Q_OBJECT macro, this
    function's return value is undefined.

    \sa QObject::inherits()
*/

/*!
    \fn bool QObject::inherits(const char *className) const

    Returns true if this object is an instance of a class that
    inherits \a className or a QObject subclass that inherits \a
    className; otherwise returns false.

    A class is considered to inherit itself.

    Example:

    \code
        QTimer *timer = new QTimer;         // QTimer inherits QObject
        timer->inherits("QTimer");          // returns true
        timer->inherits("QObject");         // returns true
        timer->inherits("QAbstractButton"); // returns false

        // QLayout inherits QObject and QLayoutItem
        QLayout *layout = new QLayout;
        layout->inherits("QObject");        // returns true
        layout->inherits("QLayoutItem");    // returns false
    \endcode

    (\l QLayoutItem is not a QObject.)

    Consider using qobject_cast<Type *>(object) instead. The method
    is both faster and safer.

    \sa metaObject(), qobject_cast()
*/

/*!
    \property QObject::objectName

    \brief the name of this object

    You can find an object by name (and type) using findChild(). You can
    find a set of objects with findChildren().

    \code
        qDebug("MyClass::setPrecision(): (%s) invalid precision %f",
               qPrintable(objectName()), newPrecision);
    \endcode

    \sa metaObject(), QMetaObject::className()
*/

QString QObject::objectName() const
{
    Q_D(const QObject);
    return d->objectName;
}

/*
    Sets the object's name to \a name.
*/
void QObject::setObjectName(const QString &name)
{
    Q_D(QObject);
    d->objectName = name;
}


#ifdef QT3_SUPPORT
/*! \internal
    QObject::child is compat but needs to call itself recursively,
    that's why we need this helper.
*/
static QObject *qChildHelper(const char *objName, const char *inheritsClass,
                             bool recursiveSearch, const QObjectList &children)
{
    if (children.isEmpty())
        return 0;

    bool onlyWidgets = (inheritsClass && qstrcmp(inheritsClass, "QWidget") == 0);
    const QLatin1String oName(objName);
    for (int i = 0; i < children.size(); ++i) {
        QObject *obj = children.at(i);
        if (onlyWidgets) {
            if (obj->isWidgetType() && (!objName || obj->objectName() == oName))
                return obj;
        } else if ((!inheritsClass || obj->inherits(inheritsClass))
                   && (!objName || obj->objectName() == oName))
            return obj;
        if (recursiveSearch && (obj = qChildHelper(objName, inheritsClass,
                                                   recursiveSearch, obj->children())))
            return obj;
    }
    return 0;
}


/*!
    Searches the children and optionally grandchildren of this object,
    and returns a child that is called \a objName that inherits \a
    inheritsClass. If \a inheritsClass is 0 (the default), any class
    matches.

    If \a recursiveSearch is true (the default), child() performs a
    depth-first search of the object's children.

    If there is no such object, this function returns 0. If there are
    more than one, the first one found is returned.
*/
QObject* QObject::child(const char *objName, const char *inheritsClass,
                         bool recursiveSearch) const
{
    Q_D(const QObject);
    return qChildHelper(objName, inheritsClass, recursiveSearch, d->children);
}
#endif

/*!
    \fn bool QObject::isWidgetType() const

    Returns true if the object is a widget; otherwise returns false.

    Calling this function is equivalent to calling
    inherits("QWidget"), except that it is much faster.
*/


/*!
    This virtual function receives events to an object and should
    return true if the event \a e was recognized and processed.

    The event() function can be reimplemented to customize the
    behavior of an object.

    \sa installEventFilter(), timerEvent(), QApplication::sendEvent(),
    QApplication::postEvent(), QWidget::event()
*/

bool QObject::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Timer:
        timerEvent((QTimerEvent*)e);
        break;

#ifdef QT3_SUPPORT
    case QEvent::ChildInsertedRequest:
        d_func()->sendPendingChildInsertedEvents();
        break;
#endif

    case QEvent::ChildAdded:
    case QEvent::ChildPolished:
#ifdef QT3_SUPPORT
    case QEvent::ChildInserted:
#endif
    case QEvent::ChildRemoved:
        childEvent((QChildEvent*)e);
        break;

    case QEvent::DeferredDelete:
        delete this;
        break;

    case QEvent::MetaCall:
        {
            Q_D(QObject);
            QMetaCallEvent *mce = static_cast<QMetaCallEvent*>(e);
            QObject *previousSender = d->currentSender;
            int previousFrom = d->currentSenderSignalIdStart;
            int previousTo = d->currentSenderSignalIdEnd;
            d->currentSender = const_cast<QObject*>(mce->sender());
            d->currentSenderSignalIdStart = mce->signalIdStart();
            d->currentSenderSignalIdEnd = mce->signalIdEnd();
#if defined(QT_NO_EXCEPTIONS)
            mce->placeMetaCall(this);
#else
            try {
                mce->placeMetaCall(this);
            } catch (...) {
                QReadLocker locker(QObjectPrivate::readWriteLock());
                if (QObjectPrivate::isValidObject(this)) {
                    Q_D(QObject);
                    d->currentSender = previousSender;
                    d->currentSenderSignalIdStart = previousFrom;
                    d->currentSenderSignalIdEnd = previousTo;
                }
                throw;
            }
#endif
            QReadLocker locker(QObjectPrivate::readWriteLock());
            if (QObjectPrivate::isValidObject(this)) {
                Q_D(QObject);
                d->currentSender = previousSender;
                d->currentSenderSignalIdStart = previousFrom;
                d->currentSenderSignalIdEnd = previousTo;
            }
            break;
        }

    case QEvent::ThreadChange: {
        QThreadData *threadData = d_func()->threadData;
        QAbstractEventDispatcher *eventDispatcher = threadData->eventDispatcher;
        if (eventDispatcher) {
            QList<QPair<int, int> > timers = eventDispatcher->registeredTimers(this);
            if (!timers.isEmpty()) {
                eventDispatcher->unregisterTimers(this);
                QMetaObject::invokeMethod(this, "_q_reregisterTimers", Qt::QueuedConnection,
                                          Q_ARG(void*, (new QList<QPair<int, int> >(timers))));
            }
        }
        break;
    }

    default:
        if (e->type() >= QEvent::User) {
            customEvent(e);
            break;
        }
        return false;
    }
    return true;
}

/*!
    \fn void QObject::timerEvent(QTimerEvent *event)

    This event handler can be reimplemented in a subclass to receive
    timer events for the object.

    QTimer provides a higher-level interface to the timer
    functionality, and also more general information about timers. The
    timer event is passed in the \a event parameter.

    \sa startTimer(), killTimer(), event()
*/

void QObject::timerEvent(QTimerEvent *)
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    child events. The event is passed in the \a event parameter.

    QEvent::ChildAdded and QEvent::ChildRemoved events are sent to
    objects when children are added or removed. In both cases you can
    only rely on the child being a QObject, or if isWidgetType()
    returns true, a QWidget. (This is because, in the
    \l{QEvent::ChildAdded}{ChildAdded} case, the child is not yet
    fully constructed, and in the \l{QEvent::ChildRemoved}{ChildRemoved}
    case it might have been destructed already).

    QEvent::ChildPolished events are sent to widgets when children
    are polished, or when polished children are added. If you receive
    a child polished event, the child's construction is usually
    completed.

    For every child widget, you receive one
    \l{QEvent::ChildAdded}{ChildAdded} event, zero or more
    \l{QEvent::ChildPolished}{ChildPolished} events, and one
    \l{QEvent::ChildRemoved}{ChildRemoved} event.

    The \l{QEvent::ChildPolished}{ChildPolished} event is omitted if
    a child is removed immediately after it is added. If a child is
    polished several times during construction and destruction, you
    may receive several child polished events for the same child,
    each time with a different virtual table.

    \sa event()
*/

void QObject::childEvent(QChildEvent * /* event */)
{
}


/*!
    This event handler can be reimplemented in a subclass to receive
    custom events. Custom events are user-defined events with a type
    value at least as large as the QEvent::User item of the
    QEvent::Type enum, and is typically a QEvent subclass. The event
    is passed in the \a event parameter.

    \sa event(), QEvent
*/
void QObject::customEvent(QEvent * /* event */)
{
}



/*!
    Filters events if this object has been installed as an event
    filter for the \a watched object.

    In your reimplementation of this function, if you want to filter
    the \a event out, i.e. stop it being handled further, return
    true; otherwise return false.

    Example:
    \code
        class MainWindow : public QMainWindow
        {
        public:
            MainWindow();

        protected:
            bool eventFilter(QObject *obj, QEvent *ev);

        private:
            QTextEdit *textEdit;
        };

        MainWindow::MainWindow()
        {
            textEdit = new QTextEdit;
            setCentralWidget(textEdit);

            textEdit->installEventFilter(this);
        }

        bool MainWindow::eventFilter(QObject *obj, QEvent *event)
        {
            if (obj == textEdit) {
                if (event->type() == QEvent::KeyPress) {
                    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
                    qDebug() << "Ate key press" << keyEvent->key();
                    return true;
                } else {
                    return false;
                }
            } else {
                // pass the event on to the parent class
                return QMainWindow::eventFilter(obj, event);
            }
        }
    \endcode

    Notice in the example above that unhandled events are passed to
    the base class's eventFilter() function, since the base class
    might have reimplemented eventFilter() for its own internal
    purposes.

    \warning If you delete the receiver object in this function, be
    sure to return true. Otherwise, Qt will forward the event to the
    deleted object and the program might crash.

    \sa installEventFilter()
*/

bool QObject::eventFilter(QObject * /* watched */, QEvent * /* event */)
{
    return false;
}

/*!
    \fn bool QObject::signalsBlocked() const

    Returns true if signals are blocked; otherwise returns false.

    Signals are not blocked by default.

    \sa blockSignals()
*/

/*!
    If \a block is true, signals emitted by this object are blocked
    (i.e., emitted signals disappear into hyperspace). If \a block is
    false, no such blocking will occur.

    The return value is the previous value of signalsBlocked().

    Note that the destroyed() signal will be emitted even if the signals
    for this object have been blocked.

    \sa signalsBlocked()
*/

bool QObject::blockSignals(bool block)
{
    Q_D(QObject);
    bool previous = d->blockSig;
    d->blockSig = block;
    return previous;
}

/*!
    Returns the thread in which the object lives.

    \sa moveToThread()
*/
QThread *QObject::thread() const
{
    return d_func()->threadData->thread;
}

/*!
    Changes the thread affinity for this object and its children. The
    object cannot be moved if it has a parent. Event processing will
    continue in the \a targetThread.

    To move an object to the main thread, use QApplication::instance()
    to retrieve a pointer to the current application, and then use
    QApplication::thread() to retrieve the thread in which the
    application lives. For example:

    \code
        myObject->moveToThread(QApplication::instance()->thread());
    \endcode

    If \a targetThread is zero, all event processing for this object
    and its children stops.

    Note that all active timers for the object will be reset. The
    timers are first stopped in the current thread and restarted (with
    the same interval) in the \a targetThread. As a result, constantly
    moving an object between threads can postpone timer events
    indefinitely.

    A QEvent::ThreadChange event is sent to this object just before
    the thread affinity is changed. You can handle this event to
    perform any special processing. Note that any new events that are
    posted to this object will be handled in the \a targetThread.

    \warning This function is \e not thread-safe; the current thread
    must be same as the current thread affinity. In other words, this
    function can only "push" an object from the current thread to
    another thread, it cannot "pull" an object from any arbitrary
    thread to the current thread.

    \sa thread()
 */
void QObject::moveToThread(QThread *targetThread)
{
    Q_D(QObject);

    if (d->threadData->thread == targetThread) {
        // object is already in this thread
        return;
    }

    if (d->parent != 0) {
        qWarning("QObject::moveToThread: Cannot move objects with a parent");
        return;
    }
    if (d->isWidget) {
        qWarning("QObject::moveToThread: Widgets cannot be moved to a new thread");
        return;
    }

    QThreadData *currentData = QThreadData::current();
    QThreadData *targetData = targetThread ? QThreadData::get2(targetThread) : new QThreadData(0);
    if (d->threadData->thread == 0 && currentData == targetData) {
        // one exception to the rule: we allow moving objects with no thread affinity to the current thread
        currentData = d->threadData;
    } else if (d->threadData != currentData) {
        qWarning("QObject::moveToThread: Current thread (%p) is not the object's thread (%p).\n"
                 "Cannot move to target thread (%p)\n",
                 d->threadData->thread, currentData->thread, targetData->thread);
        return;
    }

    // prepare to move
    d->moveToThread_helper();

    QWriteLocker locker(QObjectPrivate::readWriteLock());
    if (currentData != targetData) {
        targetData->postEventList.mutex.lock();
        while (currentData && !currentData->postEventList.mutex.tryLock()) {
            targetData->postEventList.mutex.unlock();
            targetData->postEventList.mutex.lock();
        }
    }

    // keep currentData alive (since we've got it locked)
    currentData->ref();

    // move the object
    d_func()->setThreadData_helper(currentData, targetData);

    if (currentData != targetData) {
        targetData->postEventList.mutex.unlock();
        if (currentData)
            currentData->postEventList.mutex.unlock();
    }

    // now currentData can commit suicide if it wants to
    currentData->deref();
}

void QObjectPrivate::moveToThread_helper()
{
    Q_Q(QObject);
    QEvent e(QEvent::ThreadChange);
    QCoreApplication::sendEvent(q, &e);
    for (int i = 0; i < children.size(); ++i) {
        QObject *child = children.at(i);
        child->d_func()->moveToThread_helper();
    }
}

void QObjectPrivate::setThreadData_helper(QThreadData *currentData, QThreadData *targetData)
{
    Q_Q(QObject);

    // move posted events
    int eventsMoved = 0;
    for (int i = 0; i < currentData->postEventList.size(); ++i) {
        const QPostEvent &pe = currentData->postEventList.at(i);
        if (!pe.event)
            continue;
        if (pe.receiver == q) {
            // move this post event to the targetList
            targetData->postEventList.append(pe);
            const_cast<QPostEvent &>(pe).event = 0;
            ++eventsMoved;
        }
    }
    if (eventsMoved > 0 && targetData->eventDispatcher)
        targetData->eventDispatcher->wakeUp();

    // set new thread data
    targetData->ref();
    threadData->deref();
    threadData = targetData;

    for (int i = 0; i < children.size(); ++i) {
        QObject *child = children.at(i);
        child->d_func()->setThreadData_helper(currentData, targetData);
    }
}

void QObjectPrivate::_q_reregisterTimers(void *pointer)
{
    Q_Q(QObject);
    QList<QPair<int, int> > *timerList = reinterpret_cast<QList<QPair<int, int> > *>(pointer);
    QAbstractEventDispatcher *eventDispatcher = threadData->eventDispatcher;
    for (int i = 0; i < timerList->size(); ++i) {
        const QPair<int, int> &pair = timerList->at(i);
        eventDispatcher->registerTimer(pair.first, pair.second, q);
    }
    delete timerList;
}


//
// The timer flag hasTimer is set when startTimer is called.
// It is not reset when killing the timer because more than
// one timer might be active.
//

/*!
    Starts a timer and returns a timer identifier, or returns zero if
    it could not start a timer.

    A timer event will occur every \a interval milliseconds until
    killTimer() is called. If \a interval is 0, then the timer event
    occurs once every time there are no more window system events to
    process.

    The virtual timerEvent() function is called with the QTimerEvent
    event parameter class when a timer event occurs. Reimplement this
    function to get timer events.

    If multiple timers are running, the QTimerEvent::timerId() can be
    used to find out which timer was activated.

    Example:

    \code
        class MyObject : public QObject
        {
            Q_OBJECT

        public:
            MyObject(QObject *parent = 0);

        protected:
            void timerEvent(QTimerEvent *event);
        };

        MyObject::MyObject(QObject *parent)
            : QObject(parent)
        {
            startTimer(50);     // 50-millisecond timer
            startTimer(1000);   // 1-second timer
            startTimer(60000);  // 1-minute timer
        }

        void MyObject::timerEvent(QTimerEvent *event)
        {
            qDebug() << "Timer ID:" << event->timerId();
        }
    \endcode

    Note that QTimer's accuracy depends on the underlying operating
    system and hardware. Most platforms support an accuracy of 20
    milliseconds; some provide more. If Qt is unable to deliver the
    requested number of timer events, it will silently discard some.

    The QTimer class provides a high-level programming interface with
    single-shot timers and timer signals instead of events. There is
    also a QBasicTimer class that is more lightweight than QTimer and
    less clumsy than using timer IDs directly.

    \sa timerEvent(), killTimer(), QTimer::singleShot()
*/

int QObject::startTimer(int interval)
{
    Q_D(QObject);

    if (interval < 0) {
        qWarning("QObject::startTimer: QTimer cannot have a negative interval");
        return 0;
    }

    d->pendTimer = true;                                // set timer flag

    if (!d->threadData->eventDispatcher) {
        qWarning("QObject::startTimer: QTimer can only be used with threads started with QThread");
        return 0;
    }
    return d->threadData->eventDispatcher->registerTimer(interval, this);
}

/*!
    Kills the timer with timer identifier, \a id.

    The timer identifier is returned by startTimer() when a timer
    event is started.

    \sa timerEvent(), startTimer()
*/

void QObject::killTimer(int id)
{
    Q_D(QObject);
    if (d->threadData->eventDispatcher)
        d->threadData->eventDispatcher->unregisterTimer(id);
}


/*!
    \fn QObject *QObject::parent() const

    Returns a pointer to the parent object.

    \sa children()
*/

/*!
    \fn const QObjectList &QObject::children() const

    Returns a list of child objects.
    The QObjectList class is defined in the \c{<QObject>} header
    file as the following:

    \quotefromfile src/corelib/kernel/qobject.h
    \skipto /typedef .*QObjectList/
    \printuntil QObjectList

    The first child added is the \l{QList::first()}{first} object in
    the list and the last child added is the \l{QList::last()}{last}
    object in the list, i.e. new children are appended at the end.

    Note that the list order changes when QWidget children are
    \l{QWidget::raise()}{raised} or \l{QWidget::lower()}{lowered}. A
    widget that is raised becomes the last object in the list, and a
    widget that is lowered becomes the first object in the list.

    \sa findChild(), findChildren(), parent(), setParent()
*/

#ifdef QT3_SUPPORT
static void objSearch(QObjectList &result,
                       const QObjectList &list,
                       const char  *inheritsClass,
                       bool onlyWidgets,
                       const char  *objName,
                       QRegExp           *rx,
                       bool            recurse)
{
    for (int i = 0; i < list.size(); ++i) {
        QObject *obj = list.at(i);
        bool ok = true;
        if (onlyWidgets)
            ok = obj->isWidgetType();
        else if (inheritsClass && !obj->inherits(inheritsClass))
            ok = false;
        if (ok) {
            if (objName)
                ok = (obj->objectName() == QLatin1String(objName));
#ifndef QT_NO_REGEXP
            else if (rx)
                ok = (rx->indexIn(obj->objectName()) != -1);
#endif
        }
        if (ok)                                // match!
            result.append(obj);
        if (recurse) {
            QObjectList clist = obj->children();
            if (!clist.isEmpty())
                objSearch(result, clist, inheritsClass,
                           onlyWidgets, objName, rx, recurse);
        }
    }
}

/*!
    \internal

    Searches the children and optionally grandchildren of this object,
    and returns a list of those objects that are named or that match
    \a objName and inherit \a inheritsClass. If \a inheritsClass is 0
    (the default), all classes match. If \a objName is 0 (the
    default), all object names match.

    If \a regexpMatch is true (the default), \a objName is a regular
    expression that the objects's names must match. The syntax is that
    of a QRegExp. If \a regexpMatch is false, \a objName is a string
    and object names must match it exactly.

    Note that \a inheritsClass uses single inheritance from QObject,
    the way inherits() does. According to inherits(), QWidget
    inherits QObject but not QPaintDevice. This does not quite match
    reality, but is the best that can be done on the wide variety of
    compilers Qt supports.

    Finally, if \a recursiveSearch is true (the default), queryList()
    searches \e{n}th-generation as well as first-generation children.

    If all this seems a bit complex for your needs, the simpler
    child() function may be what you want.

    This somewhat contrived example disables all the buttons in this
    window:

    \code
        QList<QObject *> list = window()->queryList("QAbstractButton"));
        foreach (QObject *obj, list)
            static_cast<QAbstractButton *>(obj)->setEnabled(false);
    \endcode

    \warning Delete the list as soon you have finished using it. The
    list contains pointers that may become invalid at almost any time
    without notice (as soon as the user closes a window you may have
    dangling pointers, for example).

    \sa child() children(), parent(), inherits(), objectName(), QRegExp
*/

QObjectList QObject::queryList(const char *inheritsClass,
                                const char *objName,
                                bool regexpMatch,
                                bool recursiveSearch) const
{
    Q_D(const QObject);
    QObjectList list;
    bool onlyWidgets = (inheritsClass && qstrcmp(inheritsClass, "QWidget") == 0);
#ifndef QT_NO_REGEXP
    if (regexpMatch && objName) {                // regexp matching
        QRegExp rx(QString::fromLatin1(objName));
        objSearch(list, d->children, inheritsClass, onlyWidgets, 0, &rx, recursiveSearch);
    } else
#endif
    {
        objSearch(list, d->children, inheritsClass, onlyWidgets, objName, 0, recursiveSearch);
    }
    return list;
}
#endif

/*!
    \fn T *QObject::findChild(const QString &name) const

    Returns the child of this object that can be cast into type T and
    that is called \a name, or 0 if there is no such object.
    Omitting the \a name argument causes all object names to be matched.
    The search is performed recursively.

    If there is more than one child matching the search, the most
    direct ancestor is returned. If there are several direct
    ancestors, it is undefined which one will be returned. In that
    case, findChildren() should be used.

    This example returns a child \l{QPushButton} of \c{parentWidget}
    named \c{"button1"}:

    \code
        QPushButton *button = parentWidget->findChild<QPushButton *>("button1");
    \endcode

    This example returns a \l{QListWidget} child of \c{parentWidget}:

    \code
        QListWidget *list = parentWidget->findChild<QListWidget *>();
    \endcode

    \warning This function is not available with MSVC 6. Use
    qFindChild() instead if you need to support that version of the
    compiler.

    \sa findChildren(), qFindChild()
*/

/*!
    \fn QList<T> QObject::findChildren(const QString &name) const

    Returns all children of this object with the given \a name that can be
    cast to type T, or an empty list if there are no such objects.
    Omitting the \a name argument causes all object names to be matched.
    The search is performed recursively.

    The following example shows how to find a list of child \l{QWidget}s of
    the specified \c{parentWidget} named \c{widgetname}:

    \code
        QList<QWidget *> widgets = parentWidget.findChildren<QWidget *>("widgetname");
    \endcode

    This example returns all \c{QPushButton}s that are children of \c{parentWidget}:

    \code
        QList<QPushButton *> allPButtons = parentWidget.findChildren<QPushButton *>();
    \endcode

    \warning This function is not available with MSVC 6. Use
    qFindChildren() instead if you need to support that version of the
    compiler.

    \sa findChild(), qFindChildren()
*/

/*!
    \fn QList<T> QObject::findChildren(const QRegExp &regExp) const
    \overload

    Returns the children of this object that can be cast to type T
    and that have names matching the regular expression \a regExp,
    or an empty list if there are no such objects.
    The search is performed recursively.

    \warning This function is not available with MSVC 6. Use
    qFindChildren() instead if you need to support that version of the
    compiler.
*/

/*!
    \fn T qFindChild(const QObject *obj, const QString &name)
    \relates QObject

    This function is equivalent to
    \a{obj}->\l{QObject::findChild()}{findChild}<T>(\a name). It is
    provided as a work-around for MSVC 6, which doesn't support
    member template functions.

    \sa QObject::findChild()
*/

/*!
    \fn QList<T> qFindChildren(const QObject *obj, const QString &name)
    \relates QObject

    This function is equivalent to
    \a{obj}->\l{QObject::findChildren()}{findChildren}<T>(\a name). It is
    provided as a work-around for MSVC 6, which doesn't support
    member template functions.

    \sa QObject::findChildren()
*/

/*!
    \fn QList<T> qFindChildren(const QObject *obj, const QRegExp &regExp)
    \relates QObject
    \overload

    This function is equivalent to
    \a{obj}->\l{QObject::findChildren()}{findChildren}<T>(\a regExp). It is
    provided as a work-around for MSVC 6, which doesn't support
    member template functions.
*/

/*!
    \internal
    \fn T qFindChild(const QObject *obj, const QString &name = QString(), T dummy = 0)
    \relates QObject
    \overload

    This function is equivalent to
    \a{obj}->\l{QObject::findChild()}{findChild}<T>(\a name). It is
    provided as a work-around for MSVC 6, which doesn't support
    member template functions.

    \sa QObject::findChild()
*/

/*!
    \internal
    \fn QList<T> qFindChildren(const QObject *obj, const QString &name = QString(), T dummy = 0)
    \relates QObject
    \overload

    This function is equivalent to
    \a{obj}->\l{QObject::findChildren()}{findChildren}<T>(\a name). It is
    provided as a work-around for MSVC 6, which doesn't support
    member template functions.

    \sa QObject::findChildren()
*/

/*!
    \internal
*/
void qt_qFindChildren_helper(const QObject *parent, const QString &name, const QRegExp *re,
                         const QMetaObject &mo, QList<void*> *list)
{
    if (!parent || !list)
        return;
    const QObjectList &children = parent->children();
    QObject *obj;
    for (int i = 0; i < children.size(); ++i) {
        obj = children.at(i);
        if (mo.cast(obj)) {
            if (re) {
                if (re->indexIn(obj->objectName()) != -1)
                    list->append(obj);
            } else {
                if (name.isNull() || obj->objectName() == name)
                    list->append(obj);
            }
        }
        qt_qFindChildren_helper(obj, name, re, mo, list);
    }
}

/*! \internal
 */
QObject *qt_qFindChild_helper(const QObject *parent, const QString &name, const QMetaObject &mo)
{
    if (!parent)
        return 0;
    const QObjectList &children = parent->children();
    QObject *obj;
    int i;
    for (i = 0; i < children.size(); ++i) {
        obj = children.at(i);
        if (mo.cast(obj) && (name.isNull() || obj->objectName() == name))
            return obj;
    }
    for (i = 0; i < children.size(); ++i) {
        obj = qt_qFindChild_helper(children.at(i), name, mo);
        if (obj)
            return obj;
    }
    return 0;
}

/*!
    Makes the object a child of \a parent.

    \sa QWidget::setParent()
*/

void QObject::setParent(QObject *parent)
{
    Q_D(QObject);
    Q_ASSERT(!d->isWidget);
    d->setParent_helper(parent);
}

void QObjectPrivate::deleteChildren()
{
    const bool reallyWasDeleted = wasDeleted;
    wasDeleted = true;
    // delete children objects
    // don't use qDeleteAll as the destructor of the child might
    // delete siblings
    for (int i = 0; i < children.count(); ++i) {
        currentChildBeingDeleted = children.at(i);
        children[i] = 0;
        delete currentChildBeingDeleted;
    }
    children.clear();
    currentChildBeingDeleted = 0;
    wasDeleted = reallyWasDeleted;
}

void QObjectPrivate::setParent_helper(QObject *o)
{
    Q_Q(QObject);
    if (o == parent)
        return;
    if (parent) {
        QObjectPrivate *parentD = parent->d_func();
        if (parentD->wasDeleted && wasDeleted
            && parentD->currentChildBeingDeleted == q) {
            // don't do anything since QObjectPrivate::deleteChildren() already
            // cleared our entry in parentD->children.
        } else {
            const int index = parentD->children.indexOf(q);
            if (parentD->wasDeleted) {
                parentD->children[index] = 0;
            } else {
                parentD->children.removeAt(index);
                if (sendChildEvents && parentD->receiveChildEvents) {
                    QChildEvent e(QEvent::ChildRemoved, q);
                    QCoreApplication::sendEvent(parent, &e);
                }
            }
        }
    }
    parent = o;
    if (parent) {
        // object hierarchies are constrained to a single thread
        if (threadData != parent->d_func()->threadData) {
            qWarning("QObject::setParent: Cannot set parent, new parent is in a different thread");
            parent = 0;
            return;
        }
        parent->d_func()->children.append(q);
        if(sendChildEvents && parent->d_func()->receiveChildEvents) {
            if (!isWidget) {
                QChildEvent e(QEvent::ChildAdded, q);
                QCoreApplication::sendEvent(parent, &e);
#ifdef QT3_SUPPORT
                if (parent->d_func()->pendingChildInsertedEvents.isEmpty()) {
                    QCoreApplication::postEvent(parent,
                                                new QEvent(QEvent::ChildInsertedRequest),
                                                Qt::HighEventPriority);
                }
                parent->d_func()->pendingChildInsertedEvents.append(q);
#endif
            }
        }
    }
}

/*!
    \fn void QObject::installEventFilter(QObject *filterObj)
    \threadsafe

    Installs an event filter \a filterObj on this object. For example:
    \code
    monitoredObj->installEventFilter(filterObj);
    \endcode

    An event filter is an object that receives all events that are
    sent to this object. The filter can either stop the event or
    forward it to this object. The event filter \a filterObj receives
    events via its eventFilter() function. The eventFilter() function
    must return true if the event should be filtered, (i.e. stopped);
    otherwise it must return false.

    If multiple event filters are installed on a single object, the
    filter that was installed last is activated first.

    Here's a \c KeyPressEater class that eats the key presses of its
    monitored objects:

    \code
        class KeyPressEater : public QObject
        {
            Q_OBJECT
            ...

        protected:
            bool eventFilter(QObject *obj, QEvent *event);
        };

        bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
        {
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
                qDebug("Ate key press %d", keyEvent->key());
                return true;
            } else {
                // standard event processing
                return QObject::eventFilter(obj, event);
            }
        }
    \endcode

    And here's how to install it on two widgets:

    \code
        KeyPressEater *keyPressEater = new KeyPressEater(this);
        QPushButton *pushButton = new QPushButton(this);
        QListView *listView = new QListView(this);

        pushButton->installEventFilter(keyPressEater);
        listView->installEventFilter(keyPressEater);
    \endcode

    The QShortcut class, for example, uses this technique to intercept
    shortcut key presses.

    \warning If you delete the receiver object in your eventFilter()
    function, be sure to return true. If you return false, Qt sends
    the event to the deleted object and the program will crash.

    \sa removeEventFilter(), eventFilter(), event()
*/

void QObject::installEventFilter(QObject *obj)
{
    Q_D(QObject);
    if (!obj)
        return;

    QWriteLocker locker(QObjectPrivate::readWriteLock());

    // clean up unused items in the list
    d->eventFilters.removeAll((QObject*)0);
    d->eventFilters.removeAll(obj);
    d->eventFilters.prepend(obj);
}

/*!
    \threadsafe

    Removes an event filter object \a obj from this object. The
    request is ignored if such an event filter has not been installed.

    All event filters for this object are automatically removed when
    this object is destroyed.

    It is always safe to remove an event filter, even during event
    filter activation (i.e. from the eventFilter() function).

    \sa installEventFilter(), eventFilter(), event()
*/

void QObject::removeEventFilter(QObject *obj)
{
    Q_D(QObject);
    QWriteLocker locker(QObjectPrivate::readWriteLock());
    d->eventFilters.removeAll(obj);
}


/*!
    \fn QObject::destroyed(QObject *obj)

    This signal is emitted immediately before the object \a obj is
    destroyed, and can not be blocked.

    All the objects's children are destroyed immediately after this
    signal is emitted.

    \sa deleteLater(), QPointer
*/

/*!
    Schedules this object for deletion.

    The object will be deleted when control returns to the event loop.

    Note that entering and leaving a new event loop (e.g., by opening a modal
    dialog) will \e not perform the deferred deletion; for the object to be
    deleted, the control must return to the event loop from which
    deleteLater() was called.

    \bold{Note:} It is safe to call this function more than once; when the
    first deferred deletion event is delivered, any pending events for the
    object are removed from the event queue.

    \sa destroyed(), QPointer
*/
void QObject::deleteLater()
{
    QCoreApplication::postEvent(this, new QEvent(QEvent::DeferredDelete));
}

/*!
    \fn QString QObject::tr(const char *sourceText, const char *comment, int n)
    \reentrant

    Returns a translated version of \a sourceText, or \a sourceText
    itself if there is no appropriate translated version. The
    translation context is Object with \a comment (0 by default).
    All QObject subclasses using the Q_OBJECT macro automatically have
    a reimplementation of this function with the subclass name as
    context.

    You can set the encoding for \a sourceText by calling QTextCodec::setCodecForTr().
    By default \a sourceText is assumed to be in Latin-1 encoding.

    Example:

    \code
        MyWindow::MyWindow()
        {
            QLabel *nameLabel = new QLabel(tr("Name:"));
            QLabel *addressLabel = new QLabel(tr("Address:", "i.e. a postal address"));
            ...
        }
    \endcode

    If \a n >= 0, all occurrences of \c %n in the resulting string
    are replaced with a decimal representation of \a n. In addition,
    depending on \a n's value, the translation text may vary.

    Example:

    \code
        int n = messages.count();
        showMessage(tr("%n message(s) saved", "", n));
    \endcode

    The table below shows what string is returned depending on the
    active translation:

    \table
    \header \o      \o{3,1} Active Translation
    \header \o \a n \o No Translation        \o French                                 \o English
    \row    \o 0    \o "0 message(s) saved"  \o "0 message sauvegard\unicode{0xE9}"    \o "0 message\bold{s} saved"
    \row    \o 1    \o "1 message(s) saved"  \o "1 message sauvegard\unicode{0xE9}"    \o "1 message saved"
    \row    \o 2    \o "2 message(s) saved"  \o "2 message\bold{s} sauvegard\unicode{0xE9}\bold{s}"  \o "2 message\bold{s} saved"
    \row    \o 37   \o "37 message(s) saved" \o "37 message\bold{s} sauvegard\unicode{0xE9}\bold{s}" \o "37 message\bold{s} saved"
    \endtable

    This idiom is more flexible than the traditional approach, i.e.,

    \code
        n == 1 ? tr("%n message saved") : tr("%n messages saved")
    \endcode

    because it also works with target languages that have several
    plural forms (e.g., Irish has a special "dual" form that should
    be used when \c n is 2), and it handles the \e n == 0 case
    correctly for languages such as French that require the singular.
    See the \l{Qt Linguist Manual} for details.

    Instead of \c %n, you can use \c %Ln to produce a localized
    representation of \a n. The conversion uses the default local,
    set using QLocal::setDefault(). (If no default locale was
    specified, the "C" locale is used.)

    \warning This method is reentrant only if all translators are
    installed \e before calling this method. Installing or removing
    translators while performing translations is not supported. Doing
    so will probably result in crashes or other undesirable behavior.

    \sa trUtf8(), QApplication::translate(), QTextCodec::setCodecForTr(), {Internationalization with Qt}
*/

/*!
    \fn QString QObject::trUtf8(const char *sourceText, const char *comment, int n)
    \reentrant

    Returns a translated version of \a sourceText, or
    QString::fromUtf8(\a sourceText) if there is no appropriate
    version. It is otherwise identical to tr(\a sourceText, \a
    comment, \a n).

    \warning This method is reentrant only if all translators are
    installed \e before calling this method. Installing or removing
    translators while performing translations is not supported. Doing
    so will probably result in crashes or other undesirable behavior.

    \warning For portability reasons, we recommend that you use
    escape sequences for specifying non-ASCII characters in string
    literals to trUtf8(). For example:

    \code
        label->setText(tr("F\374r \310lise"));
    \endcode

    \sa tr(), QApplication::translate(), {Internationalization with Qt}
*/




/*****************************************************************************
  Signals and slots
 *****************************************************************************/

static bool check_signal_macro(const QObject *sender, const char *signal,
                                const char *func, const char *op)
{
    int sigcode = (int)(*signal) - '0';
    if (sigcode != QSIGNAL_CODE) {
        if (sigcode == QSLOT_CODE)
            qWarning("Object::%s: Attempt to %s non-signal %s::%s",
                     func, op, sender->metaObject()->className(), signal+1);
        else
            qWarning("Object::%s: Use the SIGNAL macro to %s %s::%s",
                     func, op, sender->metaObject()->className(), signal);
        return false;
    }
    return true;
}

static bool check_method_code(int code, const QObject *object,
                               const char *method, const char *func)
{
    if (code != QSLOT_CODE && code != QSIGNAL_CODE) {
        qWarning("Object::%s: Use the SLOT or SIGNAL macro to "
                 "%s %s::%s", func, func, object->metaObject()->className(), method);
        return false;
    }
    return true;
}

static void err_method_notfound(int code, const QObject *object,
                                 const char *method, const char *func)
{
    const char *type = 0;
    switch (code) {
        case QSLOT_CODE:   type = "slot";   break;
        case QSIGNAL_CODE: type = "signal"; break;
    }
    if (strchr(method,')') == 0)                // common typing mistake
        qWarning("Object::%s: Parentheses expected, %s %s::%s",
                 func, type, object->metaObject()->className(), method);
    else
        qWarning("Object::%s: No such %s %s::%s",
                 func, type, object->metaObject()->className(), method);
}


static void err_info_about_objects(const char * func,
                                    const QObject * sender,
                                    const QObject * receiver)
{
    QString a = sender ? sender->objectName() : QString();
    QString b = receiver ? receiver->objectName() : QString();
    if (!a.isEmpty())
        qWarning("Object::%s:  (sender name:   '%s')", func, a.toLocal8Bit().data());
    if (!b.isEmpty())
        qWarning("Object::%s:  (receiver name: '%s')", func, b.toLocal8Bit().data());
}

/*!
    Returns a pointer to the object that sent the signal, if called in
    a slot activated by a signal; otherwise it returns 0. The pointer
    is valid only during the execution of the slot that calls this
    function.

    The pointer returned by this function becomes invalid if the
    sender is destroyed, or if the slot is disconnected from the
    sender's signal.

    \warning This function violates the object-oriented principle of
    modularity. However, getting access to the sender might be useful
    when many signals are connected to a single slot.

    \sa QSignalMapper
*/

QObject *QObject::sender() const
{
    Q_D(const QObject);
    QConnectionList * const list = ::connectionList();
    if (!list)
        return 0;

    QReadLocker locker(&list->lock);
    QConnectionList::Hash::const_iterator it = list->sendersHash.constFind(d->currentSender);
    const QConnectionList::Hash::const_iterator end = list->sendersHash.constEnd();

    // Return 0 if d->currentSender isn't in the senders hash (it has been destroyed?)
    if (it == end)
        return 0;

    // Only return d->currentSender if it's actually connected to "this"
    for (; it != end && it.key() == d->currentSender; ++it) {
        const int at = it.value();
        const QConnection &c = list->connections.at(at);

        if (c.receiver == this)
            return d->currentSender;
    }

    return 0;
}

/*!
    Returns the number of receivers connected to the \a signal.

    Since both slots and signals can be used as receivers for signals,
    and the same connections can be made many times, the number of
    receivers is the same as the number of connections made from this
    signal.

    When calling this function, you can use the \c SIGNAL() macro to
    pass a specific signal:

    \code
        if (receivers(SIGNAL(valueChanged(QByteArray))) > 0) {
            QByteArray data;
            get_the_value(&data);       // expensive operation
            emit valueChanged(data);
        }
    \endcode

    As the code snippet above illustrates, you can use this function
    to avoid emitting a signal that nobody listens to.

    \warning This function violates the object-oriented principle of
    modularity. However, it might be useful when you need to perform
    expensive initialization only if something is connected to a
    signal.
*/

int QObject::receivers(const char *signal) const
{
    int receivers = 0;
    if (signal) {
        QByteArray signal_name = QMetaObject::normalizedSignature(signal);
        signal = signal_name;
#ifndef QT_NO_DEBUG
        if (!check_signal_macro(this, signal, "receivers", "bind"))
            return 0;
#endif
        signal++; // skip code
        const QMetaObject *smeta = this->metaObject();
        int signal_index = smeta->indexOfSignal(signal);
        if (signal_index < 0) {
#ifndef QT_NO_DEBUG
            err_method_notfound(QSIGNAL_CODE, this, signal, "receivers");
#endif
            return false;
        }
        QConnectionList *list = ::connectionList();
        QReadLocker locker(&list->lock);
        QHash<const QObject *, int>::const_iterator i = list->sendersHash.constFind(this);
        while (i != list->sendersHash.constEnd() && i.key() == this) {
            if (list->connections.at(i.value()).signal == signal_index)
                ++receivers;
            ++i;
        }
    }
    return receivers;
}

/*!
    \threadsafe

    Creates a connection of the given \a type from the \a signal in
    the \a sender object to the \a method in the \a receiver object.
    Returns true if the connection succeeds; otherwise returns false.

    You must use the \c SIGNAL() and \c SLOT() macros when specifying
    the \a signal and the \a method, for example:

    \code
        QLabel *label = new QLabel;
        QScrollBar *scrollBar = new QScrollBar;
        QObject::connect(scrollBar, SIGNAL(valueChanged(int)),
                         label,  SLOT(setNum(int)));
    \endcode

    This example ensures that the label always displays the current
    scroll bar value. Note that the signal and slots parameters must not
    contain any variable names, only the type. E.g. the following would
    not work and return false:

    \code
        // WRONG
        QObject::connect(scrollBar, SIGNAL(valueChanged(int value)),
                         label, SLOT(setNum(int value)));
    \endcode

    A signal can also be connected to another signal:

    \code
        class MyWidget : public QWidget
        {
            Q_OBJECT

        public:
            MyWidget();

        signals:
            void buttonClicked();

        private:
            QPushButton *myButton;
        };

        MyWidget::MyWidget()
        {
            myButton = new QPushButton(this);
            connect(myButton, SIGNAL(clicked()),
                    this, SIGNAL(buttonClicked()));
        }
    \endcode

    In this example, the \c MyWidget constructor relays a signal from
    a private member variable, and makes it available under a name
    that relates to \c MyWidget.

    A signal can be connected to many slots and signals. Many signals
    can be connected to one slot.

    If a signal is connected to several slots, the slots are activated
    in an arbitrary order when the signal is emitted.

    The function returns true if it successfully connects the signal
    to the slot. It will return false if it cannot create the
    connection, for example, if QObject is unable to verify the
    existence of either \a signal or \a method, or if their signatures
    aren't compatible.

    A signal is emitted for every connection you make, so if you
    duplicate a connection, two signals will be emitted. You can
    always break a connection using disconnect().

    The optional \a type parameter describes the type of connection
    to establish. In particular, it determines whether a particular
    signal is delivered to a slot immediately or queued for delivery
    at a later time. If the signal is queued, the parameters must be
    of types that are known to Qt's meta-object system, because Qt
    needs to copy the arguments to store them in an event behind the
    scenes. If you try to use a queued connection and get the error
    message

    \code
        QObject::connect: Cannot queue arguments of type 'MyType'
        (Make sure 'MyType' is registered using qRegisterMetaType().)
    \endcode

    call qRegisterMetaType() to register the data type before you
    establish the connection.

    \sa disconnect(), sender(), qRegisterMetaType()
*/

bool QObject::connect(const QObject *sender, const char *signal,
                      const QObject *receiver, const char *method,
                      Qt::ConnectionType type)
{
    {
        const void *cbdata[] = { sender, signal, receiver, method, &type };
        if (QInternal::activateCallbacks(QInternal::ConnectCallback, (void **) cbdata))
            return true;
    }

#ifndef QT_NO_DEBUG
    bool warnCompat = true;
#endif
    if (type == Qt::AutoCompatConnection) {
        type = Qt::AutoConnection;
#ifndef QT_NO_DEBUG
        warnCompat = false;
#endif
    }

    if (sender == 0 || receiver == 0 || signal == 0 || method == 0) {
        qWarning("QObject::connect: Cannot connect %s::%s to %s::%s",
                 sender ? sender->metaObject()->className() : "(null)",
                 (signal && *signal) ? signal+1 : "(null)",
                 receiver ? receiver->metaObject()->className() : "(null)",
                 (method && *method) ? method+1 : "(null)");
        return false;
    }
    QByteArray tmp_signal_name;

    if (!check_signal_macro(sender, signal, "connect", "bind"))
        return false;
    const QMetaObject *smeta = sender->metaObject();
    ++signal; //skip code
    int signal_index = smeta->indexOfSignal(signal);
    if (signal_index < 0) {
        // check for normalized signatures
        tmp_signal_name = QMetaObject::normalizedSignature(signal).prepend(*(signal - 1));
        signal = tmp_signal_name.constData() + 1;
        signal_index = smeta->indexOfSignal(signal);
        if (signal_index < 0) {
            err_method_notfound(QSIGNAL_CODE, sender, signal, "connect");
            err_info_about_objects("connect", sender, receiver);
            return false;
        }
    }

    QByteArray tmp_method_name;
    int membcode = method[0] - '0';

    if (!check_method_code(membcode, receiver, method, "connect"))
        return false;
    ++method; // skip code

    const QMetaObject *rmeta = receiver->metaObject();
    int method_index = -1;
    switch (membcode) {
    case QSLOT_CODE:
        method_index = rmeta->indexOfSlot(method);
        break;
    case QSIGNAL_CODE:
        method_index = rmeta->indexOfSignal(method);
        break;
    }
    if (method_index < 0) {
        // check for normalized methods
        tmp_method_name = QMetaObject::normalizedSignature(method);
        method = tmp_method_name.constData();
        switch (membcode) {
        case QSLOT_CODE:
            method_index = rmeta->indexOfSlot(method);
            break;
        case QSIGNAL_CODE:
            method_index = rmeta->indexOfSignal(method);
            break;
        }
    }

    if (method_index < 0) {
        err_method_notfound(membcode, receiver, method, "connect");
        err_info_about_objects("connect", sender, receiver);
        return false;
    }
    if (!QMetaObject::checkConnectArgs(signal, method)) {
        qWarning("QObject::connect: Incompatible sender/receiver arguments"
                 "\n\t%s::%s --> %s::%s",
                 sender->metaObject()->className(), signal,
                 receiver->metaObject()->className(), method);
        return false;
    }

    int *types = 0;
    if ((type == Qt::QueuedConnection || type == Qt::BlockingQueuedConnection)
            && !(types = ::queuedConnectionTypes(smeta->method(signal_index).parameterTypes())))
        return false;

#ifndef QT_NO_DEBUG
    {
        QMetaMethod smethod = smeta->method(signal_index);
        QMetaMethod rmethod = rmeta->method(method_index);
        if (warnCompat) {
            if(smethod.attributes() & QMetaMethod::Compatibility) {
                if (!(rmethod.attributes() & QMetaMethod::Compatibility))
                    qWarning("QObject::connect: Connecting from COMPAT signal (%s::%s)", smeta->className(), signal);
            } else if(rmethod.attributes() & QMetaMethod::Compatibility && membcode != QSIGNAL_CODE) {
                qWarning("QObject::connect: Connecting from %s::%s to COMPAT slot (%s::%s)",
                         smeta->className(), signal, rmeta->className(), method);
            }
        }
    }
#endif
    QMetaObject::connect(sender, signal_index, receiver, method_index, type, types);
    const_cast<QObject*>(sender)->connectNotify(signal - 1);
    return true;
}


/*!
    \fn bool QObject::connect(const QObject *sender, const char *signal, const char *method, Qt::ConnectionType type) const
    \overload
    \threadsafe

    Connects \a signal from the \a sender object to this object's \a
    method.

    Equivalent to connect(\a sender, \a signal, \c this, \a method, \a type).

    Every connection you make emits a signal, so duplicate connections emit
    two signals. You can break a connection using disconnect().

    \sa disconnect()
*/

/*!
    \threadsafe

    Disconnects \a signal in object \a sender from \a method in object
    \a receiver. Returns true if the connection is successfully broken;
    otherwise returns false.

    A signal-slot connection is removed when either of the objects
    involved are destroyed.

    disconnect() is typically used in three ways, as the following
    examples demonstrate.
    \list 1
    \i Disconnect everything connected to an object's signals:

       \code
       disconnect(myObject, 0, 0, 0);
       \endcode

       equivalent to the non-static overloaded function

       \code
       myObject->disconnect();
       \endcode

    \i Disconnect everything connected to a specific signal:

       \code
       disconnect(myObject, SIGNAL(mySignal()), 0, 0);
       \endcode

       equivalent to the non-static overloaded function

       \code
       myObject->disconnect(SIGNAL(mySignal()));
       \endcode

    \i Disconnect a specific receiver:

       \code
       disconnect(myObject, 0, myReceiver, 0);
       \endcode

       equivalent to the non-static overloaded function

       \code
       myObject->disconnect(myReceiver);
       \endcode

    \endlist

    0 may be used as a wildcard, meaning "any signal", "any receiving
    object", or "any slot in the receiving object", respectively.

    The \a sender may never be 0. (You cannot disconnect signals from
    more than one object in a single call.)

    If \a signal is 0, it disconnects \a receiver and \a method from
    any signal. If not, only the specified signal is disconnected.

    If \a receiver is 0, it disconnects anything connected to \a
    signal. If not, slots in objects other than \a receiver are not
    disconnected.

    If \a method is 0, it disconnects anything that is connected to \a
    receiver. If not, only slots named \a method will be disconnected,
    and all other slots are left alone. The \a method must be 0 if \a
    receiver is left out, so you cannot disconnect a
    specifically-named slot on all objects.

    \sa connect()
*/
bool QObject::disconnect(const QObject *sender, const char *signal,
                         const QObject *receiver, const char *method)
{
    if (sender == 0 || (receiver == 0 && method != 0)) {
        qWarning("Object::disconnect: Unexpected null parameter");
        return false;
    }

    {
        const void *cbdata[] = { sender, signal, receiver, method };
        if (QInternal::activateCallbacks(QInternal::DisconnectCallback, (void **) cbdata))
            return true;
    }

    QByteArray signal_name;
    bool signal_found = false;
    if (signal) {
        signal_name = QMetaObject::normalizedSignature(signal);
        signal = signal_name;
        if (!check_signal_macro(sender, signal, "disconnect", "unbind"))
            return false;
        signal++; // skip code
    }

    QByteArray method_name;
    int membcode = -1;
    bool method_found = false;
    if (method) {
        method_name = QMetaObject::normalizedSignature(method);
        method = method_name;
        membcode = method[0] - '0';
        if (!check_method_code(membcode, receiver, method, "disconnect"))
            return false;
        method++; // skip code
    }

    /* We now iterate through all the sender's and receiver's meta
     * objects in order to also disconnect possibly shadowed signals
     * and slots with the same signature.
    */
    bool res = false;
    const QMetaObject *smeta = sender->metaObject();
    do {
        int signal_index = -1;
        if (signal) {
            signal_index = smeta->indexOfSignal(signal);
            if (signal_index < smeta->methodOffset())
                continue;
            signal_found = true;
        }

        if (!method) {
            res |= QMetaObject::disconnect(sender, signal_index, receiver, -1);
        } else {
            const QMetaObject *rmeta = receiver->metaObject();
            do {
                int method_index = rmeta->indexOfMethod(method);
                if (method_index >= 0)
                    while (method_index < rmeta->methodOffset())
                            rmeta = rmeta->superClass();
                if (method_index < 0)
                    break;
                res |= QMetaObject::disconnect(sender, signal_index, receiver, method_index);
                method_found = true;
            } while ((rmeta = rmeta->superClass()));
        }
    } while (signal && (smeta = smeta->superClass()));

    if (signal && !signal_found) {
        err_method_notfound(QSIGNAL_CODE, sender, signal, "disconnect");
        err_info_about_objects("disconnect", sender, receiver);
    } else if (method && !method_found) {
        err_method_notfound(membcode, receiver, method, "disconnect");
        err_info_about_objects("disconnect", sender, receiver);
    }
    if (res)
        const_cast<QObject*>(sender)->disconnectNotify(signal ? (signal - 1) : 0);
    return res;
}


/*!
    \threadsafe

    \fn bool QObject::disconnect(const char *signal, const QObject *receiver, const char *method)
    \overload

    Disconnects \a signal from \a method of \a receiver.

    A signal-slot connection is removed when either of the objects
    involved are destroyed.
*/

/*!
    \fn bool QObject::disconnect(const QObject *receiver, const char *method)
    \overload

    Disconnects all signals in this object from \a receiver's \a
    method.

    A signal-slot connection is removed when either of the objects
    involved are destroyed.
*/


/*!
    \fn void QObject::connectNotify(const char *signal)

    This virtual function is called when something has been connected
    to \a signal in this object.

    If you want to compare \a signal with a specific signal, use
    QLatin1String and the \c SIGNAL() macro as follows:

    \code
        if (QLatin1String(signal) == SIGNAL(valueChanged(int))) {
            // signal is valueChanged(int)
        }
    \endcode

    If the signal contains multiple parameters or parameters that
    contain spaces, call QMetaObject::normalizedSignature() on
    the result of the \c SIGNAL() macro.

    \warning This function violates the object-oriented principle of
    modularity. However, it might be useful when you need to perform
    expensive initialization only if something is connected to a
    signal.

    \sa connect(), disconnectNotify()
*/

void QObject::connectNotify(const char *)
{
}

/*!
    \fn void QObject::disconnectNotify(const char *signal)

    This virtual function is called when something has been
    disconnected from \a signal in this object.

    See connectNotify() for an example of how to compare
    \a signal with a specific signal.

    \warning This function violates the object-oriented principle of
    modularity. However, it might be useful for optimizing access to
    expensive resources.

    \sa disconnect(), connectNotify()
*/

void QObject::disconnectNotify(const char *)
{
}

/*!\internal

  \a types is a 0-terminated vector of meta types for queued
  connections.

  if \a signal_index is -1, then we effectively connect *all* signals
  from the sender to the receiver's slot
*/
bool QMetaObject::connect(const QObject *sender, int signal_index,
                          const QObject *receiver, int method_index, int type, int *types)
{
    QConnectionList *list = ::connectionList();
    if (!list)
        return false;
    QWriteLocker locker(&list->lock);
    list->addConnection(const_cast<QObject *>(sender), signal_index,
                        const_cast<QObject *>(receiver), method_index, type, types);
    if (signal_index < 0)
        sender->d_func()->connectedSignals = ~0u;
    else if (signal_index < 32)
        sender->d_func()->connectedSignals |= (1 << signal_index);
    return true;
}


/*!\internal
 */
bool QMetaObject::disconnect(const QObject *sender, int signal_index,
                             const QObject *receiver, int method_index)
{
    if (!sender)
        return false;
    QConnectionList *list = ::connectionList();
    if (!list)
        return false;
    QWriteLocker locker(&list->lock);
    return list->removeConnection(const_cast<QObject*>(sender), signal_index,
                                  const_cast<QObject*>(receiver), method_index);
}

/*!
    \fn void QMetaObject::connectSlotsByName(QObject *object)

    Searches recursively for all child objects of the given \a object, and connects
    matching signals from them to slots of \a object that follow the following form:

    \code
    void on_<widget name>_<signal name>(<signal parameters>);
    \endcode

    Let's assume our object has a child object of type QPushButton with
    the \l{QObject::objectName}{object name} \c{button1}. The slot to catch the
    button's \c{clicked()} signal would be:

    \code
    void on_button1_clicked();
    \endcode

    \sa QObject::setObjectName()
 */
void QMetaObject::connectSlotsByName(QObject *o)
{
    if (!o)
        return;
    const QMetaObject *mo = o->metaObject();
    Q_ASSERT(mo);
    const QObjectList list = qFindChildren<QObject *>(o, QString());
    for (int i = 0; i < mo->methodCount(); ++i) {
        const char *slot = mo->method(i).signature();
        Q_ASSERT(slot);
        if (slot[0] != 'o' || slot[1] != 'n' || slot[2] != '_')
            continue;
        bool foundIt = false;
        for(int j = 0; j < list.count(); ++j) {
            const QObject *co = list.at(j);
            QByteArray objName = co->objectName().toAscii();
            int len = objName.length();
            if (!len || qstrncmp(slot + 3, objName.data(), len) || slot[len+3] != '_')
                continue;
            const QMetaObject *smo = co->metaObject();
            int sigIndex = smo->indexOfMethod(slot + len + 4);
            if (sigIndex < 0) { // search for compatible signals
                int slotlen = qstrlen(slot + len + 4) - 1;
                for (int k = 0; k < co->metaObject()->methodCount(); ++k) {
                    if (smo->method(k).methodType() != QMetaMethod::Signal)
                        continue;

                    if (!qstrncmp(smo->method(k).signature(), slot + len + 4, slotlen)) {
                        sigIndex = k;
                        break;
                    }
                }
            }
            if (sigIndex < 0)
                continue;
            if (QMetaObject::connect(co, sigIndex, o, i)) {
                foundIt = true;
                break;
            }
        }
        if (foundIt) {
            // we found our slot, now skip all overloads
            while (mo->method(i + 1).attributes() & QMetaMethod::Cloned)
                  ++i;
        } else if (!(mo->method(i).attributes() & QMetaMethod::Cloned)) {
            qWarning("QMetaObject::connectSlotsByName: No matching signal for %s", slot);
        }
    }
}

static void queued_activate(QObject *sender, const QConnection &c, void **argv, int idFrom, int idTo,
                            QSemaphore *semaphore = 0)
{
    if (!c.types || c.types != &DIRECT_CONNECTION_ONLY) {
        QMetaMethod m = sender->metaObject()->method(c.signal);
        QConnection &x = const_cast<QConnection &>(c);
        int *tmp = ::queuedConnectionTypes(m.parameterTypes());
        if (!tmp) // cannot queue arguments
            tmp = &DIRECT_CONNECTION_ONLY;
        if (!q_atomic_test_and_set_ptr(&x.types, 0, tmp)) {
            if (tmp != &DIRECT_CONNECTION_ONLY)
                qFree(tmp);
        }
    }
    if (c.types == &DIRECT_CONNECTION_ONLY) // cannot activate
        return;
    int nargs = 1; // include return type
    while (c.types[nargs-1]) { ++nargs; }
    int *types = (int *) qMalloc(nargs*sizeof(int));
    void **args = (void **) qMalloc(nargs*sizeof(void *));
    types[0] = 0; // return type
    args[0] = 0; // return value
    for (int n = 1; n < nargs; ++n)
        args[n] = QMetaType::construct((types[n] = c.types[n-1]), argv[n]);
    QCoreApplication::postEvent(c.receiver, new QMetaCallEvent(c.method,
                                                               sender,
                                                               idFrom,
                                                               idTo,
                                                               nargs,
                                                               types,
                                                               args,
                                                               semaphore));
}

static void blocking_activate(QObject *sender, const QConnection &c, void **argv, int idFrom, int idTo)
{
    if (QThread::currentThread() == c.receiver->thread()) {
        qWarning("Qt: Dead lock detected while activating a BlockingQueuedConnection: "
                 "Sender is %s(%p), receiver is %s(%p)",
                 sender->metaObject()->className(), sender,
                 c.receiver->metaObject()->className(), c.receiver);
    }

#ifdef QT_NO_THREAD
    ::queued_activate(sender, c, argv, idFrom, idTo);
#else
    QSemaphore semaphore;
    ::queued_activate(sender, c, argv, idFrom, idTo, &semaphore);
    semaphore.acquire();
#endif
}

/*!\internal
 */
void QMetaObject::activate(QObject *sender, int from_signal_index, int to_signal_index, void **argv)
{
    if (sender->d_func()->blockSig)
        return;

    QConnectionList * const list = ::connectionList();
    if (!list)
        return;

    void *empty_argv[] = { 0 };
    if (qt_signal_spy_callback_set.signal_begin_callback != 0) {
        qt_signal_spy_callback_set.signal_begin_callback(sender, from_signal_index,
                                                         argv ? argv : empty_argv);
    }

    QReadLocker locker(&list->lock);

    QConnectionList::Hash::const_iterator it = list->sendersHash.constFind(sender);
    const QConnectionList::Hash::const_iterator start = it;
    const QConnectionList::Hash::const_iterator end = list->sendersHash.constEnd();

    if (start == end) {
        locker.unlock();
        if (qt_signal_spy_callback_set.signal_end_callback != 0)
            qt_signal_spy_callback_set.signal_end_callback(sender, from_signal_index);
        return;
    }

    QThreadData *currentThreadData = QThreadData::current();

    // QVarLengthArray doesn't use the same growth strategy as the rest of the Tulip classes, so we need to
    // determine the exact number of connections
    int i = 0;
    for (it = start; it != end && it.key() == sender; ++it) {
        ++i;
    }
    QVarLengthArray<int> connections(i);
    for (i = 0, it = start; it != end && it.key() == sender; ++i, ++it) {
        connections.data()[i] = it.value();
        ++list->connections[it.value()].refCount;
    }

    for (i = 0; i < connections.size(); ++i) {
        const int at = connections.constData()[connections.size() - (i + 1)];
        QConnection *c = &list->connections[at];
        --c->refCount;
        if (!c->receiver || ((c->signal < from_signal_index || c->signal > to_signal_index) &&
                             c->signal != -1))
            continue;

        // determine if this connection should be sent immediately or
        // put into the event queue
        if ((c->type == Qt::AutoConnection
             && (currentThreadData != sender->d_func()->threadData
                 || c->receiver->d_func()->threadData != sender->d_func()->threadData))
            || (c->type == Qt::QueuedConnection)) {
            ::queued_activate(sender, *c, argv, from_signal_index, to_signal_index);
            continue;
        } else if (c->type == Qt::BlockingQueuedConnection) {
            locker.unlock();
            ::blocking_activate(sender, *c, argv, from_signal_index, to_signal_index);
            locker.relock();
            continue;
        }

        const int method = c->method;
        QObject * const previousSender = c->receiver->d_func()->currentSender;
        int previousFrom = c->receiver->d_func()->currentSenderSignalIdStart;
        int previousTo = c->receiver->d_func()->currentSenderSignalIdEnd;
        c->receiver->d_func()->currentSender = sender;
        c->receiver->d_func()->currentSenderSignalIdStart = from_signal_index;
        c->receiver->d_func()->currentSenderSignalIdEnd = to_signal_index;
        locker.unlock();

        if (qt_signal_spy_callback_set.slot_begin_callback != 0)
            qt_signal_spy_callback_set.slot_begin_callback(c->receiver, method, argv ? argv : empty_argv);

#if defined(QT_NO_EXCEPTIONS)
        c->receiver->qt_metacall(QMetaObject::InvokeMetaMethod, method, argv ? argv : empty_argv);
#else
        try {
            c->receiver->qt_metacall(QMetaObject::InvokeMetaMethod, method, argv ? argv : empty_argv);
        } catch (...) {
            if (c->receiver) {
                c->receiver->d_func()->currentSender = previousSender;
                c->receiver->d_func()->currentSenderSignalIdStart = previousFrom;
                c->receiver->d_func()->currentSenderSignalIdEnd = previousTo;
            }
            throw;
        }
#endif
        c = &list->connections[at];

        if (qt_signal_spy_callback_set.slot_end_callback != 0)
            qt_signal_spy_callback_set.slot_end_callback(c->receiver, method);

        locker.relock();
        if (c->receiver) {
            c->receiver->d_func()->currentSender = previousSender;
            c->receiver->d_func()->currentSenderSignalIdStart = previousFrom;
            c->receiver->d_func()->currentSenderSignalIdEnd = previousTo;
        }
    }

    locker.unlock();

    if (qt_signal_spy_callback_set.signal_end_callback != 0)
        qt_signal_spy_callback_set.signal_end_callback(sender, from_signal_index);
}


/*!\internal
 */
void QMetaObject::activate(QObject *sender, int signal_index, void **argv)
{
    if (signal_index < 32
        && !qt_signal_spy_callback_set.signal_begin_callback
        && !qt_signal_spy_callback_set.signal_end_callback) {
        uint signal_mask = 1 << signal_index;
        if ((sender->d_func()->connectedSignals & signal_mask) == 0)
            // nothing connected to these signals, and no spy
            return;
    }
    activate(sender, signal_index, signal_index, argv);
}

/*!\internal
 */
void QMetaObject::activate(QObject *sender, const QMetaObject *m, int local_signal_index,
                           void **argv)
{
    int signal_index = m->methodOffset() + local_signal_index;
    if (signal_index < 32
        && !qt_signal_spy_callback_set.signal_begin_callback
        && !qt_signal_spy_callback_set.signal_end_callback) {
        uint signal_mask = 1 << signal_index;
        if ((sender->d_func()->connectedSignals & signal_mask) == 0)
            // nothing connected to these signals, and no spy
            return;
    }
    activate(sender, signal_index, signal_index, argv);
}

/*!\internal
 */
void QMetaObject::activate(QObject *sender, const QMetaObject *m,
                           int from_local_signal_index, int to_local_signal_index, void **argv)
{
    int offset = m->methodOffset();
    int from_signal_index = offset + from_local_signal_index;
    int to_signal_index = offset + to_local_signal_index;
    if (to_signal_index < 32
        && !qt_signal_spy_callback_set.signal_begin_callback
        && !qt_signal_spy_callback_set.signal_end_callback) {
        uint signal_mask = (1 << (to_signal_index + 1)) - 1;
        signal_mask ^= (1 << from_signal_index) - 1;
        if ((sender->d_func()->connectedSignals & signal_mask) == 0)
            // nothing connected to these signals, and no spy
            return;
    }
    activate(sender, from_signal_index, to_signal_index, argv);
}


/*****************************************************************************
  Properties
 *****************************************************************************/

#ifndef QT_NO_PROPERTIES

/*!
  Sets the value of the object's \a name property to \a value.

  If the property is defined in the class using Q_PROPERTY then
  true is returned on success and false otherwise. If the property
  is not defined using Q_PROPERTY, and therefore not listed in the
  meta-object, it is added as a dynamic property and false is returned.

  Information about all available properties is provided through the
  metaObject() and dynamicPropertyNames().

  Dynamic properties can be queried again using property() and can be
  removed by setting the property value to an invalid QVariant.
  Changing the value of a dynamic property causes a QDynamicPropertyChangeEvent
  to be sent to the object.

  \bold{Note:} Dynamic properties starting with "_q_" are reserved for internal
  purposes.

  \sa property(), metaObject(), dynamicPropertyNames()
*/
bool QObject::setProperty(const char *name, const QVariant &value)
{
    Q_D(QObject);
    const QMetaObject* meta = metaObject();
    if (!name || !meta)
        return false;

    int id = meta->indexOfProperty(name);
    if (id < 0) {
        if (!d->extraData)
            d->extraData = new QObjectPrivate::ExtraData;

        const int idx = d->extraData->propertyNames.indexOf(name);

        if (!value.isValid()) {
            if (idx == -1)
                return false;
            d->extraData->propertyNames.removeAt(idx);
            d->extraData->propertyValues.removeAt(idx);
        } else {
            if (idx == -1) {
                d->extraData->propertyNames.append(name);
                d->extraData->propertyValues.append(value);
            } else {
                d->extraData->propertyValues[idx] = value;
            }
        }

        QDynamicPropertyChangeEvent ev(name);
        QCoreApplication::sendEvent(this, &ev);

        return false;
    }
    QMetaProperty p = meta->property(id);
#ifndef QT_NO_DEBUG
    if (!p.isWritable())
        qWarning("%s::setProperty: Property \"%s\" invalid,"
                 " read-only or does not exist", metaObject()->className(), name);
#endif
    return p.write(this, value);
}

/*!
  Returns the value of the object's \a name property.

  If no such property exists, the returned variant is invalid.

  Information about all available properties is provided through the
  metaObject() and dynamicPropertyNames().

  \sa setProperty(), QVariant::isValid(), metaObject(), dynamicPropertyNames()
*/
QVariant QObject::property(const char *name) const
{
    Q_D(const QObject);
    const QMetaObject* meta = metaObject();
    if (!name || !meta)
        return QVariant();

    int id = meta->indexOfProperty(name);
    if (id < 0) {
        if (!d->extraData)
            return QVariant();
        const int i = d->extraData->propertyNames.indexOf(name);
        return d->extraData->propertyValues.value(i);
    }
    QMetaProperty p = meta->property(id);
#ifndef QT_NO_DEBUG
    if (!p.isReadable())
        qWarning("%s::property: Property \"%s\" invalid or does not exist",
                 metaObject()->className(), name);
#endif
    return p.read(this);
}

/*!
    \since 4.2

    Returns the names of all properties that were dynamically added to
    the object using setProperty().
*/
QList<QByteArray> QObject::dynamicPropertyNames() const
{
    Q_D(const QObject);
    if (d->extraData)
        return d->extraData->propertyNames;
    return QList<QByteArray>();
}

#endif // QT_NO_PROPERTIES


/*****************************************************************************
  QObject debugging output routines.
 *****************************************************************************/

static void dumpRecursive(int level, QObject *object)
{
#if defined(QT_DEBUG)
    if (object) {
        QByteArray buf;
        buf.fill('\t', level/2);
        if (level % 2)
            buf += "    ";
        QString name = object->objectName();
        QString flags = QLatin1String("");
#if 0
        if (qApp->focusWidget() == object)
            flags += 'F';
        if (object->isWidgetType()) {
            QWidget * w = (QWidget *)object;
            if (w->isVisible()) {
                QString t("<%1,%2,%3,%4>");
                flags += t.arg(w->x()).arg(w->y()).arg(w->width()).arg(w->height());
            } else {
                flags += 'I';
            }
        }
#endif
        qDebug("%s%s::%s %s", (const char*)buf, object->metaObject()->className(), name.toLocal8Bit().data(),
               flags.toLatin1().data());
        QObjectList children = object->children();
        if (!children.isEmpty()) {
            for (int i = 0; i < children.size(); ++i)
                dumpRecursive(level+1, children.at(i));
        }
    }
#else
    Q_UNUSED(level)
        Q_UNUSED(object)
#endif
}

/*!
    Dumps a tree of children to the debug output.

    This function is useful for debugging, but does nothing if the
    library has been compiled in release mode (i.e. without debugging
    information).

    \sa dumpObjectInfo()
*/

void QObject::dumpObjectTree()
{
    dumpRecursive(0, this);
}

/*!
    Dumps information about signal connections, etc. for this object
    to the debug output.

    This function is useful for debugging, but does nothing if the
    library has been compiled in release mode (i.e. without debugging
    information).

    \sa dumpObjectTree()
*/

void QObject::dumpObjectInfo()
{
#if defined(QT_DEBUG)
    qDebug("OBJECT %s::%s", metaObject()->className(),
           objectName().isEmpty() ? "unnamed" : objectName().toLocal8Bit().data());

    QConnectionList *list = ::connectionList();
    QReadLocker locker(&list->lock);


    // first, look for connections where this object is the sender
    qDebug("  SIGNALS OUT");

    QConnectionList::Hash::const_iterator it = list->sendersHash.constFind(this);
    if (it != list->sendersHash.constEnd()) {
        do {
            const QConnection &c = list->connections.at(it.value());

            // signal name
            const QMetaMethod signal = metaObject()->method(c.signal);
            qDebug("\tsignal: %s", signal.signature());

            // receiver
            const QMetaObject *receiverMetaObject = c.receiver->metaObject();
            const QMetaMethod method = receiverMetaObject->method(c.method);
            qDebug("\t  --> %s::%s %s",
                   receiverMetaObject->className(),
                   c.receiver->objectName().isEmpty() ? "unnamed" : qPrintable(c.receiver->objectName()),
                   method.signature());

            ++it;
        } while (it != list->sendersHash.constEnd() && it.key() == this);
    } else {
	qDebug( "\t<None>" );
    }

    // now look for connections where this object is the receiver
    qDebug("  SIGNALS IN");

    it = list->receiversHash.constFind(this);
    if (it != list->receiversHash.constEnd()) {
        do {
            const QConnection &c = list->connections.at(it.value());

            // method name
            const QMetaMethod method = metaObject()->method(c.method);
            qDebug("\tmethod: %s", method.signature());

            // sender
            const QMetaObject *senderMetaObject = c.sender->metaObject();
            const QMetaMethod signal = senderMetaObject->method(c.signal);
            qDebug("\t  <-- %s::%s %s",
                   senderMetaObject->className(),
                   c.sender->objectName().isEmpty() ? "unnamed" : qPrintable(c.sender->objectName()),
                   signal.signature());

            ++it;
        } while (it != list->sendersHash.constEnd() && it.key() == this);

    } else {
	qDebug("\t<None>");
    }
#endif
}

#ifndef QT_NO_USERDATA
/*!\internal
 */
uint QObject::registerUserData()
{
    static int user_data_registration = 0;
    return user_data_registration++;
}

/*!\internal
 */
QObjectUserData::~QObjectUserData()
{
}

/*!\internal
 */
void QObject::setUserData(uint id, QObjectUserData* data)
{
    Q_D(QObject);
    if (!d->extraData)
        d->extraData = new QObjectPrivate::ExtraData;

    if (d->extraData->userData.size() <= (int) id)
        d->extraData->userData.resize((int) id + 1);
    d->extraData->userData[id] = data;
}

/*!\internal
 */
QObjectUserData* QObject::userData(uint id) const
{
    Q_D(const QObject);
    if (!d->extraData)
        return 0;
    if ((int)id < d->extraData->userData.size())
        return d->extraData->userData.at(id);
    return 0;
}

#endif // QT_NO_USERDATA


#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QObject *o) {
#ifndef Q_BROKEN_DEBUG_STREAM
    if (!o)
        return dbg << "QObject(0x0) ";
    dbg.nospace() << o->metaObject()->className() << "(" << (void *)o;
    if (!o->objectName().isEmpty())
        dbg << ", name = " << o->objectName();
    dbg << ')';
    return dbg.space();
#else
    qWarning("This compiler doesn't support streaming QObject to QDebug");
    return dbg;
    Q_UNUSED(o);
#endif
}
#endif

/*!
  \fn void QObject::insertChild(QObject *object)

  Use setParent() instead, i.e., call object->setParent(this).
*/

/*!
  \fn void QObject::removeChild(QObject *object)

  Use setParent() instead, i.e., call object->setParent(0).
*/

/*!
  \fn bool QObject::isA(const char *className) const

  Compare \a className with the object's metaObject()->className() instead.
*/

/*!
  \fn const char *QObject::className() const

  Use metaObject()->className() instead.
*/

/*!
  \fn const char *QObject::name() const

  Use objectName() instead.
*/

/*!
  \fn const char *QObject::name(const char *defaultName) const

  Use objectName() instead.
*/

/*!
  \fn void QObject::setName(const char *name)

  Use setObjectName() instead.
*/

/*!
  \fn bool QObject::checkConnectArgs(const char *signal, const
  QObject *object, const char *method)

  Use QMetaObject::checkConnectArgs() instead.
*/

/*!
  \fn QByteArray QObject::normalizeSignalSlot(const char *signalSlot)

  Use QMetaObject::normalizedSignature() instead.
*/

/*!
  \fn const char *QMetaObject::superClassName() const

  \internal
*/

/*!
    \macro Q_CLASSINFO(Name, Value)
    \relates QObject

    This macro associates extra information to the class, which is
    available using QObject::metaObject(). Except for the ActiveQt
    extension, Qt doesn't use this information.

    The extra information takes the form of a \a Name string and a \a
    Value literal string.

    Example:

    \code
        class MyClass : public QObject
        {
            Q_OBJECT
            Q_CLASSINFO("Author", "Pierre Gendron")
            Q_CLASSINFO("URL", "http://www.my-organization.qc.ca")

        public:
            ...
        };
    \endcode

    \sa QMetaObject::classInfo()
*/

/*!
    \macro Q_INTERFACES(...)
    \relates QObject

    This macro tells Qt which interfaces the class implements. This
    is used when implementing plugins.

    Example:

    \quotefromfile tools/plugandpaintplugins/basictools/basictoolsplugin.h
    \skipto class BasicToolsPlugin
    \printuntil public:
    \dots
    \skipto };
    \printline };

    See the \l{tools/plugandpaintplugins/basictools}{Plug & Paint
    Basic Tools} example for details.

    \sa Q_DECLARE_INTERFACE(), Q_EXPORT_PLUGIN2(), {How to Create Qt Plugins}
*/

/*!
    \macro Q_PROPERTY(...)
    \relates QObject

    This macro declares a QObject property. The syntax is:

    \code
        Q_PROPERTY(type name
                   READ getFunction
                   [WRITE setFunction]
                   [RESET resetFunction]
                   [DESIGNABLE bool]
                   [SCRIPTABLE bool]
                   [STORED bool])
    \endcode

    For example:

    \code
        Q_PROPERTY(QString title READ title WRITE setTitle)
    \endcode

    \sa {Qt's Property System}
*/

/*!
    \macro Q_ENUMS(...)
    \relates QObject

    This macro registers one or several enum types to the meta-object
    system.

    For example:

    \code
    class MyClass : public QObject
    {
        Q_OBJECT
        Q_ENUMS(Priority)

    public:
        MyClass(QObject *parent = 0);
        ~MyClass();

        enum Priority { High, Low, VeryHigh, VeryLow };
        void setPriority(Priority priority);
        Priority priority() const;
    };
    \endcode

    If you want to register an enum that is declared in another class,
    the enum must be fully qualified with the name of the class
    defining it. In addition, the class \e defining the enum has to
    inherit QObject as well as declare the enum using Q_ENUMS().

    \sa {Qt's Property System}
*/

/*!
    \macro Q_FLAGS(...)
    \relates QObject

    This macro registers one or several "flags" types to the
    meta-object system.

    Example:

    \code
        Q_FLAGS(Options Alignment)
    \endcode

    \sa {Qt's Property System}
*/

/*!
    \macro Q_OBJECT
    \relates QObject

    The Q_OBJECT macro must appear in the private section of a class
    definition that declares its own signals and slots or that uses
    other services provided by Qt's meta-object system.

    For example:

    \quotefromfile snippets/signalsandslots/signalsandslots.h
    \skipto ObjectCounter
    \skipto include <QObject>
    \printline include
    \printline class Counter
    \printuntil };

    \sa {Meta-Object System}, {Signals and Slots}, {Qt's Property System}
*/

/*!
    \typedef QObjectList
    \relates QObject

    Synonym for QList<QObject *>.
*/

#include "moc_qobject.cpp"
