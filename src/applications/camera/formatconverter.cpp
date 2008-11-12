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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/videodev.h>

#ifdef QTOPIA_HAVE_V4L2

#include "formatconverter.h"
#include "bayerconverter.h"
#include "yuvconverter.h"

namespace camera
{

/*
    convert something to RGB32
*/

FormatConverter* FormatConverter::createFormatConverter(unsigned int type, int width, int height)
{
    switch (type)
    {
    case V4L2_PIX_FMT_RGB332:
    case V4L2_PIX_FMT_RGB555:
    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_RGB555X:
    case V4L2_PIX_FMT_RGB565X:
    case V4L2_PIX_FMT_RGB24:
    case V4L2_PIX_FMT_RGB32:
    case V4L2_PIX_FMT_GREY:
    case V4L2_PIX_FMT_YVU410:
    case V4L2_PIX_FMT_YVU420:
        return new NullConverter;
    case V4L2_PIX_FMT_YUYV:
       return new YUVConverter(V4L2_PIX_FMT_YUYV,width, height);
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUV411P:
    case V4L2_PIX_FMT_Y41P:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YUV410:
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_YYUV:
    case V4L2_PIX_FMT_HI240:
        return new NullConverter;

    case V4L2_PIX_FMT_SBGGR8:
        return new BayerConverter(width, height);
    }

    return new NullConverter;
}

void FormatConverter::releaseFormatConverter(FormatConverter* converter)
{
    delete converter;
}


// Null Converter
unsigned char* NullConverter::convert(unsigned char* src)
{
    return src;
}

QList<unsigned int> FormatConverter::supportedFormats()
{
    QList<unsigned int> list;
    list << V4L2_PIX_FMT_YUYV << V4L2_PIX_FMT_SBGGR8 << V4L2_PIX_FMT_RGB32;
    return list;
}

}   // ns camera


#endif // HAVE_V4L2
