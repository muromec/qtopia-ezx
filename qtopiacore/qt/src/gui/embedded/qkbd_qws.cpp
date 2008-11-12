/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qkbd_qws.h"

#ifndef QT_NO_QWS_KEYBOARD

#include "qwindowsystem_qws.h"
#include "qscreen_qws.h"
#include "qtimer.h"
#include <stdlib.h>


class QWSKbPrivate : public QObject
{
    Q_OBJECT
public:
    QWSKbPrivate(QWSKeyboardHandler *h) {
        handler = h;
        arTimer = new QTimer(this);
        arTimer->setSingleShot(true);
        connect(arTimer, SIGNAL(timeout()), SLOT(autoRepeat()));
        repeatdelay = 400;
        repeatperiod = 80;
    }

    void beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod) {
        unicode = uni;
        keycode = code;
        modifier = mod;
        arTimer->start(repeatdelay);
    }
    void endAutoRepeat() {
        arTimer->stop();
    }

private slots:
    void autoRepeat() {
        handler->processKeyEvent(unicode, keycode, modifier, false, true);
        handler->processKeyEvent(unicode, keycode, modifier, true, true);
        arTimer->start(repeatperiod);
    }

private:
    QWSKeyboardHandler *handler;
    int unicode;
    int keycode;
    Qt::KeyboardModifiers modifier;
    int repeatdelay;
    int repeatperiod;
    QTimer *arTimer;
};

/*!
    \class QWSKeyboardHandler
    \ingroup qws

    \brief The QWSKeyboardHandler class is a base class for keyboard
    drivers in Qtopia Core.

    Note that this class is only available in \l {Qtopia Core}.

    \l {Qtopia Core} provides ready-made drivers for several keyboard
    protocols, see the \l {Qtopia Core Character Input}{character
    input} documentation for details. Custom keyboard drivers can be
    implemented by subclassing the QWSKeyboardHandler class and
    creating a keyboard driver plugin (derived from
    QKbdDriverPlugin). \l {Qtopia Core}'s implementation of the
    QKbdDriverFactory class will automatically detect the plugin, and
    load the driver into the server application at runtime using Qt's
    \l {How to Create Qt Plugins}{plugin system}.

    The keyboard driver receives keyboard events from the system
    device and encapsulates each event with an instance of the
    QWSEvent class which it then passes to the server application (the
    server is responsible for propagating the event to the appropriate
    client). To receive keyboard events, a QWSKeyboardHandler object
    will usually create a QSocketNotifier object for the given
    device. The QSocketNotifier class provides support for monitoring
    activity on a file descriptor. When the socket notifier receives
    data, it will call the keyboard driver's processKeyEvent()
    function to send the event to the \l {Qtopia Core} server
    application for relaying to clients.


    QWSKeyboardHandler also provides functions to control
    auto-repetion of key sequences, beginAutoRepeat() and
    endAutoRepeat(), and the transformDirKey() function enabling
    transformation of arrow keys according to the display orientation.

    \sa QKbdDriverPlugin, QKbdDriverFactory, {Qtopia Core Character Input}
*/


/*!
    Constructs a keyboard driver.

    Call the QWSServer::setKeyboardHandler() function to make the
    newly created keyboard driver, the primary driver. Note that the
    primary driver is controlled by the system, i.e., the system will
    delete it upon exit.
*/
QWSKeyboardHandler::QWSKeyboardHandler()
{
    d = new QWSKbPrivate(this);
}

/*!
    Destroys this keyboard driver.

    Do not call this function if this driver is the primary keyboard
    handler, i.e., if QWSServer::setKeyboardHandler() function has
    been called passing this driver as argument. The primary keyboard
    driver is deleted by the system.
*/
QWSKeyboardHandler::~QWSKeyboardHandler()
{
    delete d;
}


/*!
    Sends a key event to the \l {Qtopia Core} server application.

    The key event is identified by its \a unicode value and the \a
    keycode, \a modifiers, \a isPress and \a autoRepeat parameters.

    The \a keycode parameter is the Qt keycode value as defined by the
    Qt::Key enum. The \a modifiers is an OR combination of
    Qt::KeyboardModifier values, indicating whether \gui
    Shift/Alt/Ctrl keys are pressed. The \a isPress parameter is true
    if the event is a key press event and \a autoRepeat is true if the
    event is caused by an auto-repeat mechanism and not an actual key
    press.

    \sa beginAutoRepeat(), endAutoRepeat(), transformDirKey()
*/
void QWSKeyboardHandler::processKeyEvent(int unicode, int keycode, Qt::KeyboardModifiers modifiers,
                        bool isPress, bool autoRepeat)
{
    qwsServer->processKeyEvent(unicode, keycode, modifiers, isPress, autoRepeat);
}

/*!
    \fn int QWSKeyboardHandler::transformDirKey(int keycode)

    Transforms the arrow key specified by the given \a keycode, to the
    orientation of the display and returns the transformed keycode.

    The \a keycode is a Qt::Key value. The values identifying arrow
    keys are:

    \list
        \o Qt::Key_Left
        \o Qt::Key_Up
        \o Qt::Key_Right
        \o Qt::Key_Down
    \endlist

    \sa processKeyEvent()
 */
int QWSKeyboardHandler::transformDirKey(int key)
{
    static int dir_keyrot = -1;
    if (dir_keyrot < 0) {
        // get the rotation
        switch (qgetenv("QWS_CURSOR_ROTATION").toInt()) {
        case 90: dir_keyrot = 1; break;
        case 180: dir_keyrot = 2; break;
        case 270: dir_keyrot = 3; break;
        default: dir_keyrot = 0; break;
        }
    }
    int xf = qt_screen->transformOrientation() + dir_keyrot;
    return (key-Qt::Key_Left+xf)%4+Qt::Key_Left;
}

/*!
    \fn void QWSKeyboardHandler::beginAutoRepeat(int unicode, int keycode, Qt::KeyboardModifiers modifier)

    Begins auto-repeating the specified key press; after a short delay
    the key press is sent periodically until the endAutoRepeat()
    function is called.

    The key press is specified by its \a unicode, \a keycode and \a
    modifier state.

    \sa endAutoRepeat(), processKeyEvent()
*/
void QWSKeyboardHandler::beginAutoRepeat(int uni, int code, Qt::KeyboardModifiers mod)
{
    d->beginAutoRepeat(uni, code, mod);
}

/*!
    Stops auto-repeating a key press.

    \sa beginAutoRepeat(), processKeyEvent()
*/
void QWSKeyboardHandler::endAutoRepeat()
{
    d->endAutoRepeat();
}

#include "qkbd_qws.moc"

#endif // QT_NO_QWS_KEYBOARD

