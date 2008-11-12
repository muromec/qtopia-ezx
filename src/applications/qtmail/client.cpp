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


#include "client.h"

Client::Client()
{
}

Client::~Client()
{
}

void Client::setAccount(QMailAccount *)
{
}

void Client::headersOnly(bool, int)
{
}

void Client::newConnection()
{
}

void Client::setSelectedMails(MailList *, bool)
{
}


void Client::quit()
{
}

bool Client::hasDeleteImmediately() const
{
    return false;
}

void Client::deleteImmediately(const QString&)
{
}

void Client::resetNewMailCount()
{
}
