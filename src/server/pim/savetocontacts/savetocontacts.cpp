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

#include <qtopiaservices.h>
#include <qtopiaipcenvelope.h>
#include <qtopiaapplication.h>

#include "savetocontacts.h"

SavePhoneNumberDialog::SavePhoneNumberDialog(QWidget *parent)
    : PhoneMessageBox(parent)
{
    setTitle( tr("Save to Contacts") );
    setText( "<qt>" + tr("Create a new contact?") + "</qt>" );
    setIcon( PhoneMessageBox::Warning );
    setButtons( PhoneMessageBox::Yes, PhoneMessageBox::No );
}

SavePhoneNumberDialog::~SavePhoneNumberDialog()
{}

void SavePhoneNumberDialog::savePhoneNumber(const QString &number)
{
    SavePhoneNumberDialog diag;
    if (QtopiaApplication::execDialog(&diag) == QAbstractMessageBox::Yes) {
        QtopiaServiceRequest req( "Contacts", "createNewContact(QString)" );
        req << number;
        req.send();
    } else {
        QtopiaServiceRequest req( "Contacts", "addPhoneNumberToContact(QString)" );
        req << number;
        req.send();
    }
}
