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

#include <QSettings>
#include <QtopiaIpcEnvelope>
#include <QContent>
#include <QContentSet>
#include <QStringList>
#include <QtopiaService>
#include <qtopialog.h>
#include "qtopiaserverapplication.h"
#include "startupapps.h"
#include "applicationlauncher.h"

class DaemonLauncher : public SystemShutdownHandler
{
Q_OBJECT
public:
    DaemonLauncher();

    struct Application {
        QString group;
        bool running;
        bool silent;
        QString name;
        QString startupMessage;
        QString restartMessage;
        QString shutdownMessage;
    };

    void startup();
    void launch(const QString &);

protected:
    virtual bool systemRestart();
    virtual bool systemShutdown();
    virtual void timerEvent(QTimerEvent *);

private slots:
    void stateChanged(const QString &,
                      ApplicationTypeLauncher::ApplicationState);

private:
    void readConfig();
    void installTasks();
    void doShutdown();

    ApplicationLauncher *m_launcher;
    Application *readApplication(QSettings &, const QString &);
    QMap<QString, Application *> m_applications;
    QMap<QString, QList<Application *> > m_groups;
    QList<char *> m_taskNames;
    int m_runningWithShutdown;
    int m_shutdownTimer;
};
Q_GLOBAL_STATIC(DaemonLauncher, daemonLauncher);

class InstallDaemons {
public:
    InstallDaemons() { daemonLauncher()->startup(); };
};
static InstallDaemons id;

static QObject * daemonLauncherTaskFunc(void *arg)
{
    const char *task = (const char *)arg;
    Q_ASSERT(task);
    DaemonLauncher *dl = daemonLauncher();
    if(dl)
        dl->launch(task);
    return 0;
}

DaemonLauncher::DaemonLauncher()
: m_runningWithShutdown(0), m_shutdownTimer(0)
{
}

void DaemonLauncher::doShutdown()
{
    Q_ASSERT(!m_shutdownTimer);
    Q_ASSERT(m_runningWithShutdown);

    for(QMap<QString, Application *>::Iterator iter = m_applications.begin();
            iter != m_applications.end(); ++iter) {

        Application *app = *iter;
        if(app->running && !app->shutdownMessage.isEmpty()) {
            qLog(QtopiaServer) << "DaemonLauncher: Shutting down application"
                               << app->name;
            QtopiaIpcEnvelope env("QPE/Application/" + app->name, app->shutdownMessage);
        }
    }

    m_shutdownTimer = startTimer(5000);
}

bool DaemonLauncher::systemRestart()
{
    if(!m_runningWithShutdown)
        return true;

    doShutdown();
    return false;
}

bool DaemonLauncher::systemShutdown()
{
    if(!m_runningWithShutdown)
        return true;

    doShutdown();
    return false;
}

void DaemonLauncher::timerEvent(QTimerEvent *)
{
    Q_ASSERT(m_shutdownTimer);
    killTimer(m_shutdownTimer);
    m_shutdownTimer = 0;
    qLog(QtopiaServer) << "DaemonLauncher: Applications did not shutdown.";
    emit proceed();
}

void DaemonLauncher::stateChanged(const QString &app,
                                  ApplicationTypeLauncher::ApplicationState s)
{
    if(s != ApplicationTypeLauncher::NotRunning)
        return;

    QMap<QString, Application *>::Iterator iter = m_applications.find(app);
    if(iter == m_applications.end() || !(*iter)->running)
        return;

    Application *appl = *iter;

    appl->running = false;
    if(!appl->restartMessage.isEmpty()) {
        Q_ASSERT(m_runningWithShutdown);
        --m_runningWithShutdown;

        if(m_shutdownTimer && !m_runningWithShutdown) {
            emit proceed();
            return;
        }
    }

    if(!appl->restartMessage.isEmpty()) {
        Q_ASSERT(m_launcher->canLaunch(app));
        {
            {
                qLog(QtopiaServer) << "DaemonLauncher: Restarting app" << app;
                QtopiaIpcEnvelope env("QPE/Application/" + app,
                                      appl->restartMessage);
            }
            Q_ASSERT(m_launcher->state(app) !=
                     ApplicationTypeLauncher::NotRunning);
            appl->running = true;
            if(!appl->restartMessage.isEmpty())
                ++m_runningWithShutdown;
        }
    } else {
        qLog(QtopiaServer) << "DaemonLauncher: Non-restartable application"
                           << app << "terminated";
    }
}

