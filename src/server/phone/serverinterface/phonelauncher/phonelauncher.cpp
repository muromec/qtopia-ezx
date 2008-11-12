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

#include <qtopiaapplication.h>

#include "qtopiaserverapplication.h"
#include <qtopiaipcenvelope.h>
#include <qtopiaipcadaptor.h>
#include <QtopiaItemDelegate>
#include <qtranslatablesettings.h>
#include <qtopiaservices.h>
#include <qdocumentproperties.h>
#include <qcategoryselector.h>
#include <qsoftmenubar.h>
#include <qtopianamespace.h>
#include <qtopiaservices.h>
#include <qspeeddial.h>
#include <qtopialog.h>
#include <qtopianamespace.h>
#include <custom.h>
#include <themedview.h>
#include <qdrmcontent.h>
#include <qwaitwidget.h>
#include <QPhoneProfile>
#include <QPhoneProfileManager>
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
#include <QExportedBackground>
#else
#include <QGlobalPixmapCache>
#endif
#include <qpainter.h>
#include <qdir.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qaction.h>
#include <qevent.h>
#include <qpixmapcache.h>
#ifndef MEDIAPLAYER_RINGTONES
#include <qsoundqss_qws.h>
#endif
#include <qscreen_qws.h>
#include <qvariant.h>
#include <QStackedWidget>
#include <QTimer>
#include <QDesktopWidget>
#include <QDebug>
#include <QMenu>
#include <QScrollArea>

#include "phonelauncher.h"
#include "windowmanagement.h"
#include "contextlabel.h"
#include "messagebox.h"

#include "contentserver.h"
#include "phonebrowser.h"
#include "homescreencontrol.h"
#include "qabstracthomescreen.h"
#include "documentview.h"
#include "phoneheader.h"
#include "alarmcontrol.h"
#include "taskmanagerservice.h"
#include "themecontrol.h"
#include "qabstractsecondarydisplay.h"
#include "contentsetlauncherview.h"
#include "themebackground_p.h"
#include "lowmemorytask.h"
#include "qtopiainputevents.h"
#include "qabstractcallpolicymanager.h"

#ifdef QTOPIA_VOIP
#include "voipmanager.h"
#endif

#ifdef QTOPIA_PHONEUI
#include "ringcontrol.h"
#include "qabstractdialerscreen.h"
#include "dialercontrol.h"
#include "dialerservice.h"
#include "callscreen.h"
#if defined(QTOPIA_TELEPHONY)
#include "callhistory.h"
#endif
#include "dialercontrol.h"
#include "messagecontrol.h"
#endif

#ifdef QTOPIA_CELL
#include "gsmkeyactions.h"
#endif

static const int NotificationVisualTimeout = 0;  // e.g. New message arrived, 0 == No timeout
static const int WarningTimeout = 5000;  // e.g. Cannot call

class RejectDlg : public QDialog
{
public:
    void rejectDlg() { reject(); }
    void hideDlg() { hide(); }
};


// declare MultiTaskProxy
class MultiTaskProxy : public TaskManagerService
{
Q_OBJECT
public:
    MultiTaskProxy(QObject * = 0);

protected:
    virtual void multitask();
    virtual void showRunningTasks();

signals:
    void doMultiTask();
    void doShowRunningTasks();
};

#ifdef QTOPIA_PHONEUI
// declare DialerServiceProxy
class DialerServiceProxy : public DialerService
{
Q_OBJECT
public:
    DialerServiceProxy(QObject * = 0);

protected:
    virtual void dialVoiceMail();
    virtual void dial( const QString& name, const QString& number );
    virtual void dial( const QString& number, const QUniqueId& contact );
    virtual void showDialer( const QString& digits );

signals:
    void doDialVoiceMail();
    void doDial( const QString& number );
    void doDial( const QString& number, const QUniqueId& contact );
    void doShowDialer( const QString& digits );
};

// define DialerServiceProxy
DialerServiceProxy::DialerServiceProxy(QObject *parent)
: DialerService(parent)
{
}

void DialerServiceProxy::dialVoiceMail()
{
    emit doDialVoiceMail();
}

void DialerServiceProxy::dial( const QString&, const QString& number )
{
    emit doDial(number);
}

void DialerServiceProxy::dial( const QString& number, const QUniqueId& contact )
{
    emit doDial(number, contact);
}

void DialerServiceProxy::showDialer( const QString& digits )
{
    if( !DialerControl::instance()->hasIncomingCall() &&
        !DialerControl::instance()->isDialing() ) {
        emit doShowDialer(digits);
    }
}
#endif

/*!
  \class PhoneLauncher
  \brief The PhoneLauncher class implments the main UI widget for the Qtopia Phone Edition.
  \ingroup QtopiaServer::PhoneUI

  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface
  */

/*!
  \internal
  */
PhoneLauncher::PhoneLauncher(QWidget *parent, Qt::WFlags fl)
    : QAbstractServerInterface(parent, fl), updateTid(0), m_header(0),
      m_context(0), stack(0), m_homeScreen(0), 
      registrationMsgId(0),
#ifdef QTOPIA_PHONEUI
      messageCount(0), activeCalls(0),
      newMessages("Communications/Messages/NewMessages"),
#endif
#ifdef QTOPIA_PHONEUI
    serviceMsgBox(0), CBSMessageBox(0),
    mCallScreen(0), m_dialer(0),
#endif
    secondDisplay(0),
#ifdef QTOPIA_PHONEUI
    mCallHistory(0),
    missedMsgBox(0), messagesMsgBox(0), incomingMsgBox(0),
    dialingMsgBox(0), callTypeMsgBox(0),
#endif
#ifdef QTOPIA_VOIP
    voipNoPresenceMsgBox(0),
#endif
    warningMsgBox(0),
#ifdef QTOPIA_PHONEUI
    alertedMissed(0),
    messageBoxFull(false),
    isSysMsg(false),
    queuedIncoming(false),
    waitingVoiceMailNumber(false),
#endif
    speeddialfeedback(0)
#ifdef QTOPIA_CELL
    , cellModem(0), gsmKeyActions(0)
#endif
#ifdef QTOPIA_PHONEUI
    , dialerSpeedDialFeedbackActive(false)
