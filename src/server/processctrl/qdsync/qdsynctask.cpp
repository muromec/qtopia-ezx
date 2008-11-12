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
#include "qtopiaserverapplication.h"
#include <QTimer>
#include <qtopialog.h>
QTOPIA_LOG_OPTION(QDSync)
#include <private/qcopenvelope_p.h>
#include "applicationlauncher.h"
#include "systemsuspend.h"
#include <time.h>
#include <unistd.h>
#include <custom.h>

#ifndef QDSYNC_STARTUP_DELAY
#define QDSYNC_STARTUP_DELAY 1000
#endif

class QDSyncTask : public SystemShutdownHandler
{
    Q_OBJECT
public:
    QDSyncTask();

private slots:
    void waitnewproc();
    void newproc();
    void applicationStateChanged( const QString &application, ApplicationTypeLauncher::ApplicationState state );
    void systemSuspending();
    void systemWaking();

private:
    bool systemRestart();
    bool systemShutdown();
    QString gadget();

    ApplicationLauncher *launcher;
    SystemSuspend *suspend;
    bool shutdown;
    time_t curTime;
    bool running;
};

QTOPIA_TASK(QDSyncTask,QDSyncTask);
QTOPIA_TASK_PROVIDES(QDSyncTask,SystemShutdownHandler);

QDSyncTask::QDSyncTask()
{
    qLog(QDSync) << "QDSyncTask::QDSyncTask";
    launcher = qtopiaTask<ApplicationLauncher>();
    Q_ASSERT(launcher);
    connect( launcher, SIGNAL(applicationStateChanged(QString,ApplicationTypeLauncher::ApplicationState)),
             this, SLOT(applicationStateChanged(QString,ApplicationTypeLauncher::ApplicationState)) );

    suspend = qtopiaTask<SystemSuspend>();
    Q_ASSERT(suspend);
    connect( suspend, SIGNAL(systemSuspending()), this, SLOT(systemSuspending()) );
    connect( suspend, SIGNAL(systemActive()), this, SLOT(systemWaking()) );

    shutdown = false;
    running = false;
    if ( launcher->canLaunch( "qdsync" ) ) {
        curTime = ::time(0);
        qLog(QDSync) << "running waitnewproc curTime" << curTime;
        QTimer::singleShot( QDSYNC_STARTUP_DELAY, this, SLOT(waitnewproc()) );
    }
}

void QDSyncTask::waitnewproc()
{
    time_t now = ::time(0);
    qLog(QDSync) << "waitnewproc now" << now;
    int elapsed = now - curTime;
    if ( elapsed > 1 ) {
        qLog(QDSync) << "elapsed" << elapsed << "greater than 1s... server busy?";
        curTime = now;
        qLog(QDSync) << "running waitnewproc curTime" << curTime;
        QTimer::singleShot( 1000, this, SLOT(waitnewproc()) );
    } else {
        newproc();
    }
}

void QDSyncTask::newproc()
{
    qLog(QDSync) << "QDSyncTask::newproc";
    QCopEnvelope e("QPE/Application/qdsync", "startDaemons()");
}

bool QDSyncTask::systemRestart()
{
    qLog(QDSync) << "QDSyncTask::systemRestart";
    return systemShutdown();
}

bool QDSyncTask::systemShutdown()
{
    qLog(QDSync) << "QDSyncTask::systemShutdown";
    if ( launcher->canLaunch( "qdsync" ) && running ) {
        QCopEnvelope e("QPE/Application/qdsync", "shutdown()");
        shutdown = true;
        return false;
    } else {
        return true;
    }
}

void QDSyncTask::applicationStateChanged( const QString &application, ApplicationTypeLauncher::ApplicationState state )
{
    if ( application == "qdsync" ) {
        qLog(QDSync) << "QDSyncTask::applicationStateChanged" << application << state;
        if ( state == ApplicationTypeLauncher::NotRunning ) {
            running = false;
            if ( shutdown ) {
                emit proceed();
            } else {
                qLog(QDSync) << "qdsync exited prematurely. Restart in 5 seconds.";
                QTimer::singleShot( 5000, this, SLOT(waitnewproc()) );
            }
        } else if ( state == ApplicationTypeLauncher::Running ) {
            running = true;
            // The app might have been started manually... start the daemons now
            newproc();
        }
    }
}

void QDSyncTask::systemSuspending()
{
    qLog(QDSync) << "QDSyncTask::systemSuspending";
    // This is setup for the Greenphone which must unload the usb gadget kernel module
    // when it suspends. QDSync must release the serial device or it will trigger a kernel panic.
    if ( launcher->canLaunch( "qdsync" ) && running && gadget() == "winserial" ) {
        qLog(QDSync) << "Stopping QDSync daemons";
        QCopEnvelope e("QPE/Application/qdsync", "stopDaemons()");
    }
    ::sleep(1);
}

void QDSyncTask::systemWaking()
{
    qLog(QDSync) << "QDSyncTask::systemWaking";
    if ( launcher->canLaunch( "qdsync" ) && running )
        QCopEnvelope e("QPE/Application/qdsync", "startDaemons()");
}

QString QDSyncTask::gadget()
{
    // This is setup for the Greenphone which writes the current gadget to /etc/gadget
    QFile f( "/etc/gadget" );
    if ( f.exists() && f.open(QIODevice::ReadOnly) ) {
        QByteArray gadget = f.readAll();
        f.close();
        return QString(gadget.trimmed());
    }
    return QString();
}

#include "qdsynctask.moc"
