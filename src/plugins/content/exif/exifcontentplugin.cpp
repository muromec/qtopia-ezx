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

#include "exifcontentplugin.h"
#include "ifd.h"
#include <QtDebug>
#include <qmimetype.h>

/*!
    \class ExifContentPlugin
    \internal

    Plugin for reading meta data from images with exif tags.
*/

ExifContentPlugin::ExifContentPlugin()
{
}

ExifContentPlugin::~ExifContentPlugin()
{
}

QStringList ExifContentPlugin::keys() const
{
    return QMimeType( QLatin1String( "image/tiff" ) ).extensions() +
           QMimeType( QLatin1String( "image/jpeg" ) ).extensions();
}

bool ExifContentPlugin::installContent( const QString &filePath, QContent *content )
{
    QFile file( filePath );

    bool success = false;

    if( file.open( QIODevice::ReadOnly ) )
    {
        if( readFile( &file, content ) )
        {
            content->setName( QFileInfo( filePath ).baseName() );
            content->setType( QMimeType( filePath ).id() );
            content->setFile( filePath );

            success = true;
        }

        file.close();
    }

    return success;
}

bool ExifContentPlugin::updateContent( QContent *content )
{
    return installContent( content->fileName(), content );
}

bool ExifContentPlugin::readFile( QFile *file, QContent *content )
{
    if( file->peek( 2 ) != "\xFF\xD8" )
        return false;

    file->read( 2 );

    QDataStream stream( file );
    stream.setByteOrder( QDataStream::BigEndian );

    while( file->read( 2 ) != "\xFF\xE1" )
    {
        if( file->atEnd() )
            return false;

        quint16 length;

        stream >> length;

        file->seek( file->pos() + length - 2 );
    }

    file->read( 2 );

    if( file->read( 4 ) != "Exif" )
        return false;

    file->read( 2 );

    qint64 baseOffset = file->pos();

    QByteArray byteOrder = file->read( 2 );

    if( byteOrder == "II" )
        stream.setByteOrder( QDataStream::LittleEndian );
    else if( byteOrder == "MM" )
        stream.setByteOrder( QDataStream::BigEndian );
    else
        return false;

    quint16 id;
    quint32 ifdOffset;

    stream >> id;
    stream >> ifdOffset;

    if( id != 0x002A )
        return false;

    file->seek( baseOffset + ifdOffset );

    readProperties( stream, baseOffset, content );

    return true;
}

QList< Ifd::Header > ExifContentPlugin::readIfdHeaders( QDataStream &stream, int baseOffset )
{
    QList< Ifd::Header > headers;

    while( true && !stream.device()->atEnd() )
    {
        quint16 headerCount;

        stream >> headerCount;

        for( quint32 i = 0; i < headerCount; i++ )
        {
            Ifd::Header header;

            stream >> header;

            headers.append( header );
        }

        quint32 ifdOffset;

        stream >> ifdOffset;

        if( ifdOffset )
            stream.device()->seek( baseOffset + ifdOffset );
        else
            break;
    }

    return headers;
}

void ExifContentPlugin::readProperties( QDataStream &stream, int baseOffset, QContent *content )
{
    QList< Ifd::Header > headers = readIfdHeaders( stream, baseOffset );

    foreach( Ifd::Header header, headers )
    {
        if( header.tag == Ifd::DateTime && header.type == Ifd::Ascii )
        {
            Ifd ifd( header, const_cast< QDataStream & >(stream), baseOffset );

            content->setProperty( QLatin1String( "CreationDate" ), ifd.toString(), QLatin1String( "Image" ) );
        }
        if( header.tag == Ifd::ImageDescription && header.type == Ifd::Ascii )
        {
            Ifd ifd( header, const_cast< QDataStream & >(stream), baseOffset );

            content->setProperty( QContent::Description, ifd.toString() );
        }
        if( header.tag == Ifd::Artist && header.type == Ifd::Ascii )
        {
            Ifd ifd( header, const_cast< QDataStream & >(stream), baseOffset );

            content->setProperty( QContent::Artist, ifd.toString() );
        }
    }
}

QTOPIA_EXPORT_PLUGIN(ExifContentPlugin);
