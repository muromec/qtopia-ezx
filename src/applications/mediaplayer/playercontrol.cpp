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

#include "playercontrol.h"
#include <QtopiaApplication>

static const int VOLUME_MAX = 100;

PlayerControl::PlayerControl( QObject* parent )
    : QObject( parent ), m_content( 0 ), m_control( 0 ), m_state( Stopped ), m_volume( VOLUME_MAX ), m_ismute( false )
{
    m_notifier = new QMediaControlNotifier( QMediaControl::name(), this );
    connect( m_notifier, SIGNAL(valid()), this, SLOT(activate()) );
}

PlayerControl::~PlayerControl()
{
    delete m_control;
    delete m_content;
}

void PlayerControl::open( const QString& url )
{
    setMediaContent( new QMediaContent( QUrl(url) ) );
}

void PlayerControl::open( const QContent& content )
{
    setMediaContent( new QMediaContent( content ) );
}

void PlayerControl::close()
{
    setMediaContent( 0 );
}

void PlayerControl::setState( State state )
{
    if( state != m_state ) {
        m_state = state;

        if( m_control ) {
            switch( m_state )
            {
            case Playing:
                m_control->start();
                QtopiaApplication::instance()->registerRunningTask( "Media Player", this );
                break;
            case Paused:
                m_control->pause();
                break;
            case Stopped:
                m_control->stop();
                QtopiaApplication::instance()->unregisterRunningTask( this );
                break;
            }
        }

        emit stateChanged( m_state );
    }
}

void PlayerControl::setVolume( int volume )
{
    if(m_volume != volume)
    {
        m_volume = volume;
        QSettings config( "Trolltech", "MediaPlayer" );
        config.setValue( "Volume", volume );
    }
}

void PlayerControl::setMute( bool mute )
{
    m_ismute = mute;
}

void PlayerControl::activate()
{
    m_control = new QMediaControl( m_content );
    connect( m_control, SIGNAL(volumeChanged(int)),
        this, SLOT(setVolume(int)) );
    connect( m_control, SIGNAL(volumeMuted(bool)),
        this, SLOT(setMute(bool)) );

    m_control->setVolume( m_volume );
    m_control->setMuted( m_ismute );

    if( state() == Playing ) {
        m_control->start();
    }
}

void PlayerControl::setMediaContent( QMediaContent* content )
{
    delete m_control;
    m_control = 0;

    m_notifier->setMediaContent( content );
    emit contentChanged( content );

    m_content = content;
}
