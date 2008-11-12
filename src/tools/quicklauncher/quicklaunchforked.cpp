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

#include "quicklaunchforked.h"

#include <qpainter.h>
#include <qtimer.h>
#include <qpointer.h>
#include <qtopiachannel.h>
#include <QIcon>
#include <qtopialog.h>

#include <qtimezone.h>
#include <qtopiaapplication.h>
#include <qpluginmanager.h>
#include <qapplicationplugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <qtopiaipcenvelope.h>
#include <QSocketNotifier>

#ifndef QT_NO_SXE
#include <private/qtransportauth_qws_p.h>
#endif

static const QString BLOCK = "/home/smarks/tmp/block";

void qt_app_reinit( const QString& newAppName );

#ifndef SINGLE_EXEC
QPluginManager *QuickLauncher::loader = 0;
QObject *QuickLauncher::appInstance = 0;
QApplicationFactoryInterface *QuickLauncher::appIface = 0;
#endif

QtopiaApplication *QuickLauncher::app = 0;
QPointer<QWidget> QuickLauncher::mainWindow;
bool QuickLauncher::validExitLoop = false;
QEventLoop *QuickLauncher::eventLoop = 0;

char **QuickLauncher::argv0 = 0;
int QuickLauncher::argv_lth;

QSocketNotifier* QuickLauncher::notifier = 0;
int QuickLauncher::ql_deadChild_pipe[2] = {0};

static void (*ql_sa_old_sigchld_handler)(int) = 0;
static void ql_sa_sigchld_handler(int signum)
{
    ::write(QuickLauncher::ql_deadChild_pipe[1], "", 1);

#if defined (QUICKLAUNCHER2_DEBUG)
    fprintf(stderr, "*** SIGCHLD\n");
#endif

    if (ql_sa_old_sigchld_handler && ql_sa_old_sigchld_handler != SIG_IGN)
        ql_sa_old_sigchld_handler(signum);
}

extern char **environ;
#ifndef SPT_BUFSIZE
#define SPT_BUFSIZE     2048
#endif
#include <stdarg.h>
void setproctitle (const char *fmt,...) {
    int        i;
    char       buf[SPT_BUFSIZE];
    va_list    ap;

    if (!QuickLauncher::argv0)
        return;

    va_start(ap, fmt);
    (void) vsnprintf(buf, SPT_BUFSIZE, fmt, ap);
    va_end(ap);

    i = strlen (buf);
    if (i > QuickLauncher::argv_lth - 2) {
        i = QuickLauncher::argv_lth - 2;
        buf[i] = '\0';
    }
    memset(QuickLauncher::argv0[0], '\0', QuickLauncher::argv_lth);       /* clear the memory area */
    (void) strcpy (QuickLauncher::argv0[0], buf);

    QuickLauncher::argv0[1] = NULL;
}

#if defined(QTOPIA_DBUS_IPC)
// For quicklaunched apps
class WaitForRaiseTask : public QObject
{
public:
    static const int timeout = 1000;

    WaitForRaiseTask(QObject * parent);
};

WaitForRaiseTask::WaitForRaiseTask(QObject *parent) : QObject(parent)
{
    QTimer::singleShot(timeout, this, SLOT(deleteLater()));
}
#endif


// ====================================================================

#ifdef SINGLE_EXEC
QPEAppMap *qpeAppMap() {
    static QPEAppMap *am = 0;
    if ( !am ) am = new QPEAppMap();
    return am;
}
void qtopia_registerApp(const char *name, qpeAppCreateFunc createFunc) {
    if ( name ) {
        QPEAppMap *am = qpeAppMap();
        am->insert(name, createFunc);
    }
}
QPEMainMap *qpeMainMap() {
    static QPEMainMap *am = 0;
    if ( !am ) am = new QPEMainMap();
    return am;
}
void qtopia_registerMain(const char *name, qpeMainFunc mainFunc) {
    if ( name ) {
        QPEMainMap *am = qpeMainMap();
        am->insert(name, mainFunc);
    }
}
#endif


// ============================================================================
//
//  QuickLauncher
//
// ============================================================================

QuickLauncher::QuickLauncher()
    : QObject(),
      channel( 0 ),
      apps(),
      pool()
{
    // Create execution service
    listenToChannel();

    // initialize the dead child pipe and make it non-blocking. in the
    // extremely unlikely event that the pipe fills up, we do not under any
    // circumstances want to block.
    ::pipe(ql_deadChild_pipe);
    ::fcntl(ql_deadChild_pipe[0], F_SETFL,
            ::fcntl(ql_deadChild_pipe[0], F_GETFL) | O_NONBLOCK);

    // set up the SIGCHLD handler, which writes a single byte to the dead
    // child pipe every time a child dies.
    struct sigaction oldAction;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = ql_sa_sigchld_handler;
    action.sa_flags = SA_NOCLDSTOP;
    ::sigaction(SIGCHLD, &action, &oldAction);
    if (oldAction.sa_handler != ql_sa_sigchld_handler)
        ql_sa_old_sigchld_handler = oldAction.sa_handler;

    notifier = new QSocketNotifier(ql_deadChild_pipe[0], QSocketNotifier::Read);
    QObject::connect( notifier,
                      SIGNAL(activated(int)),
                      this,
                      SLOT(childDied()) );

    // Inform application launcher that QuickLauncher is available
    QtopiaIpcEnvelope env("QPE/QuickLauncher", "available(int)");
    env << ::getpid();

    // Fill the child process pool
    QTimer::singleShot( 0, this, SLOT(fillPool()) );
}

