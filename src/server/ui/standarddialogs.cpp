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

#include "standarddialogs.h"
#include "shutdownimpl.h"
#include "applicationlauncher.h"
#include "qterminationhandlerprovider.h"
#include "startupapps.h"
#include <QTimer>
#include <QContent>
#include <QContentSet>
#include <QPowerStatus>
#include <QtopiaServiceRequest>
#include <QtopiaTimer>
#include "mediaservicestask.h"
#include "volumeimpl.h"
#include "alertservicetask.h"
#include <QDebug>

static bool shutdownDialog = true;
static bool defaultCrashDialog = true;
static bool preloadCrashDialog = true;
static bool powerAlertDialog = true;
static bool terminationHandlerDialog = true;
static bool volumeDialog = true;

// declare StandardDialogsImpl
class StandardDialogsImpl : public QObject
{
Q_OBJECT
public:
    StandardDialogsImpl(QObject *parent = 0);
    virtual ~StandardDialogsImpl();

    void disablePowerAlert();
    void disableTerminationHandlerDialog();

private slots:
    void shutdownRequested();
    void applicationTerminated(const QString &name,
                               ApplicationTypeLauncher::TerminationReason,
                               bool filtered);
    void preloadTerminated(const QString &name);
    void powerChanged();
    void applicationTerminated(const QString &name, const QString &text,
                               const QPixmap &icon, const QString &buttonText,
                               QtopiaServiceRequest &buttonAction);
    void volumeChanged(bool);

private:
    QMessageBox *ata;
    QStringList ata_list;
    QMessageBox *dpa;
    QPowerStatus *powerstatus;
    QtopiaTimer dpaTimer;
    QTerminationHandlerProvider *thp;
};

// define StandardDialogsImpl
static StandardDialogsImpl *sdi = 0;

