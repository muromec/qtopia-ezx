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

#include "dialerservice.h"
#include "dialercontrol.h"

/*!
    \service DialerService Dialer
    \brief Provides the Qtopia Dialer service.

    The \i Dialer service enables applications to access the system dialer
    to place outgoing calls.
*/

/*!
    \fn DialerService::DialerService( QObject *parent )
    \internal
*/

/*!
    \internal
*/
DialerService::~DialerService()
{
}

/*!
    \fn void DialerService::dialVoiceMail()

    Dial the user's voice mail service.

    This slot corresponds to the QCop service message
    \c{Dialer::dialVoiceMail()}.
*/

/*!
    \fn void DialerService::dial( const QString& name, const QString& number )

    Dial the specified \a number, tagged with the optional \a name.

    This slot corresponds to the QCop service message
    \c{Dialer::dial(QString,QString)}.
*/

/*!
    \fn void DialerService::dial( const QString& number, const QUniqueId& contact )

    Dial the specified \a contact, using the given \a number.

    This slot corresponds to the QCop service message
    \c{Dialer::dial(QUniqueId,QString)}.
*/

/*!
    \fn void DialerService::showDialer( const QString& digits )

    Displays the dialer, preloaded with \a digits.

    This slot corresponds to the QCop service message
    \c{Dialer::showDialer(QString)}.
*/
