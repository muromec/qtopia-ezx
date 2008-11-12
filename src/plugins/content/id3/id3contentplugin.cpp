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

#include "id3contentplugin.h"
#include "id3tag.h"
#include <qmimetype.h>
#include <QtDebug>

/*!
    \class Id3ContentHandler
    \internal
 */

static const char *genreLookup[] =
{
    "Blues",
    "Classic Rock",
    "Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "Alternative Rock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native America",
    "Cabaret",
    "New Wave",
    "Psychdelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock"
};

/*!
    \class Id3ContentPlugin
    \internal

    Plugin for reading content information from ID3 tags in media files.
*/

Id3ContentPlugin::Id3ContentPlugin()
{
}

Id3ContentPlugin::~Id3ContentPlugin()
{
}

QStringList Id3ContentPlugin::keys() const
{
    return QMimeType::fromId( QLatin1String( "audio/mpeg" ) ).extensions();
}

bool Id3ContentPlugin::installContent( const QString &filePath, QContent *content )
{
    QFile file( filePath );

    bool success = false;

    if( file.open( QIODevice::ReadOnly ) )
    {
        if( file.peek( 3 ) == "ID3" )
        {
            Id3Tag tag( &file );

            content->setName( name( filePath, tag ) );
            content->setFile( filePath );
            content->setType( QMimeType::fromFileName( filePath ).id() );
            content->setRole( QContent::Document );

            QList< QPair< quint32, qint64 > > frames = tag.framePositions();

            QPair< quint32, qint64 > framePair;

            foreach( framePair, frames )
            {
                if( framePair.first == *reinterpret_cast< const quint32 * >( "TPE1"  ) ||
                    framePair.first == *reinterpret_cast< const quint32 * >( "TP1\0" ) )
                {
                    content->setProperty( QContent::Artist, string( tag, framePair.second ) );
                }
                if( framePair.first == *reinterpret_cast< const quint32 * >( "TALB"  ) ||
                    framePair.first == *reinterpret_cast< const quint32 * >( "TAL\0" ) )
                {
                    content->setProperty( QContent::Album, string( tag, framePair.second ) );
                }
                if( framePair.first == *reinterpret_cast< const quint32 * >( "TCON"  ) ||
                    framePair.first == *reinterpret_cast< const quint32 * >( "TCO\0" ) )
                {
                    content->setProperty( QContent::Genre, genre( tag, framePair.second ) );
                }
                if( framePair.first == *reinterpret_cast< const quint32 * >( "TRCK"  ) ||
                    framePair.first == *reinterpret_cast< const quint32 * >( "TRK\0" ) )
                {
                    // Ignore /x in x/x format track fields
                    QString track = string( tag, framePair.second );
                    int index = track.indexOf( '/' );
                    if( index != -1 ) {
                        track = track.left( index );
                    }

                    // Prepend 0 to single digit tracks
                    if( track.count() == 1 ) {
                        track.prepend( "0" );
                    }

                    content->setProperty( QContent::Track, track );
                }
            }

            success = true;
        }
        file.close();
    }

    return success;
}

bool Id3ContentPlugin::updateContent( QContent *content )
{
    return installContent( content->fileName(), content );
}

QString Id3ContentPlugin::name( const QString &filePath, Id3Tag &tag ) const
{
    QList< QPair< quint32, qint64 > > frames = tag.framePositions();

    QPair< quint32, qint64 > framePair;

    foreach( framePair, frames )
    {
        if( framePair.first == *reinterpret_cast< const quint32 * >( "TIT2"  ) ||
            framePair.first == *reinterpret_cast< const quint32 * >( "TT2\0" ) )
        {
            QString name = string( tag, framePair.second );

            if( !name.isEmpty() )
                return name;
        }
    }

    return QFileInfo( filePath ).baseName();
}

QString Id3ContentPlugin::string( Id3Tag &tag, qint64 framePosition ) const
{
    Id3Frame *frame = tag.readFrame( framePosition );

    if( !frame )
        return QString();

    if( !frame->isTextFrame() )
    {
        delete frame;

        return QString();
    }

    QStringList strings = frame->textFields();

    delete frame;

    return strings.first();
}

QString Id3ContentPlugin::genre( Id3Tag &tag, qint64 framePosition ) const
{
    QString genreId = string( tag, framePosition );

    if( genreId.startsWith( '(' ) && genreId.endsWith( ')' ) )
        genreId = genreId.mid( 1, genreId.size() - 2 );

    bool ok;

    uint index = genreId.toInt( &ok );

    if( ok && index < sizeof( genreLookup ) / sizeof( char * ) )
        return QLatin1String( genreLookup[ index ] );
    else
        return genreId;
}


QTOPIA_EXPORT_PLUGIN(Id3ContentPlugin);