#endif
{
    QDesktopWidget *desktop = QApplication::desktop();
    QRect desktopRect = desktop->screenGeometry(desktop->primaryScreen());
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
    QExportedBackground::initExportedBackground(desktopRect.width(),
                                                desktopRect.height(),
                                                desktop->primaryScreen());
#else
    {
        //Needed to initialise cache!
        QPixmap pm;
        if(!QGlobalPixmapCache::find("qtopia", pm)) {
            pm.load("qtopia.png");
            QGlobalPixmapCache::insert("qtopia", pm);
        }
    }

#endif
    // Create callscreen
//    callScreen();

#ifdef QTOPIA_CELL
    CellBroadcastControl *cbc = CellBroadcastControl::instance();
    connect(cbc,
            SIGNAL(broadcast(CellBroadcastControl::Type,QString,QString)),
            this,
            SLOT(cellBroadcast(CellBroadcastControl::Type,QString,QString)));

    QSupplementaryServices *supp = new QSupplementaryServices( "modem", this );
    connect( supp, SIGNAL(unstructuredNotification(QSupplementaryServices::UnstructuredAction,QString)),
             this, SLOT(unstructuredNotification(QSupplementaryServices::UnstructuredAction,QString)) );
#endif

#ifndef QT_NO_TRANSLATION //load translation for profile names
    QtopiaApplication::loadTranslations("QtopiaDefaults");
#endif
    setGeometry(desktopRect);

    QObject::connect(ThemeControl::instance(), SIGNAL(themeChanged()),
                     this, SLOT(loadTheme()));

    // Create phone header
    header();

    // Create phone context bar
    createContext();

    // Create home screen
    m_homeScreen = qtopiaWidget<QAbstractHomeScreen>(this);
    // Homescreen covers the entire screen
    if (m_homeScreen->geometry() != rect())
        m_homeScreen->setGeometry(0, 0, width(), height());
    HomeScreenControl::instance()->setHomeScreen(m_homeScreen);

    QObject::connect(m_homeScreen, SIGNAL(speedDial(QString)),
                     this, SLOT(activateSpeedDial(QString)));
    QObject::connect(m_homeScreen, SIGNAL(showPhoneBrowser()),
                     this, SLOT(showPhoneLauncher()));
    QObject::connect(m_homeScreen, SIGNAL(keyLockedChanged(bool)),
                     this, SLOT(keyStateChanged(bool)));
#ifdef QTOPIA_PHONEUI
    QObject::connect(m_homeScreen, SIGNAL(showCallScreen()),
                     this, SLOT(showCallScreen()));
    QObject::connect(m_homeScreen, SIGNAL(showMissedCalls()),
                     this, SLOT(showMissedCalls()));
    QObject::connect(m_homeScreen, SIGNAL(showCallHistory()),
                     this, SLOT(showCallHistory()));
    QObject::connect(m_homeScreen, SIGNAL(dialNumber(QString)),
                     this, SLOT(showSpeedDialer(QString)));
    QObject::connect(m_homeScreen, SIGNAL(callEmergency(QString)),
                    this, SLOT(requestDial(QString)));
#endif

    // Create secondary display
    secondaryDisplay();

    // Implement multi task service
    MultiTaskProxy *multiTaskProxy = new MultiTaskProxy(this);
    connect(multiTaskProxy, SIGNAL(doMultiTask()),
            this, SLOT(multitaskPressed()));
    connect(multiTaskProxy, SIGNAL(doShowRunningTasks()),
            this, SLOT(showRunningTasks()));

#ifdef QTOPIA_PHONEUI
    // Implement dialer service
    DialerServiceProxy *dialerServiceProxy = new DialerServiceProxy(this);
    connect(dialerServiceProxy, SIGNAL(doDialVoiceMail()),
            this, SLOT(dialVoiceMail()));
    connect(dialerServiceProxy, SIGNAL(doDial(QString)),
            this, SLOT(requestDial(QString)));
    connect(dialerServiceProxy, SIGNAL(doDial(QString,QUniqueId)),
            this, SLOT(requestDial(QString,QUniqueId)));
    connect(dialerServiceProxy, SIGNAL(doShowDialer(QString)),
            this, SLOT(showDialer(QString)));
#endif

    // Listen to system channel
    QtopiaChannel* sysChannel = new QtopiaChannel( "QPE/System", this );
    connect( sysChannel, SIGNAL(received(QString,QByteArray)),
             this, SLOT(sysMessage(QString,QByteArray)) );

    phoneBrowser()->resetToView("Main"); // better to get initial icon load cost now, rather then when user clicks.

    showHomeScreen(0);
    m_homeScreen->setFocus();

#ifdef QTOPIA_PHONEUI
    connect(DialerControl::instance(), SIGNAL(missedCount(int)),
            this, SLOT(missedCount(int)));
    connect(MessageControl::instance(), SIGNAL(messageCount(int,bool,bool,bool)),
            this, SLOT(messageCountChanged(int,bool,bool,bool)));
    connect(MessageControl::instance(), SIGNAL(smsMemoryFull(bool)),
            this, SLOT(smsMemoryFull(bool)));
    connect(MessageControl::instance(), SIGNAL(messageRejected()),
            this, SLOT(messageRejected()));
    connect(DialerControl::instance(), SIGNAL(activeCount(int)),
            this, SLOT(activeCallCount(int)));
    connect(DialerControl::instance(), SIGNAL(autoAnswerCall()),
            this, SLOT(acceptIncoming()));
    connect(DialerControl::instance(), SIGNAL(stateChanged()),
            this, SLOT(stateChanged()));
#endif

#ifdef QTOPIA_CELL
    cellModem = qtopiaTask<CellModemManager>();
    connect(cellModem, SIGNAL(planeModeEnabledChanged(bool)),
            this, SLOT(registrationChanged()));
    gsmKeyActions = new GsmKeyActions(this);
#endif

#if defined(QTOPIA_TELEPHONY)
    // Hook onto registration state changes for all call policy managers.
    // The standard ones are CellModemManager and VoIPManager.
    QList<QAbstractCallPolicyManager *> managers;
    managers = qtopiaTasks<QAbstractCallPolicyManager>();
    foreach (QAbstractCallPolicyManager *manager, managers) {
        connect(manager,
                SIGNAL(registrationChanged(QTelephony::RegistrationState)),
                this,
                SLOT(registrationChanged()));
    }
#endif

#ifdef QTOPIA_PHONEUI
    // Voice mail
    serviceNumbers = new QServiceNumbers( QString(), this );
    connect( serviceNumbers, SIGNAL(serviceNumber(QServiceNumbers::NumberId,QString)),
             this, SLOT(serviceNumber(QServiceNumbers::NumberId,QString)) );

    m_homeScreen->setNewMessages(newMessages.value().toInt());

    connect(&newMessages, SIGNAL(contentsChanged()),
            this, SLOT(newMessagesChanged()));

    // Don't alert user until count changes.
    alertedMissed = DialerControl::instance()->missedCallCount();
#endif
//    ThemeControl::instance()->refresh();

    connect( ContentSetViewService::instance(), SIGNAL(showContentSet()),
             this, SLOT(showContentSet()) );

    LowMemoryTask* lmt = qtopiaTask<LowMemoryTask>();
    if (!lmt)
	qWarning() << "NO LOWMEMORYTASK";
    connect(lmt,SIGNAL(showWarning(QString,QString)),this,SLOT(showWarning(QString,QString)));

#ifdef QTOPIA_PHONEUI
    registrationChanged();
#endif

    loadTheme();
}

/*!
  \internal
  */
PhoneLauncher::~PhoneLauncher()
{
    delete stack;
    delete m_header;
    delete m_context;
#ifdef QTOPIA_PHONEUI
    delete mCallScreen;
#if defined(QTOPIA_TELEPHONY)
    if(mCallHistory)
        delete mCallHistory;
#endif
    if( m_dialer )
        delete m_dialer;
#endif
    delete speeddialfeedback;
    if ( secondDisplay )
        delete secondDisplay;
}

/*!
  \internal
  */
void PhoneLauncher::showEvent(QShowEvent *e)
{
    QTimer::singleShot(0, m_homeScreen, SLOT(show()));
    QTimer::singleShot(0, m_homeScreen, SLOT(applyHomeScreenImage()));
    QTimer::singleShot(0, this, SLOT(updateBackground()));
    if (secondDisplay)
        QTimer::singleShot(0, this, SLOT(applySecondaryBackgroundImage()));
#ifdef QTOPIA_PHONEUI
    QTimer::singleShot(0, this, SLOT(initializeCallHistory()));
#endif
    header()->show();
    if (QSoftMenuBar::keys().count())
        context()->show();
    QWidget::showEvent(e);
}

