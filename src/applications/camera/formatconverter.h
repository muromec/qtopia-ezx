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

#ifndef __QTOPIA_CAMERA_FORMATCONVERTER_H
#define __QTOPIA_CAMERA_FORMATCONVERTER_H

#include <QList>

namespace camera
{

/*
    convert something to RGB32
*/

class FormatConverter
{
public:

    virtual ~FormatConverter() {}

    virtual unsigned char* convert(unsigned char* src) = 0;

    static FormatConverter* createFormatConverter(unsigned int type, int width, int height);
    static void releaseFormatConverter(FormatConverter* converter);
    static QList<unsigned int> supportedFormats();
};


class NullConverter : public FormatConverter
{
public:

    virtual unsigned char* convert(unsigned char* src);
};

}   // ns camera

#endif  //__QTOPIA_CAMERA_FORMATCONVERTER_H
