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

#include <qdebug.h>

#include <qmediacontrol.h>
#include <qmediavideocontrol.h>
#include <qmediavideocontrolserver.h>

#include <qmediahandle_p.h>

#include "helixplayer.h"
#include "qmediahelixsettingsserver.h"

#include "helixsession.h"



namespace qtopia_helix
{


// {{{ HelixSessionPrivate
class HelixSessionPrivate
{
public:
    QUuid                       id;
    QString                     url;
    QString                     domain;
    QStringList                 interfaces;
    IHXClientEngine*            engine;
    HelixPlayer*                player;
    QMediaVideoControlServer*   videoControlServer;
    HelixVideo*                 videoWidget;
    bool                        ismute;
    bool                        suspended;
    quint32                     position;
    quint32                     oldposition;
    quint32                     length;
    QtopiaMedia::State          state;
};
// }}}

// {{{ HelixSession
HelixSession::HelixSession(IHXClientEngine* engine, QUuid const& id, QString const& url):
    d(new HelixSessionPrivate)
{
    d->id = id;
    d->url = url;
    d->interfaces << QMediaControl::name();
    d->engine = engine;
    d->videoControlServer = 0;
    d->videoWidget = 0;
    d->ismute = false;
    d->suspended = false;
    d->position = 0;
    d->length = 0;
    d->state = QtopiaMedia::Stopped;

    startupPlayer();
}

HelixSession::~HelixSession()
{
    stop();

    shutdownPlayer();

    delete d->videoControlServer;
    delete d->videoWidget;
}

void HelixSession::start()
{
    if (d->player->playerState() == QtopiaMedia::Stopped) {
        d->player->open(d->url);
    }

    d->player->play();
}

void HelixSession::pause()
{
    d->player->pause();
}

void HelixSession::stop()
{
    d->player->stop();
}

void HelixSession::suspend()
{
    pause();
    d->oldposition = d->player->position();
    d->suspended = true;
    stop();
}

void HelixSession::resume()
{
    start();
    QTimer::singleShot(50, this, SLOT(delaySeek()));
}

void HelixSession::seek(quint32 ms)
{
    d->player->seek(ms);
}

quint32 HelixSession::length()
{
    return d->player->length();
}

void HelixSession::setVolume(int volume)
{
    d->player->setVolume(volume);
}

int HelixSession::volume() const
{
    return d->player->volume();
}

void HelixSession::setMuted(bool mute)
{
    d->player->setMuted(mute);
}

bool HelixSession::isMuted() const
{
    return d->ismute;
}

QtopiaMedia::State HelixSession::playerState() const
{
    return d->state;
}

QString HelixSession::errorString()
{
    return d->player->errorString();
}

void HelixSession::setDomain(QString const& domain)
{
    d->domain = domain;
}

QString HelixSession::domain() const
{
    return d->domain;
}

QStringList HelixSession::interfaces()
{
    return d->interfaces;
}

QString HelixSession::id() const
{
    return d->id;
}

QString HelixSession::reportData() const
{
    return d->url;
}

// {{{ Observer
void HelixSession::update(Subject* subject)
{
    static const int MS_PER_S = 1000; // 1000 milliseconds per second

    PlayerState*    playerState;
    VideoRender*    videoRender;
    ErrorReport*    errorReport;

    if ((playerState = qobject_cast<PlayerState*>(d->player)) == subject)
    {
        QtopiaMedia::State state = playerState->playerState();

        if (d->state != state)
        {
            d->state = state;
            if (!d->suspended)
                emit playerStateChanged(d->state);
        }
    }
    else
    if (qobject_cast<PlaybackStatus*>(d->player) == subject)
    {
        quint32 position = (d->player->position() / MS_PER_S) * MS_PER_S;
        if( d->position != position ) {
            d->position = position;
            emit positionChanged(d->position);
        }

        quint32 length = d->player->length();
        if( d->length != length ) {
            d->length = length;
            emit lengthChanged(d->length);
        }
    }
    else
    if ((qobject_cast<VolumeControl*>(d->player)) == subject)
    {
        emit volumeChanged(d->player->volume());

        bool mute = d->player->isMuted();
        if( d->ismute != mute ) {
            d->ismute = mute;

            emit volumeMuted(d->ismute);
        }
    }
    else
    if ((videoRender = qobject_cast<VideoRender*>(d->player)) == subject)
    {
        if (videoRender->hasVideo())
        {
            d->videoWidget = videoRender->createVideoWidget();

            d->videoControlServer = new QMediaVideoControlServer(QMediaHandle(d->id));
            d->videoControlServer->setRenderTarget(d->videoWidget->winId());

            d->interfaces.append(QMediaVideoControl::name());

            emit interfaceAvailable(QMediaVideoControl::name());
        }
        else
        {
            d->interfaces.removeAll(QMediaVideoControl::name());
            emit interfaceUnavailable(QMediaVideoControl::name());

            delete d->videoWidget;
            d->videoWidget = 0;

            delete d->videoControlServer;
            d->videoControlServer = 0;
        }
    }
    else
    if ((errorReport = qobject_cast<ErrorReport*>(d->player)) == subject)
    {
        emit playerStateChanged(QtopiaMedia::Error);
    }
}
// }}}

void HelixSession::delaySeek()
{
    static int count = 0;

    if (d->suspended && count++ > 2)
    {
        d->suspended = false;
        seek(d->oldposition);
        d->player->play();
        count = 0;
    }
    else
        QTimer::singleShot(200, this, SLOT(delaySeek()));
}

void HelixSession::startupPlayer()
{
    d->player = new HelixPlayer(d->engine);

    PlayerState *playerState = qobject_cast<PlayerState*>(d->player);
    if (playerState != 0)
        playerState->attach(this);

    ErrorReport *error = qobject_cast<ErrorReport*>(d->player);
    if (error != 0)
        error->attach(this);

    PlaybackStatus *status = qobject_cast<PlaybackStatus*>(d->player);
    if (status != 0)
        status->attach(this);

    VolumeControl *volume = qobject_cast<VolumeControl*>(d->player);
    if (volume != 0)
        volume->attach(this);

    VideoRender *videoRender = qobject_cast<VideoRender*>(d->player);
    if (videoRender != 0)
        videoRender->attach(this);
}

void HelixSession::shutdownPlayer()
{
    PlayerState *playerState = qobject_cast<PlayerState*>(d->player);
    if (playerState != 0)
        playerState->detach(this);

    ErrorReport *error = qobject_cast<ErrorReport*>(d->player);
    if (error != 0)
        error->detach(this);

    PlaybackStatus *status = qobject_cast<PlaybackStatus*>(d->player);
    if (status != 0)
        status->detach(this);

    VolumeControl *volume = qobject_cast<VolumeControl*>(d->player);
    if (volume != 0)
        volume->detach(this);

    VideoRender *videoRender = qobject_cast<VideoRender*>(d->player);
    if (videoRender != 0)
        videoRender->detach(this);

    delete d->player;
}
// }}}


}   // ns qtopia_helix

