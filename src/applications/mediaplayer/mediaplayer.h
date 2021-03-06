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

#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "playlist.h"

#include <QtGui>

class PlayerControl;
class PlayerWidget;
class MediaBrowser;
class RequestHandler;
class QMediaContentContext;

class MediaPlayer : public QWidget
{
    Q_OBJECT
public:
    MediaPlayer( QWidget* parent = 0, Qt::WFlags f = 0 );
    ~MediaPlayer();

    bool isPlayerVisible() const;
    void setPlayerVisible( bool visible );

    // Open playlist in player
    void setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );
    QExplicitlySharedDataPointer<Playlist> playlist() const { return m_playlist; }

    // Open url in player
    void openUrl( const QString& url );

    static MediaPlayer *instance();

    PlayerWidget *playerWidget();

public slots:
    // Load and parse playlist from file
    void setDocument( const QString& doc );

private slots:
    void playingChanged( const QModelIndex& index );

protected:
    void keyPressEvent( QKeyEvent* e );
    void closeEvent( QCloseEvent* e );

private:
    QLayout *m_layout;
    PlayerControl *m_playercontrol;
    PlayerWidget *m_playerwidget;
    bool m_closeonback, m_acceptclose;
    QMediaContentContext *context;

    RequestHandler *m_requesthandler;
    MediaBrowser *m_mediabrowser;
    QExplicitlySharedDataPointer< Playlist > m_playlist;
};

#endif // MEDIAPLAYER_H
