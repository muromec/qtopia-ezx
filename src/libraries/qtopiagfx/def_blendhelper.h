/****************************************************************************
**
** This file is part of the Qt Extended Opensource Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** version 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#ifndef DEF_BLENDHELPER_H
#define DEF_BLENDHELPER_H

static inline unsigned short qConvertRgb32To16(unsigned int c)
{
    return (((c) >> 3) & 0x001f) |
           (((c) >> 5) & 0x07e0) |
           (((c) >> 8) & 0xf800);
}

static inline unsigned int qConvertRgb32To18(unsigned int c)
{
    return (((c) >> 2) & 0x0003f) |
           (((c) >> 4) & 0x00fc0) |
           (((c) >> 6) & 0x3f000);
}

static inline unsigned int qConvertRgb18To32(unsigned short c)
{
    return ((c << 2) & 0x00FC) |
           ((c << 4) & 0xFC00) |
           ((c << 6) & 0xFC0000) |
           0xFF030303;
}

static inline unsigned int qConvertRgb16To32(unsigned short c)
{
    return ((c << 3) & 0x00F8) |
           ((c << 5) & 0xFC00) |
           ((c << 8) & 0xF80000) |
           0xFF070307;
}

static inline unsigned int premul(unsigned int val, unsigned char alpha)
{
    if(alpha == 0xFF)
        return val;
    else if(alpha == 0x00)
        return 0;
    else
        return ((((val & 0x00FF00FF) * alpha) >> 8) & 0x00FF00FF) |
               ((((val & 0xFF00FF00) >> 8) * alpha) & 0xFF00FF00);
}

static inline unsigned int premul_noextents(unsigned int val, unsigned char alpha)
{
    return ((((val & 0x00FF00FF) * alpha) >> 8) & 0x00FF00FF) |
           ((((val & 0xFF00FF00) >> 8) * alpha) & 0xFF00FF00);
}

static inline unsigned int premul_nozero(unsigned int val, unsigned char alpha)
{
    if(alpha == 0xFF)
        return val;
    else
        return ((((val & 0x00FF00FF) * alpha) >> 8) & 0x00FF00FF) |
               ((((val & 0xFF00FF00) >> 8) * alpha) & 0xFF00FF00);
}

static inline unsigned int premul_nozero18(unsigned int val, unsigned char alpha)
{
    if(alpha == 0xFF)
        return val;
    else
        return ((((val & 0x0003F03F) * alpha) >> 6) & 0x0003F03F) |
               ((((val & 0x00000FC0) >> 6) * alpha) & 0x00000FC0);
}


static inline void argb32p_rgb16_inplace(unsigned short *dest,
                                         unsigned int *src)
{
    unsigned char alpha = *src >> 24;

    if(alpha == 0x00) {
    } else if(alpha == 0xFF) {
        *dest = qConvertRgb32To16(*src);
    } else {
        alpha = 0xff - alpha;
        *dest = qConvertRgb32To16((premul_nozero(qConvertRgb16To32(*dest), alpha) & 0xFFFFFF) + *src);
    }
}


static inline void rgb32_rgb16_opacity_inplace(unsigned short *dest,
                                               unsigned int *src,
                                               unsigned char opacity)
{
    unsigned int srcval = premul_nozero(*src, opacity);
    if(srcval & 0xFF000000) {
        unsigned int destval = premul_nozero(qConvertRgb16To32(*dest), 0xFF - opacity);
        *dest = qConvertRgb32To16(srcval + destval);
    }
}

static inline void rgb16_rgb16_opacity_inplace(unsigned short *dest,
                                               unsigned short *src,
                                               unsigned char opacity)
{
    unsigned int srcval = premul_nozero(qConvertRgb16To32(*src), opacity);
    if(srcval & 0xFF000000) {
        unsigned int destval = premul_nozero(qConvertRgb16To32(*dest), 0xFF - opacity);
        *dest = qConvertRgb32To16(srcval + destval);
    }
}

static inline void rgb16_rgb32_opacity_inplace(unsigned int *dest,
                                               unsigned short *src,
                                               unsigned char opacity)
{
    unsigned int srcval = premul_nozero(qConvertRgb16To32(*src), opacity);
    if(srcval & 0xFF000000) {
        int destval = premul_nozero(qConvertRgb16To32(*dest), 0xFF - opacity);
        *dest = 0xFF000000 | (srcval + destval);
    }
}


static inline void argb32p_rgb16_opacity_inplace(unsigned short *dest,
                                                 unsigned int *src,
                                                 unsigned char opacity)
{
    unsigned int srcval = *src;
    unsigned char alpha = srcval >> 24;

    if(alpha == 0x00) {
    } else {
        srcval = premul_nozero(srcval, opacity);
        alpha = srcval >> 24;
        if(alpha == 0x00) {
        } else if(alpha == 0xFF) {
            *dest = qConvertRgb32To16(srcval);
        } else {
            alpha = 0xFF - alpha;

            *dest =
                qConvertRgb32To16((premul_nozero(qConvertRgb16To32(*dest),
                                                 alpha) & 0xFFFFFF) + srcval);
        }
    }
}
static inline void argb32p_rgb18_inplace(unsigned char *dest,
                                         unsigned int *src)
{
    unsigned char alpha = *src >> 24;
    unsigned int dest32 = *(int *)dest & 0x003FFFFF;

    if(alpha == 0x00) {
    } else if(alpha == 0xFF) {
        dest32 = qConvertRgb32To18(*src);
    } else {
        alpha = 0xff - alpha;
        dest32 = qConvertRgb32To18((premul_nozero(qConvertRgb18To32(dest32), alpha) & 0xFFFFFF) + *src);
    }

    *dest = dest32 & 0xFF;
    dest++;
    *dest = (dest32 >> 8) & 0xFF;
    dest++;
    *dest = (dest32 >> 16) & 0xFF;
    dest++;
}

static inline void argb32p_rgb18_opacity_inplace(unsigned char *dest,
                                                 unsigned int *src,
                                                 unsigned char opacity)
{
    unsigned int srcval = *src;
    unsigned char alpha = srcval >> 24;

    unsigned int dest32 = 0;//xFFFFFFFF;

    if(alpha == 0x00) {
    } else {
        srcval = premul_nozero(srcval, opacity);
        alpha = srcval >> 24;
        if(alpha == 0x00) {
        } else if(alpha == 0xFF) {
            dest32 = qConvertRgb32To18(srcval);

            *dest = dest32 & 0xFF;
            dest++;
            *dest = (dest32 >> 8) & 0xFF;
            dest++;
            *dest = (dest32 >> 16) & 0xFF;
            dest++;


        } else {
            alpha = 0xFF - alpha;

            dest32 = qConvertRgb32To18((premul_nozero(qConvertRgb18To32(0), alpha) & 0xFFFFFF) + srcval);

            *dest = dest32 & 0xFF;
            dest++;
            *dest = (dest32 >> 8) & 0xFF;
            dest++;
            *dest = (dest32 >> 16) & 0xFF;
            dest++;


        }
    }
}

static inline void argb24p_rgb18_inplace(unsigned char *dest,
                                         unsigned int *src)
{
    unsigned char alpha = (*src >> 18) & 0x3F;
    unsigned int dest32 = 0;//xFFFFFFFF;

    if(alpha == 0x00) {
    } else if(alpha == 0xFF) {
        dest32 = *src;
    } else {
        alpha = 0xff - alpha;
        dest32 = (premul_nozero18(0, alpha) & 0xFFFFFF) + *src;
    }

    *dest = dest32 & 0xFF;
    dest++;
    *dest = (dest32 >> 8) & 0xFF;
    dest++;
    *dest = (dest32 >> 16) & 0xFF;
    dest++;
}

static inline void argb24p_rgb18_opacity_inplace(unsigned char *dest,
                                                 unsigned int *src,
                                                 unsigned char opacity)
{
    unsigned int srcval = *src;
    unsigned char alpha = (srcval >> 18) & 0x3F;

    unsigned int dest32 = 0;//xFFFFFFFF;

    if(alpha == 0x00) {
    } else {
        srcval = premul_nozero18(srcval, opacity);
        alpha =  alpha = (srcval >> 18) & 0x3F;
        if(alpha == 0x00) {
        } else if(alpha == 0xFF) {
            dest32 = srcval;

            *dest = dest32 & 0xFF;
            dest++;
            *dest = (dest32 >> 8) & 0xFF;
            dest++;
            *dest = (dest32 >> 16) & 0xFF;
            dest++;


        } else {
            alpha = 0xFF - alpha;

            dest32 = *dest;
            dest++;
            dest32 |= (*dest << 8);
            dest++;
            dest32 |= (*dest << 16);
            dest--;
            dest--;

            dest32 = (premul_nozero18(dest32, alpha) & 0xFFFFFF) + srcval;

            *dest = dest32 & 0xFF;
            dest++;
            *dest = (dest32 >> 8) & 0xFF;
            dest++;
            *dest = (dest32 >> 16) & 0xFF;
            dest++;


        }
    }
}

static inline void argb32p_rgb32_opacity_inplace(unsigned int *dest,
                                                 unsigned int *src,
                                                 unsigned char opacity)
{
    unsigned char alpha = *src >> 24;

    if(alpha == 0x00) {
    } else {
        unsigned int srcval = premul(*src, opacity);
        alpha = srcval >> 24;
        if(alpha == 0x00) {
        } else if(alpha == 0xFF) {
            *dest = srcval;
        } else {
            alpha = 0xFF - alpha;
            *dest = ((premul_nozero(*dest, alpha) & 0xFFFFFF) + srcval) | 0xFF000000;
        }
    }
}

static inline void argb32p_rgb32_inplace(unsigned int *dest,
                                         unsigned int *src)
{
    unsigned char alpha = *src >> 24;

    if(alpha == 0x00) {
    } else if(alpha == 0xFF) {
        *dest = *src;
    } else {
        alpha = 0xff - alpha;
        *dest = ((premul_nozero(*dest, alpha) & 0xFFFFFF) + *src) | 0xFF000000;
    }
}

#endif