/*!
  \internal
  */
void PhoneLauncher::callPressed()
{
#ifdef QTOPIA_PHONEUI
    // Called if server windows are not on top
    showCallHistory();
#endif
}

/*!
  \internal
  */
void PhoneLauncher::hangupPressed()
{
    // Called if server windows are not on top
#ifdef QTOPIA_PHONEUI
    DialerControl *control = DialerControl::instance();
    if (control->isConnected() || control->isDialing() ||
        control->hasIncomingCall()) {
        // We have an in-progress call, so hang it up rather than
        // close all running applications.
        showCallScreen();
        control->endCall();
    } else
#endif
    {
        topLevelWidget()->raise();
        QtopiaIpcEnvelope e("QPE/System","close()");
        hideAll();
        showHomeScreen(0);
    }
}

/*!
  \internal
  */
void PhoneLauncher::multitaskPressed()
{
    // When pressed, home screen is shown (without quiting apps).
    // When pressed again soon after (X msec), next running app is shown.

    QStringList runningApps = appMon.runningApplications();

    QStringList sortedapps = runningApps;
    sortedapps.removeAll("sipagent"); // workaround
    sortedapps.sort();

    int to_run=-1;
    if ( m_homeScreen->isActiveWindow() ) {
        // first application
        if ( sortedapps.count() == 0 ) {
            showRunningTasks(); // (gives error message)
            return;
        }
        to_run=multitaskingcursor=0;
    } else if ( multitaskingMultipressTimer.isActive() ) {
        // next application...
        if ( ++multitaskingcursor >= sortedapps.count() ) {
            to_run = -1;
        } else {
            to_run=multitaskingcursor;
        }
    }
    if ( to_run < 0 ) {
        multitaskingcursor=0;
        showHomeScreen(0);
    } else {

        QStringList sortedapps = runningApps;
        sortedapps.sort();
        QContent app(sortedapps[to_run], false);
        multitaskingMultipressTimer.start(2500,this);
        if ( app.isValid() )
            app.execute();
    }
}

/*!
  \internal
  */
void PhoneLauncher::showRunningTasks()
{
    // XXX Should:
    // XXX  - Work out which app was on top when invoked.
    // XXX  - Make "Back" go back to that app.
    // XXX  - Go above StaysOnTop windows (eg. menu popups, QCalendarWidget popups)
    // XXX Difficult, because this is just a folderview.

    hideAll();
    phoneBrowser()->moveToView("Folder/Running");
    phoneBrowser()->showMaximized();
    phoneBrowser()->raise();
    phoneBrowser()->activateWindow();
    phoneBrowser()->topLevelWidget()->setWindowTitle(tr("Running"));
}

/*!
  \internal
  */
void PhoneLauncher::showContentSet()
{
    hideAll();
    phoneBrowser()->moveToView("Folder/ContentSet");
    phoneBrowser()->showMaximized();
    phoneBrowser()->raise();
    phoneBrowser()->activateWindow();
}

/*!
  \internal
  */
void PhoneLauncher::loadTheme()
{
    bool v = isVisible();

    qLog(UI) << "Load theme";
    m_homeScreen->hide();

    QDesktopWidget *desktop = QApplication::desktop();
    QRect desktopRect = desktop->screenGeometry(desktop->primaryScreen());

    // header - not lazy
    WindowManagement::dockWindow(header(), WindowManagement::Top, header()->reservedSize());
    if ( v ) header()->show();

    // context bar - not lazy
    WindowManagement::dockWindow(context(), WindowManagement::Bottom, context()->reservedSize());
    if (v && QSoftMenuBar::keys().count())
        context()->show();

    // home screen - not lazy
    m_homeScreen->setGeometry(desktopRect);
    m_homeScreen->updateInformation();

#ifdef QTOPIA_PHONEUI
    // call screen - lazy
    CallScreen *cs = callScreen(false);
    if(cs) {
        cs->setGeometry(desktopRect.x(), desktopRect.y()+header()->sizeHint().height(),
                        desktopRect.width(),
                        desktopRect.height() - header()->sizeHint().height() - context()->height() );
    }
#endif

    // update position of launcher stack - lazy
    if(phoneBrowser(false)) {
        if ( !phoneBrowser()->isHidden() )
            phoneBrowser()->showMaximized();
        else
            phoneBrowser()->setGeometry(desktopRect.x(), desktopRect.y()+header()->sizeHint().height(),
                        desktopRect.width(),
                        desktopRect.height() - header()->sizeHint().height() - context()->height());
    }

    initInfo();

    if ( v ) {
        QTimer::singleShot(0, m_homeScreen, SLOT(show()));
        QTimer::singleShot(0, this, SLOT(updateBackground()));
    }
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void PhoneLauncher::showMissedCalls()
{
    showCallHistory(true);
}

/*!
  \internal
  Displays the call screen.
*/
void PhoneLauncher::showCallScreen()
{
    if( !callScreen()->sourceLoaded() ) {
        callScreen()->loadSource();
    }
    callScreen()->showMaximized();
    callScreen()->raise();
    callScreen()->activateWindow();
}
#endif

/*!
  \internal
  */
void PhoneLauncher::initInfo()
{
    AlarmControl::instance();

    update();

    timerEvent(0);

#ifdef QTOPIA_PHONEUI
    int missCalls = DialerControl::instance()->missedCallCount();
    if ( missCalls != 0 )
        missedCount(missCalls);
#endif
}

/*!
  \internal
  */
void PhoneLauncher::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}

/*!
  \internal
  */
void PhoneLauncher::closeEvent(QCloseEvent *e)
{
    e->ignore();
}

/*!
  \internal
  */
void PhoneLauncher::sysMessage(const QString& message, const QByteArray &data)
{
    QDataStream stream( data );
    if ( message == "showHomeScreen()" ) {
        hideAll();
        showHomeScreen(0);
    } else if ( message == QLatin1String("showPhoneLauncher()")) {
        showPhoneLauncher();
    } else if ( message == QLatin1String("showHomeScreenAndToggleKeylock()") ) {
        showHomeScreen(2);
    } else if ( message == QLatin1String("showHomeScreenAndKeylock()") ) {
        showHomeScreen(3);
    } else if ( message == "applyStyleSplash()" ) {
        raise();
        QWaitWidget *waitWidget = new QWaitWidget( this );
        waitWidget->show();
        qApp->processEvents();
        ThemeControl::instance()->refresh();
        polishWindows();
        updateLauncherIconSize();
        m_homeScreen->applyHomeScreenImage();
        if (secondDisplay)
            applySecondaryBackgroundImage();
        delete waitWidget;
        lower();
    } else if ( message == "applyStyleNoSplash()" ) {
        qApp->processEvents();
        ThemeControl::instance()->refresh();
        polishWindows();
        updateLauncherIconSize();
        m_homeScreen->applyHomeScreenImage();
        if (secondDisplay)
            applySecondaryBackgroundImage();
    } else if ( message == "applyHomeScreenImage()" ) {
        m_homeScreen->applyHomeScreenImage();
        updateBackground();
    } else if ( message == "applySecondaryBackgroundImage()" ) {
        applySecondaryBackgroundImage();
    } else if ( message == "updateHomeScreenInfo()" ) {
        m_homeScreen->updateHomeScreenInfo();
    } else if ( message == "serverKey(int,int)" ) {
        int key,press;
        stream >> key >> press;
        if ( key == Qt::Key_Call && press ) {
            callPressed();
        } else if ( (key == Qt::Key_Hangup || key == Qt::Key_Flip) && press ) {
            hangupPressed();
        }
    }
}


