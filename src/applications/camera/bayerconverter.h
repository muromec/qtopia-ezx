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
** See below for additional copyright and license information.
**
**
****************************************************************************/

#ifndef __QTOPIA_CAMERA_BAYERCONVERTER_H
#define __QTOPIA_CAMERA_BAYERCONVERTER_H

#include "formatconverter.h"

namespace camera
{

class BayerConverter : public FormatConverter
{
public:
    BayerConverter(int width, int height);
    virtual ~BayerConverter();

    virtual unsigned char* convert(unsigned char* src);

private:

    int             m_width;
    int             m_height;
    unsigned char*  m_buf;
};

}

#endif  //__QTOPIA_CAMERA_BAYERCONVERTER_H

