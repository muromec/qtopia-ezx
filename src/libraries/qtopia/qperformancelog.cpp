/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include <qtopia/qperformancelog.h>
#include <qtopia/private/qperformancelog_p.h>

#include <QThread>
#include <QTimer>
#include <QStringList>
#include <QApplication>
#include <sys/times.h>
#include <unistd.h>
#include <time.h>

#ifdef QTOPIA_TEST_HOST
# include <QDebug>
# define qLog(A) qDebug()
# define qLogEnabled(A) (true)
#else
# ifndef Q_WS_X11
#  include <qtopia/private/testslaveinterface_p.h>
#  define QTOPIA_USE_TEST_SLAVE 1
# endif
# include <qtopia/qtopiaapplication.h>
# include <qtopiabase/qtopialog.h>
# include <qtopiabase/Qtopia>
# include <errno.h>
#endif // ! QTOPIA_TEST_HOST


const int QPerformanceLogData::Timeout = 2000;

QString QPerformanceLogData::toString() const
{
    return QString("%1 : %2 : %3 : %4%5%6").arg(ident).arg(appTime/1000000).arg(serverTime/1000000).arg(QPerformanceLog::stringFromEvent(event)).arg((event != QPerformanceLog::NoEvent) ? " " : "").arg(msg);
}

class QPerformanceLogPrivate : public QObject
{
Q_OBJECT
public:
    void send( const QPerformanceLogData &msg );

    static QPerformanceLogPrivate *instance();

private slots:
    void sendUnsent();

private:
    QPerformanceLogPrivate();
    ~QPerformanceLogPrivate();

    QList<QPerformanceLogData> unsent;
    QTimer timer;
};
#include "qperformancelog.moc"

qint64 q_gettime_ns()
{
    struct timespec spec;
    if (0 != clock_gettime(
#if _POSIX_MONOTONIC_CLOCK > 0
                CLOCK_MONOTONIC,
#else
                CLOCK_REALTIME,
#endif
                &spec))
        Q_ASSERT(0);
    return spec.tv_nsec + Q_INT64_C(1000000000)*qint64(spec.tv_sec);
}

static qint64 start_time = q_gettime_ns();
static qint64 server_start_time;

#ifndef QTOPIA_TEST_HOST
class StartTimeSetup
{
public:
    StartTimeSetup();
};

StartTimeSetup::StartTimeSetup()
{
    bool set = false;
    do {
        QByteArray t = qgetenv("QTOPIA_PERFTEST_LAUNCH");
        if (t.isEmpty() || (t.count(':') != 2 && t.count(':') != 0)) break;

        if (t.count(':') == 0) {
            server_start_time = t.toLongLong(&set);
            break;
        }

        QList<QByteArray> tl = t.split(':');
        bool ok = true;
        int h, m, s, ms = 0;
        h = tl[0].toInt(&ok); if (!ok) break;
        m = tl[1].toInt(&ok); if (!ok) break;

        if (tl[2].count('.') == 1) {
            QList<QByteArray> tll = tl[2].split('.');
            s = tll[0].toInt(&ok); if (!ok) break;
            ms = tll[1].left(3).toInt(&ok); if (!ok) break;
        } else {
            s = tl[2].toInt(&ok); if (!ok) break;
        }

        // launchMs is the time in milliseconds since _midnight_, so we can't
        // use it as-is.  Figure out what the ms since midnight is now, and subtrct
        // that difference from the current time.
        qint64 launchMs = ((h*60 + m)*60 + s)*1000 + ms;
        QTime now(QTime::currentTime());
        qint64 nowMs = ((now.hour()*60 + now.minute())*60 + now.second())*1000 + now.msec();

        server_start_time = q_gettime_ns() - (nowMs - launchMs)*Q_INT64_C(1000000);

        set = true;

    } while(0);

    if (!set) {
        server_start_time = q_gettime_ns();
    }
    ::setenv("QTOPIA_PERFTEST_LAUNCH", QByteArray::number(server_start_time), 1);
    QPerformanceLog() << "QPerformanceLog server_start_time set to " << QString::number(server_start_time);
}

static StartTimeSetup time_setup;
#endif // QTOPIA_TEST_HOST

QPerformanceLogPrivate *QPerformanceLogPrivate::instance()
{
    static QPerformanceLogPrivate *s = new QPerformanceLogPrivate();
    return s;
}

QPerformanceLogPrivate::QPerformanceLogPrivate() : QObject()
{
    connect(&timer, SIGNAL(timeout()), this, SLOT(sendUnsent()));
}

QPerformanceLogPrivate::~QPerformanceLogPrivate()
{
    sendUnsent();
}

void QPerformanceLogPrivate::sendUnsent()
{
#if !defined(QTOPIA_TEST_HOST) && defined(QTOPIA_USE_TEST_SLAVE)
    if (unsent.count() == 0) {
        timer.stop();
        return;
    }

    TestSlaveInterface *i = 0;
    if (QtopiaApplication::instance()) i = QtopiaApplication::instance()->testSlave();
    if (!i || !i->isConnected()) return;

    QVariantList vl;
    /* Send all unsent messages as a single QTestMessage, to keep overhead as small as possible */
    while (!unsent.isEmpty()) {
        QVariantMap map;
        QPerformanceLogData data = unsent.takeFirst();
        map["message"] = data.msg;
        map["event"] = (int)data.event;
        map["ident"] = data.ident;
        map["appTime"] = data.appTime;
        map["serverTime"] = data.serverTime;
        vl.append(map);
    }

    QVariantMap msgMap;
    msgMap["data"] = vl;
    i->postMessage("Performance", msgMap);

    timer.stop();
#endif
}

