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

#include "standarddevicefeatures.h"
#include "qtopiaserverapplication.h"
#include "cameramonitor.h"
#include "qtopiainputevents.h"

#include "systemsuspend.h"

#include <qvaluespace.h>

#include <QDebug>
#include <QtopiaFeatures>
#include <QPowerStatus>
#include <QtopiaTimer>

static bool cameraMonitor = true;
static bool clamFlipKeyMonitor = true;
static bool uiTimeValues = true;
static bool inputFeatures = true;

class TimeControl : public QObject
{
Q_OBJECT
public:
    TimeControl(QObject *parent = 0);

private slots:
    void doTimeTick();
    void dateFormatChanged();
    void clockChanged(bool);
    void systemSuspending();
    void systemWaking();

private:
    QtopiaTimer *m_timer;
    QValueSpaceObject timeValueSpace;
};

class StandardDeviceFeaturesImpl : public QObject, public QtopiaKeyboardFilter
{
    Q_OBJECT
public:
    StandardDeviceFeaturesImpl(QObject *parent=0);

    void disableCameraMonitor();
    void disableClamshellMonitor();
    void disableUITimeValues();

    // key filter.
    bool filter(int unicode, int keycode, int modifiers, bool press,
                bool autoRepeat);

private:
    CameraMonitor *camera;
    QValueSpaceItem *backlightVsi;
    QValueSpaceObject *clamshellVso;
    TimeControl *time;
    bool clamOpen;
};

static StandardDeviceFeaturesImpl *sdfi = 0;

StandardDeviceFeaturesImpl::StandardDeviceFeaturesImpl(QObject *parent)
: QObject(parent), camera(0), clamshellVso(0), time(0), clamOpen(true)
{
    sdfi = this;

    if (cameraMonitor) {
        // CameraMonitor keeps the camera QtopiaFeatures upto date
        camera = new CameraMonitor(this);
    }

    if (clamFlipKeyMonitor) {
        QtopiaInputEvents::addKeyboardFilter(this);
        clamshellVso = new QValueSpaceObject("/Hardware/Devices");
        clamshellVso->setAttribute("ClamshellOpen", clamOpen);
    }

    if(uiTimeValues)
        time = new TimeControl(this);

    if(inputFeatures) {
        QSettings btnCfg(Qtopia::defaultButtonsFile(), QSettings::IniFormat);
        btnCfg.beginGroup("Device");
        QStringList ilist = btnCfg.value("Input").toString().split(',', QString::SkipEmptyParts);
        if (!ilist.isEmpty()) {
            for(int ii = 0; ii < ilist.count(); ++ii)
                QtopiaFeatures::setFeature(ilist.at(ii));
        } else {
            if (Qtopia::mousePreferred()) {
                //# ifdef QPE_NEED_CALIBRATION
                QtopiaFeatures::setFeature("Calibrate");
                //# endif
                QtopiaFeatures::setFeature("Touchscreen");
            } else {
                QtopiaFeatures::setFeature("Keypad");
            }
        }
    }
}

void StandardDeviceFeaturesImpl::disableUITimeValues()
{
    delete time;
    time = 0;
}


void StandardDeviceFeaturesImpl::disableCameraMonitor()
{
    delete camera;
    camera = 0;
}

void StandardDeviceFeaturesImpl::disableClamshellMonitor()
{
    delete clamshellVso;
    clamshellVso = 0;
}

bool StandardDeviceFeaturesImpl::filter(int unicode, int keycode,
                                    int modifiers, bool press, bool autoRepeat)
{
    Q_UNUSED(unicode);
    Q_UNUSED(modifiers);
    Q_UNUSED(autoRepeat);

    if (clamFlipKeyMonitor && keycode == Qt::Key_Flip) {
        clamOpen = !press;
        clamshellVso->setAttribute("ClamshellOpen", clamOpen);
    }

    return false;
}

QTOPIA_TASK(StandardDeviceFeatures, StandardDeviceFeaturesImpl);

/*!
  \namespace StandardDeviceFeatures
  \ingroup QtopiaServer
  \brief The StandardDeviceFeatures namespace contains methods to disable
  Qtopia's standard device feature handling.

  The core Qtopia sub-systems rely on various device specific status
  information being maintained in the value space.
  To ease customizability, all the simple hardware device monitoring
  in Qtopia are collectively managed in one place - the
  \c {StandardDeviceFeatures} server task.

  To customize hardware device monitoring, an integrator needs only to implement their
  own version and disable Qtopia's default behaviour through one of the methods
  available in the StandardDeviceFeatures namespace.

  The documentation for each of the following methods lists both what
  device it monitors, as well as the expected result when the device state
  changes.  The intent is to make the task of replicating each very simple.
 */

/*!
  The camera monitor detects when camera are available on the device and
  advertises them using the QtopiaFeatures API as the "Camera" feature.

  Invoking this method will disable the default camera monitoring.
 */
void StandardDeviceFeatures::disableCameraMonitor()
{
    cameraMonitor = false;
    if (sdfi) sdfi->disableCameraMonitor();
}

