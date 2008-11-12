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

#include "qabstracthomescreen.h"

#include <QAction>
#include <QDebug>
#include <QKeyEvent>
#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QTimer>

#include <qtopiaapplication.h>
#include <qtopiaipcenvelope.h>
#include <qtopiaservices.h>

#include <qsoftmenubar.h>
#include <qdevicebuttonmanager.h>
#include <QValueSpaceObject>

#include "touchscreenlockdlg.h"
#include "qtopiainputevents.h"
#ifdef QTOPIA_CELL
#include "cellmodemmanager.h"
#endif
#ifdef QTOPIA_PHONEUI
#include "dialercontrol.h"
#endif
#include "themebackground_p.h"
#include "taskmanagerservice.h"
#include "pressholdgate.h"
#include <qdrmcontent.h>
#if defined(QTOPIA_BLUETOOTH) || defined(QTOPIA_INFRARED)
#include "obexservicemanager.h"
#include "receivewindow.h"
#endif

/*!
  Returns true if the keypad is locked.
 */
bool QAbstractHomeScreen::keyLocked() const
{
    return keyLock->locked();
}

#ifdef QTOPIA_CELL
/*!
  \internal
  */
bool QAbstractHomeScreen::simLocked() const
{
    return !simLock->open();
}
#endif

/*!
  Locks the keypad if \a lock is true.
 */
void QAbstractHomeScreen::setKeyLocked(bool lock) const
{
    if (lock)
        keyLock->lock();
    else
        keyLock->unlock();
}


/*!
  \class QAbstractHomeScreen
  \brief The QAbstractHomeScreen class allows developers to replace the "home screen" portion of the Phone UI.
  \ingroup QtopiaServer::PhoneUI::TTSmartPhone

  The QAbstractHomeScreen interface is part of the
  \l {QtopiaServerApplication#qtopia-server-widgets}{server widgets framework} and allows developers 
  to replace the standard home screen in Qtopia's phone UI.
  A small tutorial on how to develop new server widgets using one of the abstract widgets as base can
  be found in QAbstractServerInterface class documentation.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
  */

/*!
  Construct a new QAbstractHomeScreen instance, with the specified \a parent
  and widget \a flags.
*/
QAbstractHomeScreen::QAbstractHomeScreen(QWidget *parent, Qt::WFlags flags)
        : QWidget(parent, flags)
{
    lockMsgId = 0;
    infoMsgId = 0;
    missedCalls = 0;
    newMessages = 0;
    smsMemoryFull = 0;
#ifdef QTOPIA_CELL
    emLock = 0;
    simLock = 0;
#endif
    keyLock = new BasicKeyLock(this);

    QObject::connect(keyLock, SIGNAL(stateChanged(BasicKeyLock::State)),
                     this, SLOT(showLockInformation()));
    QObject::connect(keyLock, SIGNAL(keyLockDetected()),
                     keyLock, SLOT(lock()));

#ifdef QTOPIA_CELL
    emLock = new BasicEmergencyLock(this);
    simLock = new BasicSimPinLock(this);

    QObject::connect(emLock,
                     SIGNAL(stateChanged(BasicEmergencyLock::State,QString)),
                     this, SLOT(showLockInformation()));
    QObject::connect(simLock,
                     SIGNAL(stateChanged(BasicSimPinLock::State,QString)),
                     this, SLOT(showLockInformation()));
    QObject::connect(emLock, SIGNAL(dialEmergency(QString)),
                     this, SIGNAL(callEmergency(QString)));
    QObject::connect(emLock, SIGNAL(dialEmergency(QString)),
                     simLock, SLOT(reset()));
    QObject::connect(emLock, SIGNAL(dialEmergency(QString)),
                     keyLock, SLOT(reset()));
#endif

    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_InputMethodEnabled);

    setFocusPolicy(Qt::StrongFocus);

    QMenu *contextMenu = QSoftMenuBar::menuFor(this);
    m_contextMenu = contextMenu;
    contextMenu->installEventFilter(this);

    actionLock = new QAction(QIcon(":icon/padlock"), tr("Key Lock"), this);
    if (Qtopia::mousePreferred()) {
        connect(actionLock, SIGNAL(triggered()), this, SLOT(lockScreen()));
    } else {
        connect(actionLock, SIGNAL(triggered()), keyLock, SLOT(lock()));
    }
    contextMenu->addAction(actionLock);

    QAction *actionProfile = new QAction(QIcon(":icon/Note"), tr("Profile..."), this);
    connect(actionProfile, SIGNAL(triggered()), this, SLOT(showProfileSelector()));
    contextMenu->addAction(actionProfile);

