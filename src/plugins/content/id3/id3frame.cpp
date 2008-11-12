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

#include "id3frame.h"
#include "id3tag.h"
#include <QBuffer>
#include <QTextStream>
#include <QtDebug>
#include <QtCore/qendian.h>

/*!
    \class Id3Frame
    \internal
 */

Id3Frame::Id3Frame( const Header &h, const QByteArray &d, quint8 id3MajorVersion, bool tagUnsynchronisation )
    : header( h )
    , data( d )
    , majorVersion( id3MajorVersion )
{
    if( header.flags & Unsynchronisation || tagUnsynchronisation )
        data = Id3Tag::unsynchronise( data );

    if( header.flags & Compression )
        data = qUncompress( data );

    if( header.flags & DataLengthIndicator && data.size() >= 4 )
    {
        {
            QDataStream stream( data );

            stream.setByteOrder( QDataStream::BigEndian );

            stream >> header.size;

            header.size = Id3Tag::convertSyncSafeInteger( header.size );
        }

        data = data.right( data.size() - 4 );
    }
}

Id3Frame::~Id3Frame()
{

}

bool Id3Frame::isTextFrame() const
{
    return header.idBytes[ 0 ] == 'T';
}

quint32 Id3Frame::id() const
{
    return header.id;
}

QStringList Id3Frame::textFields()
{
    QStringList strings;

    if( header.idBytes[ 0 ] == 'T' )
    {
        QBuffer buffer( &data );

        buffer.open( QIODevice::ReadOnly );

        char encoding = buffer.read( 1 )[ 0 ];

        QTextStream textStream( &buffer );

        switch( encoding )
        {
            case 0x00:
                textStream.setCodec( "ISO 8859-1" );
                break;
            case 0x01:
                textStream.setCodec( "UTF-16" );
                break;
            case 0x02:
                textStream.setCodec( "UTF-16E" );
                break;
            case 0x03:
                textStream.setCodec( "UTF-8" );
        }

        QString all = textStream.readAll();

        if( all.endsWith( QChar( '\0' ) ) )
            all.chop( 1 );

        return all.split( majorVersion == 2 ? '\\' : '\0' );
    }

    return QStringList();
}

Id3Frame::Header Id3Frame::readHeader( QDataStream &stream, quint32 version )
{
    Id3Frame::Header header;

    if( (version & 0xFF00) == 0x0200 )
    {
        stream.readRawData( header.idBytes, 3 );
        header.idBytes[ 3 ] = '\0';

        union
        {
            quint32 size;
            char sizeBytes[ 4 ];
        };

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        sizeBytes[ 3 ] = '\0';
        stream.readRawData( &(sizeBytes[ 0 ]), 3 );
#else
        sizeBytes[ 0 ] = '\0';
        stream.readRawData( &(sizeBytes[ 1 ]), 3 );
#endif
        header.size = qFromBigEndian( size );
        header.flags = 0;
    }
    else
    {
        if( stream.readRawData( header.idBytes, 4 ) == 4 ) {
            stream >> header.size;
            stream >> header.flags;
        } else {
            header.id = 0;
        }
    }

    return header;
}