/*!
  The clamshell monitor is responsible monitoring when a clamshell style
  phone (flip phone) is opened or closed and updating the value space.
  The value that must be maintained is:

  \list
  \i /Hardware/Device/ClamshellOpen - true if the clamshell is in an open state.
  \endlist

  The default clamshell monitoring uses Qt::Key_Flip press events
  to indicate a closed state and Qt::Key_Flip release events to indicate
  an open state.

  Invoking this method will disable the default clamshell monitoring.
 */
void StandardDeviceFeatures::disableClamshellMonitor()
{
    clamFlipKeyMonitor = false;
    if (sdfi) sdfi->disableClamshellMonitor();
}

/*!
  The current system date and time is kept updated in the following value space
  keys:

  \table
  \header \o Key \o Description
  \row \o \c {/UI/DisplayTime/Time} \o The current time with minute resolution (eg "22:13").
  \row \o \c {/UI/DisplayTime/Date} \o The current date in expanded form (eg. "31 Aug 06").
  \row \o \c {/UI/DisplayTime/BriefDate} \o The current date in reduced form (eg. "31/08/06").
  \endtable

  While it is possible, and correct, for applications to source this
  information directly through the QTime and QDate APIs, these values can be
  used for displaying the time in title bars or other incidental locations.
  By doing so, these time or date displays will remain in sync with one
  another.

  Invoking this method will disable the creation and updating of the listed
  value space keys.
*/
void StandardDeviceFeatures::disableUITimeValues()
{
    uiTimeValues = false;
    if (sdfi) sdfi->disableUITimeValues();
}

/*!
  The QtopiaFeatures class allows applications to query certain properties about
  the Qtopia configuration.  Based on the configured input modes, the following
  features are set automatically:

  \table
  \header \o Feature \o Description
  \row \o \c {Touchscreen} \o The primary input for the device is via a
  touchscreen.  That is, \c {Qtopia::mousePreferred() == true}.
  \row \o \c {Calibrate} \o The touch screen device requires calibration.  This
  is set if \c {Touchscreen} is set.
  \row \o \c {KeyPad} \o The primary input for the device is via a keypad.  That
  is, \c {Qtopia::mousePreferred() == false}.
  \endtable.

  The device integrator can override these default features by setting the
  \c {Device/Input} value in Qtopia's defaultbuttons configuration file.  The
  value should be a comma separated list of the features to set.  If \i {any}
  features are specified in this manner, the automatic features above are not
  set.

  Invoking this method will disable the setting of features automatically.
 */
void StandardDeviceFeatures::disableInputFeatures()
{
    Q_ASSERT(!sdfi && !"StandardDeviceFeatures::disableInputFeatures() must be called before the instantiation of the StandardDeviceFeatures task.");
    inputFeatures = false;
}

void TimeControl::doTimeTick()
{
    clockChanged(QTimeString::currentAMPM());
    dateFormatChanged();

    m_timer->stop();
    QTime time = QTime::currentTime();
    m_timer->start((60 - time.second()) * 1000 + 500, 
                   QtopiaTimer::PauseWhenInactive);
}

void TimeControl::dateFormatChanged()
{
    QString ldate = QTimeString::localYMD(QDate::currentDate(), QTimeString::Short);
    QString sdate = QTimeString::numberDateString(QDate::currentDate());
    timeValueSpace.setAttribute("Date", ldate);
    timeValueSpace.setAttribute("BriefDate", sdate);
}

void TimeControl::clockChanged(bool)
{
    timeValueSpace.setAttribute("Time",
            QTimeString::localHM(QDateTime::currentDateTime().time(), QTimeString::Short));
}

void TimeControl::systemSuspending()
{
    timeValueSpace.setAttribute("Date", QString());
    timeValueSpace.setAttribute("BriefDate", QString());
    timeValueSpace.setAttribute("Time", QString());
}

void TimeControl::systemWaking()
{
    clockChanged(QTimeString::currentAMPM());
    dateFormatChanged();
}

TimeControl::TimeControl(QObject *parent)
: QObject(parent), m_timer(0), timeValueSpace("/UI/DisplayTime")
{
    m_timer = new QtopiaTimer(this);
    QObject::connect(m_timer, SIGNAL(timeout()), this, SLOT(doTimeTick()));
    doTimeTick();
    QObject::connect(qApp, SIGNAL(timeChanged()), this, SLOT(doTimeTick()));
    QObject::connect(qApp, SIGNAL(dateFormatChanged()), this, SLOT(dateFormatChanged()));
    QObject::connect(qApp, SIGNAL(clockChanged(bool)), this, SLOT(clockChanged(bool)));

    QObject::connect(qtopiaTask<SystemSuspend>(), SIGNAL(systemSuspending()),
                     this, SLOT(systemSuspending()));
    QObject::connect(qtopiaTask<SystemSuspend>(), SIGNAL(systemWaking()),
                     this, SLOT(systemWaking()));
}

#include "standarddevicefeatures.moc"
