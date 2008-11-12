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

#ifndef CONTENTSERVERINTERFACE_P_H
#define CONTENTSERVERINTERFACE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcontentset.h>

#include <QThread>
#include <QMutex>

/*!
  \internal
  \obsolete
  \class ContentServerInterface
  \mainclass
  Pure virtual base class to provide interface for calling server.
  Alternate notification system has been put in place, and this class will be dissapearing in the near future.
*/
class QTOPIA_EXPORT ContentServerInterface : public QThread
{
public:
    explicit ContentServerInterface( QObject *parent = 0 );
    virtual ~ContentServerInterface();
    // implementations must reimplement QThread::run()

    static bool inServerProcess();
    static ContentServerInterface *server();
    static void addCLS( QContentSet * );
    static void removeCLS( QContentSet * );

    static QList<QContentSet *> &sets();
protected:
    static ContentServerInterface *serverInstance;
    static QMutex listMutex;
    static QList<QContentSet *> linkSets;
};

#endif
