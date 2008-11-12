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

#ifndef SERVICEREQUEST_H
#define SERVICEREQUEST_H

#include "menumodel.h"

#include <QtCore>

class ServiceRequest
{
public:
    enum Type { PushTitle, PushMenu, OpenUrl, OpenPlaylist, CuePlaylist, PlayNow, ShowPlayer, TriggerSlot, Compound, User = 0xff };

    virtual ~ServiceRequest() { }

    virtual Type type() const = 0;
    virtual ServiceRequest* clone() const = 0;
};

class PushTitleRequest : public ServiceRequest
{
public:
    explicit PushTitleRequest( const QString& title )
        : m_title( title )
    { }

    QString title() const { return m_title; }

    // ServiceRequest
    Type type() const { return PushTitle; }
    ServiceRequest* clone() const { return new PushTitleRequest( m_title ); }

private:
    QString m_title;
};

class PushMenuRequest : public ServiceRequest
{
public:
    explicit PushMenuRequest( MenuModel* menu )
        : m_menu( menu )
    { }

    MenuModel* menu() { return m_menu; }

    // ServiceRequest
    Type type() const { return PushMenu; }
    ServiceRequest* clone() const { return new PushMenuRequest( m_menu ); }

private:
    MenuModel *m_menu;
};

class OpenUrlRequest : public ServiceRequest
{
public:
    explicit OpenUrlRequest( const QString& url )
        : m_url( url )
    { }

    QString url() const { return m_url; }

    // ServiceRequest
    Type type() const { return OpenUrl; }
    ServiceRequest* clone() const { return new OpenUrlRequest( m_url ); }

private:
    QString m_url;
};

class Playlist;

class OpenPlaylistRequest : public ServiceRequest
{
public:
    explicit OpenPlaylistRequest( QExplicitlySharedDataPointer<Playlist> playlist )
        : m_playlist( playlist )
    { }

    QExplicitlySharedDataPointer<Playlist> playlist() const { return m_playlist; }

    // ServiceRequest
    Type type() const { return OpenPlaylist; }
    ServiceRequest* clone() const { return new OpenPlaylistRequest( m_playlist ); }

private:
    QExplicitlySharedDataPointer<Playlist> m_playlist;
};

class CuePlaylistRequest : public ServiceRequest
{
public:
    explicit CuePlaylistRequest( QExplicitlySharedDataPointer<Playlist> playlist )
        : m_playlist( playlist )
    { }

    QExplicitlySharedDataPointer<Playlist> playlist() const { return m_playlist; }

    // ServiceRequest
    Type type() const { return CuePlaylist; }
    ServiceRequest* clone() const { return new CuePlaylistRequest( m_playlist ); }

private:
    QExplicitlySharedDataPointer<Playlist> m_playlist;
};

class PlayNowRequest : public ServiceRequest
{
public:
    explicit PlayNowRequest( QExplicitlySharedDataPointer<Playlist> playlist )
        : m_playlist( playlist )
    { }

    QExplicitlySharedDataPointer<Playlist> playlist() const { return m_playlist; }

    // ServiceRequest
    Type type() const { return PlayNow; }
    ServiceRequest* clone() const { return new PlayNowRequest( m_playlist ); }

private:
    QExplicitlySharedDataPointer<Playlist> m_playlist;
};

class ShowPlayerRequest : public ServiceRequest
{
public:
    // ServiceRequest
    Type type() const { return ShowPlayer; }
    ServiceRequest* clone() const { return new ShowPlayerRequest; }
};

class TriggerSlotRequest : public ServiceRequest
{
public:
    TriggerSlotRequest( QObject *receiver, const char* member )
        : m_receiver( receiver ), m_member( member )
    { }

    QObject* receiver() const { return m_receiver; }
    const char* member() const { return m_member; }

    // ServiceRequest
    Type type() const { return TriggerSlot; }
    ServiceRequest* clone() const { return new TriggerSlotRequest( m_receiver, m_member ); }

private:
    QObject* m_receiver;
    const char* m_member;
};

class CompoundRequest : public ServiceRequest
{
public:
    explicit CompoundRequest( const QList<ServiceRequest*>& requests );
    ~CompoundRequest();

    QList<ServiceRequest*> requests() const { return m_requests; }

    // ServiceRequest
    Type type() const { return Compound; }
    ServiceRequest* clone() const { return new CompoundRequest( m_requests ); }

private:
    QList<ServiceRequest*> m_requests;
};

inline CompoundRequest::CompoundRequest( const QList<ServiceRequest*>& requests )
{
    foreach( ServiceRequest* request, requests ) {
        m_requests.append( request->clone() );
    }
}

inline CompoundRequest::~CompoundRequest()
{
    foreach( ServiceRequest* request, m_requests ) {
        delete request;
    }
}

#endif // SERVICEREQUEST_H
