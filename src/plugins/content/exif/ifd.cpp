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

#include "ifd.h"
#include <QtDebug>

/*!
    \class Ifd
    \internal
*/

template< typename T >
void readArray( T **array, const Ifd::Header &header, QDataStream &stream, qint64 baseOffset )
{
    *array = new T[ header.count ];

    stream.device()->seek( baseOffset + header.offset );

    for( uint i = 0; i < header.count; i++ )
        stream >> (*array)[ i ];
}

Ifd::Ifd( const Header &h, QDataStream &stream, qint64 baseOffset )
    : header( h )
    , base( 0 )
{
    if( header.count > 1 )
    {
        switch( header.type )
        {
        case Byte:
        case Undefined:
        case Ascii:
            {
                bytes = new char[ header.count ];

                if( header.count <= 4 )
                {
                    for( quint32 i = 0; i < header.count; i++ )
                        bytes[ i ] = header.offsetBytes[ i ];
                }
                else
                {
                    stream.device()->seek( baseOffset + header.offset );

                    stream.readRawData( bytes, header.count );
                }
            }
        case Short:
            if( header.count == 2 )
            {
                uShorts = new quint16[ 2 ];

                uShorts[ 0 ] = header.offsetShorts[ 0 ];
                uShorts[ 1 ] = header.offsetShorts[ 1 ];
            }
            else
                readArray( &uShorts, h, stream, baseOffset );
            break;
        case Long:
            readArray( &uLongs,h, stream, baseOffset );
            break;
        case Rational:
            readArray( &uRationals, h, stream, baseOffset );
            break;
        case SignedLong:
            readArray( &sLongs, h, stream, baseOffset );
            break;
        case SignedRational:
            readArray( &sRationals, h, stream, baseOffset );
        }
    }
    else
    {
        switch( header.type )
        {
        case Byte:
        case Undefined:
        case Ascii:
            byte = header.offsetBytes[ 0 ];
            break;
        case Short:
            uShort = header.offsetShorts[ 0 ];
            break;
        case Long:
        case SignedLong:
            uLong = header.offset;
            break;
        case Rational:
            stream.device()->seek( baseOffset + header.offset );
            stream >> uRational;
            break;
        case SignedRational:
            stream.device()->seek( baseOffset + header.offset );
            stream >> sRational;
        }
    }
}

Ifd::~Ifd()
{
    if( header.count > 1 )
    {
        switch( header.type )
        {
        case Byte:
        case Undefined:
        case Ascii:          delete [] bytes;      break;
        case Short:          delete [] uShorts;    break;
        case Long:           delete [] uLongs;     break;
        case Rational:       delete [] uRationals; break;
        case SignedLong:     delete [] sLongs;     break;
        case SignedRational: delete [] sRationals;
        };
    }
}

quint16 Ifd::tag() const
{
    return header.tag;
}

Ifd::Type Ifd::type() const
{
    return (Ifd::Type)header.type;
}

quint32 Ifd::count() const
{
    return header.count;
}

quint8 Ifd::toByte() const
{
    return header.count == 1 && (header.type == Byte || header.type == Undefined) ? byte : 0;
}

quint16 Ifd::toShort() const
{
    return header.count == 1 && header.type == Short ? uShort : 0;
}

quint32 Ifd::toLong() const
{
    return header.count == 1 && header.type == Long ? uLong : 0;
}

Ifd::URational Ifd::toRational() const
{
    static const URational rational = { 0, 0 };

    return header.count == 1 && header.type == Rational ? uRational : rational;
}

qint32 Ifd::toSignedLong() const
{
    return header.count == 1 && header.type == SignedLong ? sLong : 0;
}

Ifd::SRational Ifd::toSignedRational() const
{
    static const SRational rational = { 0, 0 };

    return header.count == 1 && header.type == SignedRational ? sRational : rational;
}

QByteArray Ifd::toByteArray() const
{
    if( header.type == Byte || header.type == Undefined )
    {
        if( header.count == 1 )
            return QByteArray( byte, 1 );
        else if( header.count > 1 )
            return QByteArray( bytes, header.count );
    }
    return QByteArray();
}

QString Ifd::toString() const
{
    return header.type == Ascii && header.count > 1 ? QString( QByteArray( bytes, header.count ) ) : QString();
}

QVector< quint16 > Ifd::toShortArray() const
{
    QVector< quint16 > vector;

    if( header.type == Short )
    {
        if( header.count == 1 )
            vector.append( uShort );
        if( header.count > 1 )
        {
            vector.resize( header.count );

            for( uint i = 0; i < header.count; i++ )
                vector[ i ] = uShorts[ i ];
        }
    }

    return vector;
}

QVector< quint32 > Ifd::toLongArray() const
{
    QVector< quint32 > vector;

    if( header.type == Long )
    {
        if( header.count == 1 )
            vector.append( uLong );
        if( header.count > 1 )
        {
            vector.resize( header.count );

            for( uint i = 0; i < header.count; i++ )
                vector[ i ] = uLongs[ i ];
        }
    }

    return vector;
}

QVector< Ifd::URational > Ifd::toRationalArray() const
{
    QVector< URational > vector;

    if( header.type == Rational )
    {
        if( header.count == 1 )
            vector.append( uRational );
        if( header.count > 1 )
        {
            vector.resize( header.count );

            for( uint i = 0; i < header.count; i++ )
                vector[ i ] = uRationals[ i ];
        }
    }

    return vector;
}

QVector< qint32 > Ifd::toSignedLongArray() const
{
    QVector< qint32 > vector;

    if( header.type == SignedLong )
    {
        if( header.count == 1 )
            vector.append( sLong );
        if( header.count > 1 )
        {
            vector.resize( header.count );

            for( uint i = 0; i < header.count; i++ )
                vector[ i ] = sLongs[ i ];
        }
    }

    return vector;
}

QVector< Ifd::SRational > Ifd::toSignedRationalArray() const
{
    QVector< SRational > vector;

    if( header.type == SignedRational )
    {
        if( header.count == 1 )
            vector.append( sRational );
        if( header.count > 1 )
        {
            vector.resize( header.count );

            for( uint i = 0; i < header.count; i++ )
                vector[ i ] = sRationals[ i ];
        }
    }

    return vector;
}

QDataStream &operator >>( QDataStream &stream, Ifd::Header &header )
{
    stream >> header.tag;
    stream >> header.type;
    stream >> header.count;

    switch( header.type )
    {
    case Ifd::Byte:
    case Ifd::Ascii:
    case Ifd::Undefined:
        if( header.count <= 4 )
        {
            stream.readRawData( header.offsetBytes, 4 );
            break;
        }
    case Ifd::Short:
        if( header.count <= 2 )
        {
            stream >> header.offsetShorts[ 0 ];
            stream >> header.offsetShorts[ 1 ];
            break;
        }
    default:
        stream >> header.offset;
    }

    return stream;
}

QDataStream &operator >>( QDataStream &stream, Ifd::URational &rational )
{
    stream >> rational.numerator;
    stream >> rational.denominator;

    return stream;
}

QDataStream &operator >>( QDataStream &stream, Ifd::SRational &rational )
{
    stream >> rational.numerator;
    stream >> rational.denominator;

    return stream;
}

