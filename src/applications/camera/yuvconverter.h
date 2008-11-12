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

#ifndef __QTOPIA_CAMERA_YUVCONVERTER_H
#define __QTOPIA_CAMERA_YUVCONVERTER_H

#include "formatconverter.h"

namespace camera
{

class YUVConverter : public FormatConverter
{
public:
    YUVConverter(unsigned int type, int width, int height);
    virtual ~YUVConverter();

    virtual unsigned char* convert(unsigned char* src);

private:

    unsigned int   m_type;  // V4L2_PIX_FMT_YUV_*
    int             m_width;
    int             m_height;
    unsigned char*  m_buf;
    int ui,vi,y1i,y2i;

};

}

#endif  //__QTOPIA_CAMERA_YUYVCONVERTER_H