#ifdef QTOPIA_PHONEUI
    connect(DialerControl::instance(), SIGNAL(stateChanged()),
            this, SLOT(phoneStateChanged()));

    actionCalls = new QAction(QIcon(":icon/phone/missedcall"), tr("Missed Calls..."), this);
    connect(actionCalls, SIGNAL(triggered()), this, SLOT(viewMissedCalls()));
    contextMenu->addAction(actionCalls);
    actionCalls->setEnabled(false);
    actionCalls->setVisible(false);

    actionMessages = new QAction(QIcon(":icon/phone/sms"), tr("New Messages..."), this);
    connect(actionMessages, SIGNAL(triggered()), this, SLOT(viewNewMessages()));
    contextMenu->addAction(actionMessages);
    actionMessages->setEnabled(false);
    actionMessages->setVisible(false);
#endif

#ifdef QTOPIA_CELL
    CellBroadcastControl *cbc = CellBroadcastControl::instance();
    connect(cbc,
            SIGNAL(broadcast(CellBroadcastControl::Type,QString,QString)),
            this,
            SLOT(cellBroadcast(CellBroadcastControl::Type,QString,QString)));
#endif

    QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::NoLabel);

// when using ts with a popup input method, this makes the input method popup.
// disabling the hint prevents shortcuts on ts, although.
// 
//    if (Qtopia::mousePreferred()) {
//        QtopiaApplication::setInputMethodHint(this, "phoneonly");
//        setFocus();
//    }

    speeddialTimer = new QTimer(this);
    speeddialTimer->setSingleShot(true);
    speeddial_preedit = 0;
    connect(speeddialTimer, SIGNAL(timeout()), this, SLOT(activateSpeedDial()));

    ph = new PressHoldGate("HomeScreen", this);
    connect(ph, SIGNAL(activate(int,bool,bool)), this, SLOT(specialButton(int,bool)));

#if defined(QTOPIA_BLUETOOTH) || defined(QTOPIA_INFRARED)
    ObexServiceManager *obexmgr = new ObexServiceManager(this);
    ReceiveWindow *recvWindow = new ReceiveWindow(this);

    connect(obexmgr, SIGNAL(receiveInitiated(int,QString,QString,QString)),
            recvWindow, SLOT(receiveInitiated(int,QString,QString,QString)));
    connect(obexmgr, SIGNAL(sendInitiated(int,QString,QString,QString)),
            recvWindow, SLOT(sendInitiated(int,QString,QString,QString)));
    connect(obexmgr, SIGNAL(progress(int,qint64,qint64)),
            recvWindow, SLOT(progress(int,qint64,qint64)));
    connect(obexmgr, SIGNAL(completed(int,bool)),
            recvWindow, SLOT(completed(int,bool)));
    connect(recvWindow, SIGNAL(abortTransfer(int)),
            obexmgr, SLOT(abortTransfer(int)));
#endif

    installEventFilter(this);
    showLockInformation();
    updateHomeScreenInfo();
}

/*!
  \internal
  */
void QAbstractHomeScreen::activateSpeedDial()
{
    if (speeddialdown)
        emit speedDial(speeddial);
    if (speeddial_preedit)
        speeddial_activated_preedit = speeddial.right(speeddial_preedit);
    speeddial_preedit = 0;
    speeddial.clear();
}

/*!
  \internal
  */
QAbstractHomeScreen::~QAbstractHomeScreen()
{
}

/*!
  \internal
  */
void QAbstractHomeScreen::showPinboxInformation(const QString &, const QString &)
{
}

/*!
  \internal
  */void QAbstractHomeScreen::activatePinbox(bool)
{
}

/*!
  \internal
  */
void QAbstractHomeScreen::updateBackground()
{
}

/*!
  \internal
  */
void QAbstractHomeScreen::updateHomeScreenInfo()
{
}

/*!
  \internal
  */
