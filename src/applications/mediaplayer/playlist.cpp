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

#include <QUrl>
#include <QContent>
#include <QDebug>

#include "playlist.h"


QExplicitlySharedDataPointer<Playlist> Playlist::construct_playlist( const QString& filename )
{
    QExplicitlySharedDataPointer<Playlist> result;

    QUrl    url(filename, QUrl::StrictMode);

    if (url.isValid()) {
        QString scheme = url.scheme();
        if (scheme == "http")
            result = new BasicPlaylist(QStringList(filename));
        else if (scheme == "file" || scheme.isEmpty()) {
            QString checkedName = url.path();
            if (filename.endsWith(".m3u"))
                result = new M3UPlaylist(checkedName);
            else if (checkedName.endsWith(".pls"))
                result = new PLSPlaylist(checkedName);
            else
                result = new BasicPlaylist(QStringList(checkedName));
        }
    }

    return result;
}

MyShufflePlaylist::MyShufflePlaylist( const QContentFilter& filter )
    : m_recent( 15 ), m_filter( filter )
{
    // Generate initial shuffle
    for( int i = 0; i < 6; ++i ) {
        QString random = randomTrack();
        if( !random.isNull() ) {
            m_list.append( random );
        }
    }
}

MyShufflePlaylist::~MyShufflePlaylist()
{ }

void MyShufflePlaylist::setPlaying( const QModelIndex& index )
{
    if( m_playing != index ) {
        if( index.row() > 2 ) {
            for( int i = 0; i < index.row() - 2; ++i ) { // ### FIXME tidy up
                m_list.pop_front();

                // If tracks cued, add a cued track
                // Otherwise, add a random track
                if( m_cued.count() ) {
                    m_list.append( m_cued.takeFirst() );
                } else {
                    QString random = randomTrack();
                    if( !random.isNull() ) {
                        m_list.append( random );
                    }
                }
            }

            m_playing = MyShufflePlaylist::index( 2 );

            // ### FIXME dirty
            emit dataChanged( MyShufflePlaylist::index( 0 ), MyShufflePlaylist::index( m_list.count() - 1 ) );
        } else {
            m_playing = index;
        }

        emit playingChanged( m_playing );
    }
}

QModelIndex MyShufflePlaylist::playing() const
{
    return m_playing;
}

static const QString mapping[3] = {"frequent","infrequent","never"};

void MyShufflePlaylist::setProbability( const QModelIndex& index, Probability probability )
{
    QContent track = m_list[index.row()];

    track.setProperty( "rating", mapping[probability] );
    track.commit();
}

void MyShufflePlaylist::reset()
{
    QContentFilter filter = m_filter & QContentFilter( QContentFilter::Synthetic, "none/rating/never" );

    QContentList tracks = QContentSet( filter ).items();
    foreach( QContent track, tracks ) {
        track.setProperty( "rating", QString() );
        track.commit();
    }
}

void MyShufflePlaylist::cue( QExplicitlySharedDataPointer<Playlist> playlist )
{
    for( int i = 0; i < playlist->rowCount(); ++i ) {
        QString track = playlist->data( playlist->index( i ), Playlist::Url ).toString();
        m_recent.insert( track );

        if( m_cued.count() < 3 ) {
            m_list.insert( 3 + m_cued.count(), track );
            m_cued.prepend( m_list.takeLast() );
        } else {
            m_cued.insert( m_cued.count() - 3, track );
        }
    }

    // ### FIXME dirty
    emit dataChanged( MyShufflePlaylist::index( 0 ), MyShufflePlaylist::index( m_list.count() - 1 ) );
}

void MyShufflePlaylist::playNow( QExplicitlySharedDataPointer<Playlist> playlist )
{
    for( int i = 0; i < playlist->rowCount(); ++i ) {
        QString track = playlist->data( playlist->index( i ), Playlist::Url ).toString();
        m_recent.insert( track );

        m_list.insert( 3 + i, track );
        m_cued.prepend( m_list.takeLast() );
    }

    // ### FIXME dirty
    emit dataChanged( MyShufflePlaylist::index( 0 ), MyShufflePlaylist::index( m_list.count() - 1 ) );

    setPlaying( index( 3 ) );
}