/*!
  \internal
  */
void PhoneLauncher::showHomeScreen(int state)
{
    // state: 0 -> no screensaver calls
    //        1 -> showHomeScreen called by screensaver
    //        2 -> showHomeScreen called when lock key is toggled
    //        3 -> showHomeScreen called to key lock

#ifdef QTOPIA_PHONEUI
    if (state != 0 && activeCalls) {
        return;
    }
#endif

    rejectModalDialog();
    topLevelWidget()->raise();
    m_homeScreen->raise();
    m_homeScreen->show();
    m_homeScreen->setFocus();
    topLevelWidget()->activateWindow();

    if(phoneBrowser(false)) {
        phoneBrowser()->hide();
    }

    if (state == 1) {
        //QtopiaIpcEnvelope closeApps( "QPE/System", "close()" );
        if (!Qtopia::mousePreferred()) {
            QSettings c("Trolltech","qpe");
            c.beginGroup("HomeScreen");
            QString lockType = c.value( "AutoKeyLock", "Disabled" ).toString();
            if (lockType == "Enabled")
                m_homeScreen->setKeyLocked(true);
        }
    } else if (state == 2 || state == 3) {
        if (!m_homeScreen->keyLocked())
            m_homeScreen->setKeyLocked(true);
        else if (state == 2)
            QtopiaInputEvents::processKeyEvent(0, BasicKeyLock::lockKey(), 0, true, false);
    }
}

/*!
  \internal
  */
void PhoneLauncher::rejectModalDialog()
{
    // Last resort.  We shouldn't have modal dialogs in the server, but
    // just in case we need to get rid of them when a call arrives.  This
    // is a bad thing to do, but far less dangerous than missing a call.
    // XXX Known modals:
    //  - category edit dialog
    QWidgetList list = QApplication::topLevelWidgets();
    QList<QPointer<RejectDlg> > dlgsToDelete;

    foreach(QWidget *w, list)
        if (w->isVisible() && w->inherits("QDialog"))
            dlgsToDelete.append((RejectDlg*)w);

    foreach(QPointer<RejectDlg> d, dlgsToDelete) {
        if (!d)
            continue;

        if (d->testAttribute(Qt::WA_ShowModal)) {
            qWarning("Rejecting modal dialog: %s", d->metaObject()->className());
            d->rejectDlg();
        } else {
            qWarning("Hiding non-modal dialog: %s", d->metaObject()->className());
            d->hideDlg();
        }
    }
}

/*!
  \internal
  */
