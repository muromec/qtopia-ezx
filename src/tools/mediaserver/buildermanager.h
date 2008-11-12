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

#ifndef __MEDIASERVER_BUILDERMANAGER_H
#define __MEDIASERVER_BUILDERMANAGER_H

#include <QString>

#include <QMediaSessionBuilder>


class QMediaServerSession;


namespace mediaserver
{

class BuilderManagerPrivate;

class BuilderManager
{
public:
    BuilderManager();
    ~BuilderManager();

    void addBuilders(QString const& engineName,
                     QMediaSessionBuilderList const& builderList);

    void removeBuilders(QString const& engineName,
                    QMediaSessionBuilderList const& builderList);

    QMediaServerSession* createSession(QMediaSessionRequest const& sessionRequest);
    void destroySession(QMediaServerSession* mediaSession);

private:
    BuilderManagerPrivate*  d;
};

}   // ns mediaserver

#endif  // __MEDIASERVER_BUILDERMANAGER_H