QVariant MyShufflePlaylist::data( const QModelIndex& index, int role ) const 
{
    if( !index.isValid() ) {
        return QVariant();
    }

    if( index.row() < 0 || index.row() >= m_list.count() ) {
        return QVariant();
    }

    QContent content = m_list[index.row()];

    if( content.isValid() ) {
        switch( role )
        {
        case Playlist::Title:
            return content.name();
        case Playlist::Url:
            return content.fileName();
        case Playlist::Artist:
            return content.property( QContent::Artist );
        case Playlist::Album:
            return content.property( QContent::Album );
        case Playlist::Genre:
            return content.property( QContent::Genre );
        case Playlist::AlbumCover:
            {
            QString coverfile = QFileInfo( content.fileName() ).path() + "/cover.jpg";
            if( QFile::exists( coverfile ) ) {
                return coverfile;
            }
            }
            break;
        default:
            // Ignore
            break;
        }
    }
    return QVariant();
}

int MyShufflePlaylist::rowCount( const QModelIndex& ) const
{
    return m_list.count();
}

// 8 biased array
static const int array[8] = {0,1,1,1,1,1,1,2};

QString MyShufflePlaylist::randomTrack() const
{
    static const int MISS_THRESHOLD = 15;

    int bias = array[rand()%8];

    QContentFilter filter = m_filter & ~QContentFilter( QContentFilter::Synthetic, "none/rating/never" );

    switch( bias )
    {
    case 2:
        filter = filter & QContentFilter( QContentFilter::Synthetic, "none/rating/frequent" );
    case 1:
        filter = filter & ~QContentFilter( QContentFilter::Synthetic, "none/rating/infrequent" );
    case 0:
        break;
    }

    QContentList list = QContentSet( filter ).items();

    if( list.isEmpty() ) {
        list = QContentSet( m_filter & ~QContentFilter( QContentFilter::Synthetic, "none/rating/never" ) ).items();
    }

    int misses = 0;
    while( !list.isEmpty() ) {
        int index = rand() % list.count();
        QString file = list[index].fileName();

        if( m_recent.contains( file ) && misses < MISS_THRESHOLD ) {
            ++misses;
            // Go fish
        } else {
            m_recent.insert( file );
            return file;
        }
    }

    return QString();
}

void BasicPlaylist::setPlaying( const QModelIndex& index )
{
    if( m_playing != index ) {
        emit playingChanged( m_playing = index );
    }
}

QModelIndex BasicPlaylist::playing() const
{
    return m_playing;
}

void BasicPlaylist::cue( QExplicitlySharedDataPointer<Playlist> playlist )
{
    int cuepos = m_urls.count();
    beginInsertRows( QModelIndex(), cuepos, cuepos + playlist->rowCount() - 1 );

    for( int i = 0; i < playlist->rowCount(); ++i ) {
        m_urls.append( playlist->data( playlist->index( i ), Playlist::Url ).toString() );
    }

    endInsertRows();
}

void BasicPlaylist::playNow( QExplicitlySharedDataPointer<Playlist> playlist )
{
    int cuepos = m_playing.isValid() ? m_playing.row() + 1 : m_urls.count();

    beginInsertRows( QModelIndex(), cuepos, cuepos + playlist->rowCount() - 1 );

    for( int i = 0; i < playlist->rowCount(); ++i ) {
        m_urls.insert( cuepos + i, playlist->data( playlist->index( i ), Playlist::Url ).toString() );
    }

    endInsertRows();

    setPlaying( index( cuepos ) );
}

void BasicPlaylist::remove( const QModelIndex& index )
{
    if( index == m_playing ) {
        emit playingChanged( m_playing = QModelIndex() );
    }

    beginRemoveRows( QModelIndex(), index.row(), index.row() );

    m_urls.removeAt( index.row() );

    endRemoveRows();
}

