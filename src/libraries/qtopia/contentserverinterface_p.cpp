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

#include "contentserverinterface_p.h"
#include <qtopialog.h>

ContentServerInterface *ContentServerInterface::serverInstance = 0;
QList<QContentSet *> ContentServerInterface::linkSets;
QMutex ContentServerInterface::listMutex;

ContentServerInterface::ContentServerInterface( QObject *parent )
    : QThread( parent )
{
}

ContentServerInterface::~ContentServerInterface()
{
}

/*!
  \internal
  Boolean true if we are "in process" with the content server
  Assumes that the ContentServer instance is initialised at boot
  time - prior to that results are undefined.
*/
bool ContentServerInterface::inServerProcess()
{
    return ContentServerInterface::serverInstance != 0;
}

/*!
  \internal
  Return a pointer to the server instance.

  Note this method will not cause an instance to be created.  This will
  return NULL unless an implementing class has created an instance.
*/
ContentServerInterface *ContentServerInterface::server()
{
    return serverInstance;
}

/*!
  Return a reference to the internally maintained list of
  this process's ContentLinkSets
*/
QList<QContentSet *> &ContentServerInterface::sets()
{
    return linkSets;
}

/*!
  Add the QContentSet \a cls to the global (to this process)
  QContentSet list, so that it can receive updates.

    TODO:  Maybe the mutex is not required here, as the order of the
    sets in the list is not significant.  It is not required if the
    internal counts in the Qt implementation of QList are atomically
    incremented.  Serializing access to this list may impact on
    performance.
*/
void ContentServerInterface::addCLS( QContentSet *cls )
{
    qLog(DocAPI) << "ContentServerInterface::addCLS";
    QMutexLocker ml( &listMutex );
    if (!linkSets.contains(cls))
        linkSets.append( cls );
}

/*!
  \internal
  Add the QContentSet \a cls to the global (to this process)
  QContentSet list, so that it can receive updates.

  TODO -  see above re mutex
*/

void ContentServerInterface::removeCLS( QContentSet *cls )
{
    qLog(DocAPI) << "ContentServerInterface::removeCLS";
    QMutexLocker ml( &listMutex );
    linkSets.removeAll( cls );
}
