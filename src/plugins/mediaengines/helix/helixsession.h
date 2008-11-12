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

#ifndef __QTOPIA_MEDIA_HELIXSESSION_H
#define __QTOPIA_MEDIA_HELIXSESSION_H

#include <QString>
#include <QUuid>

#include <QMediaServerSession>

#include "observer.h"

class IHXClientEngine;

namespace qtopia_helix
{

class HelixSessionPrivate;

class HelixSession :
    public QMediaServerSession,
    public Observer
{
    Q_OBJECT

public:
    HelixSession(IHXClientEngine* engine, QUuid const& id, QString const& url);
    ~HelixSession();

    void start();
    void pause();
    void stop();

    void suspend();
    void resume();

    void seek(quint32 ms);
    quint32 length();

    void setVolume(int volume);
    int volume() const;

    void setMuted(bool mute);
    bool isMuted() const;

    QtopiaMedia::State playerState() const;

    QString errorString();

    void setDomain(QString const& domain);
    QString domain() const;

    QStringList interfaces();

    QString id() const;
    QString reportData() const;

    void update(Subject* subject);

private slots:
    void delaySeek();

private:
    void startupPlayer();
    void shutdownPlayer();

    HelixSessionPrivate*    d;
};

}


#endif  //__QTOPIA_MEDIA_HELIXSESSION_H