void QPerformanceLogPrivate::send( const QPerformanceLogData &data )
{
#ifndef QTOPIA_TEST_HOST
    if (QtopiaApplication::instance() && !QtopiaApplication::instance()->testSlave()) return;
    unsent << data;
    if (!timer.isActive() && qApp) {
        if (timer.thread() != qApp->thread()) timer.moveToThread(qApp->thread());
        timer.start(QPerformanceLogData::Timeout);
    }
#else
    Q_UNUSED(data);
#endif
}

/*!
  \internal
  \class QPerformanceLog
  \brief The QPerformanceLog class implements a performance logging mechanism available to all Qtopia applications.
\if defined(QTOPIA_TEST)
  \ingroup qtopiatest_systemtest
\endif

  It provides a similar behaviour to qDebug() and qLog(), but every
  message automatically contains two timestamps: milliseconds since the current application
  has started, and milliseconds since the Qtopia Core window server has started.

  By default, messages are output via qLog(Performance), respecting the qLog() settings.

  If the QTOPIA_PERFTEST environment variable is set, messages will also be sent to
  a connected QtopiaTest system test (if any), and will always be output to the local
  console, overriding qLog() settings.

  Any string data can be output in a performance log.  To make log parsing easier,
  some predefined values are provided for events which are commonly of interest for
  performance testing.  These are represented by the QPerformanceLog::Event flags.

  The below example shows how this class can be used to measure the time for a specific task
  from within a system test.

  Example code residing in a Qtopia application named "Dog Walker":
  \code
    QPerformanceLog() << QPerformanceLog::Begin << "walk to park";
    // Outputs 'Dog Walker : <ms_since_appstart> : <ms_since_qpestart> : begin walk to park'
    while ( !at( Locations::Park ) ) {
        walk( directionOf(Locations::Park) );
    }
    QPerformanceLog() << QPerformanceLog::End << "walk to park";
    // Outputs 'Dog Walker : <ms_since_appstart> : <ms_since_qpestart> : end walk to park'
  \endcode

\if defined(QTOPIA_TEST)
  \sa QSystemTest
\endif
*/

/*!
    \enum QPerformanceLog::EventType

    This enum provides a simple way of logging common occurrences such as the
    beginning and ending of a particular task.  Each QPerformanceLog instance has
    an associated event value which is constructed by combining values from the following
    list using the bitwise OR operator:

    \value NoEvent          Log message is not related to any event described by QPerformanceLog::EventType.
    \value Begin            Log message signifies the beginning of a specific event.
    \value End              Log message signifies the end of a specific event.
    \value LibraryLoading   Log message is related to the loading of shared libraries.  This value is used internally by Qtopia.
    \value EventLoop        Log message is related to the application's global event loop.  This value is used internally by Qtopia.
    \value MainWindow       Log message is related to the construction of the application's main window.  This value is used internally by Qtopia.

    By streaming these enum values into a QPerformanceLog(), processing of performance
    data from within a system test is made easier.  In particular, using Begin and End,
    along with a unique identifying string for a particular event in your program,
    allows the amount of time taken for a particular event to be easily determined.
*/

/*!
    Construct a performance logger for application \a applicationName.
    If \a applicationName is empty, the name of the current application is used.
*/
QPerformanceLog::QPerformanceLog( QString const &applicationName )
{
    if (enabled()) {
        qint64 now = q_gettime_ns();
        data = new QPerformanceLogData;
        data->event = NoEvent;
        data->ident = ((applicationName.isEmpty() && qApp) ? qApp->applicationName() : applicationName);
        {
            static char warned = 0;
            if (start_time > now || server_start_time > now && !warned) {
                warned = 1;
                qWarning("QPerformanceLog: start time seems to be in the future!");
            }
        }
        data->appTime    = now - start_time;
        data->serverTime = now - server_start_time;
    }
}

/*!
    Destroy the performance log object and output the performance data.
*/
QPerformanceLog::~QPerformanceLog()
{
    if (!enabled()) return;

#ifndef QTOPIA_TEST_HOST
    if (QPerformanceLogPrivate::instance() && data)
        QPerformanceLogPrivate::instance()->send( *data );
#endif

    if (data) {
        qDebug() << qPrintable(data->toString());
        delete data;
    }
}

/*!
    Returns true if QPerformanceLog is enabled.
    QPerformanceLog is enabled if either qLog(Performance) is enabled or the
    QTOPIA_PERFTEST environment variable is set to "1".
*/
bool QPerformanceLog::enabled()
{
    static bool ret = qLogEnabled(Performance) || (!qgetenv("QTOPIA_PERFTEST").isEmpty());
    return ret;
}

/*!
    Append \a string to the log message.
*/
QPerformanceLog &QPerformanceLog::operator<<(QString const &string) {
    if (enabled())
        data->msg += string + " ";
    return *this;
}

/*!
    Append \a event to the log message.
    If this log message already has an event, the new event will be equal to
    the old event bitwise OR'd with \a event.
*/
QPerformanceLog &QPerformanceLog::operator<<(Event const &event) {
    if (enabled())
        data->event |= event;
    return *this;
}

/*!
    Returns a string representation of \a event.
*/
QString QPerformanceLog::stringFromEvent(Event const &event)
{
    QString ret;
    if (event == NoEvent) return ret;

    if (event.testFlag(Begin))          ret += " begin";
    if (event.testFlag(End))            ret += " end";

    if (event.testFlag(LibraryLoading)) ret += " loading_libraries";
    if (event.testFlag(EventLoop))      ret += " event_loop";
    if (event.testFlag(MainWindow))     ret += " main_window_create";

    return ret.trimmed();
}

/*!
    \internal
    \deprecated
    Can't remove, BIC.
*/
void QPerformanceLog::adjustTimezone( QTime &preAdjustTime )
{
    Q_UNUSED(preAdjustTime);
}

