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

#include "qabstractserverinterface.h"
#include "windowmanagement.h"

/*!
  \class QAbstractServerInterface
  \brief The QAbstractServerInterface class allows the Qtopia Server UI to be
         replaced.
  \ingroup QtopiaServer
  \ingroup QtopiaServer::PhoneUI::TTSmartPhone

  While it is usually easier to customize one of the existing Qtopia server
  interfaces, when building a product on top of the Qtopia system, it is
  sometimes desirable to replace the entire Qtopia server user interface.
  Qtopia supports replacing the server user interface through the
  QAbstractServerInterface class.

  The Qtopia server user interface is reponsible for direct interaction with
  the user.  Core system services, such as telephony and application launching,
  are handled independently within the system and are not affected by replacing
  the UI.  Likewise, Qtopia APIs that require a server-side UI component, such
  as QSoftMenuBar, also include server side APIs, QSoftMenuBarProvider in the
  case of QSoftMenuBar, which allows their functionality to be maintained or
  adapted to the new UI model.  In this way, it is relatively simple to replace
  the server's visual interface while maintaining a functioning and featureful
  Qtopia platform.

  Replacing the entire server user interface consists of three steps:
  \list 1
  \o Implement a QAbstractServerInterface derived class that provides the user
     interface.

     The Qtopia server itself provides no UI outside of this implementation
     (with the exception of some dialogs, all of which can be disabled through
     the StandardDialogs class) and so the implementation must provide all
     aspects of the user interface.

     Some helpful widget and backend classes are provided that make implementing
     a server UI easier.  The documentation for these classes can be perused
     in the \l QtopiaServer group.

  \o Mark the QAbstractServerInterface implementation as a replacable widget
     using the QTOPIA_REPLACE_WIDGET() or QTOPIA_REPLACE_WIDGET_OVERRIDE()
     macros.

     For example, if creating a new UI implementation class named
     TestServerInterface, the following stub code may be appropriate:

     \code
     // testserverinterface.h
     class TestServerInterface : public QAbstractServerInterface
     {
     Q_OBJECT
     public:
         TestServerInterface(QWidget *parent = 0, Qt::WFlags flags = 0);
         // ... declaration ...
     };

     // testserverinterface.cpp

     // ... definition ...

     QTOPIA_REPLACE_WIDGET(QAbstractServerInterface, TestServerInterface);
     \endcode

     Note that QAbstractServerInterface derived type, like all that use the
     widget replacement mechanism, must have a standard
     \c {(QWidget *, Qt::WFlags)} constructor.

  \o Set the custom implementation as the in-use server interface.

     During startup, the Qtopia Server uses the standard qtopiaWidget() method
     to acquire a server interface instance.  As such, all the mechanisms
     available for configuring server widgets also work in configuring the
     server interface.  In summary,

     \list 1
     \o Explicitly specify the new server interface in the
        \c {Mapping/ServerInterface} key of the \c {Trolltech/ServerWidgets}
        configuration file.
     \o Specify the default in the \c {Trolltech/ServerWidgets} file such that
        it will select your implementation.
     \o Remove all other implementations of QAbstractServerInterface from the
        build.
     \o Use QTOPIA_REPLACE_WIDGET_OVERRIDE() to ensure that the correct
        implementation is always selected.
     \endlist

  \endlist

  The server interface is constructed after all other Qtopia Server tasks.  The
  server constructs it as a toplevel, frameless widget.  If during startup an
  appropriate QAbstractServerInterface implementation cannot be found, the
  system will run without a primary user interface.

  For more details can be found in the \l {QtopiaServerApplication#qtopia-server-widgets}{server widget documentation}.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*!
  Construct a new QAbstractServerInterface with the specified \a parent and
  widget \a flags.
 */
QAbstractServerInterface::QAbstractServerInterface(QWidget *parent, Qt::WFlags flags)
    : QWidget(parent, flags)
{
    // If the server UI is created as frameless, we want it to be
    // below all other windows at all times.
    if ((flags & Qt::FramelessWindowHint) != 0)
        WindowManagement::setLowestWindow(this);
}