void BasicPlaylist::clear()
{
    if( rowCount() <= 0 )
	return;

    beginRemoveRows( QModelIndex(), 0, rowCount() - 1 );

    m_urls.clear();

    endRemoveRows();

    emit playingChanged( m_playing = QModelIndex() );
}

QString BasicPlaylist::suggestedName() const
{
    return tr( "Saved Playlist" );
}

void BasicPlaylist::save( const QString& name )
{
    QContent file;
    file.setName( name );
    file.setType( "audio/mpegurl" );

    QIODevice *io = file.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream stream( io );
    foreach( QString url, m_urls ) {
        stream << url << endl;
    }
    stream.flush();
    delete io;

    file.commit();
}

QVariant BasicPlaylist::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() ) {
        return QVariant();
    }

    if( index.row() < 0 || index.row() >= m_urls.count() ) {
        return QVariant();
    }

    QString item = m_urls[index.row()];

    QContent content;
    if( !item.contains( "://" ) ) {
        content = QContent( item );
    }

    if( content.isValid() ) {
        switch( role )
        {
        case Playlist::Title:
            return content.name();
        case Playlist::Url:
            return item;
        case Playlist::Artist:
            return content.property( QContent::Artist );
        case Playlist::Album:
            return content.property( QContent::Album );
        case Playlist::Genre:
            return content.property( QContent::Genre );
        case Playlist::AlbumCover:
            {
            QString coverfile = QFileInfo( item ).path() + "/cover.jpg";
            if( QFile::exists( coverfile ) ) {
                return coverfile;
            }
            }
            break;
        default:
            // Ignore
            break;
        }
    } else {
        switch( role )
        {
        case Playlist::Title:
        case Playlist::Url:
            return item;
        default:
            // Ignore
            break;
        }
    }

    return QVariant();
}

int BasicPlaylist::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED(parent);
    return m_urls.count();
}

static QStringList parse_m3u_playlist( const QString& filename )
{
    QStringList urls;

    // Open file and populate playlist
    QFile file( filename );

    if( file.open( QIODevice::ReadOnly ) ) {
        // For each line of playlist
        QTextStream stream( &file );
        while( !stream.atEnd() ) {
            QString line = stream.readLine();

            // Ignore blank lines and comments
            if( !line.isEmpty() && line[0] != '#' ) {
                // Add item to list of items
                urls << line;
            }
        }
    }

    return urls;
}

M3UPlaylist::M3UPlaylist( const QString& filename )
    : BasicPlaylist( parse_m3u_playlist( filename ) )
{

}

static inline QString extract_value( const QString& string )
{
    return string.right( string.length() - string.indexOf( '=' ) - 1 );
}

PLSPlaylist::PLSPlaylist( const QString& filename )
{
    QFile file( filename );

    if( file.open( QIODevice::ReadOnly ) ) {
        // For each line of playlist
        QTextStream stream( &file );
        enum { FILE, TITLE } state = FILE;
        Item item;
        while( !stream.atEnd() ) {
            QString line = stream.readLine();

            switch( state )
            {
            case FILE:
                if( line.toLower().startsWith( "file" ) ) {
                    item.url = extract_value( line );
                    state = TITLE;
                }
                break;
            case TITLE:
                if( line.toLower().startsWith( "title" ) ) {
                    item.title = extract_value( line );
                } else {
                    item.title = item.url;
                }

                m_items.append( item );

                state = FILE;
                break;
            }
        }
        // ### TODO clean up here, eof while in title
    }
}

QVariant PLSPlaylist::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() ) {
        return QVariant();
    }

    if( index.row() < 0 || index.row() >= m_items.count() ) {
        return QVariant();
    }

    switch( role )
    {
    case Qt::DisplayRole:
        return m_items[index.row()].title;
    case Qt::UserRole:
        return m_items[index.row()].url;
    default:
        // Ignore
        break;
    }

    return QVariant();
}

int PLSPlaylist::rowCount( const QModelIndex& parent ) const
{
    Q_UNUSED(parent);
    return m_items.count();
}

