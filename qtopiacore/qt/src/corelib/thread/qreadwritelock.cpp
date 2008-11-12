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

#include "qplatformdefs.h"
#include "qreadwritelock.h"

#ifndef QT_NO_THREAD
#include "qmutex.h"
#include "qthread.h"
#include "qwaitcondition.h"

struct QReadWriteLockPrivate
{
    QReadWriteLockPrivate()
        : accessCount(0), currentWriter(0), waitingReaders(0), waitingWriters(0)
    { }

    QMutex mutex;
    QWaitCondition readerWait;
    QWaitCondition writerWait;

    int accessCount;
    Qt::HANDLE currentWriter;
    int waitingReaders;
    int waitingWriters;
};

/*! \class QReadWriteLock
    \brief The QReadWriteLock class provides read-write locking.

    \threadsafe

    \ingroup thread
    \ingroup environment

    A read-write lock is a synchronization tool for protecting
    resources that can be accessed for reading and writing. This type
    of lock is useful if you want to allow multiple threads to have
    simultaneous read-only access, but as soon as one thread wants to
    write to the resource, all other threads must be blocked until
    the writing is complete.

    In many cases, QReadWriteLock is a direct competitor to QMutex.
    QReadWriteLock is a good choice if there are many concurrent
    reads and writing occurs infrequently.

    Example:

    \code
        QReadWriteLock lock;

        void ReaderThread::run()
        {
            ...
            lock.lockForRead();
            read_file();
            lock.unlock();
            ...
        }

        void WriterThread::run()
        {
            ...
            lock.lockForWrite();
            write_file();
            lock.unlock();
            ...
        }
    \endcode

    To ensure that writers aren't blocked forever by readers, readers
    attempting to obtain a lock will not succeed if there is a blocked
    writer waiting for access, even if the lock is currently only
    accessed by other readers. Also, if the lock is accessed by a
    writer and another writer comes in, that writer will have
    priority over any readers that might also be waiting.

    Like QMutex, a QReadWriteLock can be recursively locked by the
    same thread. In such cases, unlock() must be called the same
    number of times lockForWrite() or lockForRead() was called. Note
    that the lock type cannot be changed when trying to lock
    recursively, i.e. it is not possible to lock for reading in a
    thread that already has locked for writing (and vice versa).

    \sa QReadLocker, QWriteLocker, QMutex, QSemaphore
*/

/*!
    Constructs a QReadWriteLock object.

    \sa lockForRead(), lockForWrite()
*/
QReadWriteLock::QReadWriteLock()
    :d(new QReadWriteLockPrivate())
{
}

/*!
    Destroys the QReadWriteLock object.

    \warning Destroying a read-write lock that is in use may result
    in undefined behavior.
*/
QReadWriteLock::~QReadWriteLock()
{
    delete d;
}

/*!
    Locks the lock for reading. This function will block the current
    thread if any thread (including the current) has locked for
    writing.

    \sa unlock() lockForWrite() tryLockForRead()
*/
void QReadWriteLock::lockForRead()
{
    QMutexLocker lock(&d->mutex);

    while (d->accessCount < 0 || d->waitingWriters) {
        ++d->waitingReaders;
        d->readerWait.wait(&d->mutex);
        --d->waitingReaders;
     }

    ++d->accessCount;
    Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::lockForRead()", "Overflow in lock counter");
}

/*!
    Attempts to lock for reading. If the lock was obtained, this
    function returns true, otherwise it returns false instead of
    waiting for the lock to become available, i.e. it does not block.

    The lock attempt will fail if another thread has locked for
    writing.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForRead()
*/
bool QReadWriteLock::tryLockForRead()
{
   QMutexLocker lock(&d->mutex);

    if (d->accessCount < 0)
        return false;

    ++d->accessCount;
    Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::lockForRead()", "Overflow in lock counter");

    return true;
}

