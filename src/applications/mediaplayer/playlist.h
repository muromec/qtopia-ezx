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

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <qcontentset.h>
#include <QSharedData>

#include <QtGui>

class Playlist : public QAbstractListModel, public QSharedData
{
    Q_OBJECT
public:
    enum DataRole { Title = Qt::DisplayRole, Url = Qt::UserRole, Artist, Album, Genre, AlbumCover };

    virtual QModelIndex playing() const = 0;
    virtual void setPlaying( const QModelIndex& index ) = 0;

    static QExplicitlySharedDataPointer<Playlist> construct_playlist( const QString& filename );
signals:
    void playingChanged( const QModelIndex& index );
};

class PlaylistCue
{
public:
    virtual ~PlaylistCue() { }

    virtual void cue( QExplicitlySharedDataPointer<Playlist> playlist ) = 0;
    virtual void playNow( QExplicitlySharedDataPointer<Playlist> playlist ) = 0;
};

Q_DECLARE_INTERFACE(PlaylistCue,
    "com.trolltech.Qtopia.MediaPlayer.PlaylistCue/1.0")

class PlaylistRemove
{
public:
    virtual ~PlaylistRemove() { }

    // Remove item given by index from playlist
    virtual void remove( const QModelIndex& index ) = 0;
    // Remove all items from playlist
    virtual void clear() = 0;
};

Q_DECLARE_INTERFACE(PlaylistRemove,
    "com.trolltech.Qtopia.MediaPlayer.PlaylistRemove/1.0")

class PlaylistSave
{
public:
    virtual ~PlaylistSave() { }

    // Suggest name for playlist
    virtual QString suggestedName() const = 0;
    // Save playlist using given name
    virtual void save( const QString& name ) = 0;
};

Q_DECLARE_INTERFACE(PlaylistSave,
    "com.trolltech.Qtopia.MediaPlayer.PlaylistSave/1.0")

class PlaylistMyShuffle
{
public:
    virtual ~PlaylistMyShuffle() { }

    enum Probability { Frequent, Infrequent, Never };

    // Set track probability
    virtual void setProbability( const QModelIndex& index, Probability probability ) = 0;

    // Reset all track ratings
    virtual void reset() = 0;
};

Q_DECLARE_INTERFACE(PlaylistMyShuffle,
    "com.trolltech.Qtopia.MediaPlayer.PlaylistMyShuffle/1.0")

class StringCache
{
public:
    StringCache( int size )
        : m_cache( size ), m_index( 0 ), m_size( size )
    { }

    void insert( const QString& string )
    {
        m_cache[m_index++%m_size] = string;
    }

    bool contains( const QString& string ) const
    {
        return m_cache.contains( string );
    }

private:
    QVector<QString> m_cache;
    int m_index, m_size;
};

class MyShufflePlaylist : public Playlist,
    public PlaylistMyShuffle,
    public PlaylistCue
{
    Q_OBJECT
    Q_INTERFACES(PlaylistMyShuffle)
    Q_INTERFACES(PlaylistCue)
public:
    MyShufflePlaylist( const QContentFilter& filter = QContentFilter() );
    ~MyShufflePlaylist();

    // PlaylistMyShuffle
    void setProbability( const QModelIndex& index, Probability probability );
    void reset();

    // PlaylistCue
    void cue( QExplicitlySharedDataPointer<Playlist> playlist );
    void playNow( QExplicitlySharedDataPointer<Playlist> playlist );

    // Playlist
    QModelIndex playing() const;
    void setPlaying( const QModelIndex& index );

    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;

    int rowCount( const QModelIndex& parent = QModelIndex() ) const;

private:
    // Return the filename of random track
    // Track is biased towards frequent
    QString randomTrack() const;

    mutable StringCache m_recent;
    QContentFilter m_filter;
    QStringList m_list;
    QStringList m_cued;
    QPersistentModelIndex m_playing;
};

class BasicPlaylist : public Playlist,
    public PlaylistCue,
    public PlaylistRemove,
    public PlaylistSave
{
    Q_OBJECT
    Q_INTERFACES(PlaylistCue)
    Q_INTERFACES(PlaylistRemove)
    Q_INTERFACES(PlaylistSave)
public:
    BasicPlaylist( const QStringList& urls = QStringList() )
        : m_urls( urls )
    { }

    // Playlist
    void setPlaying( const QModelIndex& index );
    QModelIndex playing() const;

    // PlaylistCue
    void cue( QExplicitlySharedDataPointer<Playlist> playlist );
    void playNow( QExplicitlySharedDataPointer<Playlist> playlist );

    // PlaylistRemove
    void remove( const QModelIndex& index );
    void clear();

    // PlaylistSave
    QString suggestedName() const;
    void save( const QString& name );

    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const;

private:
    QStringList m_urls;
    QPersistentModelIndex m_playing;
};

class M3UPlaylist : public BasicPlaylist
{
public:
    M3UPlaylist( const QString& filename );
};

class PLSPlaylist : public Playlist
{
public:
    PLSPlaylist( const QString& filename );

    void setPlaying( const QModelIndex& index ) { m_playing = index; }
    QModelIndex playing() const { return m_playing; }

    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const;

private:
    struct Item
    {
        QString url;
        QString title;
    };

    QPersistentModelIndex m_playing;
    QList<Item> m_items;
};

#endif // PLAYLIST_H
