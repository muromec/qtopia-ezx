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

#ifndef _ID3TAG_H
#define _ID3TAG_H

#include "id3frame.h"
#include <QList>
#include <QPair>
#include <QDataStream>
#include <QBuffer>

class Id3Tag
{
public:
    enum TagHeaderFlag
    {
        Unsynchronisation = 0x80,
        HasExtendedHeader = 0x40,
        Experimental = 0x20,
        HasFooter = 0x10
    };

    enum TagExtendedHeaderFlag
    {
        Update = 0x40,
        Crc = 0x20,
        Restrictions = 0x10
    };

    enum TagRestrictionFlag
    {
        TagSizeMask = 0xC0,
        TagSize128_1024 = 0x00,
        TagSize64_128 = 0x40,
        TagSize32_40 =  0x80,
        TagSize32_4 = 0xC0,

        TextEncodingUtf8 = 0x20,

        TextLengthMask = 0x18,
        TextLength1024 = 0x08,
        TextLength128  = 0x10,
        TextLength30   = 0x18,

        ImageEncodingPngJpeg = 0x04,

        ImageSizeMask = 0x03,
        ImageSizeLe256 = 0x01,
        ImageSizeLe64 = 0x02,
        ImageSizeEq64 = 0x03
    };

    struct Header
    {
        char id[ 3 ];
        union
        {
            quint16 version;
            struct
            {
                quint8 minorVersion;
                quint8 majorVersion;
            };
        };
        quint8 flags;
        quint32 size;
    };

    Id3Tag( QIODevice * );
    ~Id3Tag();

    quint32 size() const;

    bool isValid() const;

    Id3Frame *readFrame( quint64 position );

    QList< QPair< quint32, qint64 > > framePositions() const;

    static quint32 convertSyncSafeInteger( quint32 syncSafe );

    static QByteArray unsynchronise( const QByteArray &data );

private:
    void readExtendedHeaders();
    void readFrameHeaders();

    bool isValidFrame(const Id3Frame::Header &header) const;

    Header header;

    QList< QPair< quint32, qint64 > > frames;

    qint64 offset;

    QDataStream stream;

    QBuffer tagBuffer;

    bool unsynchroniseFrames;

    bool isSyncSafe;
    bool isNotSyncSafe;
};

#endif
