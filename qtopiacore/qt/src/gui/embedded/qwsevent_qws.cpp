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

#include "qwsevent_qws.h"

QWSEvent *QWSEvent::factory(int type)
{
    QWSEvent *event = 0;
    switch (type) {
    case QWSEvent::Connected:
        event = new QWSConnectedEvent;
        break;
    case QWSEvent::MaxWindowRect:
        event = new QWSMaxWindowRectEvent;
        break;
    case QWSEvent::Mouse:
        event = new QWSMouseEvent;
        break;
    case QWSEvent::Focus:
        event = new QWSFocusEvent;
        break;
    case QWSEvent::Key:
        event = new QWSKeyEvent;
        break;
    case QWSEvent::Region:
        event = new QWSRegionEvent;
        break;
    case QWSEvent::Creation:
        event = new QWSCreationEvent;
        break;
#ifndef QT_NO_QWS_PROPERTIES
    case QWSEvent::PropertyNotify:
        event = new QWSPropertyNotifyEvent;
        break;
    case QWSEvent::PropertyReply:
        event = new QWSPropertyReplyEvent;
        break;
#endif // QT_NO_QWS_PROPERTIES
    case QWSEvent::SelectionClear:
        event = new QWSSelectionClearEvent;
        break;
    case QWSEvent::SelectionRequest:
        event = new QWSSelectionRequestEvent;
        break;
    case QWSEvent::SelectionNotify:
        event = new QWSSelectionNotifyEvent;
        break;
#ifndef QT_NO_COP
    case QWSEvent::QCopMessage:
        event = new QWSQCopMessageEvent;
        break;
#endif
    case QWSEvent::WindowOperation:
        event = new QWSWindowOperationEvent;
        break;

#ifndef QT_NO_QWS_INPUTMETHODS
    case QWSEvent::IMEvent:
        event = new QWSIMEvent;
        break;
    case QWSEvent::IMQuery:
        event = new QWSIMQueryEvent;
        break;
    case QWSEvent::IMInit:
        event = new QWSIMInitEvent;
        break;
#endif
#ifndef QT_NO_QWSEMBEDWIDGET
    case QWSEvent::Embed:
        event = new QWSEmbedEvent;
        break;
#endif
    case QWSEvent::Font:
        event = new QWSFontEvent;
        break;
    default:
        qCritical("QWSEvent::factory() : Unknown event type %08x!", type);
    }
    return event;
}

/*!
    \class QWSEvent
    \ingroup qws

    \brief The QWSEvent class encapsulates an event in Qtopia Core.

    When running a \l {Qtopia Core} application, it either runs as a
    server or connects to an existing server. All system generated
    events are passed to the server application which then propagates
    the event to the appropriate client.

    Whenever the server receives an event, it queries its stack of
    top-level windows to find the window containing the event's
    position. Each window can identify the client application that
    created it, and returns its ID to the server upon
    request. Finally, the server forwards the event, encapsulated by
    an instance of the QWSEvent class, to the appropriate client.

    \image qtopiacore-client.png

    The server communicates with the client applications over the UNIX
    domain socket. You can retrieve direct access to all the events a
    client receives from the server, by reimplementing QApplication's
    \l {QApplication::}{qwsEventFilter()} function.

    QWSEvent provides the \l Type enum specifying the origin of the
    event. Internally, each type is represented by a QWSEvent
    subclass, e.g., \c QWSKeyEvent.

    \sa QWSServer, QWSClient, {Qtopia Core Architecture}
*/

/*!
    \enum QWSEvent::Type

    This enum describes the origin of the event.

    \value NoEvent No event has occurred.
    \value Connected An application has connected to the server.
    \value Mouse A mouse button is pressed or released, or the mouse cursor is moved.
             See also \l {Qtopia Core Pointer Handling}.
    \value Focus A window has lost or received focus.
    \value Key A key is pressed or released. See also \l {Qtopia Core Character Input}.
    \value Region A region has changed.
    \value Creation The server has created an ID, typically for a window.
    \value PropertyNotify A property has changed.
    \value PropertyReply The server is responding to a request for a property's value.
    \value SelectionClear A selection is deleted.
    \value SelectionRequest The server has queried for a selection.
    \value SelectionNotify A new selection has been created.
    \value MaxWindowRect The server has changed the maximum window for an application.
    \value QCopMessage A new Qt Cop message has appeared. See also QCopChannel
    \value WindowOperation A window operation, e.g. resizing, has occurred.
    \value IMEvent An input method has been used  to enter text for languages with
              non-Latin alphabets. See also QWSInputMethod.
    \value IMQuery An input method query for a specified property has occurred.
             See also QWSInputMethod.
    \value NEvent The number of events has changed.
    \value Embed An event used internally to implement embedded windows. See also
           QWSEmbedWidget.
    \omitvalue Font
    \omitvalue IMInit
*/

/*!
   \fn  QWSMouseEvent *QWSEvent::asMouse()
   \internal
*/

/*!
   \fn  int QWSEvent::window()
   \internal
*/

/*!
   \fn  int QWSEvent::window() const
   \internal
*/

/*!
   \fn  QWSEvent *QWSEvent::factory(int type)
   \internal
*/

/*!
   \fn  QWSEvent::QWSEvent( int t, int len, char * ptr)
   \internal
*/