void DaemonLauncher::launch(const QString &group)
{
    if(!m_launcher) {
        m_launcher = qtopiaTask<ApplicationLauncher>();
        Q_ASSERT(m_launcher &&
                 "ApplicationLauncher required for DaemonLauncher");

        QObject::connect(m_launcher, SIGNAL(applicationStateChanged(QString,ApplicationTypeLauncher::ApplicationState)), this, SLOT(stateChanged(QString,ApplicationTypeLauncher::ApplicationState)));
    }

    QMap<QString, QList<Application *> >::Iterator iter = m_groups.find(group);
    Q_ASSERT(iter != m_groups.end());
    const QList<Application *> &apps = *iter;

    qLog(QtopiaServer) << "DaemonLauncher: Launching" << group;
    for(int ii = 0; ii < apps.count(); ++ii) {
        if(apps.at(ii)->running) {
            qLog(QtopiaServer) << "    Skipping running application"
                               << apps.at(ii)->name;
        } else if(!m_launcher->canLaunch(apps.at(ii)->name)) {
            if(!apps.at(ii)->silent) {
                qWarning() << "DaemonLauncher: Unable to launch missing "
                              "application" << apps.at(ii)->name;
            }
        } else if(!apps.at(ii)->startupMessage.isEmpty()) {
            {
                qLog(QtopiaServer) << "DaemonLauncher: Launching app" << apps.at(ii)->name;
                QtopiaIpcEnvelope env("QPE/Application/" + apps.at(ii)->name, apps.at(ii)->startupMessage);
            }
            Q_ASSERT(m_launcher->state(apps.at(ii)->name) !=
                     ApplicationTypeLauncher::NotRunning);
            if(!apps.at(ii)->restartMessage.isEmpty())
                ++m_runningWithShutdown;
        }
    }
}

void DaemonLauncher::startup()
{
    readConfig();
    installTasks();
}

void DaemonLauncher::installTasks()
{
    for(QMap<QString, QList<Application *> >::ConstIterator iter =
            m_groups.begin(); iter != m_groups.end(); ++iter) {
        Q_ASSERT(iter->count());

        char *taskName = new char[iter.key().length() + 1];
        ::strcpy(taskName, iter.key().toLatin1().constData());
        Q_ASSERT(QString(taskName) == iter.key());
        QtopiaServerApplication::addTask(taskName, true,
                                         daemonLauncherTaskFunc,
                                         (void *)taskName);
        m_taskNames.append(taskName);
    }
}

void DaemonLauncher::readConfig()
{
    QSettings cfg("Trolltech", "BackgroundApplications");
    int count = cfg.value("Count", 0).toInt();

    for(int ii = 0; ii < count; ++ii) {
        QString appstr = "Application" + QString::number(ii);
        cfg.beginGroup(appstr);
        Application *app = readApplication(cfg, appstr);
        cfg.endGroup();
        if(m_applications.contains(app->name)) {
            qWarning() << "DaemonLauncher: Duplicate application" << app->name
                       << "discarded.";
        } else {
            m_applications.insert(app->name, app);
            m_groups[app->group].append(app);
        }
    }
}