void QuickLauncher::listenToChannel()
{
    QString ch("QPE/QuickLauncher/pid/");
    ch += QString::number(::getpid());
    qLog(Quicklauncher) << "listening on" << ch;
    channel = new QtopiaIpcAdaptor( ch );

    QtopiaIpcAdaptor::connect(
        channel,
        MESSAGE( execute( const QStringList& ) ),
        this,
        SLOT(execute(QStringList)),
        QtopiaIpcAdaptor::SenderIsChannel );

    QtopiaIpcAdaptor::connect(
        channel,
        MESSAGE( execute( const QString& ) ),
        this,
        SLOT(execute(QString)),
        QtopiaIpcAdaptor::SenderIsChannel );

    QtopiaIpcAdaptor::connect(
        channel,
        MESSAGE( run( const QStringList& ) ),
        this,
        SLOT(run(QStringList)),
        QtopiaIpcAdaptor::SenderIsChannel );

    QtopiaIpcAdaptor::connect(
        channel,
        MESSAGE( quit() ),
        this,
        SLOT(quit()),
        QtopiaIpcAdaptor::SenderIsChannel );
}

QuickLauncher::~QuickLauncher()
{
    delete channel;
    delete notifier;
}


void QuickLauncher::disconnectChannel()
{
    if ( channel != 0 )
        return;

    QtopiaIpcAdaptor::disconnect(
        channel,
        MESSAGE( execute( const QStringList& ) ),
        this,
        SLOT(execute(QStringList)) );

    QtopiaIpcAdaptor::disconnect(
        channel,
        MESSAGE( execute( const QString& ) ),
        this,
        SLOT(execute(QString)) );

    QtopiaIpcAdaptor::disconnect(
        channel,
        MESSAGE( run( const QStringList& ) ),
        this,
        SLOT(run(QStringList)) );

    QtopiaIpcAdaptor::disconnect(
        channel,
        MESSAGE( quit() ),
        this,
        SLOT(quit()) );
}

void QuickLauncher::reinit()
{
    qLog(Quicklauncher) << "QuickLauncher::reinit()";

    // Cleanup stuff we don't need from the parent process, but don't
    // disconnect as we still want the parent process to receive
    // messages
    if ( channel != 0 ) {
        channel->deleteLater();
        channel = 0;
    }

    if ( notifier != 0 ) {
        delete notifier;
        notifier = 0;
    }

    pool.clear();

    // Reintialise the application and server connections
    QString appName = app->arguments().at(0);
    int sep = appName.lastIndexOf( '/' );
    if ( sep > 0 )
        appName = appName.mid( sep+1 );

    qt_app_reinit( appName );

    // Create new service
    listenToChannel();
}

void QuickLauncher::exec( int /*argc*/, char **argv )
{
    QString appName = argv[0];
    int sep = appName.lastIndexOf( '/' );
    if ( sep > 0 )
        appName = appName.mid( sep+1 );

#ifndef QT_NO_SXE
    // loader invokes the constructor - need to clear the key before this
    guaranteed_memset( _key, 0, QSXE_KEY_LEN );
#endif
    appInstance = loader->instance(appName);
    appIface = qobject_cast<QApplicationFactoryInterface*>(appInstance);

#ifndef QT_NO_SXE
    if ( appIface )
        appIface->setProcessKey( appName );
#endif
    qt_app_reinit( appName );

    if ( appIface ) {
        mainWindow = appIface->createMainWindow( appName );
    }
    if ( mainWindow ) {
        QDialog *dlg = qobject_cast<QDialog *>(mainWindow);
        if (!dlg) {
            if ( mainWindow->metaObject()->indexOfSlot("setDocument(QString)") != -1 ) {
                app->showMainDocumentWidget( mainWindow );
            } else {
                app->showMainWidget( mainWindow );
            }
        }
    } else {
        qLog(Quicklauncher) << "Could not create application main window";
        exit(-1);
    }
}

void QuickLauncher::execute( const QStringList& arguments )
{
    qLog(Quicklauncher) << "QuickLauncher::execute(" << arguments << ") pid="
                        << ::getpid();
    doQuickLaunch( arguments );
}

void QuickLauncher::execute( const QString& argument )
{
    qLog(Quicklauncher) << "QuickLauncher::execute(" << argument << ") pid="
                        << ::getpid();
    QStringList arguments;
    arguments << argument;
    doQuickLaunch( arguments );
}

