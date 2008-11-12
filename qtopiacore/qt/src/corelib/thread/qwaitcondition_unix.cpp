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
#include "qwaitcondition.h"
#include "qmutex.h"
#include "qatomic.h"
#include "qmutex_p.h"
#include "qstring.h"
#include <errno.h>

#ifndef QT_NO_THREAD

static void report_error(int code, const char *where, const char *what)
{
    if (code != 0)
        qWarning("%s: %s failure: %s", where, what, qPrintable(qt_error_string(code)));
}



struct QWaitConditionPrivate {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int waiters;
    int wakeups;
};


/*!
    \class QWaitCondition
    \brief The QWaitCondition class provides a condition variable for
    synchronizing threads.

    \threadsafe

    \ingroup thread
    \ingroup environment

    QWaitCondition allows a thread to tell other threads that some
    sort of condition has been met. One or many threads can block
    waiting for a QWaitCondition to set a condition with wakeOne() or
    wakeAll(). Use wakeOne() to wake one randomly selected condition or
    wakeAll() to wake them all.

    For example, let's suppose that we have three tasks that should
    be performed whenever the user presses a key. Each task could be
    split into a thread, each of which would have a
    \l{QThread::run()}{run()} body like this:

    \code
        forever {
            mutex.lock();
            keyPressed.wait(&mutex);
            do_something();
            mutex.unlock();
        }
    \endcode

    Here, the \c keyPressed variable is a global variable of type
    QWaitCondition.

    A fourth thread would read key presses and wake the other three
    threads up every time it receives one, like this:

    \code
        forever {
            getchar();
            keyPressed.wakeAll();
        }
    \endcode

    The order in which the three threads are woken up is undefined.
    Also, if some of the threads are still in \c do_something() when
    the key is pressed, they won't be woken up (since they're not
    waiting on the condition variable) and so the task will not be
    performed for that key press. This issue can be solved using a
    counter and a QMutex to guard it. For example, here's the new
    code for the worker threads:

    \code
        forever {
            mutex.lock();
            keyPressed.wait(&mutex);
            ++count;
            mutex.unlock();

            do_something();

            mutex.lock();
            --count;
            mutex.unlock();
        }
    \endcode

    Here's the code for the fourth thread:

    \code
        forever {
            getchar();

            mutex.lock();
            // Sleep until there are no busy worker threads
            while (count > 0) {
                mutex.unlock();
                sleep(1);
                mutex.lock();
            }
            keyPressed.wakeAll();
            mutex.unlock();          
        }
    \endcode

    The mutex is necessary because the results of two threads
    attempting to change the value of the same variable
    simultaneously are unpredictable.

    Wait conditions are a powerful thread synchronization primitive.
    The \l{threads/waitconditions}{Wait Conditions} example shows how
    to use QWaitCondition as an alternative to QSemaphore for
    controlling access to a circular buffer shared by a producer
    thread and a consumer thread.

    \sa QMutex, QSemaphore, QThread, {Wait Conditions Example}
*/

/*!
    Constructs a new wait condition object.
*/
QWaitCondition::QWaitCondition()
{
    d = new QWaitConditionPrivate;
    report_error(pthread_mutex_init(&d->mutex, NULL), "QWaitCondition", "mutex init");
    report_error(pthread_cond_init(&d->cond, NULL), "QWaitCondition", "cv init");
    d->waiters = d->wakeups = 0;
}


/*!
    Destroys the wait condition object.
*/
QWaitCondition::~QWaitCondition()
{
    report_error(pthread_cond_destroy(&d->cond), "QWaitCondition", "cv destroy");
    report_error(pthread_mutex_destroy(&d->mutex), "QWaitCondition", "mutex destroy");
    delete d;
}

/*!
    Wakes one thread waiting on the wait condition. The thread that
    is woken up depends on the operating system's scheduling
    policies, and cannot be controlled or predicted.

    If you want to wake up a specific thread, the solution is
    typically to use different wait conditions and have different
    threads wait on different conditions.

    \sa wakeAll()
*/
void QWaitCondition::wakeOne()
{
    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wakeOne()", "mutex lock");
    d->wakeups = qMin(d->wakeups + 1, d->waiters);
    report_error(pthread_cond_signal(&d->cond), "QWaitCondition::wakeOne()", "cv signal");
    report_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wakeOne()", "mutex unlock");
}

/*!
    Wakes all threads waiting on the wait condition. The order in
    which the threads are woken up depends on the operating system's
    scheduling policies and cannot be controlled or predicted.

    \sa wakeOne()
 */
void QWaitCondition::wakeAll()
{
    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wakeAll()", "mutex lock");
    d->wakeups = d->waiters;
    report_error(pthread_cond_broadcast(&d->cond), "QWaitCondition::wakeAll()", "cv broadcast");
    report_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wakeAll()", "mutex unlock");
}

/*!
    Releases the locked \a mutex and wait on the wait condition.
    The \a mutex must be initially locked by the calling thread. If
    \a mutex is not in a locked state, this function returns
    immediately. If \a mutex is a recursive mutex, this function
    returns immediately. The \a mutex will be unlocked, and the
    calling thread will block until either of these conditions is
    met:

    \list
    \o Another thread signals it using wakeOne() or wakeAll(). This
       function will return true in this case.
    \o \a time milliseconds has elapsed. If \a time is \c ULONG_MAX
       (the default), then the wait will never timeout (the event
       must be signalled). This function will return false if the
       wait timed out.
    \endlist

    The mutex will be returned to the same locked state. This
    function is provided to allow the atomic transition from the
    locked state to the wait state.

    \sa wakeOne(), wakeAll()
 */
bool QWaitCondition::wait(QMutex *mutex, unsigned long time)
{
    if (! mutex)
        return false;

    if (mutex->d->recursive) {
        qWarning("QWaitCondition: cannot wait on recursive mutexes");
        return false;
    }

    report_error(pthread_mutex_lock(&d->mutex), "QWaitCondition::wait()", "mutex lock");
    ++d->waiters;
    mutex->unlock();

    int code;
    forever {
        if (time != ULONG_MAX) {
            struct timeval tv;
            gettimeofday(&tv, 0);

            timespec ti;
            ti.tv_nsec = (tv.tv_usec + (time % 1000) * 1000) * 1000;
            ti.tv_sec = tv.tv_sec + (time / 1000) + (ti.tv_nsec / 1000000000);
            ti.tv_nsec %= 1000000000;

            code = pthread_cond_timedwait(&d->cond, &d->mutex, &ti);
        } else {
            code = pthread_cond_wait(&d->cond, &d->mutex);
        }
        if (code == 0 && d->wakeups == 0) {
            // many vendors warn of spurios wakeups from
            // pthread_cond_wait(), especially after signal delivery,
            // even though POSIX doesn't allow for it... sigh
            continue;
        }
        break;
    }

    Q_ASSERT_X(d->waiters > 0, "QWaitCondition::wait", "internal error (waiters)");
    --d->waiters;
    if (code == 0) {
        Q_ASSERT_X(d->wakeups > 0, "QWaitCondition::wait", "internal error (wakeups)");
        --d->wakeups;
    }
    report_error(pthread_mutex_unlock(&d->mutex), "QWaitCondition::wait()", "mutex unlock");
    mutex->lock();

    if (code && code != ETIMEDOUT)
        report_error(code, "QWaitCondition::wait()", "cv wait");

    return (code == 0);
}
#endif // QT_NO_THREAD
