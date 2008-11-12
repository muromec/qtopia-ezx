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

#include "qabstractsecondarydisplay.h"

/*!
  \class QAbstractSecondaryDisplay
  \brief The QAbstractSecondaryDisplay class allows developers to replace the "secondary screen" portion of the Phone UI.


  The QAbstractSecondaryDisplay interface is part of is part of the 
  \l {QtopiaServerApplication#qtopia-server-widgets}{server widgets framework}
  and represents the portion of the phone UI that is shown on a smaller
  utility screen such as on clam shell phones.
  
  A small tutorial on how to develop new server widgets using one of the abstract widgets as base can
  be found in QAbstractServerInterface class documentation.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.

  \ingroup QtopiaServer::PhoneUI::TTSmartPhone

  */

/*! \fn QAbstractSecondaryDisplay::QAbstractSecondaryDisplay(QWidget *parent, Qt::WFlags flags)

  Construct a new QAbstractSecondaryDisplay with the specified \a parent and
  widget \a flags.
 */

/*!
  \fn void QAbstractSecondaryDisplay::applyBackgroundImage()

  Applies a background image to the secondary display
  */

/*!
  \fn void QAbstractSecondaryDisplay::updateBackground()

   Persist the new background, after applying it.
*/