StandardDialogsImpl::StandardDialogsImpl(QObject *parent)
: QObject(parent), ata(0), dpa(0), thp(0)
{
    sdi = this;

    if(shutdownDialog) {
        QObject::connect(QtopiaServerApplication::instance(), SIGNAL(shutdownRequested()), this, SLOT(shutdownRequested()));
    }

    if(volumeDialog && qtopiaTask<MediaServicesTask>()) {
        // QObject::connect(qtopiaTask<MediaServicesTask>(), SIGNAL(volumeChanged(bool)), this, SLOT(volumeChanged(bool)));
         // NOTE: The Volume Widget is now modified via ValueSpace Item: /Volume/GlobalVolume
         static VolumeDialogImpl *vd = 0;
         if ( !vd ) {
             vd = new VolumeDialogImpl(0, Qt::Tool| Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
         }

    }

    if(defaultCrashDialog) {
        ApplicationLauncher *launcher = qtopiaTask<ApplicationLauncher>();
        if(launcher)
            QObject::connect(launcher, SIGNAL(applicationTerminated(QString,ApplicationTypeLauncher::TerminationReason,bool)), this, SLOT(applicationTerminated(QString,ApplicationTypeLauncher::TerminationReason,bool)));
    }

    if(preloadCrashDialog) {
        StartupApplications *apps = qtopiaTask<StartupApplications>();
        if(apps)
            QObject::connect(apps, SIGNAL(preloadCrashed(QString)), this, SLOT(preloadTerminated(QString)));
    }

    if(powerAlertDialog) {
        QSettings cfg("Trolltech", "HardwareAccessories");
        int redisplay = 
            cfg.value("PowerAlertDialog/DisplayPeriod", 300).toInt();
        if(redisplay > 0) {
            powerstatus = new QPowerStatus(0);
            dpaTimer.setInterval(redisplay * 1000, 
                                 QtopiaTimer::PauseWhenInactive);
            QObject::connect(&dpaTimer, SIGNAL(timeout()), this, SLOT(powerChanged()));
            QObject::connect(powerstatus, SIGNAL(batteryStatusChanged(QPowerStatus::BatteryStatus)), this, SLOT(powerChanged()));
            QObject::connect(powerstatus, SIGNAL(batteryChargingChanged(bool)),
                             this, SLOT(powerChanged()));
        }
    }

    if(terminationHandlerDialog) {
        thp = new QTerminationHandlerProvider(this);
        QObject::connect(thp, SIGNAL(applicationTerminated(QString,QString,QPixmap,QString,QtopiaServiceRequest&)), this, SLOT(applicationTerminated(QString,QString,QPixmap,QString,QtopiaServiceRequest&)));
    }
}

StandardDialogsImpl::~StandardDialogsImpl()
{
    if (ata) {
	delete ata;
	ata = 0;
    }
    if (dpa) {
	delete dpa;
	dpa = 0;
    }
}

void StandardDialogsImpl::powerChanged()
{
    if(dpa) {
        delete dpa;
        dpa = 0;
    }

    if ( !powerstatus )
        return;

    QString str;
    if(!powerstatus->batteryCharging()) {
        QSettings cfg("Trolltech", "HardwareAccessories");
        switch(powerstatus->batteryStatus()) {
            case QPowerStatus::VeryLow:
                str = tr( "Battery is running very low." );
                dpaTimer.setInterval(cfg.value("PowerAlertDialog/DisplayPeriod", 300).toInt() * 1000,
                                     QtopiaTimer::PauseWhenInactive);
                break;
            case QPowerStatus::Critical:
                str = tr( "Battery level is critical!\n"
                          "Please recharge now!" );
                dpaTimer.setInterval(cfg.value("PowerAlertDialog/CriticalDisplayPeriod", 60).toInt() * 1000);
                break;
            default:
                break;
        }
    }

    if(!str.isEmpty()) {
        dpa = new QMessageBox( tr("Battery Status"), tr("Low Battery"),
                               QMessageBox::Critical,
                               QMessageBox::Ok | QMessageBox::Default,
                               QMessageBox::NoButton, QMessageBox::NoButton,
                               0 );
        dpa->setText("<qt>" + str + "</qt>");

        Qtopia::soundAlarm();

        dpaTimer.start();
        dpa->show();
    } else {
        dpaTimer.stop();
    }
}

void StandardDialogsImpl::applicationTerminated(
        const QString &, const QString &text,
        const QPixmap &icon, const QString &buttonText,
        QtopiaServiceRequest &buttonAction)
{
    bool executeAction = !buttonAction.isNull();
    if( !text.isEmpty() ) {
        bool ba = !buttonAction.isNull() && !buttonText.isEmpty();

        QString error_title = tr("Application terminated");

        QMessageBox* box = new QMessageBox(error_title, text, QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default, ba ? QMessageBox::No : QMessageBox::NoButton, QMessageBox::NoButton);

        if(ba)
            box->setButtonText(QMessageBox::No, buttonText);
        if(!icon.isNull() )
            box->setIconPixmap(icon);

        executeAction = box->exec() == QMessageBox::No;
    }

    if(executeAction)
        buttonAction.send();
}

void StandardDialogsImpl::disableTerminationHandlerDialog()
{
    if(thp) { delete thp; thp = 0; }
}

void StandardDialogsImpl::disablePowerAlert()
{
    if(powerstatus) { delete powerstatus; powerstatus = 0; }
    dpaTimer.stop();
    if(dpa) { delete dpa; dpa = 0; }
}

void StandardDialogsImpl::shutdownRequested()
{
    if(shutdownDialog) {
        ShutdownImpl *sd = new ShutdownImpl(0);
        sd->setAttribute(Qt::WA_DeleteOnClose);
        connect(sd, SIGNAL(shutdown(QtopiaServerApplication::ShutdownType)),
                QtopiaServerApplication::instance(), SLOT(shutdown(QtopiaServerApplication::ShutdownType)));
        sd->showMaximized();
    }
}


void StandardDialogsImpl::volumeChanged(bool up)
{
    if (volumeDialog && qtopiaTask<MediaServicesTask>()) {
        static VolumeDialogImpl *vd = 0;
        if ( !vd ) {
            vd = new VolumeDialogImpl(0, Qt::Tool| Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
        }
        vd->setVolume(up);
        vd->show();
    }
}

/*!
  \internal
  Raise the "Application terminated" message in an information
  message box. Don't raise it if the application terminated
  normally or because it was killed.
 */
void StandardDialogsImpl::applicationTerminated(const QString &name,
        ApplicationTypeLauncher::TerminationReason reason,
        bool filtered)
{
    if (ApplicationTypeLauncher::Normal == reason ||
	ApplicationTypeLauncher::Killed == reason ||
	!defaultCrashDialog || filtered)
        return;

    QContent app(name,false);
    if(app.isNull()) return;
    QString appname = Qtopia::dehyphenate(app.name());

    if ( !ata || !ata->isVisible() )
        ata_list.clear();

#ifndef QT_BUG_QMessageBoxSetTextNoUpdate
    delete ata;
    ata = 0;
#endif

    if ( !ata )
        ata = new QMessageBox(QString(), QString(), QMessageBox::Critical, QMessageBox::Ok | QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
    QString error_title;
    QString error_details;
    if ( !ata_list.contains(appname) )
        ata_list.append(appname);
    QString l = ata_list.join(", ");
    if ( ata_list.count() == 1 ) {
        error_title = tr("Application terminated");
        error_details = tr("<qt><b>%1</b> was terminated due to application error.</qt>").arg(l);
    } else {
        error_title = tr("Applications terminated");
        error_details = tr("<qt><b>%1</b> were terminated due to application errors.</qt>").arg(l);
    }

    ata->setWindowTitle(error_title);
    ata->setText(error_details);
    ata->show();
}

void StandardDialogsImpl::preloadTerminated(const QString &name)
{
    if(!preloadCrashDialog) return;

    QContent app(name,false);
    if(app.isNull()) return;
    QString appname = Qtopia::dehyphenate(app.name());

    QString error_title = tr("Application terminated");
    QString error_details = tr("<qt><b>%1</b> was terminated due to application error.  (Fast loading has been disabled for this application. Tap and hold the application icon to reenable it.)</qt>").arg(appname);

    QMessageBox::information(0, error_title, error_details);
}

QTOPIA_TASK(StandardDialogs, StandardDialogsImpl);


// define StandardDialogs
/*!
  \namespace StandardDialogs
  \ingroup QtopiaServer
  \brief The StandardDialogs namespace contains methods to disable Qtopia's standard dialog popups.

  The core Qtopia sub-systems often generate information that needs to be
  displayed to the user.  To ease customizability, all the simple dialogs and
  notifications in Qtopia are collectively managed in one place - the
  \c {StandardDialogs} server task.

  To customize one of these dialogs, an integrator needs only to implement their
  own version and disable Qtopia's default behaviour through one of the methods
  available in the StandardDialogs namespace.

  The documentation for each of the following methods lists both what the
  dialog it disables is, as well as the system behaviour to which it responds.
  The intent is to make the task of replicating each very simple.
 */

/*!
  The shutdown dialog allows users to terminate Qtopia, restart Qtopia, shutdown
  the system or reboot the system.  The default implementation is a simple
  dialog allowing the user to select from one of these four options.

  The shutdown dialog appears in response to the
  QtopiaServerApplication::shutdownRequested() signal.  Once the user has
  selected their desired option, the dialog invokes the
  QtopiaServerApplication::shutdown() method and closes.

  Invoking this method will disable the shutdown dialog.
 */
void StandardDialogs::disableShutdownDialog()
{
    shutdownDialog = false;
}

/*!
  The volume dialog provides users the feedback on volume changes.

  The volume dialog appears in response
  to the MediaServicesTask::volumeChanged() signal.
*/
void StandardDialogs::disableVolumeDialog()
{
    volumeDialog = false;
}

/*!
  The default crash handler dialog appears whenever an unfiltered application
  crash occurs.  An application crash may be filtered by the
  ApplicationTerminationHandler interface.  The default implementation is a
  standard QMessageBox alerting the user to the name of the crashed application.

  The dialog appears whenever the ApplicationLauncher::applicationTerminated()
  signal is emitted with the filtered parameter set to false.

  Invoking this method will disable the default crash dialog.
 */
void StandardDialogs::disableDefaultCrashDialog()
{
    defaultCrashDialog = false;
}

/*!
  The preload crash dialog appears whenever a preloaded application crashes.
  The default implementation is a standard QMessageBox alerting the user to the
  name of the crashed application, and informing them that it has been removed
  from the preload list.

  The dialog appears whenever the StartupApplications::preloadCrashed() signal
  is emitted.

  Invoking this method will diable the preload crash dialog.
 */
void StandardDialogs::disablePreloadCrashDialog()
{
    preloadCrashDialog = false;
}

/*!
  The power alert dialog appears whenever the device battery runs low.  The
  default implementation is a standard QMessageBox and an audible sound
  alerting the user to the low power situation.

  The dialog appears when the battery level is very low or critical.
  The dialog will be redisplayed every five minutes, when very low, or every
  minute, when critical, until the power state improves or the device is placed
  on a charger.  The default implementation uses the QPowerStatus to determine
  this information.

  The period at which the power alert dialog redisplays can be configured by
  setting the \c {PowerAlertDialog/DisplayPeriod} and
  \c {PowerAlertDialog/CriticalDisplayPeriod} values to the period in seconds
  in the \c {Trolltech/HardwareAccessories} configuration file.  If not
  specified the default values of 300 seconds (5 minutes) and 60 seconds are
  used for \c {PowerAlertDialog/DisplayPeriod} and
  \c {PowerAlertDialog/CriticalDisplayPeriod} respectively.  Setting
  \c {PowerAlertDialog/DisplayPeriod} to 0 disables the power alert dialog.

  Invoking this method will disable the power alert dialog.
*/
void StandardDialogs::disablePowerAlertDialog()
{
    powerAlertDialog = false;
    if(sdi) sdi->disablePowerAlert();
}

/*!
  Applications can install custom termination messages using the
  QTerminationHandler class.  The default implementation displays a standard
  QMessageBox alerting th euser to the termination and the specified reason.

  The dialog appears whenever the QTerminationHandlerProvider class emits the
  QTerminationHandlerProvider::applicationTerminated() signal.

  Invoking this method will disable the default termination handler dialog.
 */
void StandardDialogs::disableTerminationHandlerDialog()
{
    terminationHandlerDialog = false;
    if(sdi) sdi->disableTerminationHandlerDialog();
}

#include "standarddialogs.moc"