void QAbstractHomeScreen::applyHomeScreenImage()
{
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void QAbstractHomeScreen::setMissedCalls(int m)
{
    actionCalls->setEnabled(m);
    actionCalls->setVisible(m);
}

/*!
  \internal
  */
void QAbstractHomeScreen::setNewMessages(int m)
{
    newMessages = m;

    actionMessages->setEnabled(newMessages || smsMemoryFull);
    actionMessages->setVisible(newMessages || smsMemoryFull);
}

/*!
  \internal
  */
void QAbstractHomeScreen::setSmsMemoryFull(bool full)
{
    smsMemoryFull = full;

    actionMessages->setEnabled(newMessages || smsMemoryFull);
    actionMessages->setVisible(newMessages || smsMemoryFull);
}
#endif

/*!
  \internal
  */
void QAbstractHomeScreen::setContextBarLocked(bool lock, bool waiting)
{
    if ( lock || waiting ) {
        // set the phone to locked - we do it both when we _require_ a lock
        // and when we are not yet sure if we need to lock (waiting).
        QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::NoLabel);
        QSoftMenuBar::setLabel(this, BasicKeyLock::lockKey(), "unlock", tr("Unlock"));

#ifdef QTOPIA_CELL
        if(!simLock->number().isEmpty())
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::BackSpace);
        else
#endif
            if ( DialerControl::instance()->allCalls().count() )
                QSoftMenuBar::setLabel(this, Qt::Key_Back, "phone/calls", tr("Calls"));
            else
                QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::NoLabel);

        // now blank out the context menu while we wait for the modem to respond.
        if ( waiting ) {
            // We have not yet decided whether we need a PIN or not.  Blank context menu.
            QSoftMenuBar::setLabel(this, BasicKeyLock::lockKey(), QSoftMenuBar::NoLabel);
            QSoftMenuBar::setLabel(this, Qt::Key_Select, QSoftMenuBar::NoLabel);
            QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::NoLabel);
        }
    } else {
        QSoftMenuBar::setLabel(this, Qt::Key_Select, "qpe/menu", tr("Menu"));
        if (BasicKeyLock::lockKey() == QSoftMenuBar::menuKey()) {
            QSoftMenuBar::setLabel(this, BasicKeyLock::lockKey(), QSoftMenuBar::Options);
        } else {
            QSoftMenuBar::clearLabel(this, BasicKeyLock::lockKey());
        }
    }
}

/*!
  \internal
  */
void QAbstractHomeScreen::showProfileSelector()
{
    QtopiaServiceRequest e("Profiles", "showProfiles()");
    e.send();
}


/*!
  \internal
  */
