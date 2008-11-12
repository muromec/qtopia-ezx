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



#include "maillist.h"

void MailList::clear()
{
    QListIterator<dList*> it(sortedList);
    while ( it.hasNext() )
        delete it.next();
    sortedList.clear();
    currentPos = 0;
}

int MailList::count()
{
    return sortedList.count();
}

int MailList::size()
{
    int total = 0;
    QListIterator<dList*> it(sortedList);
    while ( it.hasNext() )
        total += it.next()->size;

    return total;
}

int MailList::currentSize()
{
    dList *mPtr;

    if (currentPos == 0)
        return -1;

    mPtr = sortedList.at(currentPos - 1);
    return mPtr->size;
}

QMailId MailList::currentId()
{
    dList *mPtr;

    if (currentPos == 0)
        return QMailId();

    mPtr = sortedList.at(currentPos - 1);
    return mPtr->internalId;
}

QString MailList::currentMailbox()
{
    dList *mPtr;

    if (currentPos == 0)
        return QString();

    mPtr = sortedList.at(currentPos - 1);
    return mPtr->fromBox;
}

QString* MailList::first()
{
    dList *mPtr;

    if (sortedList.count() == 0)
        return NULL;

    mPtr = sortedList.at(0);
    currentPos = 1;
    return &(mPtr->serverId);
}

QString* MailList::next()
{
    dList *mPtr;

    if ( currentPos >= static_cast<uint>(sortedList.count()))
        return NULL;

    mPtr = sortedList.at(currentPos);
    currentPos++;
    return &(mPtr->serverId);
}

void MailList::sizeInsert(QString serverId, uint size, QMailId id, QString box)
{
    int x = 0;

    dList *newEntry = new dList;
    newEntry->serverId = serverId;
    newEntry->size = size;
    newEntry->internalId = id;
    newEntry->fromBox = box;

    QListIterator<dList*> it(sortedList);
    while ( it.hasNext() ) {
        if (newEntry->size < it.next()->size) {
            sortedList.insert(x, newEntry);
            return;
        }
        ++x;
    }
    sortedList.append(newEntry);
}

void MailList::append(QString serverId, uint size, QMailId id, QString box)
{
    dList *newEntry = new dList;
    newEntry->serverId = serverId;
    newEntry->size = size;
    newEntry->internalId = id;
    newEntry->fromBox = box;

    sortedList.append(newEntry);
}

void MailList::moveFront(QString serverId)
{
    dList *currentPtr;
    uint tempPos;

    tempPos = currentPos;
    if ( tempPos >= static_cast<uint>(sortedList.count()) )
        return;
    currentPtr = sortedList.at(tempPos);
    while ( ((tempPos+1) < static_cast<uint>(sortedList.count())) && ( currentPtr->serverId != serverId) ) {
        tempPos++;
        currentPtr = sortedList.at(tempPos);
    }

    if ( (currentPtr != NULL) && (currentPtr->serverId == serverId) ) {
        qWarning(QString("moved to front, message: " + serverId).toLatin1());

        dList *itemPtr = sortedList.takeAt(tempPos);
        sortedList.insert(currentPos, itemPtr);
    }
}

//only works if mail is not already in download
bool MailList::remove(QString serverId)
{
    dList *currentPtr;
    uint tempPos;

    tempPos = currentPos;
    if ( tempPos >= static_cast<uint>(sortedList.count()) )
        return false;
    currentPtr = sortedList.at(tempPos);
    while ( ((tempPos + 1) < static_cast<uint>(sortedList.count())) && ( currentPtr->serverId != serverId) ) {
        tempPos++;
        currentPtr = sortedList.at(tempPos);
    }

    if ( (currentPtr != NULL) && (currentPtr->serverId == serverId) ) {
        qWarning(QString("deleted message: " + serverId).toLatin1());
        sortedList.removeAt(tempPos);

        return true;
    }
    return false;
}

