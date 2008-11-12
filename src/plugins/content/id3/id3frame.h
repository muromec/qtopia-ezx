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

#ifndef _ID3FRAME_H
#define _ID3FRAME_H

#include <QtGlobal>
#include <QByteArray>
#include <QStringList>

class Id3Frame
{
public:

    struct Header
    {
        union
        {
            quint32 id;
            char idBytes[ 4 ];
        };
        quint32 size;
        quint16 flags;
    };

    enum FrameFlag
    {
        TagAlter = 0x4000,
        FileAlter = 0x0200,
        ReadOnly = 0x0100,

        Grouping = 0x0040,

        Compression         = 0x0008,
        Encryption          = 0x0004,
        Unsynchronisation   = 0x0002,
        DataLengthIndicator = 0x0001
    };

    Id3Frame( const Header &h, const QByteArray &d, quint8 Id3MajorVersion, bool tagUnsynchronisation );
    ~Id3Frame();

    bool isTextFrame() const;

    quint32 id() const;

    QStringList textFields();

    static Header readHeader( QDataStream &stream, quint32 version );

private:

    void unsynchronise();

    Header header;
    QByteArray data;
    quint8 majorVersion;
};

#endif
