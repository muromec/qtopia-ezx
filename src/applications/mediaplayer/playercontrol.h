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

#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H

#include <qmediatools.h>
#include <qmediacontent.h>
#include <qmediacontrol.h>

#include <QtCore>
#include <QPhoneCallManager>

class PlayerControl : public QObject
{
    Q_OBJECT
public:
    PlayerControl( QObject* parent = 0 );
    ~PlayerControl();

    enum State { Playing, Paused, Stopped };

    void open( const QString& url );
    void open( const QContent& content );
    void close();

    State state() const { return m_state; }
    void setState( State state );

    int volume() const { return m_volume; }

signals:
    void contentChanged( QMediaContent* content );
    void stateChanged( PlayerControl::State state );

public slots:
    void setVolume( int volume );
    void setMute( bool mute );

private slots:
    void activate();
    void callsChange ( const QList<QPhoneCall> & calls )  ;

private:
    void setMediaContent( QMediaContent* content );

    QMediaControlNotifier *m_notifier;
    QMediaContent *m_content;
    QMediaControl *m_control;
    QPhoneCallManager *callMgr;
    State m_state;
    State m_prevState;
    int m_volume;
    bool m_ismute;
};

#endif // PLAYERCONTROL_H