void PhoneLauncher::showPhoneLauncher()
{
    phoneBrowser()->resetToView("Main");

    phoneBrowser()->showMaximized();
    phoneBrowser()->raise();
    phoneBrowser()->activateWindow();
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void PhoneLauncher::missedCount(int count)
{
    m_homeScreen->setMissedCalls(count);

    if(m_homeScreen->keyLocked()
#ifdef QTOPIA_CELL
        || m_homeScreen->simLocked()
#endif
    ){
        // do not show alert this time
        // but reset missed count so when the phone is unlocked the alert is shown.
        resetMissedCalls();
    } else {
        showAlertDialogs();
    }
}

/*!
  \internal
  */
void PhoneLauncher::messageCountChanged(int count, bool full, bool fromSystem, bool notify)
{ 
    messageBoxFull = full;
    isSysMsg = fromSystem;
    if (count != messageCount) {
        messageCount = count;
        if (notify && messageCount) {
            QtopiaServiceRequest req("Messages","viewNewMessages(bool)");
            req << false;
            req.send();
        }
    }
}

/*!
  \internal
  */
void PhoneLauncher::smsMemoryFull(bool full)
{
    m_homeScreen->setSmsMemoryFull(full);
}

/*!
  \internal
  */
void PhoneLauncher::messageRejected()
{
    showWarning(tr("Message Rejected"),
                tr("<qt>An incoming message was rejected because "
                   "there is insufficient space to store it.</qt>"));
}

/*!
  \internal
  */
void PhoneLauncher::activeCallCount(int count)
{
    activeCalls = count;
}

/*!
  \internal
  */
void PhoneLauncher::registrationChanged()
{
    QList<QAbstractCallPolicyManager *> managers;
    QString pix;
    QString msg;

    managers = qtopiaTasks<QAbstractCallPolicyManager>();
    foreach (QAbstractCallPolicyManager *manager, managers) {
        if (pix.isEmpty())
            pix = manager->registrationIcon();
        QString newMsg = manager->registrationMessage();
        if (!newMsg.isEmpty()) {
            if (!msg.isEmpty())
                msg += "<br>";
            msg.append(newMsg);
        }
    }

    if (registrationMsgId) {
        m_homeScreen->clearInformation(registrationMsgId);
        registrationMsgId = 0;
    }

    if (!msg.isEmpty())
        registrationMsgId = m_homeScreen->showInformation(pix, msg);
}

#endif

/*!
  \internal
  */
void PhoneLauncher::timerEvent(QTimerEvent * tev)
{
    if(tev && tev->timerId() == updateTid) {
        update();
        killTimer(updateTid);
        updateTid = 0;
        return;
    }

    if ( tev && multitaskingMultipressTimer.timerId() == tev->timerId() ) {
        multitaskingMultipressTimer.stop();
        return;
    }
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  Shows the Missed Calls dialog if there are any missed calls.
*/
void PhoneLauncher::showAlertDialogs()
{
    if (isVisible()) {
        if(DialerControl::instance()->missedCallCount() &&
           DialerControl::instance()->missedCallCount() != alertedMissed) {
            alertedMissed = DialerControl::instance()->missedCallCount();
            if (!missedMsgBox) {
                QString missedMsg = tr("Do you wish to view missed calls?");
                missedMsgBox = QAbstractMessageBox::messageBox(m_homeScreen, tr("Missed Call"),
                        "<qt>"+missedMsg+"</qt>", QAbstractMessageBox::Information,
                        QAbstractMessageBox::Yes, QAbstractMessageBox::No);
                connect(missedMsgBox, SIGNAL(finished(int)), this, SLOT(messageBoxDone(int)));
            }
            missedMsgBox->setTimeout(NotificationVisualTimeout, QAbstractMessageBox::No);
            QtopiaApplication::showDialog(missedMsgBox);
        }
    }
}
#endif

/*!
  \internal
  */
void PhoneLauncher::keyStateChanged(bool locked)
{
#ifdef QTOPIA_PHONEUI
    if(!locked)
        showAlertDialogs();
#else
    Q_UNUSED(locked);
#endif
}

/*!
  \internal
  */
void PhoneLauncher::updateBackground()
{
    m_homeScreen->updateBackground();
}

/*!
  \internal
  */
void PhoneLauncher::applySecondaryBackgroundImage()
{
    if ( secondDisplay ) {
        secondDisplay->applyBackgroundImage();
        secondDisplay->updateBackground();
    }
}

/*!
  \internal
  */
void PhoneLauncher::polishWindows()
{
    ThemeBackground::polishWindows(ThemeBackground::PrimaryScreen);
    if ( secondDisplay ) {
        ThemeBackground::polishWindows(ThemeBackground::SecondaryScreen);
    }
}

/*!
  \internal
  */
void PhoneLauncher::paintEvent(QPaintEvent *)
{
    QPixmap pm(":image/qpelogo");
    QPainter p(this);
    p.drawPixmap((width()-pm.width())/2,( height()-pm.height())/2, pm);
}

/*!
  \internal
  */
void PhoneLauncher::updateLauncherIconSize()
{
}

// Creation methods
/*!
  \internal
  */
PhoneHeader *PhoneLauncher::header()
{
    if(!m_header) {
        m_header = new PhoneHeader(0);
        WindowManagement::protectWindow(m_header);
        ThemeControl::instance()->registerThemedView(m_header, "Title");
    }

    return m_header;
}

/*!
  \internal
  */
void PhoneLauncher::createContext()
{
    Q_ASSERT(!m_context);

    m_context = new ContextLabel(0, Qt::FramelessWindowHint |
                                    Qt::Tool |
                                    Qt::WindowStaysOnTopHint );

    m_context->move(QApplication::desktop()->screenGeometry().topLeft()+QPoint(0,50)); // move to the correct screen
    WindowManagement::protectWindow(m_context);
    m_context->setAttribute(Qt::WA_GroupLeader);
    ThemeControl::instance()->registerThemedView(m_context, "Context");
}

/*!
  \internal
  */
ContextLabel *PhoneLauncher::context()
{
    Q_ASSERT(m_context);
    return m_context;
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void PhoneLauncher::showSpeedDialer(const QString &digits)
{
    showDialer(digits, true);
}

/*!
  \internal
  Displays the dialer, preloaded with \a digits.  If \a speedDial
  is true, the digits will be considered for use in speed dial (press and
  hold).
*/
void PhoneLauncher::showDialer(const QString &digits, bool speedDial)
{
    if (!dialer())
        return;

    if(speedDial) {
        dialer()->reset();
        dialer()->appendDigits(digits);
    } else {
        dialer()->setDigits(digits);
    }

    if ( callScreen() )
        dialer()->setGeometry( callScreen()->geometry() );

    dialer()->showMaximized();
    dialer()->raise();
}

/*!
  \internal
  Displays the call history window.  If \a missed is true, the missed
  calls tab will be on top.  If \a hint is not empty, it is used to
  highlight a matching call.
*/
void PhoneLauncher::showCallHistory(bool missed, const QString &hint)
{
#if defined(QTOPIA_TELEPHONY)
    if ( !callHistory() )
        initializeCallHistory();

    callHistory()->reset();
    if (missed || DialerControl::instance()->missedCallCount() > 0)
        callHistory()->showMissedCalls();
    if( hint.length() )
        callHistory()->setFilter( hint );

    callHistory()->refresh();

    if( !callHistory()->isHidden() )
    {
        callHistory()->raise();
    }
    else
    {
        callHistory()->showMaximized();
    }
#else
    Q_UNUSED(missed);
    Q_UNUSED(hint);
#endif
}

/*!
  \internal
  Initialize call history window.
  This delayed initilization will shorten the first launch time.
*/
void PhoneLauncher::initializeCallHistory()
{
#if defined(QTOPIA_TELEPHONY)
    if ( !mCallHistory ) {
        mCallHistory = new CallHistory(DialerControl::instance()->callList(), 0);
        connect(callHistory(), SIGNAL(viewedMissedCalls()),
                this, SLOT(resetMissedCalls()) );
        connect(callHistory(), SIGNAL(viewedMissedCalls()),
                DialerControl::instance(), SLOT(resetMissedCalls()) );
        connect(callHistory(),
                SIGNAL(requestedDial(QString,QUniqueId)),
                this,
                SLOT(requestDial(QString,QUniqueId)));
    }
#endif
}
#endif

#ifdef QTOPIA_CELL

/*!
  \internal
  */
void PhoneLauncher::unstructuredNotification
    ( QSupplementaryServices::UnstructuredAction action, const QString& data )
{
    QString text;
    if ( !data.isEmpty() ) {
        text = data;
    } else {
        switch ( action ) {
            case QSupplementaryServices::TerminatedByNetwork:
                text = tr("Operation terminated by network");
                break;

            case QSupplementaryServices::OtherLocalClientResponded:
                text = tr("Other local client has responded");
                break;

            case QSupplementaryServices::OperationNotSupported:
                text = tr("Operation is not supported");
                break;

            case QSupplementaryServices::NetworkTimeout:
                text = tr("Operation timed out");
                break;

            default:
                text = tr("Network response: %1").arg((int)action);
                break;
        }
    }
    ussdMessage(text);
}

#endif // QTOPIA_CELL

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void PhoneLauncher::ussdMessage(const QString &text)
{
    QString title = tr("Service request");
    QString displayText = "<qt>" + text + "</qt>";
    if (!serviceMsgBox) {
        serviceMsgBox = QAbstractMessageBox::messageBox(0, title, displayText,
                                       QAbstractMessageBox::Information);
    } else {
        serviceMsgBox->setWindowTitle(title);
        serviceMsgBox->setText(displayText);
    }
    QtopiaApplication::showDialog(serviceMsgBox);
}
#endif

#ifdef QTOPIA_CELL

/*!
  \internal
  */
void PhoneLauncher::cellBroadcast(CellBroadcastControl::Type type,
                                  const QString &chan, const QString &text)
{
    if(type != CellBroadcastControl::Popup)
        return;

    // remove the previous message box if there is any
    // user will see the latest one only
    if (CBSMessageBox) {
        delete CBSMessageBox;
        CBSMessageBox = 0;
    }
    CBSMessageBox = QAbstractMessageBox::messageBox(m_homeScreen, chan,
                                   text, QAbstractMessageBox::Information,
                                   QAbstractMessageBox::No);
    QtopiaApplication::showDialog(CBSMessageBox);
}

#endif // QTOPIA_CELL


/*!
  \internal
  Hides the Call History, Dialer and Call Screen.
*/
void PhoneLauncher::hideAll()
{
#ifdef QTOPIA_PHONEUI
    if (callScreen(false))
        callScreen(false)->close();
#if defined(QTOPIA_TELEPHONY)
    if (callHistory())
        callHistory()->close();
#endif
    if (dialer(false))
        dialer(false)->close();
#endif
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void PhoneLauncher::stateChanged()
{
    callScreen()->stateChanged(); // We must know that this is updated before we continue

    if (DialerControl::instance()->hasIncomingCall()) {
        if ((callTypeMsgBox && callTypeMsgBox->isVisible())
#ifdef QTOPIA_VOIP
            || (voipNoPresenceMsgBox && voipNoPresenceMsgBox->isVisible())
#endif
            ) {
            // assume the request to call is cancelled
            queuedCall = QString();
            queuedCallType = QString();
            queuedCallContact = QUniqueId();
        }
        showCallScreen();
    }

    if (queuedIncoming && !DialerControl::instance()->hasIncomingCall()) {
        // incoming call has gone away
        queuedIncoming = false;
        delete incomingMsgBox;
        incomingMsgBox = 0;
    }

    if (queuedIncoming || !queuedCall.isEmpty()) {
        bool haveFg = DialerControl::instance()->hasActiveCalls();
        if (haveFg && !DialerControl::instance()->hasCallsOnHold()) {
            DialerControl::instance()->hold();
        } else if (!haveFg) {
            showCallScreen();
            if (queuedIncoming) {
                queuedIncoming = false;
                DialerControl::instance()->accept();
            } else {
                QString n(queuedCall);
                queuedCall = QString();
                queuedCallContact = QUniqueId();
                DialerControl::instance()->dial(n,DialerControl::instance()->callerIdNeeded(n), queuedCallType, queuedCallContact);
            }
        }
    }
}

/*!
  \internal
  */
void PhoneLauncher::acceptIncoming()
{
    if (DialerControl::instance()->hasIncomingCall()) {
        showCallScreen();
        if(DialerControl::instance()->hasActiveCalls() &&
           DialerControl::instance()->hasCallsOnHold()) {
            if (!incomingMsgBox) {
                incomingMsgBox = QAbstractMessageBox::messageBox(
                    callScreen(),
                    tr("End current call?"),
                    tr("<qt>Do you wish to end the current call before answering the incoming call?</qt>"),
                    QAbstractMessageBox::Warning, QAbstractMessageBox::Yes, QAbstractMessageBox::No);
                connect(incomingMsgBox, SIGNAL(finished(int)), this, SLOT(messageBoxDone(int)));
            }
            QtopiaApplication::showDialog(incomingMsgBox);
            queuedIncoming = true;
        } else {
            DialerControl::instance()->accept();
        }
    }
}

/*!
  \internal
  Accepts an incoming call.  Has no affect if an incoming call is not
  currently available.
*/
void PhoneLauncher::messageBoxDone(int r)
{
    QAbstractMessageBox *box = (QAbstractMessageBox*)sender();
    if (box == missedMsgBox && r == QAbstractMessageBox::Yes) {
        showCallHistory(true);
    } else if (box == dialingMsgBox) {
        if (r == QAbstractMessageBox::Yes)
            DialerControl::instance()->endCall();
        else
        {
            queuedCall = QString();
            queuedCallContact = QUniqueId();
        }
    } else if (box == incomingMsgBox) {
        if (r == QAbstractMessageBox::Yes) {
            DialerControl::instance()->endCall();
        } else {
            queuedIncoming = false;
            if (DialerControl::instance()->hasIncomingCall())
                DialerControl::instance()->incomingCall().hangup();
        }
    } else if (box == callTypeMsgBox) {
        switch(r) {
        case 0:
            queuedCallType = "Voice";     // No tr
            dialNumber(queuedCall, queuedCallContact, queuedCallType);
            break;
        case 1:
            queuedCallType = "VoIP";      // No tr
#ifdef QTOPIA_VOIP
            if ( VoIPManager::instance()->isAvailable( queuedCall ) ) {
                dialNumber(queuedCall, queuedCallContact, queuedCallType);
            } else {
                if (!voipNoPresenceMsgBox) {
                    voipNoPresenceMsgBox = QAbstractMessageBox::messageBox(callScreen(), tr("Unavailable"),
                        tr("<qt>The selected contact appears to be unavailable. Do you still wish to make a call?</qt>"),
                        QAbstractMessageBox::Warning, QAbstractMessageBox::Yes, QAbstractMessageBox::No);
                    connect(voipNoPresenceMsgBox, SIGNAL(finished(int)), this, SLOT(messageBoxDone(int)));
                }
                QtopiaApplication::showDialog(voipNoPresenceMsgBox);
            }
#endif
            break;
        default:
            break;
        }
    }
#ifdef QTOPIA_VOIP
    else if (box == voipNoPresenceMsgBox) {
        if (r == QAbstractMessageBox::Yes) {
            dialNumber(queuedCall, queuedCallContact, queuedCallType);
        } else {
            queuedCall = QString();
            queuedCallType = QString();
            queuedCallContact = QUniqueId();
        }
    }
#endif
}
#endif

/*!
  \internal
  This function is now a public slot.
 */
void PhoneLauncher::showWarning(const QString &title, const QString &text)
{
    if (!warningMsgBox)
        warningMsgBox =
	    QAbstractMessageBox::messageBox(0,title,text,
					    QAbstractMessageBox::Warning);
    warningMsgBox->setWindowTitle(title);
    warningMsgBox->setText(text);
    warningMsgBox->setTimeout(WarningTimeout,QAbstractMessageBox::NoButton);
    QtopiaApplication::showDialog(warningMsgBox);
}

#ifdef QTOPIA_PHONEUI
/*!
  Dials number \a n.  The current call state is first checked to confirm
  that it is possible to make a call.  If it is not possible to make a
  call, the user will be notified, otherwise the call screen will be shown
  and the call initiated.
  \internal
*/
void PhoneLauncher::dialNumber(const QString &n, const QUniqueId &c, const QString &callType)
{
    if (callType.isEmpty() || n.isEmpty())
        return;

    // Save service request history.
    /*
    QtopiaServiceRequest req("Dialer", "dial(QString,QUniqueId)");
    req << n << c;
    QString label;
    QString icon(":icon/phone/phone"); // no tr
    if (!c.isNull()) {
        QContactModel *m = ServerContactModel::instance();
        QContact contact = m->contact(c);
        label = tr("Call %1").arg(contact.label());
        QMap<QContact::PhoneType, QString> numbers = contact.phoneNumbers();
        QMap<QContact::PhoneType, QString>::iterator it;
        for (it = numbers.begin(); it != numbers.end(); ++it) {
            if (*it == n) {
                icon = contact.phoneIconResource(it.key());
                break;
            }
        }
    } else {
        label = tr("Call %1").arg(n);
    }
    QtopiaServiceHistoryModel::insert(req, label, icon);
    */

    showCallScreen();

    if (DialerControl::instance()->hasActiveCalls() && DialerControl::instance()->hasCallsOnHold()) {
        if (!dialingMsgBox) {
            dialingMsgBox = QAbstractMessageBox::messageBox(
                    callScreen(),
                    tr("End current call?"),
                    tr("<qt>Do you wish to end the current call before begining the new call?</qt>"),
                    QAbstractMessageBox::Warning, QAbstractMessageBox::Yes, QAbstractMessageBox::No);
             connect(dialingMsgBox, SIGNAL(finished(int)), this, SLOT(messageBoxDone(int)));
        }
        QtopiaApplication::showDialog(dialingMsgBox);
    } else if (DialerControl::instance()->hasActiveCalls()) {
        DialerControl::instance()->hold();
    } else {
        DialerControl::instance()->dial( n, DialerControl::instance()->callerIdNeeded(n), callType, c );
    }
}

class CallTypeSelector : public PhoneMessageBox
{
    Q_OBJECT
public:
    CallTypeSelector( const QList<QAbstractCallPolicyManager *>& managers,
                      QWidget *parent = 0 );
    ~CallTypeSelector();

    QAbstractCallPolicyManager *selectedPolicyManager() const;

private slots:
    void itemActivated();

private:
    QListWidget *list;
    QList<QAbstractCallPolicyManager *> managers;
};

CallTypeSelector::CallTypeSelector
    ( const QList<QAbstractCallPolicyManager *>& managers, QWidget *parent )
    : PhoneMessageBox( parent )
{
    this->managers = managers;

    setIcon(QAbstractMessageBox::Warning);
    setText(tr("Which type of call do you wish to make?"));

    list = new QListWidget(this);
    list->setSortingEnabled(true);
    list->setItemDelegate(new QtopiaItemDelegate);
    list->setFrameStyle(QFrame::NoFrame);
    addContents(list);

    foreach ( QAbstractCallPolicyManager *manager, managers ) {
        QListWidgetItem *item = new QListWidgetItem(list);
        item->setText(manager->trCallType());
        item->setIcon(QIcon(":icon/" + manager->callTypeIcon()));
        item->setData(Qt::UserRole, manager->callType());
        list->addItem(item);
    }
    list->setCurrentItem(list->item(0));

    connect(list, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(itemActivated()));
}

CallTypeSelector::~CallTypeSelector()
{
}

QAbstractCallPolicyManager *CallTypeSelector::selectedPolicyManager() const
{
    QListWidgetItem *current = list->currentItem();
    if ( !current )
        return 0;
    QString callType = current->data(Qt::UserRole).toString();
    foreach ( QAbstractCallPolicyManager *manager, managers ) {
        if ( manager->callType() == callType )
            return manager;
    }
    return 0;
}

void CallTypeSelector::itemActivated()
{
    done(QAbstractMessageBox::Yes);
}

/*!
  \internal
  */
void PhoneLauncher::requestDial(const QString &n, const QUniqueId &c)
{
    if (n.isEmpty())
        return;

    int numberAtSymbol = n.count('@');
    if (numberAtSymbol > 1) {
        showWarning(tr("Incorrect Number format"),
                tr("<qt>Unable to make a phone call.</qt>"));
        return;
    }

    // Ask all of the call policy managers what they want to do
    // with this phone number.
    QAbstractCallPolicyManager::CallHandling handling;
    QList<QAbstractCallPolicyManager *> managers;
    QList<QAbstractCallPolicyManager *> candidates;
    QAbstractCallPolicyManager *chosenManager = 0;
    managers = qtopiaTasks<QAbstractCallPolicyManager>();
    foreach (QAbstractCallPolicyManager *manager, managers) {
        handling = manager->handling(n);
        if ( handling == QAbstractCallPolicyManager::MustHandle ) {
            chosenManager = manager;
            break;
        } else if ( handling == QAbstractCallPolicyManager::NeverHandle ) {
            chosenManager = 0;
            candidates.clear();
            break;
        } else if ( handling == QAbstractCallPolicyManager::CanHandle ) {
            candidates.append( manager );
        }
    }

    // Bail out if nothing can dial this number at this time.
    if (!chosenManager && candidates.isEmpty()) {
        showWarning(tr("No GSM/VoIP Network"),
                tr("<qt>No phone call is possible.</qt>"));
        return;
    }

    queuedCall = n;
    queuedCallType = QString();
    queuedCallContact = c;

#ifdef QTOPIA_CELL
    if(cellModem->planeModeEnabled()) {
        // At the moment, plane mode is the only way to turn
        // the GSM module off.
        showWarning(tr("Airplane Mode"),
            tr("<qt>Phone calls cannot be made in Airplane Mode.</qt>"));
        return;
    }
#endif

    // Determine which call policy manager to use.
    if (!chosenManager) {
        if (candidates.size() == 1) {
            // Only one call policy manager is active, so use that.
            chosenManager = candidates[0];
        } else {
            // Clear queued call information while the dialog is up
            // to prevent the call from being automatically dialed
            // if we get to PhoneLauncher::stateChanged() while
            // the dialog is on-screen.
            queuedCall = QString();
            queuedCallType = QString();
            queuedCallContact = QUniqueId();

            // More than one is active, so we have to ask the user.
            CallTypeSelector selector( candidates );
            if (QtopiaApplication::execDialog(&selector) != QAbstractMessageBox::Yes) {
                return;
            }
            chosenManager = selector.selectedPolicyManager();
            if ( !chosenManager ) {
                // Shouldn't happen, but recover gracefully anyway.
                return;
            }

            // Re-instate the queued call information.
            queuedCall = n;
            queuedCallType = QString();
            queuedCallContact = c;
        }
    }

    // Get the chosen call type.
    queuedCallType = chosenManager->callType();

#ifdef QTOPIA_VOIP
    // Ask the user to confirm if the called party is not present.
    if (!chosenManager->isAvailable(queuedCall)) {
        if (!voipNoPresenceMsgBox) {
            voipNoPresenceMsgBox = QAbstractMessageBox::messageBox(callScreen(), tr("Unavailable"),
                tr("<qt>The selected contact appears to be unavailable. Do you still wish to make a call?</qt>"),
                QAbstractMessageBox::Warning, QAbstractMessageBox::Yes, QAbstractMessageBox::No);
            connect(voipNoPresenceMsgBox, SIGNAL(finished(int)), this, SLOT(messageBoxDone(int)));
        }
        QtopiaApplication::showDialog(voipNoPresenceMsgBox);
        return;
    }
#endif

    // Dial the specified number.
    dialNumber(queuedCall, queuedCallContact, queuedCallType);
}

/*!
  \internal
  */
void PhoneLauncher::serviceNumber(QServiceNumbers::NumberId id, const QString& number)
{
    if ( id == QServiceNumbers::VoiceMail ) {
        // The "waitingVoiceMailNumber" flag is used to prevent
        // false positives when the "phonesettings" program
        // queries for the voice mail number.
        if ( waitingVoiceMailNumber ) {
            waitingVoiceMailNumber = false;
            if ( number.length() > 0 ) {
                requestDial( number );
            } else {
                showWarning(tr("Voice Mail"),
                            tr("<qt>Voice mail number is not set.</qt>"));
            }
        }
    }
}

/*!
  \internal
    Dial the voice mail number in the SIM.
*/
void PhoneLauncher::dialVoiceMail()
{
    waitingVoiceMailNumber = true;
    if ( serviceNumbers->available() )
        serviceNumbers->requestServiceNumber( QServiceNumbers::VoiceMail );
    else
        serviceNumber( QServiceNumbers::VoiceMail, QString() );
}

/*!
  \internal
  */
void PhoneLauncher::resetMissedCalls()
{
    alertedMissed = 0;
}

/*!
  \internal
  */
QAbstractHomeScreen *PhoneLauncher::homeScreen() const
{
    return m_homeScreen;
}

/*!
  \internal
  */
QAbstractDialerScreen *PhoneLauncher::dialer(bool create) const
{
    if(create && !m_dialer) {
        m_dialer = qtopiaWidget<QAbstractDialerScreen>(0);
        if (!m_dialer) {
            qLog(UI) << "Unable to create the Dialer Screen";
            return 0;
        }
        m_dialer->move(QApplication::desktop()->screenGeometry().topLeft());

        connect(m_dialer,
                SIGNAL(requestDial(QString,QUniqueId)),
                this,
                SLOT(requestDial(QString,QUniqueId)));
        connect(m_dialer, SIGNAL(speedDial(QString)),
                this, SLOT(speedDial(QString)) );

    #ifdef QTOPIA_CELL
        if(gsmKeyActions)
            gsmKeyActions->setDialer(m_dialer);
    #endif
    }

    return m_dialer;
}

/*!
  \internal
  */
CallScreen *PhoneLauncher::callScreen(bool create) const
{
    if(create && !mCallScreen) {
        mCallScreen = new CallScreen(DialerControl::instance(), 0);
        mCallScreen->move(QApplication::desktop()->screenGeometry().topLeft());
        RingControl *rc = qtopiaTask<RingControl>();
        if(rc)
            QObject::connect(mCallScreen, SIGNAL(muteRing()),
                             rc, SLOT(muteRing()));
        QObject::connect(mCallScreen, SIGNAL(listEmpty()),
                         mCallScreen, SLOT(hide()));
        QObject::connect(mCallScreen, SIGNAL(acceptIncoming()),
                         this, SLOT(acceptIncoming()));
        ThemeControl::instance()->registerThemedView(mCallScreen, "CallScreen");

#ifdef QTOPIA_CELL
        if (gsmKeyActions) {
            QObject::connect( mCallScreen, SIGNAL(filterKeys(QString,bool&)),
                    gsmKeyActions, SLOT(filterKeys(QString,bool&)) );
            QObject::connect( mCallScreen, SIGNAL(filterSelect(QString,bool&)),
                    gsmKeyActions, SLOT(filterSelect(QString,bool&)) );
            QObject::connect( mCallScreen, SIGNAL(testKeys(QString,bool&)),
                    gsmKeyActions, SLOT(testKeys(QString,bool&)) );
        }
#endif
    }
    return mCallScreen;

}

void PhoneLauncher::speedDialActivated()
{
    if (dialerSpeedDialFeedbackActive && dialer(false)) {
        dialer(false)->reset();
        dialer(false)->hide();
    }
    dialerSpeedDialFeedbackActive = false;
}

#endif

/*!
  \internal
  */
void PhoneLauncher::speedDial( const QString& input )
{
    if (activateSpeedDial(input)) {
#ifdef QTOPIA_PHONEUI
        dialerSpeedDialFeedbackActive = true;
#endif
    }
}

/*!
  \internal
  Activate speed dial associated with \a input.
 */
bool PhoneLauncher::activateSpeedDial( const QString& input )
{
    if ( !speeddialfeedback ) {
        speeddialfeedback = new QSpeedDialFeedback;
#ifdef QTOPIA_PHONEUI
        connect(speeddialfeedback, SIGNAL(requestSent()), this, SLOT(speedDialActivated()));
#endif
    }

    QString sel = input;
    if ( input.isEmpty() ) {
        speeddialfeedback->setBlindFeedback(false);
        sel = QSpeedDial::selectWithDialog(this);
    } else {
        speeddialfeedback->setBlindFeedback(true);
    }

    QDesktopWidget *desktop = QApplication::desktop();
    QtopiaServiceDescription* r = QSpeedDial::find(sel);
    if(r)
    {
        speeddialfeedback->show(desktop->screen(desktop->primaryScreen()),sel,*r);
        return !r->request().isNull();
    }
    else
    {
        if (!input.isEmpty())
            speeddialfeedback->show(desktop->screen(desktop->primaryScreen()),sel,QtopiaServiceDescription());
        return false;
    }
}


/*!
  \internal
  */
QAbstractBrowserScreen *PhoneLauncher::phoneBrowser(bool create) const
{
    if(!stack && create) {
        stack = qtopiaWidget<QAbstractBrowserScreen>();
        if (stack)
            stack->move(QApplication::desktop()->screenGeometry().topLeft());
        else
            qFatal("Phone launcher requires the presence of a browser screen");
    }

    return stack;
}

/*!
  \internal
  */
QAbstractSecondaryDisplay *PhoneLauncher::secondaryDisplay(bool create) const
{
    QDesktopWidget *desktop = QApplication::desktop();
    if (!secondDisplay && create && desktop->numScreens() > 1) {
        secondDisplay = qtopiaWidget<QAbstractSecondaryDisplay>(0,
                                            Qt::FramelessWindowHint | Qt::Tool);
        if (secondDisplay){
            secondDisplay->setGeometry(desktop->screenGeometry(1));
            secondDisplay->show();
        }else{
            qLog(UI) << "Unable to create the Secondary Display";
        }
    }

    return secondDisplay;
}

void PhoneLauncher::newMessagesChanged()
{
    m_homeScreen->setNewMessages(newMessages.value().toInt());
}


// define MultiTaskProxy
MultiTaskProxy::MultiTaskProxy(QObject *parent)
: TaskManagerService(parent)
{
}

void MultiTaskProxy::multitask()
{
    emit doMultiTask();
}

void MultiTaskProxy::showRunningTasks()
{
    emit doShowRunningTasks();
}

// define QSpeedDialFeedback
QSpeedDialFeedback::QSpeedDialFeedback() :
    QFrame(0, (Qt::Tool | Qt::FramelessWindowHint)),
    timerId(0), blind(true)
{
    setFrameStyle(QFrame::WinPanel|QFrame::Raised);
    QVBoxLayout *vb = new QVBoxLayout(this);
    vb->setMargin(8);
    icon = new QLabel(this);
    vb->addWidget(icon);
    label = new QLabel(this);
    label->setWordWrap(true);
    vb->addWidget(label);
    icon->setAlignment(Qt::AlignCenter);
    label->setAlignment(Qt::AlignCenter);
}

// is user "dialing-blind", or picking from screen?
void QSpeedDialFeedback::setBlindFeedback(bool on)
{
    blind = on;
}

void QSpeedDialFeedback::show(QWidget* center, const QString& input, const QtopiaServiceDescription& r)
{
    req = r.request();
#ifdef QTOPIA_PHONEUI
    if ( blind ) {
        RingControl *rc = qtopiaTask<RingControl>();
        if ( req.isNull() ) {
            if(rc) rc->playSound(":sound/speeddial/nak");
        } else {
            if(rc) rc->playSound(":sound/speeddial/ack");
        }
    }
#endif
    if ( req.isNull() ) {
        QIcon p(":icon/cancel");
        icon->setPixmap(p.pixmap(style()->pixelMetric(QStyle::PM_LargeIconSize)));
        label->setText(tr("No speed dial %1").arg(input));
    } else {
        // Often times these are actually icons, not images...
        QIcon p(":icon/" + r.iconName());
        if (p.isNull())
            p = QIcon(":image/" + r.iconName());
        icon->setPixmap(p.pixmap(style()->pixelMetric(QStyle::PM_LargeIconSize)));
        label->setText(r.label());
    }
    QtopiaApplication::sendPostedEvents(this, QEvent::LayoutRequest);
    QRect w = center->topLevelWidget()->geometry();
    // Make sure the label wraps (and leave a small amount around the edges)
    // (which is 2xlayout margin + a little bit)
    int labelmargins = layout()->margin() * 2 + layout()->spacing() * 2;
    label->setMaximumSize(w.width() - labelmargins, w.height() - labelmargins);
    QSize sh = sizeHint();
    //We have to set the minimumsize before we change the geometry
    //because setGeometry is based on it and for some weired reason
    //Minimumsize is set to the size of the previous geometry.
    //This is a problem when changing from a bigger to a smaller geometry
    //and seems to be a bug in Qt.
    setMinimumSize(sh.width(),sh.height());
    setGeometry(w.x()+(w.width()-sh.width())/2,
                w.y()+(w.height()-sh.height())/2,sh.width(),sh.height());
    QFrame::show();
    activateWindow();
    setFocus();
    if( Qtopia::mousePreferred() || !blind ) {
        if ( !req.isNull() ) {
            req.send();
            emit requestSent();
        }
        timerId = startTimer(1000);
    }
}

void QSpeedDialFeedback::timerEvent(QTimerEvent*)
{
    killTimer(timerId);
    close();
}

void QSpeedDialFeedback::keyReleaseEvent(QKeyEvent* ke)
{
    if ( !ke->isAutoRepeat() ) {
        if ( !req.isNull() ) {
            req.send();
            emit requestSent();
        }
        close();
    }
}

void QSpeedDialFeedback::mouseReleaseEvent(QMouseEvent*)
{
    if ( !req.isNull() ) {
        req.send();
        emit requestSent();
    }
    close();
}

QTOPIA_REPLACE_WIDGET(QAbstractServerInterface, PhoneLauncher);

#include "phonelauncher.moc"