/*! \overload

    Attempts to lock for reading. This function returns true if the
    lock was obtained; otherwise it returns false. If another thread
    has locked for writing, this function will wait for at most \a
    timeout milliseconds for the lock to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lockForRead(), i.e. this function will wait forever until
    lock can be locked for reading when \a timeout is negative.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForRead()
*/
bool QReadWriteLock::tryLockForRead(int timeout)
{
    QMutexLocker lock(&d->mutex);

    while (d->accessCount < 0 || d->waitingWriters) {
        ++d->waitingReaders;
        bool success = d->readerWait.wait(&d->mutex, timeout < 0 ? ULONG_MAX : timeout);
        --d->waitingReaders;
        if (!success)
            return false;
     }

    ++d->accessCount;
    Q_ASSERT_X(d->accessCount > 0, "QReadWriteLock::tryLockForRead()", "Overflow in lock counter");

    return true;
}

 /*!
    Locks the lock for writing. This function will block the current
    thread if another thread has locked for reading or writing.

    \sa unlock() lockForRead() tryLockForWrite()
 */
void QReadWriteLock::lockForWrite()
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = QThread::currentThreadId();

    while (d->accessCount != 0) {
        if (d->accessCount < 0 && self == d->currentWriter) {
            // recursive write lock
            break;
        }

        ++d->waitingWriters;
        d->writerWait.wait(&d->mutex);
        --d->waitingWriters;
    }

    d->currentWriter = self;
    --d->accessCount;
    Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()", "Overflow in lock counter");

}

/*!
    Attempts to lock for writing. If the lock was obtained, this
    function returns true; otherwise, it returns false immediately.

    The lock attempt will fail if another thread has locked for
    reading or writing.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForWrite()
*/
bool QReadWriteLock::tryLockForWrite()
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = QThread::currentThreadId();

    if (d->accessCount != 0) {
        if (d->accessCount > 0 || self != d->currentWriter) {
            // either locked for reading, or another thread has locked for writing
            return false;
        }
    }

    d->currentWriter = self;
    --d->accessCount;
    Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()", "Overflow in lock counter");

    return true;
}

/*! \overload

    Attempts to lock for writing. This function returns true if the
    lock was obtained; otherwise it returns false. If another thread
    has locked for reading or writing, this function will wait for at
    most \a timeout milliseconds for the lock to become available.

    Note: Passing a negative number as the \a timeout is equivalent to
    calling lockForWrite(), i.e. this function will wait forever until
    lock can be locked for writing when \a timeout is negative.

    If the lock was obtained, the lock must be unlocked with unlock()
    before another thread can successfully lock it.

    \sa unlock() lockForWrite()
*/
bool QReadWriteLock::tryLockForWrite(int timeout)
{
    QMutexLocker lock(&d->mutex);

    Qt::HANDLE self = QThread::currentThreadId();

    while (d->accessCount != 0) {
        if (d->accessCount < 0 && self == d->currentWriter) {
            // recursive write lock
            break;
        }

        ++d->waitingWriters;
        bool success = d->writerWait.wait(&d->mutex, timeout < 0 ? ULONG_MAX : timeout);
        --d->waitingWriters;

        if (!success)
            return false;
    }

    d->currentWriter = self;
    --d->accessCount;
    Q_ASSERT_X(d->accessCount < 0, "QReadWriteLock::lockForWrite()", "Overflow in lock counter");

    return true;
}

/*!
    Unlocks the lock.

    Attempting to unlock a lock that is not locked is an error, and will result
    in program termination.

    \sa lockForRead() lockForWrite() tryLockForRead() tryLockForWrite()
*/
void QReadWriteLock::unlock()
{
    QMutexLocker lock(&d->mutex);

    Q_ASSERT_X(d->accessCount != 0, "QReadWriteLock::unlock()", "Cannot unlock an unlocked lock");

    if ((d->accessCount > 0 && --d->accessCount == 0) || (d->accessCount < 0 && ++d->accessCount == 0)) {
        d->currentWriter = 0;
        if (d->waitingWriters) {
            d->writerWait.wakeOne();
        } else if (d->waitingReaders) {
            d->readerWait.wakeAll();
        }
   }
}

