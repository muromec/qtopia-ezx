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

#ifndef __MEDIASERVER_MEDIAAGENT_H
#define __MEDIASERVER_MEDIAAGENT_H

#include <QObject>


class QMediaSessionRequest;
class QMediaServerSession;
class QMediaEngineInformation;

namespace mediaserver
{

class MediaAgentSession;

class MediaAgentPrivate;

class MediaAgent : public QObject
{
    Q_OBJECT
    friend class MediaAgentSession;

public:
    ~MediaAgent();

    QMediaServerSession* createSession(QMediaSessionRequest const& sessionRequest);
    void destroySession(QMediaServerSession* mediaSession);

    static MediaAgent* instance();

private:
    MediaAgent();

    void initialize();

    void sessionStarting(MediaAgentSession* session);
    void sessionStopped(MediaAgentSession* session);

    MediaAgentPrivate*  d;
};


}   // ns mediaserver

#endif  // __MEDIASERVER_MEDIAAGENT_H

