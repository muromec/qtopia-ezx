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

#ifndef PLAYERWIDGET_H
#define PLAYERWIDGET_H

#include "playercontrol.h"
#include "statewidget.h"
#include "playlist.h"

#include <media.h>
#include <qmediacontent.h>
#include <qmediacontrol.h>
#include <qmediatools.h>
#include <private/activitymonitor_p.h>

#include <QtGui>

class VisualizationWidget;

class PlayerControl;

class ProgressView;
class VolumeView;
class SeekView;

class ThrottleKeyMapper;

class TrackInfoWidget;
class TrackInfoDialog;

class VoteDialog;

class RepeatState;
class RepeatDialog;

class PlayerWidget : public QWidget
{
    Q_OBJECT
public:
    PlayerWidget( PlayerControl* control, QWidget* parent = 0 );
    ~PlayerWidget();

   QExplicitlySharedDataPointer<Playlist> playlist() const { return m_playlist; }
    // Set playlist and begin playback
    void setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );

    bool eventFilter( QObject* o, QEvent* e );

#ifdef QTOPIA_KEYPAD_NAVIGATION
#ifndef NO_HELIX
    QAction *settingsAction(){ return m_settingsaction; }
#endif
#endif

signals:
    void contentChanged( QMediaContent* content );

public slots:
    void setMediaContent( QMediaContent* content );

private slots:
    void activate();
    void deactivate();

    void activateVideo();
    void deactivateVideo();

    void displayErrorMessage( const QString& message );

    void changeState( QtopiaMedia::State state );

    void setMuteDisplay( bool mute );

    void playingChanged( const QModelIndex& index );

    void pingMonitor();

    void showProgress();

    void cycleView();

    void continuePlaying();

    void toggleMute();

    void execSettings();

    void execTrackInfoDialog();

    void execVoteDialog();

    void execRepeatDialog();

    void delayMenuCreation();

protected:
    void keyPressEvent( QKeyEvent* e );
    void keyReleaseEvent( QKeyEvent* e );

    void showEvent( QShowEvent* e );
    void hideEvent( QHideEvent* e );

private:
    enum View { Progress, Volume, Seek };

    View view() const { return m_currentview; }
    void setView( View view );

    void setVideo( QWidget* widget );
    void removeVideo();

    void setCurrentTrack( const QModelIndex& index );

    void openCurrentTrack();
    void playCurrentTrack();

    PlayerControl *m_playercontrol;
    StateWidget *m_statewidget;

    QMediaContent *m_content;
    QMediaControl *m_mediacontrol;
    QMediaContentContext *m_context;

#ifdef QTOPIA_KEYPAD_NAVIGATION
#ifndef NO_HELIX
    QAction *m_settingsaction;
#endif
#endif

    QVBoxLayout *m_videolayout;
    QWidget *m_videowidget;
    VisualizationWidget *m_visualization;

#ifndef NO_HELIX
    QWidget *m_helixlogoaudio;
    QWidget *m_helixlogovideo;
#endif

    ProgressView *m_progressview;
    VolumeView *m_volumeview;
    SeekView *m_seekview;

    View m_currentview;

    ActivityMonitor *m_monitor;
    QTimer *m_pingtimer;

    ThrottleKeyMapper *m_mapper;

    QExplicitlySharedDataPointer< Playlist > m_playlist;
    QPersistentModelIndex m_currenttrack;

    bool m_continue;

    bool m_ismute;
    QAction *m_muteaction;
    QWidget *m_muteicon;

    TrackInfoWidget *m_trackinfo;
    TrackInfoDialog *m_trackinfodialog;

    QAction *m_voteaction;
    VoteDialog *m_votedialog;

    QAction *m_repeataction;
    RepeatState *m_repeatstate;
    RepeatDialog *m_repeatdialog;
};

#ifndef NO_HELIX
class MediaPlayerSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    MediaPlayerSettingsDialog( QWidget* parent = 0 );

    // QDialog
    void accept();

private:
    void readConfig();
    void writeConfig();

    void applySettings();

    QMediaContent *m_content;

    QComboBox *m_speedcombo;

    QLineEdit *m_connecttimeout;
    QLineEdit *m_servertimeout;
    QValidator *m_validator;
};
#endif

#endif // PLAYERWIDGET_H
