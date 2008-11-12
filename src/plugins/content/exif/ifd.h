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

#ifndef _IFD_H
#define _IFD_H

#include <qtopiaglobal.h>
#include <QVector>
#include <QDataStream>

class Ifd
{
public:

    enum Tag
    {
        ImageWidth       = 0x0100,
        ImageLength      = 0x0101,
        DateTime         = 0x0132,
        ImageDescription = 0x010E,
        Artist           = 0x013B,
        Make             = 0x010F,
        Model            = 0x0110,
        Software         = 0x0131
    };

    enum Type
    {
        Byte           =  1,
        Ascii          =  2,
        Short          =  3,
        Long           =  4,
        Rational       =  5,
        Undefined      =  7,
        SignedLong     =  9,
        SignedRational = 10
    };

    struct Header
    {
        quint16 tag;
        quint16 type;
        quint32 count;
        union
        {
            char offsetBytes[ 4 ];
            quint16 offsetShorts[ 2 ];
            quint32 offset;
        };
    };

    struct URational
    {
        quint32 numerator;
        quint32 denominator;
    };

    struct SRational
    {
        qint32 numerator;
        qint32 denominator;
    };

    Ifd( const Header &h, QDataStream &stream, qint64 baseOffset );
    ~Ifd();

    quint16 tag()   const;
    Type    type()  const;
    quint32 count() const;

    quint8    toByte()           const;
    quint16   toShort()          const;
    quint32   toLong()           const;
    URational toRational()       const;
    qint32    toSignedLong()     const;
    SRational toSignedRational() const;

    QByteArray           toByteArray()           const;
    QString              toString()              const;
    QVector< quint16   > toShortArray()          const;
    QVector< quint32   > toLongArray()           const;
    QVector< URational > toRationalArray()       const;
    QVector< qint32    > toSignedLongArray()     const;
    QVector< SRational > toSignedRationalArray() const;


private:

    Header header;

    union
    {
        char      *bytes;
        quint16   *uShorts;
        quint32   *uLongs;
        URational *uRationals;
        qint32    *sLongs;
        SRational *sRationals;

        quint8    byte;
        quint16   uShort;
        quint32   uLong;
        URational uRational;
        qint32    sLong;
        SRational sRational;

        quint64 base;
    };
};

QDataStream &operator >>( QDataStream &stream, Ifd::Header &header );
QDataStream &operator >>( QDataStream &stream, Ifd::URational &rational );
QDataStream &operator >>( QDataStream &stream, Ifd::SRational &rational );

#endif