void QAbstractHomeScreen::showLockInformation()
{
#ifdef QTOPIA_CELL
    bool lock = keyLock->locked() || !simLock->open();
    bool waiting = simLock->state() == BasicSimPinLock::Waiting;
#else
    bool lock = keyLock->locked();
    bool waiting = false;
#endif
    emit keyLockedChanged(lock);

    QString text;
    QString pix(":image/padlock");

    if (lock) {
        if (!Qtopia::mousePreferred())
//            QtopiaApplication::setInputMethodHint(this, QtopiaApplication::AlwaysOff);
//        else
            QtopiaInputEvents::suspendMouse();

        m_contextMenu->hide();
        setContextBarLocked(lock, waiting);
    } else {
        setContextBarLocked(lock, waiting);
    }

    if (!lock) {
        // No lock
        if (!Qtopia::mousePreferred())
//            QtopiaApplication::setInputMethodHint(this, "phoneonly");
//        else
            QtopiaInputEvents::resumeMouse();
    }
#ifdef QTOPIA_CELL
    else if (emLock->emergency()) {
        // disable the lock key cause it can use the emergency number to unlock sim.
        QSoftMenuBar::setLabel(this, BasicKeyLock::lockKey(), QSoftMenuBar::NoLabel);

        // Emergency dial!
        bool partial =
            (BasicEmergencyLock::PartialEmergencyNumber == emLock->state());
        QString number = emLock->emergencyNumber();

        if (partial) {
            text = QString("<b>") + number + QString("</b>");
            if (Qtopia::mousePreferred())
                QSoftMenuBar::setLabel(this, BasicKeyLock::lockKey(), QSoftMenuBar::NoLabel);
        } else {
            text = tr("Emergency number <b>%1</b><br>Press "
                      "<font color=#008800>Call</font> to dial.").arg(number);
            pix = ":image/emergency";
            if (Qtopia::mousePreferred())
                QSoftMenuBar::setLabel(this, BasicKeyLock::lockKey(), "phone/calls", tr("Call"));
        }
    }
#endif
    else if (keyLock->locked()) {
        // Key lock
#ifdef QTOPIA_CELL
        // Clear partial emergency numbers or we won't be able to
        // type a new one in once the bad key has been cleared.
        emLock->reset();
#endif
        switch (keyLock->state()) {
        case BasicKeyLock::KeyLocked:
            break;
        case BasicKeyLock::KeyLockIncorrect: {
            text = tr("Press %1 then * to unlock phone");
            int lkey = BasicKeyLock::lockKey();
            if (lkey == Qt::Key_Context1)
                text = text.arg(tr("Unlock"));
            else
                text = text.arg(QString(QKeySequence(lkey)));
        }
        break;
        case BasicKeyLock::KeyLockToComplete:
            text = tr("Now press * to unlock phone");
            break;
        default:
            Q_ASSERT(!"Unknown state");
            break;
        };
    } else {
#ifdef QTOPIA_CELL
        // Sim lock
        bool waiting = false;
        switch (simLock->state()) {
        case BasicSimPinLock::VerifyingSimPuk:
        case BasicSimPinLock::VerifyingSimPin:
        case BasicSimPinLock::Waiting:
            text = tr("Please Wait...", "please wait while the phone checks to see if it needs a pin number, or if the most recently entered pin number is correct");
            waiting = true;
            break;
        case BasicSimPinLock::SimPinRequired:
            text = tr("Enter PIN then press %1<br>");
            break;
        case BasicSimPinLock::SimPukRequired:
            text = tr("Enter PUK then press %1<br>");
            break;
        case BasicSimPinLock::NewSimPinRequired:
            text = tr("Enter new PIN then press %1<br>");
            break;
        case BasicSimPinLock::Pending:
            if (!Qtopia::mousePreferred())
                text = tr("Please Wait...");
            waiting = true;
            break;
        default:
            Q_ASSERT(!"Unknown state");
            break;
        }

        if (!waiting) {
            text.append(QString(simLock->number().length(), '*'));

            int lkey = BasicSimPinLock::lockKey();
            if (lkey == Qt::Key_Context1)
                text = text.arg("Unlock");
            else
                text = text.arg(QString(QKeySequence(lkey)));
        }
#endif // QTOPIA_CELL
    }

    if (lockMsgId) {
        clearInformation(lockMsgId);
        lockMsgId = 0;
    }

    if (!text.isEmpty()) {
        if (Qtopia::mousePreferred()) {
            showPinboxInformation(pix, text);
        } else {
            lockMsgId = showInformation(pix, text, 0);
        }
    } else if (Qtopia::mousePreferred()) {
        activatePinbox(false);
    }
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void QAbstractHomeScreen::viewNewMessages()
{
    QtopiaServiceRequest req("Messages", "viewNewMessages(bool)");
    req << true;
    req.send();
}

/*!
  \internal
  */
void QAbstractHomeScreen::viewMissedCalls()
{
    emit showMissedCalls();
}
#endif

/*!
  \internal
  */
void QAbstractHomeScreen::inputMethodEvent(QInputMethodEvent *e)
{
    speeddialdown = true;
    QString ctext = e->commitString();
    QString ptext = e->preeditString();
    speeddial = speeddial.left(speeddial.length() - speeddial_preedit);
    if (!ctext.isEmpty()) {
        if (ctext != speeddial_activated_preedit) {
            speeddial += ctext;
            speeddial_preedit = 0;
        }
    }

    if (!ptext.isEmpty()) {
        speeddial += ptext;
        speeddial_preedit = ptext.length();
    }

    speeddial_activated_preedit.clear();

    if (!speeddial.isEmpty())
        speeddialTimer->start(1000);
}

/*!
  \internal
  */
void QAbstractHomeScreen::specialButton(int keycode, bool held)
{
    const QDeviceButton* button =
        QDeviceButtonManager::instance().buttonForKeycode(keycode, "HomeScreen");
    if (button) {
        QtopiaServiceRequest sr;
        if (held) {
            sr = button->heldAction();
        } else {
            sr = button->pressedAction();
        }
        if (!sr.isNull())
            sr.send();
    }
}

/*!
  \internal
  */
bool QAbstractHomeScreen::eventFilter(QObject *, QEvent *e)
{
#ifdef QTOPIA_CELL
    bool locked = keyLock->locked() || !simLock->open();
#else
    bool locked = keyLock->locked();
#endif

    if (locked && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *)e;
        // there might be emergency calls
        if ( DialerControl::instance()->allCalls().count() ) {
           if ( ke->key() == Qt::Key_Back
#ifdef QTOPIA_CELL
                   && !simLock->number().length()
#endif
                   )
                emit showCallScreen();
            else if ( ke->key() == Qt::Key_Hangup )
                DialerControl::instance()->endCall();
        }
#ifdef QTOPIA_CELL
        if (!emLock->processKeyEvent((QKeyEvent *)e)) {
#endif
            if (keyLock->locked())
                keyLock->processKeyEvent((QKeyEvent *)e);
#ifdef QTOPIA_CELL
            else
                simLock->processKeyEvent((QKeyEvent *)e);
        }
#endif

        return true;
    } else if (!keyLock->locked()
#ifdef QTOPIA_CELL
               && simLock->open()
#endif
               && e->type() == QEvent::KeyPress) {
        keyLock->processKeyEvent((QKeyEvent *)e);
        return false;
    } else if (locked && e->type() == QEvent::KeyRelease) {
        return true;
    } else if (locked && e->type() == QEvent::WindowActivate) {
        // make sure the pin entry dialog is shown if sim is still locked.
        showLockInformation();
    }
    return false;
}