/*!
    \class QReadLocker
    \brief The QReadLocker class is a convenience class that
    simplifies locking and unlocking read-write locks for read access.

    \threadsafe

    \ingroup thread
    \ingroup environment

    The purpose of QReadLocker (and QWriteLocker) is to simplify
    QReadWriteLock locking and unlocking. Locking and unlocking
    statements or in exception handling code is error-prone and
    difficult to debug. QReadLocker can be used in such situations
    to ensure that the state of the lock is always well-defined.

    Here's an example that uses QReadLocker to lock and unlock a
    read-write lock for reading:

    \code
        QReadWriteLock lock;

        QByteArray readData()
        {
            QReadLocker locker(&lock);
            ...
            return data;
        }
    \endcode

    It is equivalent to the following code:

    \code
        QReadWriteLock lock;

        QByteArray readData()
        {
            locker.lockForRead();
            ...
            locker.unlock();
            return data;
        }
    \endcode

    The QMutexLocker documentation shows examples where the use of a
    locker object greatly simplifies programming.

    \sa QWriteLocker, QReadWriteLock
*/

/*!
    \fn QReadLocker::QReadLocker(QReadWriteLock *lock)

    Constructs a QReadLocker and locks \a lock for reading. The lock
    will be unlocked when the QReadLocker is destroyed. If \c lock is
    zero, QReadLocker does nothing.

    \sa QReadWriteLock::lockForRead()
*/

/*!
    \fn QReadLocker::~QReadLocker()

    Destroys the QReadLocker and unlocks the lock that was passed to
    the constructor.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QReadLocker::unlock()

    Unlocks the lock associated with this locker.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QReadLocker::relock()

    Relocks an unlocked lock.

    \sa unlock()
*/

/*!
    \fn QReadWriteLock *QReadLocker::readWriteLock() const

    Returns a pointer to the read-write lock that was passed
    to the constructor.
*/

/*!
    \class QWriteLocker
    \brief The QWriteLocker class is a convenience class that
    simplifies locking and unlocking read-write locks for write access.

    \threadsafe

    \ingroup thread
    \ingroup environment

    The purpose of QWriteLocker (and QReadLocker is to simplify
    QReadWriteLock locking and unlocking. Locking and unlocking
    statements or in exception handling code is error-prone and
    difficult to debug. QWriteLocker can be used in such situations
    to ensure that the state of the lock is always well-defined.

    Here's an example that uses QWriteLocker to lock and unlock a
    read-write lock for writing:

    \code
        QReadWriteLock lock;

        void writeData(const QByteArray &data)
        {
            QWriteLocker locker(&lock);
            ...
        }
    \endcode

    It is equivalent to the following code:

    \code
        QReadWriteLock lock;

        void writeData(const QByteArray &data)
        {
            locker.lockForWrite();
            ...
            locker.unlock();
        }
    \endcode

    The QMutexLocker documentation shows examples where the use of a
    locker object greatly simplifies programming.

    \sa QReadLocker, QReadWriteLock
*/

/*!
    \fn QWriteLocker::QWriteLocker(QReadWriteLock *lock)

    Constructs a QWriteLocker and locks \a lock for writing. The lock
    will be unlocked when the QWriteLocker is destroyed. If \c lock is
    zero, QWriteLocker does nothing.

    \sa QReadWriteLock::lockForWrite()
*/

/*!
    \fn QWriteLocker::~QWriteLocker()

    Destroys the QWriteLocker and unlocks the lock that was passed to
    the constructor.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QWriteLocker::unlock()

    Unlocks the lock associated with this locker.

    \sa QReadWriteLock::unlock()
*/

/*!
    \fn void QWriteLocker::relock()

    Relocks an unlocked lock.

    \sa unlock()
*/

/*!
    \fn QReadWriteLock *QWriteLocker::readWriteLock() const

    Returns a pointer to the read-write lock that was passed
    to the constructor.
*/

#endif // QT_NO_THREAD
