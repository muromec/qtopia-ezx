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

/*
 * convert ROUTINE TAKEN FROM:
 *
 * Sonix SN9C101 based webcam basic I/F routines
 * Copyright (C) 2004 Takafumi Mizuno <taka-qce@ls-a.jp>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "bayerconverter.h"

namespace camera
{

BayerConverter::BayerConverter(int width, int height):
    m_width(width),
    m_height(height)
{
    m_buf = new unsigned char[width * height * 4];  // 4 = 32bpp / 8 bpb
}

BayerConverter::~BayerConverter()
{
    delete m_buf;
}

unsigned char* BayerConverter::convert(unsigned char* src)
{
    int             size = m_width * m_height;
    unsigned long   *dst = (unsigned long*) m_buf;

    register unsigned long   dstVal;

    for (int i = 0; i < size; ++i)
    {
        dstVal = 0xFF000000;

        if ((i / m_width) % 2 == 0)
        {
            if ((i % 2) == 0)
            {
                /* B */
                if ((i > m_width) && ((i % m_width) > 0) )
                {
                    dstVal |= ((*(src-m_width-1)+*(src-m_width+1)+ *(src+m_width-1)+*(src+m_width + 1)) / 4) << 16 |
                              ((*(src-1)+*(src+1)+*(src+m_width)+*(src-m_width))/4) << 8 |
                              *src;
                }
                else
                {
                    dstVal |= *(src + m_width + 1) << 16 |
                              (*(src + 1) + *(src + m_width)) / 2 << 8 |
                              *src;
                }
            }
            else
            {
                /* (B)G */
                if ((i > m_width) && ((i % m_width) < (m_width - 1)))
                {
                    dstVal |= (*(src+m_width)+*(src-m_width))/2 << 16 |
                              *src << 8 |
                              (*(src-1)+*(src+1))/2;
                }
                else
                {
                    dstVal |= *(src+m_width) << 16 |
                              *src << 8 |
                              *(src - 1);
                }
            }
        }
        else {

            if ((i % 2) == 0)
            {
                /* G(R) */
                if ((i < (m_width * (m_height - 1))) && ((i % m_width) > 0) )
                {
                    dstVal |= ((*(src - 1) + *(src + 1)) / 2) << 16 |
                              *src << 8 |
                              (*(src + m_width) + *(src - m_width)) / 2;
                }
                else
                {
                    dstVal |= *(src + 1) << 16 |
                              *src << 8 |
                              *(src - m_width);
                }
            }
            else
            {
                /* R */
                if (i < (m_width * (m_height - 1)) && ((i % m_width) < (m_width - 1)) )
                {
                    dstVal |= *src << 16 |
                              (*(src-1)+*(src+1)+ *(src-m_width)+*(src+m_width))/4 << 8 |
                              (*(src-m_width-1)+*(src-m_width+1)+ *(src+m_width-1)+*(src+m_width+1))/4;
                }
                else
                {
                    dstVal |= *src << 16 |
                              (*(src-1)+*(src-m_width))/2 << 8 |
                              *(src-m_width-1);
                }
            }
        }

        *dst++ = dstVal;
            ++src;
    }

    return m_buf;
}

}   // ns camera