/*!
  \fn void QAbstractHomeScreen::showPhoneBrowser()
  \internal
  */

/*!
  \fn void QAbstractHomeScreen::speedDial(const QString &)
  \internal
  */

/*!
  \fn void QAbstractHomeScreen::keyLockedChanged(bool)
  \internal
  */

/*!
  \internal
  */
void QAbstractHomeScreen::focusInEvent(QFocusEvent *)
{
    // Avoid repaint.
}

/*!
  \internal
  */
void QAbstractHomeScreen::focusOutEvent(QFocusEvent *)
{
    // Avoid repaint.
}

/*!
  \internal
  */
void QAbstractHomeScreen::keyPressEvent(QKeyEvent *k)
{
    // also needs to respond to IM events....
    char ch = k->text()[0].toLatin1();
    if ((ch >= '0' && ch <= '9')
#ifdef QTOPIA_VOIP
            || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
#endif
            || ch == 'p' || ch == 'P' || ch == '+' || ch == 'w'
            || ch == 'W' || ch == '#' || ch == '*' || ch == '@') {

#ifdef QTOPIA_PHONEUI
        if (DialerControl::instance()->isDialing()) {
            // send the digits as dtmf tones to the active calls.
            QList<QPhoneCall> activeCalls = DialerControl::instance()->activeCalls();
            if (activeCalls.count())
                activeCalls.first().tone(k->text());
            emit showCallScreen();
        } else {
            if (!k->isAutoRepeat()) {
                emit dialNumber(k->text().toLower());
            }
        }
#else
        if (ch >= '0' && ch <= '9' && !k->isAutoRepeat()) {
            speeddialdown = true;
            speeddial += ch;
            speeddialTimer->start(1000);
        }
#endif
        return;
    }
    switch (k->key()) {
#ifdef QTOPIA_PHONEUI
    case Qt::Key_Call:
    case Qt::Key_Yes:
        if (!DialerControl::instance()->hasIncomingCall() && !keyLocked() &&
                !DialerControl::instance()->isDialing())
            emit showCallHistory();
        break;
#endif
    case Qt::Key_Flip: {
        QSettings cfg("Trolltech", "Phone");
        cfg.beginGroup("FlipFunction");
        if (!cfg.value("hangup").toBool())
            break;
        // (else FALL THROUGH)
    }
#ifdef QTOPIA_PHONEUI
    case Qt::Key_Hangup:
    case Qt::Key_No:
        if (DialerControl::instance()->allCalls().count())
            emit showCallScreen();
        DialerControl::instance()->endCall();
        break;
    case Qt::Key_Back:
        if (DialerControl::instance()->allCalls().count())
            emit showCallScreen();
        k->accept();
        break;
#else
    case Qt::Key_Back:
        k->accept();
        break;
#endif
    case Qt::Key_Select:
        k->accept();
        emit showPhoneBrowser();
        break;

    default:
        ph->filterDeviceButton(k->key(), true, k->isAutoRepeat());
        k->ignore();
    }
}