DaemonLauncher::Application *
DaemonLauncher::readApplication(QSettings &cfg, const QString &app)
{
    bool silent = cfg.value("Silent", false).toBool();

    QString appName = cfg.value("ApplicationName").toString();
    QString serviceName = cfg.value("ServiceName").toString();
    if(!appName.isEmpty() && !serviceName.isEmpty()) {
        if(!silent)
            qWarning() << "DaemonLauncher: Application name" << appName
                       << "overrides service name" << serviceName << "for"
                       << app;
    } else if(appName.isEmpty() && !serviceName.isEmpty()) {
        appName = QtopiaService::binding(serviceName);
    }

    if(appName.isEmpty()) {
        if(!silent)
            qWarning() << "DaemonLauncher: Cannot resolve application name for"
                       << app;
        return 0;
    }

    QString group = cfg.value("TaskGroup").toString();
    if(group.isEmpty()) {
        if(!silent)
            qWarning() << "DaemonLauncher: Application" << app << "does not"
                          "include a group specification";
        return 0;
    }

    QString startupMessage = cfg.value("StartupMessage").toString();
    if(startupMessage.isEmpty() && !silent)
        qWarning() << "DaemonLauncher: No startup message specified for"
                   << app;
    QString restartMessage = cfg.value("RestartMessage").toString();
    QString shutdownMessage = cfg.value("ShutdownMessage").toString();
    if(!startupMessage.isEmpty() && !serviceName.isEmpty())
        startupMessage.prepend(serviceName + "::");
    if(!shutdownMessage.isEmpty() && !serviceName.isEmpty())
        shutdownMessage.prepend(serviceName + "::");

    Application *appl = new Application;
    appl->name = appName;
    appl->group = group;
    appl->running = false;
    appl->silent = silent;
    appl->startupMessage = startupMessage;
    appl->restartMessage = restartMessage;
    appl->shutdownMessage = shutdownMessage;
    return appl;
}

// declare StartupApplicationsPrivate
class StartupApplicationsPrivate : public ApplicationTerminationHandler
{
Q_OBJECT
public:
    StartupApplicationsPrivate(QObject *parent)
    : ApplicationTerminationHandler(parent) {}

    virtual bool terminated(const QString &,
                            ApplicationTypeLauncher::TerminationReason);

signals:
    void preloadCrashed(const QString &);
};

// define StartupApplicationsPrivate
bool StartupApplicationsPrivate::terminated(const QString &name,
        ApplicationTypeLauncher::TerminationReason reason)
{
    if(ApplicationTypeLauncher::Normal == reason) return false;

    QContent app(name, false);
    if( app.isNull() ) return false;

    // Disable crashing preloaded applications
    if(app.isPreloaded()) {
        QSettings cfg("Trolltech","Launcher");
        cfg.beginGroup("AppLoading");
        QStringList apps = cfg.value("PreloadApps").toString().split(',');
        QString exe = app.executableName();
        apps.removeAll(exe);
        cfg.setValue("PreloadApps", apps.join(QString(',')));

        return true;
    } else {
        return false;
    }
}

