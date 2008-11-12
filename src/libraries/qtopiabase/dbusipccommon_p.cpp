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

#include "dbusipccommon_p.h"
#include <QByteArray>

#include <ctype.h>

static QChar hex_table[]={
    QChar('0'),
    QChar('1'),
    QChar('2'),
    QChar('3'),
    QChar('4'),
    QChar('5'),
    QChar('6'),
    QChar('7'),
    QChar('8'),
    QChar('9'),
    QChar('A'),
    QChar('B'),
    QChar('C'),
    QChar('D'),
    QChar('E'),
    QChar('F')
};

#define CONVERT_TO_HEX(dest, offset, byte)\
    dest[offset] = QChar('_'); \
    dest[offset+2] = hex_table[byte & 0x0f]; \
    dest[offset+1] = hex_table[(byte >> 4) & 0x0f];

#define CONVERT_FROM_HEX(src, soffset, dst, doffset)\
    char uc = 0; \
    if ((src[soffset] >= QChar('0')) && (src[soffset] <= QChar('9'))) \
        uc = src[soffset].unicode() - '0'; \
    else if ((src[soffset] >= QChar('A')) && (src[soffset] <= QChar('Z'))) \
        uc = 10 + src[soffset].unicode() - 'A'; \
    else \
        uc = 10 + src[soffset].unicode() - 'a'; \
    if ((src[soffset+1] >= QChar('0')) && (src[soffset+1] <= QChar('9'))) \
        uc = (uc << 4) + src[soffset+1].unicode() - '0'; \
    else if ((src[soffset+1] >= QChar('A')) && (src[soffset+1] <= QChar('Z'))) \
        uc = (uc << 4) + 10 + src[soffset+1].unicode() - 'A'; \
    else \
        uc = (uc << 4) + 10 + src[soffset+1].unicode() - 'a'; \
    dst[doffset] = QChar(uc);

void convert_qcop_message_name_to_dbus(const QString &msg,
                                       QString &dbusMsg)
{
    int len = msg.length();
    dbusMsg.resize(len * 3 + 3);

    int j = 0;

    const QChar *src = msg.constData();
    QChar *dst = dbusMsg.data();

    if (isdigit(src[0].unicode())) {
        dst[j++] = QChar('_');
        dst[j++] = QChar('0');
        dst[j++] = QChar('0');
    }

    for (int i = 0; i < len; i++) {
        if (((src[i] < QChar('0')) || (src[i] > QChar('9'))) &&
              ((src[i] < QChar('A')) || (src[i] > QChar('Z'))) &&
              ((src[i] < QChar('a')) || (src[i] > QChar('z')))) {
            CONVERT_TO_HEX(dst, j, src[i].unicode());
            j+=3;
              }
              else {
                  dst[j++] = src[i];
              }
    }

    dst[j] = QChar('\0');
    dbusMsg.resize(j);
}

void convert_dbus_to_qcop_message_name(const QString &dbusMsg,
                                       QString &msg)
{
    int len = dbusMsg.length();
    msg.resize(len);

    int j = 0;

    const QChar *src = dbusMsg.constData();
    QChar *dst = msg.data();

    int i = 0;
    if ((len >= 3) && (src[0] == QChar('_'))) {
        if ((src[1] == src[2]) && (src[1] == QChar('0')))
            i += 3;
    }

    for (; i < len; i++) {
        if (src[i] == QChar('_')) {
            CONVERT_FROM_HEX(src, i+1, dst, j)
                    j++;
            i+=2;
        }
        else {
            dst[j++] = src[i];
        }
    }

    dst[j] = QChar('\0');
    msg.resize(j);
}

void convert_qcop_channel_to_dbus_path(const QString &channel,
                                       QString &dbusPath)
{
    dbusPath = dbusPathBase;
    QString path;
    convert_qcop_message_name_to_dbus(channel, path);
    dbusPath.append(path);
}

void convert_dbus_path_to_qcop_channel(const QString &path,
                                       QString &channel)
{
    QString mangledPart = path.mid(dbusPathBase.length());
    convert_dbus_to_qcop_message_name(mangledPart, channel);
}