void QuickLauncher::run( const QStringList& arguments )
{
    qLog(Quicklauncher) << "QuickLauncher::run(" << arguments << ") pid="
                        << ::getpid();
    runApplication( arguments );
}

void QuickLauncher::quit()
{
    qLog(Quicklauncher) << "QuickLauncher::quit() pid=" << ::getpid();
    validExitLoop = true;
    eventLoop->exit();
    QTimer::singleShot( 5, app, SLOT(quit()) );
    qLog(Quicklauncher) << "Quicklauncher received quit: exiting...";
}

void QuickLauncher::childDied()
{
    char buffer;
    ::read(ql_deadChild_pipe[0], &buffer, 1);

    int status = 0;
    pid_t died = ::wait(&status);

    QMap<int, QString>::Iterator iter = apps.find(died);
    if(iter == apps.end()) return;

    if(WIFEXITED(status)) {
        qLog(Quicklauncher) << "QuickLauncher: Sending exited (pid ="
                            << died << ")";
        QtopiaIpcEnvelope env("QPE/QuickLauncher", "exited(int)");
        env << died;
    } else {
        qLog(Quicklauncher) << "QuickLauncher: Sending crashed (pid ="
                            << died << ")";
        QtopiaIpcEnvelope env("QPE/QuickLauncher", "crashed(int)");
        env << died;
    }

    apps.erase(iter);
}

void QuickLauncher::fillPool()
{
    qLog(Quicklauncher) << "QuickLauncher::fillPool() - current count="
                        << pool.count();
    while ( pool.count() < 1 ) {
        int pid = ::fork();
        if ( 0 == pid ) {
            // Child process
            reinit();

            // the child should return as it shouldn't execute anymore code
            // in this method
            return;
        } else {
            qLog(Quicklauncher) << "added" << pid << "to the pool";
            // Parent process
            pool.append( pid );
        }
    }
}

void QuickLauncher::doQuickLaunch( const QStringList& argList )
{
    qLog(Quicklauncher) << "QuickLauncher::doQuickLaunch()";

    // Need at least one child in the pool
    bool poolEmpty = false;

    // Get the pid
    int pid = -1;
    if ( pool.count() == 0 ) {
        qLog(Quicklauncher) << "\tPOOL EMPTY!";
        poolEmpty = true;
        pid = ::fork();
        if ( 0 == pid ) {
            // Child process, reinitialise and start the app running straight away
            reinit();
            runApplication( argList );

            // the child should return as it shouldn't execute anymore code
            // in this method
            return;
        }
    } else {
        pid = pool.first();
        pool.removeFirst();
    }

    // Send a message if we are using a pooled child
    if ( !poolEmpty ) {
        QString qlch("QPE/QuickLauncher/pid/");
        qlch += QString::number( pid );

        qLog(Quicklauncher) << "\tsending run to" << qlch;
        {
            QtopiaIpcEnvelope childEnv( qlch, "run(QStringList)" );
            childEnv << argList;
        }

    }

    apps.insert( pid, argList.at(0) );

    // Send a message back to the QuickExeApplicationLauncher to say we've
    // started the process and give the new pid. Do this from the parent process
    // so that we don't have to provide each quicklaunched app with a SXE profile
    // for this message.
    {
        QString appName = argList[0];
        int sep = appName.lastIndexOf( '/' );
        if ( sep > 0 )
            appName = appName.mid( sep + 1 );

        QtopiaIpcEnvelope launcherEnv( "QPE/QuickLauncher", "running(QString,int)" );
        launcherEnv << appName << pid;
    }

    QTimer::singleShot( 500, this, SLOT(fillPool()) );
}

void QuickLauncher::runApplication( const QStringList& argList )
{
    qLog(Quicklauncher) << "QuickLauncher::runApplication()";

    // Cleanup the channel including disconnection (we don't care about
    // quicklauncher messages anymore)
    disconnectChannel();
    if ( channel != 0 ) {
        channel->deleteLater();
        channel = 0;
    }

    static int myargc = argList.count();
    static char **myargv = new char *[myargc + 1];
    for ( int j = 0; j < myargc; j++ ) {
        myargv[j] = new char [strlen(argList.at(j).toAscii().constData())+1];
        strcpy( myargv[j], argList.at(j).toAscii().constData() );
    }
    myargv[myargc] = NULL;

    // Change name of process
    setproctitle(myargv[0]);

    connect(app, SIGNAL(lastWindowClosed()), app, SLOT(hideOrQuit()));
    app->initApp( myargc, myargv );

#if defined(QTOPIA_DBUS_IPC)
    //app->registerRunningTask("WaitForDBUSRaise", new WaitForRaiseTask(0));
#endif
    validExitLoop = true;
    eventLoop->exit();
    exec( myargc, myargv );

    // Cleanup
    for ( int i = 0; i < ( myargc + 1 ); ++i ) {
        delete[] myargv[i];
    }
    delete[] myargv;
}