/*!
  \class StartupApplications
  \ingroup QtopiaServer::Task
  \brief The StartupApplications class launches applications preemptively at startup.

  The StartupApplications provides a Qtopia Server Task.  Qtopia Server Tasks 
  are documented in full in the QtopiaServerApplication class documentation.

  \table
  \row \o Task Name \o StartupApplications
  \row \o Interfaces \o None
  \row \o Services \o None
  \endtable

  Qtopia provides support for preloading GUI applications, and for launching
  background daemon-type applications during startup.  The StartupApplications
  class provides this functionality.

  \section1 Preloaded Applications

  Preloaded applications are started immediately after Qtopia starts, but in
  a hidden state.  Preloading an application ensures that it is available for
  almost instantaneouse "startup" whenever the user accesses it.  However, as
  preloaded applications continue to run indefinately, they consume system
  resources such as RAM and scheduler time even when not actively in use.  Only
  applications used very frequently should be preloaded.

  An application can be preloaded by adding it to the
  \c {AppLoading\PreloadApps} item list in the \c {Trolltech/Launcher}
  configuration file.  The following example shows both the \c addressbook and
  \c qtmail applications set to preload.

  \code
  [AppLoading]
  PreloadApps=addressbook,qtmail
  \endcode

  \section1 Background Applications

  Background applications allow simple QCop messages to be sent to an
  application during startup, whenever it terminates or at system shutdown.
  Background applications allow Qtopia to launch and control simple daemon-style
  servers.

  Background applications are configured in the
  \c {Trolltech/BackgroundApplications} configuration file.  The file has the
  following form:

  \code
  [General]
  Count = <daemon count>

  [Application<X>]
  ApplicationName=<Application Name>
  ServiceName=<Service Name>
  TaskGroup=<Qtopia Task Group>
  Silent=<true/false>
  StartupMessage=<Message>
  RestartMessage=<Message>
  ShutdownMessage=<Message>
  \endcode

  The \c {Count} argument specifies the number of background applications that
  follow.  Settings for each are grouped under \c {Application<X>} sections,
  where \c X is from \c 0 to \c {Count - 1}.  For example, if \c {Count} were
  two, there should be \c {Application0} and \c {Application1} sections.

  For each configured application, the following options are available:
  \table
  \header \o Option \o Description
  \row \o \c {ApplicationName} \o The name of the application to send the QCop messages to.
  \row \o \c {ServiceName} \o A name of a service to send QCop messages to.  The default provider of the service will be used.  If both \c {ApplicationName} and \c {ServiceName} are specified, the \c {ServiceName} option is ignored.
  \row \o \c {TaskGroup} \o The name of the group to which the background application belogs.  Group names allow a collection of applications to be started together at startup by inserting the group name into the \c {Tasks.cfg} Qtopia startup file.  Task groups are not optional.
  \row \o \c {Silent} \o If true, warning messages about this application will be suppressed.  For example, if the application or service cannot be found a qWarning() will only appear if \c {Silent} is false or omitted.
  \row \o \c {StartupMessage} \o A message to send when the application is started.
  \row \o \c {RestartMessage} \o A message to send when the application exits.  This message is optional and will only be sent if the application had previously been started with \c {StartupMessage}.
  \row \o \c {ShutdownMessage} \o A message to send at system shutdown.  This message is optional and will only be sent if the application had previously been started with \c {StartupMessage}.  Following this message, the application is expected to exit within 5 seconds.
  \endtable
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*!
  Create the StartupApplications class with the specified \a parent.
 */
StartupApplications::StartupApplications(QObject *parent)
: QObject(parent)
{
    StartupApplicationsPrivate *d = new StartupApplicationsPrivate(this);
    QObject::connect(d, SIGNAL(preloadCrashed(QString)),
                     this, SIGNAL(preloadCrashed(QString)));
    QtopiaServerApplication::addAggregateObject(this, d);
    QtopiaServerApplication::addAggregateObject(this, daemonLauncher());

    QSettings cfg("Trolltech","Launcher");
    cfg.beginGroup("AppLoading");
    QStringList apps = cfg.value("PreloadApps").toString().split(',', QString::SkipEmptyParts);
    foreach (QString app, apps) {
        QtopiaIpcEnvelope e("QPE/Application/"+app, "enablePreload()");
    }
}

/*!
  \fn void StartupApplications::preloadCrashed(const QString &application)

  Emitted whenever a preloaded \a application crashes.  A crashing preloaded
  application will be automatically removed from the preloaded application
  list.

  The PreloadApplication class filter crashes, using the
  ApplicationTerminationHandler interface.
 */

QTOPIA_TASK(StartupApplications, StartupApplications);
QTOPIA_TASK_PROVIDES(StartupApplications, StartupApplications);
QTOPIA_TASK_PROVIDES(StartupApplications, ApplicationTerminationHandler);
QTOPIA_TASK_PROVIDES(StartupApplications, SystemShutdownHandler);

#include "startupapps.moc"
