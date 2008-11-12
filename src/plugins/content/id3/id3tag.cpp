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

#include "id3tag.h"
#include <QtDebug>

/*!
    \class Id3Tag
    \internal
*/
Id3Tag::Id3Tag( QIODevice *device )
    : stream( device )
    , isSyncSafe(false)
    , isNotSyncSafe(false)
{
    stream.setByteOrder( QDataStream::BigEndian );

    stream.readRawData( header.id, 3 );

    stream >> header.version;
    stream >> header.flags;
    stream >> header.size;

    header.size = convertSyncSafeInteger( header.size );

    if( !isValid() )
        return;

    if( header.majorVersion == 3 && header.flags & Unsynchronisation )
    {
        QByteArray tagData = unsynchronise( device->read( header.size ) );

        tagBuffer.setData( tagData );

        if( !tagBuffer.open( QIODevice::ReadOnly ) )
            return;

        stream.setDevice( &tagBuffer );
    }

    offset = device->pos();

    unsynchroniseFrames = header.majorVersion == 4 && header.flags & Unsynchronisation;

    if( header.flags & HasExtendedHeader )
        readExtendedHeaders();

    readFrameHeaders();
}

Id3Tag::~Id3Tag()
{
    if( tagBuffer.isOpen() )
        tagBuffer.close();
}

quint32 Id3Tag::size() const
{
    return header.size;
}

bool Id3Tag::isValid() const
{
    return header.id[ 0 ] == 'I' &&
           header.id[ 1 ] == 'D' &&
           header.id[ 2 ] == '3' &&
           header.size > 0       &&
           header.majorVersion >= 2 &&
           header.majorVersion <= 4;
}

Id3Frame *Id3Tag::readFrame( quint64 position )
{
    stream.device()->seek( position );

    Id3Frame::Header frameHeader = Id3Frame::readHeader( stream, header.version );

    if (isSyncSafe)
        frameHeader.size = convertSyncSafeInteger(frameHeader.size);

    QByteArray data = stream.device()->read( frameHeader.size );

    return new Id3Frame( frameHeader, data, header.majorVersion, unsynchroniseFrames );
}

QList< QPair< quint32, qint64 > > Id3Tag::framePositions() const
{
    return frames;
}

void Id3Tag::readExtendedHeaders()
{
    qint64 pos = stream.device()->pos();

    quint32 size;

    stream >> size;

    if( header.majorVersion == 4 )
        size = convertSyncSafeInteger( size );

    stream.device()->seek( pos + size );
}

void Id3Tag::readFrameHeaders()
{
    for (;;) {
        qint64 pos = stream.device()->pos();

        Id3Frame::Header frameHeader = Id3Frame::readHeader(stream, header.version);

        if (!isValidFrame(frameHeader)) {
            return;
        } else if (isSyncSafe) {
            frameHeader.size = convertSyncSafeInteger(frameHeader.size);
        } else if (!isNotSyncSafe && header.majorVersion == 4 && frameHeader.size >= 0x80) {
            if ((isSyncSafe = stream.device()->pos() + frameHeader.size > offset + header.size)) {
                frameHeader.size = convertSyncSafeInteger(frameHeader.size);
            } else if (!(isNotSyncSafe = frameHeader.size & 0x80808080)) {
                Id3Frame::Header lastHeader = frameHeader;
                qint64 lastPos = pos;

                quint32 syncSafeSize = convertSyncSafeInteger(frameHeader.size);

                stream.device()->seek(stream.device()->pos() + syncSafeSize);

                pos = stream.device()->pos();

                frameHeader = Id3Frame::readHeader(stream, header.version);

                if ((isSyncSafe = isValidFrame(frameHeader))) {
                    lastHeader.size = syncSafeSize;
                    frameHeader.size = convertSyncSafeInteger(frameHeader.size);
                } else {
                    stream.device()->seek(
                            stream.device()->pos() + lastHeader.size - 10 - syncSafeSize);

                    pos = stream.device()->pos();

                    frameHeader = Id3Frame::readHeader(stream, header.version);

                    isNotSyncSafe = isValidFrame(frameHeader);
                }

                frames.append(QPair<quint32, qint64>(lastHeader.id, lastPos));
            }
        }

        frames.append(QPair<quint32, qint64>(frameHeader.id, pos));

        if (stream.device()->pos() + frameHeader.size >= offset + header.size)
            return;

        stream.device()->seek(stream.device()->pos() + frameHeader.size);
    }
}

bool Id3Tag::isValidFrame(const Id3Frame::Header &header) const
{
    return header.size != 0
            && (header.idBytes[0] >= '0' && header.idBytes[0] <= 'Z' || header.idBytes[0] == '\0')
            && (header.idBytes[1] >= '0' && header.idBytes[1] <= 'Z')
            && (header.idBytes[2] >= '0' && header.idBytes[2] <= 'Z')
            && (header.idBytes[3] >= '0' && header.idBytes[3] <= 'Z' || header.idBytes[3] == '\0');
}

quint32 Id3Tag::convertSyncSafeInteger( quint32 syncSafe )
{
    return ((syncSafe & 0x7F000000) >> 0x03) |
           ((syncSafe & 0x007F0000) >> 0x02) |
           ((syncSafe & 0x00007F00) >> 0x01) |
           ((syncSafe & 0x0000007F)        );
}

QByteArray Id3Tag::unsynchronise( const QByteArray &data )
{
    if( data.size() == 0 )
        return data;

    int shift = 0;

    QByteArray unsynced = data;

    for( int i = 1; i < data.size(); i++ )
    {
        if( data[ i - 1 ] == (char)0xFF && data[ i ] == (char)0x00 )
            shift++;
        else if( shift )
            unsynced[ i - shift ] = data[ i ];
    }

    if( shift )
        unsynced.chop( shift );

    return unsynced;
}