/*!
  \internal
  */
void QAbstractHomeScreen::keyReleaseEvent(QKeyEvent *k)
{
    if (!k->isAutoRepeat())
        speeddialdown = false;
    if (ph->filterDeviceButton(k->key(), false, k->isAutoRepeat()))
        k->accept();
}

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void QAbstractHomeScreen::showCallHistory(bool missed, const QString &/*hint*/)
{
    if (!DialerControl::instance()->hasIncomingCall() &&
            !DialerControl::instance()->isDialing()) {
        if (missed)
            emit showMissedCalls();
        else
            emit showCallHistory();
    }
}

/*!
  \internal
  */
void QAbstractHomeScreen::phoneStateChanged()
{
    if (DialerControl::instance()->allCalls().count()) {
        QSoftMenuBar::setLabel(this, Qt::Key_Back, "phone/calls", tr("Calls"));
    } else {
        QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::NoLabel);
    }

    if (DialerControl::instance()->hasIncomingCall() ||
            DialerControl::instance()->isConnected() ||
            DialerControl::instance()->isDialing()) {

        if (keyLock->locked())
            setContextBarLocked(false);

        actionLock->setEnabled(false);
    } else {
        if (keyLock->locked())
            setContextBarLocked(true);

        actionLock->setEnabled(true);
    }
}
#endif

/*!
  \fn void QAbstractHomeScreen::loadPlugins()
  \internal
 */

/*!
  \fn void QAbstractHomeScreen::setLockLevel(int)
  \internal
 */

/*!
  \fn bool QAbstractHomeScreen::InfoData::operator<=(const InfoData &d) const
  \internal
 */

/*!
  \fn bool QAbstractHomeScreen::InfoData::operator<(const InfoData &d) const
  \internal
 */

/*!
  \fn bool QAbstractHomeScreen::InfoData::operator>(const InfoData &d) const
  \internal
 */

/*!
  \internal
  */
uint QAbstractHomeScreen::showInformation(const QString &pix, const QString &text, int priority)
{
    static uint nextInfoId = 0;

    InfoData info;
    info.id = ++nextInfoId;
    info.priority = priority;
    info.text = text;
    info.pixmap = QPixmap(pix);

    infoData.append(info);
    qSort(infoData);

    updateInformation();

    return info.id;
}

/*!
  \internal
  */
void QAbstractHomeScreen::lockScreen()
{
    new TouchScreenLockDialog(0, Qt::WindowStaysOnTopHint);
}

/*!
  \internal
  */
void QAbstractHomeScreen::clearInformation(uint id)
{
    QList<InfoData>::Iterator it;
    for (it = infoData.begin(); it != infoData.end(); ++it) {
        if ((*it).id == (int)id) {
            infoData.erase(it);
            break;
        }
    }

    updateInformation();
}

/*!
  \internal
  */
void QAbstractHomeScreen::updateInformation()
{
}

#ifdef QTOPIA_CELL

/*!
  \internal
  */
void QAbstractHomeScreen::cellBroadcast(CellBroadcastControl::Type type,
                                        const QString &chan,
                                        const QString &text)
{
    if (type == CellBroadcastControl::Background) {
        // clear pervious message if not yet deleted
        // otherwise it will be shown forever
        if ( infoMsgId )
            hideCBSMessage();

        QString disp = chan + "\n" + text;
        infoMsgId = showInformation(":image/antenna", disp);
        QTimer::singleShot(10000, this, SLOT(hideCBSMessage()));
    }
}

#endif // QTOPIA_CELL

#ifdef QTOPIA_PHONEUI
/*!
  \internal
  */
void QAbstractHomeScreen::hideCBSMessage()
{
    if ( !infoMsgId )
        return;

    clearInformation(infoMsgId);
    infoMsgId = 0;
}
#endif

