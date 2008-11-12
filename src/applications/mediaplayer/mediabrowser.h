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

#ifndef MEDIABROWSER_H
#define MEDIABROWSER_H

#include "playercontrol.h"
#include "requesthandler.h"
#include "menumodel.h"
#include "menuview.h"

#include <qmediacontent.h>
#include <private/activitymonitor_p.h>

class TitleBar;
class PlaylistMenuModel;
class ItemDelegate;
class CustomView;
class AnimationDirector;
class HelpDirector;
class IndexHistory;
class ActionGroup;
class Playlist;

class MediaBrowser : public QWidget
{
    Q_OBJECT
public:
    MediaBrowser( PlayerControl* control, RequestHandler* handler = 0, QWidget* parent = 0 );
    ~MediaBrowser();

    void setCurrentPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );

    bool hasBack() const;
    void goBack();

    // QWidget
    bool eventFilter( QObject* o, QEvent* e );

public slots:
    void setMediaContent( QMediaContent* content );

private slots:
    void executeSelectedAction( const QModelIndex& index );
    void executeHeldAction( const QModelIndex& index );
    void executeHeldLongAction( const QModelIndex& index );

    void removePlaylistItem();
    void clearPlaylist();

    void savePlaylist();

    void generateMyShuffle();
    void resetMyShuffle();

    void enableNowPlaying();

    void directCurrentChange();
    void directSelectRelease();

    void executeShowPlayerRequest();

    void delayMenuCreation();
    void execSettings();

private:
    RequestHandler *m_requesthandler;
    RequestHandler *m_requesthandlerparent;

    CustomView *m_view;
    MenuStack *m_stack;

    AnimationDirector *m_director;
    HelpDirector *m_helpdirector;

    ItemDelegate *m_delegate;

    IndexHistory *m_history;

    SimpleMenuModel *m_mainmenu;
    PlaylistMenuModel *m_currentplaylistmenu;
    PlaylistMenuModel *m_myshufflemenu;

    ActionGroup *m_removegroup;
    ActionGroup *m_savegroup;
    ActionGroup *m_resetgroup;

    TitleBar *m_titlebar;

    QExplicitlySharedDataPointer< Playlist > m_playlist;
    bool m_hasnowplaying;

    ActivityMonitor *m_browsermonitor;
    bool m_focused;
};

#endif // MEDIABROWSER_H
