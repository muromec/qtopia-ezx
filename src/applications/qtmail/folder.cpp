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


#include "folder.h"
#include "search.h"
#include "emailfolderlist.h"

Folder::Folder(int type)
{
    _folderType = type;
}

Folder::~Folder()
{
}


// dummy.  Must be reimplemented
bool Folder::matchesEmail(const QMailMessage& message) const
{
    Q_UNUSED(message);
    return true;
}

// dummy.  Must be reimplemented
QString Folder::mailbox() const
{
    return MailboxList::InboxString;
}

QString Folder::name() const
{
    return _name;
}

QString Folder::fullName() const
{
    return _name;
}

QString Folder::displayName() const
{
    return name();
}

void Folder::setName(QString str)
{
    _name = str;
}

QSoftMenuBar::StandardLabel Folder::menuLabel() const
{
    if (mailbox() == MailboxList::DraftsString)
        return QSoftMenuBar::Edit;
    return QSoftMenuBar::View;
}

/*  SystemFolder  */

SystemFolder::SystemFolder(int systemType, const QString &mailbox)
        : Folder( FolderTypeSystem )
{
    _systemType = systemType;
    _mailbox = mailbox;
    _search = NULL;

    if ( _systemType == SystemTypeSearch ) {
        _search = new Search();
        _search->setMailbox( _mailbox );
    }
}

bool SystemFolder::matchesEmail(const QMailMessage& message) const
{
    switch ( _systemType ) {
        case SystemTypeSearch: return _search->matches(message);
        default: return true;           // SystemTypeMailbox
    }
}

QString SystemFolder::mailbox() const
{
    if ( _systemType == SystemTypeSearch )
        return ( _search->mailbox() );

    return _mailbox;
}

bool SystemFolder::isSearch() const
{
    return _systemType == SystemTypeSearch;
}

int SystemFolder::systemType() const
{
    return _systemType;
}

QString SystemFolder::name() const
{
    return MailboxList::mailboxTrName( _mailbox );
}

void SystemFolder::setSearch(Search *in)
{
    if ( _search )
        delete _search;

    _search = in;
}

Search* SystemFolder::search() const
{
    return _search;
}

/*  SearchFolder  */

SearchFolder::SearchFolder(Search *in)
    : Folder( FolderTypeSearch )
{
    _search = in;
}

SearchFolder::~SearchFolder()
{
    delete _search;
}

void SearchFolder::setSearch(Search *in)
{
    _search = in;
}

Search* SearchFolder::search() const
{
    return _search;
}

bool SearchFolder::matchesEmail(const QMailMessage& message) const
{
    return _search->matches(message);
}

QString SearchFolder::mailbox() const
{
    return _search->mailbox();
}

QString SearchFolder::name() const
{
    return _search->name();
}

