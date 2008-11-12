/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/
#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpainter_p.h>
#include <private/qmath_p.h>
#include <private/qdrawhelper_x86_p.h>
#include <math.h>
#define MASK(src, a) src = BYTE_MUL(src, a)

#if defined(Q_OS_IRIX) && defined(Q_CC_GNU) && __GNUC__ == 3 && __GNUC__ < 4 && QT_POINTER_SIZE == 8
#define Q_IRIX_GCC3_3_WORKAROUND
//
// work around http://gcc.gnu.org/bugzilla/show_bug.cgi?id=14484
//
static uint gccBug(uint value) __attribute__((noinline));
static uint gccBug(uint value)
{
    return value;
}
#endif

/*
  constants and structures
*/

static const int fixed_scale = 1 << 16;
static const int half_point = 1 << 15;
static const int buffer_size = 2048;

struct LinearGradientValues
{
    qreal dx;
    qreal dy;
    qreal l;
    qreal off;
};

struct RadialGradientValues
{
    qreal dx;
    qreal dy;
    qreal a;
};

struct Operator;
typedef uint *QT_FASTCALL (*DestFetchProc)(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length);
typedef void QT_FASTCALL (*DestStoreProc)(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length);
typedef const uint *QT_FASTCALL (*SourceFetchProc)(uint *buffer, const Operator *o, const QSpanData *data, int y, int x, int length);


struct Operator
{
    QPainter::CompositionMode mode;
    DestFetchProc dest_fetch;
    DestStoreProc dest_store;
    SourceFetchProc src_fetch;
    CompositionFunctionSolid funcSolid;
    CompositionFunction func;
    union {
        LinearGradientValues linear;
        RadialGradientValues radial;
//        TextureValues texture;
    };
};

/*
  Destination fetch. This is simple as we don't have to do bounds checks or
  transformations
*/

static uint * QT_FASTCALL destFetchMono(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x80 >> (x & 7)) ? 0xff000000 : 0xffffffff;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchMonoLsb(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x1 << (x & 7)) ? 0xff000000 : 0xffffffff;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchRGB32(uint *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    uint *data = (uint *)rasterBuffer->scanLine(y) + x;
    // This should work without us having to fix the alpha channel manually.
//     for (int i = 0; i < length; ++i)
//         data[i] |= 0xff000000;
    return data;
}

static uint * QT_FASTCALL destFetchARGB32(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const uint *data = (const uint *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = PREMUL(data[i]);
    return buffer;
}

static uint * QT_FASTCALL destFetchARGB32P(uint *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    return (uint *)rasterBuffer->scanLine(y) + x;
}

static uint * QT_FASTCALL destFetchRGB16(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const ushort *data = (const ushort *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = qConvertRgb16To32(data[i]);
    return buffer;
}

static const DestFetchProc destFetchProc[QImage::NImageFormats] =
{
    0, // Format_Invalid
    destFetchMono, // Format_Mono,
    destFetchMonoLsb, // Format_MonoLSB
    0, // Format_Indexed8
    destFetchRGB32, // Format_RGB32
    destFetchARGB32, // Format_ARGB32,
    destFetchARGB32P, // Format_ARGB32_Premultiplied
    destFetchRGB16    // Format_RGB16
};

/*
   Returns the color in the mono destination color table
   that is the "nearest" to /color/.
*/
static inline QRgb findNearestColor(QRgb color, QRasterBuffer *rbuf)
{
    QRgb color_0 = PREMUL(rbuf->destColor0);
    QRgb color_1 = PREMUL(rbuf->destColor1);
    color = PREMUL(color);

    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int rx, gx, bx;
    int dist_0, dist_1;

    rx = r - qRed(color_0);
    gx = g - qGreen(color_0);
    bx = b - qBlue(color_0);
    dist_0 = rx*rx + gx*gx + bx*bx;

    rx = r - qRed(color_1);
    gx = g - qGreen(color_1);
    bx = b - qBlue(color_1);
    dist_1 = rx*rx + gx*gx + bx*bx;

    if (dist_0 < dist_1)
        return color_0;
    return color_1;
}

/*
  Destination store.
*/

static void QT_FASTCALL destStoreMono(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 0x80 >> (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else {
                data[x >> 3] |= 0x80 >> (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 0x80 >> (x & 7);
            else
                data[x >> 3] &= ~(0x80 >> (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreMonoLsb(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 1 << (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else {
                data[x >> 3] |= 1 << (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 1 << (x & 7);
            else
                data[x >> 3] &= ~(1 << (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreARGB32(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uint *data = (uint *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        data[i] = INV_PREMUL(buffer[i]);
}

static void QT_FASTCALL destStoreRGB16(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    quint16 *data = (quint16*)rasterBuffer->scanLine(y) + x;
    qt_memconvert<quint16, quint32>(data, buffer, length);
}

static const DestStoreProc destStoreProc[QImage::NImageFormats] =
{
    0, // Format_Invalid
    destStoreMono, // Format_Mono,
    destStoreMonoLsb, // Format_MonoLSB
    0, // Format_Indexed8
    0, // Format_RGB32
    destStoreARGB32, // Format_ARGB32,
    0, // Format_ARGB32_Premultiplied
    destStoreRGB16 // Format_RGB16
};

/*
  Source fetches

  This is a bit more complicated, as we need several fetch routines for every surface type

  We need 5 fetch methods per surface type:
  untransformed
  transformed
  transformed tiled
  transformed bilinear
  transformed bilinear tiled

  We don't need bounds checks for untransformed, but we need them for the other ones.

  The generic implementation does pixel by pixel fetches
*/

static uint QT_FASTCALL fetchPixel_Mono(const uchar *scanLine, int x, const QVector<QRgb> *rgb)
{
    bool pixel = scanLine[x>>3] & (0x80 >> (x & 7));
    if (rgb) return rgb->at(pixel ? 1 : 0);
    return pixel ? 0xff000000 : 0xffffffff;
}

static uint QT_FASTCALL fetchPixel_MonoLSB(const uchar *scanLine, int x, const QVector<QRgb> *rgb)
{
    bool pixel = scanLine[x>>3] & (0x1 << (x & 7));
    if (rgb) return rgb->at(pixel ? 1 : 0);
    return pixel ? 0xff000000 : 0xffffffff;
}

static uint QT_FASTCALL fetchPixel_Indexed8(const uchar *scanLine, int x, const QVector<QRgb> *rgb)
{
    return PREMUL(rgb->at(scanLine[x]));
}

static uint QT_FASTCALL fetchPixel_RGB32(const uchar *scanLine, int x, const QVector<QRgb> *)
{
    return ((const uint *)scanLine)[x] | 0xff000000;
}

static uint QT_FASTCALL fetchPixel_ARGB32(const uchar *scanLine, int x, const QVector<QRgb> *)
{
    return PREMUL(((const uint *)scanLine)[x]);
}

static uint QT_FASTCALL fetchPixel_ARGB32_Premultiplied(const uchar *scanLine, int x, const QVector<QRgb> *)
{
    return ((const uint *)scanLine)[x];
}

static uint QT_FASTCALL fetchPixel_RGB16(const uchar *scanLine, int x, const QVector<QRgb> *)
{
    return qConvertRgb16To32(((const ushort *)scanLine)[x]);
}

typedef uint QT_FASTCALL (*FetchPixelProc)(const uchar *scanLine, int x, const QVector<QRgb> *);

static const FetchPixelProc fetchPixelProc[QImage::NImageFormats] =
{
    0,
    fetchPixel_Mono,
    fetchPixel_MonoLSB,
    fetchPixel_Indexed8,
    fetchPixel_RGB32,
    fetchPixel_ARGB32,
    fetchPixel_ARGB32_Premultiplied,
    fetchPixel_RGB16
};

enum TextureBlendType {
    BlendUntransformed,
    BlendTiled,
    BlendTransformed,
    BlendTransformedTiled,
    BlendTransformedBilinear,
    BlendTransformedBilinearTiled,
    NBlendTypes
};

static const uint * QT_FASTCALL fetch_generic(uint *buffer, const Operator *, const QSpanData *data,
                                             int y, int x, int length)
{
    FetchPixelProc fetch = fetchPixelProc[data->texture.format];
    const uchar *scanLine = data->texture.scanLine(y);
    for (int i = 0; i < length; ++i)
        buffer[i] = fetch(scanLine, x + i, data->texture.colorTable);
    return buffer;
}

static const uint * QT_FASTCALL fetchTransformed_generic(uint *buffer, const Operator *, const QSpanData *data,
                                                         int y, int x, int length)
{
    FetchPixelProc fetch = fetchPixelProc[data->texture.format];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);
    bool affine = !data->m13 && !data->m23;

    int fx = int((data->m21 * (y + 0.5)
                  + data->m11 * (x + 0.5) + data->dx) * fixed_scale);
    int fy = int((data->m22 * (y + 0.5)
                  + data->m12 * (x + 0.5) + data->dy) * fixed_scale);

    const uint *end = buffer + length;
    uint *b = buffer;
    if (affine) {
        while (b < end) {
            int px = fx >> 16;
            int py = fy >> 16;

            bool out = (px < 0) || (px >= image_width)
                       || (py < 0) || (py >= image_height);

            const uchar *scanLine = data->texture.scanLine(py);
            *b = out ? uint(0) : fetch(scanLine, px, data->texture.colorTable);
            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        int fw = int((data->m13 * (x + 0.5)
                      + data->m23 * (y + 0.5) + 1.) * fixed_scale);
        if (!fw)
            fw = 1;
        while (b < end) {
            int px = fx/fw;
            int py = fy/fw;

            bool out = (px < 0) || (px >= image_width)
                       || (py < 0) || (py >= image_height);

            const uchar *scanLine = data->texture.scanLine(py);
            *b = out ? uint(0) : fetch(scanLine, px, data->texture.colorTable);
            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

static const uint * QT_FASTCALL fetchTransformedTiled_generic(uint *buffer, const Operator *, const QSpanData *data,
                                                              int y, int x, int length)
{
    FetchPixelProc fetch = fetchPixelProc[data->texture.format];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);
    bool affine = !data->m13 && !data->m23;

    int fx = int((data->m21 * (y + 0.5)
                  + data->m11 * (x + 0.5) + data->dx) * fixed_scale);
    int fy = int((data->m22 * (y + 0.5)
                  + data->m12 * (x + 0.5) + data->dy) * fixed_scale);

    const uint *end = buffer + length;
    uint *b = buffer;
    if (affine) {
        while (b < end) {
            int px = fx >> 16;
            int py = fy >> 16;

            px %= image_width;
            py %= image_height;
            if (px < 0) px += image_width;
            if (py < 0) py += image_height;

            const uchar *scanLine = data->texture.scanLine(py);
            *b = fetch(scanLine, px, data->texture.colorTable);
            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        int fw = int((data->m13 * (x + 0.5)
                     + data->m23 * (y + 0.5) + 1.) * fixed_scale);
        if (!fw)
            fw = 1;
        while (b < end) {
            int px = fx/fw;
            int py = fy/fw;

            px %= image_width;
            py %= image_height;
            if (px < 0) px += image_width;
            if (py < 0) py += image_height;

            const uchar *scanLine = data->texture.scanLine(py);
            *b = fetch(scanLine, px, data->texture.colorTable);
            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

static const uint * QT_FASTCALL fetchTransformedBilinear_generic(uint *buffer, const Operator *, const QSpanData *data,
                                                                 int y, int x, int length)
{
    FetchPixelProc fetch = fetchPixelProc[data->texture.format];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);
    bool affine = !data->m13 && !data->m23;

    int fx = int((data->m21 * (y + 0.5)
                  + data->m11 * (x + 0.5) + data->dx) * fixed_scale) - half_point;
    int fy = int((data->m22 * (y + 0.5)
                  + data->m12 * (x + 0.5) + data->dy) * fixed_scale) - half_point;

    const uint *end = buffer + length;
    uint *b = buffer;
    if (affine) {
        while (b < end) {
            int x1 = (fx >> 16);
            int x2 = x1 + 1;
            int y1 = (fy >> 16);
            int y2 = y1 + 1;

            int distx = ((fx - (x1 << 16)) >> 8);
            int disty = ((fy - (y1 << 16)) >> 8);
            int idistx = 256 - distx;
            int idisty = 256 - disty;

            x1 = qBound(0, x1, image_width - 1);
            x2 = qBound(0, x2, image_width - 1);
            y1 = qBound(0, y1, image_height - 1);
            y2 = qBound(0, y2, image_height - 1);

            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            uint tl = fetch(s1, x1, data->texture.colorTable);
            uint tr = fetch(s1, x2, data->texture.colorTable);
            uint bl = fetch(s2, x1, data->texture.colorTable);
            uint br = fetch(s2, x2, data->texture.colorTable);

            uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
            uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
            *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        int fw = int((data->m13 * (x + 0.5)
                      + data->m23 * (y + 0.5) + 1.) * fixed_scale);
        if (!fw)
            fw = 1;
        while (b < end) {
            int x1 = fx/fw;
            int x2 = x1 + 1;
            int y1 = fy/fw;
            int y2 = y1 + 1;

            int distx = ((fx -(x1*fw)) >> 8) & 0xff;
            int disty = ((fy -(y1*fw)) >> 8) & 0xff;
            int idistx = 256 - distx;
            int idisty = 256 - disty;

            x1 = qBound(0, x1, image_width - 1);
            x2 = qBound(0, x2, image_width - 1);
            y1 = qBound(0, y1, image_height - 1);
            y2 = qBound(0, y2, image_height - 1);

            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            uint tl = fetch(s1, x1, data->texture.colorTable);
            uint tr = fetch(s1, x2, data->texture.colorTable);
            uint bl = fetch(s2, x1, data->texture.colorTable);
            uint br = fetch(s2, x2, data->texture.colorTable);

            uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
            uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
            *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

static const uint * QT_FASTCALL fetchTransformedBilinearTiled_generic(uint *buffer, const Operator *, const QSpanData *data,
                                                                     int y, int x, int length)
{
    FetchPixelProc fetch = fetchPixelProc[data->texture.format];

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    bool affine = !data->m13 && !data->m23;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    int fx = int((data->m21 * (y + 0.5)
                  + data->m11 * (x + 0.5) + data->dx) * fixed_scale) - half_point;
    int fy = int((data->m22 * (y + 0.5)
                  + data->m12 * (x + 0.5) + data->dy) * fixed_scale) - half_point;

    const uint *end = buffer + length;
    uint *b = buffer;
    if (affine) {
        while (b < end) {
            int x1 = (fx >> 16);
            int x2 = x1 + 1;
            int y1 = (fy >> 16);
            int y2 = y1 + 1;

            int distx = ((fx - (x1 << 16)) >> 8);
            int disty = ((fy - (y1 << 16)) >> 8);
            int idistx = 256 - distx;
            int idisty = 256 - disty;

            x1 %= image_width;
            x2 %= image_width;
            y1 %= image_height;
            y2 %= image_height;

            if (x1 < 0) x1 += image_width;
            if (x2 < 0) x2 += image_width;
            if (y1 < 0) y1 += image_height;
            if (y2 < 0) y2 += image_height;

            Q_ASSERT(x1 >= 0 && x1 < image_width);
            Q_ASSERT(x2 >= 0 && x2 < image_width);
            Q_ASSERT(y1 >= 0 && y1 < image_height);
            Q_ASSERT(y2 >= 0 && y2 < image_height);

            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            uint tl = fetch(s1, x1, data->texture.colorTable);
            uint tr = fetch(s1, x2, data->texture.colorTable);
            uint bl = fetch(s2, x1, data->texture.colorTable);
            uint br = fetch(s2, x2, data->texture.colorTable);

            uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
            uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
            *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        int fw = int((data->m13 * (x + 0.5)
                      + data->m23 * (y + 0.5) + 1.) * fixed_scale);
        if (!fw)
            fw = 1;
        while (b < end) {
            int x1 = fx/fw;
            int x2 = x1 + 1;
            int y1 = fy/fw;
            int y2 = y1 + 1;

            int distx = ((fx -(x1*fw)) >> 8) & 0xff;
            int disty = ((fy -(y1*fw)) >> 8) & 0xff;
            int idistx = 256 - distx;
            int idisty = 256 - disty;

            x1 %= image_width;
            x2 %= image_width;
            y1 %= image_height;
            y2 %= image_height;

            if (x1 < 0) x1 += image_width;
            if (x2 < 0) x2 += image_width;
            if (y1 < 0) y1 += image_height;
            if (y2 < 0) y2 += image_height;

            Q_ASSERT(x1 >= 0 && x1 < image_width);
            Q_ASSERT(x2 >= 0 && x2 < image_width);
            Q_ASSERT(y1 >= 0 && y1 < image_height);
            Q_ASSERT(y2 >= 0 && y2 < image_height);

            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            uint tl = fetch(s1, x1, data->texture.colorTable);
            uint tr = fetch(s1, x2, data->texture.colorTable);
            uint bl = fetch(s2, x1, data->texture.colorTable);
            uint br = fetch(s2, x2, data->texture.colorTable);

            uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
            uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
            *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}


static const SourceFetchProc sourceFetch[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        fetch_generic,   // Mono
        fetch_generic,   // MonoLsb
        fetch_generic,   // Indexed8
        fetch_generic,   // RGB32
        fetch_generic,   // ARGB32
        fetch_generic,   // ARGB32_Premultiplied
        fetch_generic    // RGB16
    },
    // Tiled
    {
        0, // Invalid
        fetch_generic,   // Mono
        fetch_generic,   // MonoLsb
        fetch_generic,   // Indexed8
        fetch_generic,   // RGB32
        fetch_generic,   // ARGB32
        fetch_generic,   // ARGB32_Premultiplied
        fetch_generic    // RGB16
    },
    // Transformed
    {
        0, // Invalid
        fetchTransformed_generic,   // Mono
        fetchTransformed_generic,   // MonoLsb
        fetchTransformed_generic,   // Indexed8
        fetchTransformed_generic,   // RGB32
        fetchTransformed_generic,   // ARGB32
        fetchTransformed_generic,   // ARGB32_Premultiplied
        fetchTransformed_generic    // RGB16
    },
    {
        0, // TransformedTiled
        fetchTransformedTiled_generic,   // Mono
        fetchTransformedTiled_generic,   // MonoLsb
        fetchTransformedTiled_generic,   // Indexed8
        fetchTransformedTiled_generic,   // RGB32
        fetchTransformedTiled_generic,   // ARGB32
        fetchTransformedTiled_generic,   // ARGB32_Premultiplied
        fetchTransformedTiled_generic    // RGB16
    },
    {
        0, // Bilinear
        fetchTransformedBilinear_generic,   // Mono
        fetchTransformedBilinear_generic,   // MonoLsb
        fetchTransformedBilinear_generic,   // Indexed8
        fetchTransformedBilinear_generic,   // RGB32
        fetchTransformedBilinear_generic,   // ARGB32
        fetchTransformedBilinear_generic,   // ARGB32_Premultiplied
        fetchTransformedBilinear_generic    // RGB16
    },
    {
        0, // BilinearTiled
        fetchTransformedBilinearTiled_generic,   // Mono
        fetchTransformedBilinearTiled_generic,   // MonoLsb
        fetchTransformedBilinearTiled_generic,   // Indexed8
        fetchTransformedBilinearTiled_generic,   // RGB32
        fetchTransformedBilinearTiled_generic,   // ARGB32
        fetchTransformedBilinearTiled_generic,   // ARGB32_Premultiplied
        fetchTransformedBilinearTiled_generic    // RGB16
    },
};


static uint qt_gradient_pixel(const GradientData *data, qreal pos)
{
    int ipos = qRound(pos * GRADIENT_STOPTABLE_SIZE - 1);

  // calculate the actual offset.
    if (ipos < 0 || ipos >= GRADIENT_STOPTABLE_SIZE) {
        if (data->spread == QGradient::RepeatSpread) {
            ipos = ipos % GRADIENT_STOPTABLE_SIZE;
            ipos = ipos < 0 ? GRADIENT_STOPTABLE_SIZE + ipos : ipos;

        } else if (data->spread == QGradient::ReflectSpread) {
            const int limit = GRADIENT_STOPTABLE_SIZE * 2 - 1;
            ipos = ipos % limit;
            ipos = ipos < 0 ? limit + ipos : ipos;
            ipos = ipos >= GRADIENT_STOPTABLE_SIZE ? limit - ipos : ipos;

        } else {
            if (ipos < 0) ipos = 0;
            else if (ipos >= GRADIENT_STOPTABLE_SIZE) ipos = GRADIENT_STOPTABLE_SIZE-1;
        }
    }

    Q_ASSERT(ipos >= 0);
    Q_ASSERT(ipos < GRADIENT_STOPTABLE_SIZE);

    return data->colorTable[ipos];
}

#ifdef Q_WS_QWS

#define FIXPT_BITS 8
#define FIXPT_SIZE (1<<FIXPT_BITS)

static uint qt_gradient_pixel_fixed(const GradientData *data, int fixed_pos)
{
    int ipos = ((fixed_pos + FIXPT_SIZE / 2) >> FIXPT_BITS) - 1;

    // calculate the actual offset.
    if (ipos < 0 || ipos >= GRADIENT_STOPTABLE_SIZE) {
        if (data->spread == QGradient::RepeatSpread) {
            ipos = ipos % GRADIENT_STOPTABLE_SIZE;
            ipos = ipos < 0 ? GRADIENT_STOPTABLE_SIZE + ipos : ipos;

        } else if (data->spread == QGradient::ReflectSpread) {
            const int limit = GRADIENT_STOPTABLE_SIZE * 2 - 1;
            ipos = ipos % limit;
            ipos = ipos < 0 ? limit + ipos : ipos;
            ipos = ipos >= GRADIENT_STOPTABLE_SIZE ? limit - ipos : ipos;

        } else {
            if (ipos < 0) ipos = 0;
            else if (ipos >= GRADIENT_STOPTABLE_SIZE) ipos = GRADIENT_STOPTABLE_SIZE-1;
        }
    }

    Q_ASSERT(ipos >= 0);
    Q_ASSERT(ipos < GRADIENT_STOPTABLE_SIZE);

    return data->colorTable[ipos];
}
#endif

static void QT_FASTCALL getLinearGradientValues(LinearGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.linear.end.x - data->gradient.linear.origin.x;
    v->dy = data->gradient.linear.end.y - data->gradient.linear.origin.y;
    v->l = v->dx * v->dx + v->dy * v->dy;
    v->off = 0;
    if (v->l != 0) {
        v->dx /= v->l;
        v->dy /= v->l;
        v->off = -v->dx * data->gradient.linear.origin.x - v->dy * data->gradient.linear.origin.y;
    }
}

#ifdef Q_WS_QWS
static const uint * QT_FASTCALL fetchLinearGradient(uint *buffer, const Operator *op, const QSpanData *data,
                                                    int y, int x, int length)
{
    const uint *b = buffer;
    qreal t, inc;

    bool affine = true;
    qreal rx=0, ry=0;
    if (op->linear.l == 0) {
        t = inc = 0;
    } else {
        rx = data->m21 * y + data->m11 * x + data->dx;
        ry = data->m22 * y + data->m12 * x + data->dy;
        t = op->linear.dx*rx + op->linear.dy*ry + op->linear.off;
        inc = op->linear.dx * data->m11 + op->linear.dy * data->m12;
        affine = !data->m13 && !data->m23;

        if (affine) {
            t *= GRADIENT_STOPTABLE_SIZE;
            inc *= GRADIENT_STOPTABLE_SIZE;
        }
    }

    const uint *end = buffer + length;
    if (affine) {
        if (inc > -1e-5 && inc < 1e-5) {
            QT_MEMFILL_UINT(buffer, length, qt_gradient_pixel_fixed(&data->gradient, int(t * FIXPT_SIZE)));
        } else {
            if (t+inc*length < qreal(INT_MAX >> (FIXPT_BITS + 1)) &&
                t+inc*length > qreal(INT_MIN >> (FIXPT_BITS + 1))) {
                // we can use fixed point math
                int t_fixed = int(t * FIXPT_SIZE);
                int inc_fixed = int(inc * FIXPT_SIZE);
                while (buffer < end) {
                    *buffer = qt_gradient_pixel_fixed(&data->gradient, t_fixed);
                    t_fixed += inc_fixed;
                    ++buffer;
                }
            } else {
                // we have to fall back to float math
                while (buffer < end) {
                    *buffer = qt_gradient_pixel(&data->gradient, t/GRADIENT_STOPTABLE_SIZE);
                    t += inc;
                    ++buffer;
                }
            }
        }
    } else { // fall back to float math here as well
        qreal rw = data->m23 * y + data->m13 * x + 1.;
        while (buffer < end) {
            qreal x = rx/rw;
            qreal y = ry/rw;
            t = (op->linear.dx*x + op->linear.dy *y) + op->linear.off;

            *buffer = qt_gradient_pixel(&data->gradient, t);
            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }

    return b;
}
#else
static const uint * QT_FASTCALL fetchLinearGradient(uint *buffer, const Operator *op, const QSpanData *data,
                                                    int y, int x, int length)
{
    const uint *b = buffer;
    qreal t, inc;

    bool affine = true;
    qreal rx=0, ry=0;
    if (op->linear.l == 0) {
        t = inc = 0;
    } else {
        rx = data->m21 * y + data->m11 * x + data->dx;
        ry = data->m22 * y + data->m12 * x + data->dy;
        t = op->linear.dx*rx + op->linear.dy*ry + op->linear.off;
        inc = op->linear.dx * data->m11 + op->linear.dy * data->m12;
        affine = !data->m13 && !data->m23;
    }

    const uint *end = buffer + length;
    if (affine) {
        if (inc > -1e-6 && inc < 1e-6) {
            QT_MEMFILL_UINT(buffer, length, qt_gradient_pixel(&data->gradient, t));
        } else {
            while (buffer < end) {
                *buffer = qt_gradient_pixel(&data->gradient, t);

                t += inc;
                ++buffer;
            }
        }
    } else {
        qreal rw = data->m23 * y + data->m13 * x + 1.;
        while (buffer < end) {
            qreal x = rx/rw;
            qreal y = ry/rw;
            t = (op->linear.dx*x + op->linear.dy *y) + op->linear.off;

            *buffer = qt_gradient_pixel(&data->gradient, t);
            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }

    return b;
}
#endif

static inline qreal determinant(qreal a, qreal b, qreal c)
{
    return (b * b) - (4 * a * c);
}

// function to evaluate real roots
static inline qreal realRoots(qreal a, qreal b, qreal detSqrt)
{
    return (-b + detSqrt)/(2 * a);
}

static void QT_FASTCALL getRadialGradientValues(RadialGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.radial.center.x - data->gradient.radial.focal.x;
    v->dy = data->gradient.radial.center.y - data->gradient.radial.focal.y;
    v->a = data->gradient.radial.radius*data->gradient.radial.radius - v->dx*v->dx - v->dy*v->dy;
}

static const uint * QT_FASTCALL fetchRadialGradient(uint *buffer, const Operator *op, const QSpanData *data,
                                                    int y, int x, int length)
{
    const uint *b = buffer;
    qreal rx = data->m21 * (y + 0.5)
               + data->dx + data->m11 * (x + 0.5);
    qreal ry = data->m22 * (y + 0.5)
               + data->dy + data->m12 * (x + 0.5);
    bool affine = !data->m13 && !data->m23;
    //qreal r  = data->gradient.radial.radius;

    const uint *end = buffer + length;
    if (affine) {
        rx -= data->gradient.radial.focal.x;
        ry -= data->gradient.radial.focal.y;
        while (buffer < end) {
            qreal b  = 2*(rx*op->radial.dx + ry*op->radial.dy);
            qreal det = determinant(op->radial.a, b , -(rx*rx + ry*ry));
            qreal s = realRoots(op->radial.a, b, qSqrt(det));

            *buffer = qt_gradient_pixel(&data->gradient,  s);

            rx += data->m11;
            ry += data->m12;
            ++buffer;
        }
    } else {
        qreal rw = data->m23 * (y + 0.5)
                   + 1. + data->m13 * (x + 0.5);
        if (!rw)
            rw = 1;
        while (buffer < end) {
            qreal gx = rx/rw - data->gradient.radial.focal.x;
            qreal gy = ry/rw - data->gradient.radial.focal.y;
            qreal b  = 2*(gx*op->radial.dx + gy*op->radial.dy);
            qreal det = determinant(op->radial.a, b , -(gx*gx + gy*gy));
            qreal s = realRoots(op->radial.a, b, qSqrt(det));

        *buffer = qt_gradient_pixel(&data->gradient, s);

            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }

    return b;
}

static const uint * QT_FASTCALL fetchConicalGradient(uint *buffer, const Operator *, const QSpanData *data,
                                                     int y, int x, int length)
{
    const uint *b = buffer;
    qreal rx = data->m21 * (y + 0.5)
               + data->dx + data->m11 * (x + 0.5);
    qreal ry = data->m22 * (y + 0.5)
               + data->dy + data->m12 * (x + 0.5);
    bool affine = !data->m13 && !data->m23;

    const uint *end = buffer + length;
    if (affine) {
        rx -= data->gradient.conical.center.x;
        ry -= data->gradient.conical.center.y;
        while (buffer < end) {
            qreal angle = atan2(ry, rx) + data->gradient.conical.angle;

            *buffer = qt_gradient_pixel(&data->gradient, 1. - angle / (2*Q_PI));

            rx += data->m11;
            ry += data->m12;
            ++buffer;
        }
    } else {
        qreal rw = data->m23 * (y + 0.5)
                   + 1. + data->m13 * (x + 0.5);
        if (!rw)
            rw = 1;
        while (buffer < end) {
            qreal angle = atan2(ry/rw - data->gradient.conical.center.x,
                                rx/rw - data->gradient.conical.center.y)
                          + data->gradient.conical.angle;

            *buffer = qt_gradient_pixel(&data->gradient, 1. - angle / (2*Q_PI));

            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }
    return b;
}



/* The constant alpha factor describes an alpha factor that gets applied
   to the result of the composition operation combining it with the destination.

   The intent is that if const_alpha == 0. we get back dest, and if const_alpha == 1.
   we get the unmodified operation

   result = src op dest
   dest = result * const_alpha + dest * (1. - const_alpha)

   This means that in the comments below, the first line is the const_alpha==255 case, the
   second line the general one.

   In the lines below:
   s == src, sa == alpha(src), sia = 1 - alpha(src)
   d == dest, da == alpha(dest), dia = 1 - alpha(dest)
   ca = const_alpha, cia = 1 - const_alpha

   The methods exist in two variants. One where we have a constant source, the other
   where the source is an array of pixels.
*/

/*
  result = 0
  d = d * cia
*/
static void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint, uint const_alpha)
{
    if (const_alpha == 255) {
        QT_MEMFILL_UINT(dest, length, 0);
    } else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(dest[i], ialpha);
    }
}

static void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        QT_MEMFILL_UINT(dest, length, 0);
    } else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(dest[i], ialpha);
    }
}

/*
  result = s
  dest = s * ca + d * cia
*/
static void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        int ialpha = 255 - const_alpha;
        color = BYTE_MUL(color, const_alpha);
        for (int i = 0; i < length; ++i)
            dest[i] = color + BYTE_MUL(dest[i], ialpha);
    }
}

static void QT_FASTCALL comp_func_Source(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        int ialpha = 255 - const_alpha;
        for (int i = 0; i < length; ++i)
            dest[i] = INTERPOLATE_PIXEL_255(src[i], const_alpha, dest[i], ialpha);
    }
}

/*
  result = s + d * sia
  dest = (s + d * sia) * ca + d * cia
       = s * ca + d * (sia * ca + cia)
       = s * ca + d * (1 - sa*ca)
*/
static void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);
        for (int i = 0; i < length; ++i)
            dest[i] = color + BYTE_MUL(dest[i], qAlpha(~color));
    }
}

static void QT_FASTCALL comp_func_SourceOver(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            uint s = src[i];
            dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    }
}

/*
  result = d + s * dia
  dest = (d + s * dia) * ca + d * cia
       = d + s * dia * ca
*/
static void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        dest[i] = d + BYTE_MUL(color, qAlpha(~d));
    }
}

static void QT_FASTCALL comp_func_DestinationOver(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            dest[i] = d + BYTE_MUL(src[i], qAlpha(~d));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = d + BYTE_MUL(s, qAlpha(~d));
        }
    }
}

/*
  result = s * da
  dest = s * da * ca + d * cia
*/
static void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(color, qAlpha(dest[i]));
    } else {
        color = BYTE_MUL(color, const_alpha);
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(d), d, cia);
        }
    }
}

static void QT_FASTCALL comp_func_SourceIn(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(src[i], qAlpha(dest[i]));
    } else {
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, cia);
        }
    }
}

/*
  result = d * sa
  dest = d * sa * ca + d * cia
       = d * (sa * ca + cia)
*/
static void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    }
    for (int i = 0; i < length; ++i) {
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

static void QT_FASTCALL comp_func_DestinationIn(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(dest[i], qAlpha(src[i]));
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint a = BYTE_MUL(qAlpha(src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], a);
        }
    }
}

/*
  result = s * dia
  dest = s * dia * ca + d * cia
*/

static void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(color, qAlpha(~dest[i]));
    } else {
        color = BYTE_MUL(color, const_alpha);
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, cia);
        }
    }
}

static void QT_FASTCALL comp_func_SourceOut(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(src[i], qAlpha(~dest[i]));
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, cia);
        }
    }
}

/*
  result = d * sia
  dest = d * sia * ca + d * cia
       = d * (sia * ca + cia)
*/
static void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(~color);
    if (const_alpha != 255)
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    for (int i = 0; i < length; ++i)
        dest[i] = BYTE_MUL(dest[i], a);
}

static void QT_FASTCALL comp_func_DestinationOut(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i)
            dest[i] = BYTE_MUL(dest[i], qAlpha(~src[i]));
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint sia = BYTE_MUL(qAlpha(~src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], sia);
        }
    }
}

/*
  result = s*da + d*sia
  dest = s*da*ca + d*sia*ca + d *cia
       = s*ca * da + d * (sia*ca + cia)
       = s*ca * da + d * (1 - sa*ca)
*/
static void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
    }
    uint sia = qAlpha(~color);
    for (int i = 0; i < length; ++i)
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(dest[i]), dest[i], sia);
}

static void QT_FASTCALL comp_func_SourceAtop(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    }
}

/*
  result = d*sa + s*dia
  dest = d*sa*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sa*ca + cia)
*/
static void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
        a = qAlpha(color) + 255 - const_alpha;
    }
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(d, a, color, qAlpha(~d));
    }
}

static void QT_FASTCALL comp_func_DestinationAtop(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(d, qAlpha(s), s, qAlpha(~d));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            uint a = qAlpha(s) + cia;
            dest[i] = INTERPOLATE_PIXEL_255(d, a, s, qAlpha(~d));
        }
    }
}

/*
  result = d*sia + s*dia
  dest = d*sia*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sia*ca + cia)
       = s*ca * dia + d * (1 - sa*ca)
*/
static void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    uint sia = qAlpha(~color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, sia);
    }
}

static void QT_FASTCALL comp_func_XOR(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            uint s = src[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    }
}

static const uint AMASK = 0xff000000;
static const uint RMASK = 0x00ff0000;
static const uint GMASK = 0x0000ff00;
static const uint BMASK = 0x000000ff;

struct FullCoverage {
    inline void store(uint *dest, const uint src) const
    {
        *dest = src;
    }
};

struct PartialCoverage {
    inline PartialCoverage(uint const_alpha)
        : ca(const_alpha)
        , ica(255 - const_alpha)
    {
    }

    inline void store(uint *dest, const uint src) const
    {
        *dest = INTERPOLATE_PIXEL_255(src, ca, *dest, ica);
    }

private:
    const uint ca;
    const uint ica;
};

static inline int mix_alpha(int da, int sa)
{
    return 255 - ((255 - sa) * (255 - da) >> 8);
}

/*
    Dca' = Sca.Da + Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca
*/
template <typename T>
static inline void comp_func_solid_Plus_impl(uint *dest, int length, uint color, const T &coverage)
{
    uint s = color;

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
#define MIX(mask) (qMin(((qint64(s)&mask) + (qint64(d)&mask)), qint64(mask)))
        d = (MIX(AMASK) | MIX(RMASK) | MIX(GMASK) | MIX(BMASK));
#undef MIX
        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_solid_Plus(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Plus_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Plus_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Plus_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

#define MIX(mask) (qMin(((qint64(s)&mask) + (qint64(d)&mask)), qint64(mask)))
        d = (MIX(AMASK) | MIX(RMASK) | MIX(GMASK) | MIX(BMASK));
#undef MIX

        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_Plus(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Plus_impl(dest, src, length, FullCoverage());
    else
        comp_func_Plus_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
    Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int multiply_op(int dst, int src, int da, int sa)
{
    return (src * dst + src * (255 - da) + dst * (255 - sa)) >> 8;
}

template <typename T>
static inline void comp_func_solid_Multiply_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Multiply(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Multiply_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Multiply_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Multiply_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Multiply(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Multiply_impl(dest, src, length, FullCoverage());
    else
        comp_func_Multiply_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca - Sca.Dca
*/
template <typename T>
static inline void comp_func_solid_Screen_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) 255 - (((255-a) * (255-b)) >> 8)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Screen(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Screen_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Screen_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Screen_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) 255 - (((255-a) * (255-b)) >> 8)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Screen(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Screen_impl(dest, src, length, FullCoverage());
    else
        comp_func_Screen_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
    if 2.Dca < Da
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int overlay_op(int dst, int src, int da, int sa)
{
    const int temp = src * (255 - da) + dst * (255 - sa);
    if (2 * dst < da)
        return (2 * src * dst + temp) >> 8;
    else
        return (sa * da - 2 * (da - dst) * (sa - src) + temp) >> 8;
}

template <typename T>
static inline void comp_func_solid_Overlay_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Overlay(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Overlay_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Overlay_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Overlay_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Overlay(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Overlay_impl(dest, src, length, FullCoverage());
    else
        comp_func_Overlay_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
    Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    Da'  = Sa + Da - Sa.Da
*/
static inline int darken_op(int dst, int src, int da, int sa)
{
    return (qMin(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa)) >> 8;
}

template <typename T>
static inline void comp_func_solid_Darken_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) darken_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Darken(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Darken_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Darken_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Darken_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) darken_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Darken(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Darken_impl(dest, src, length, FullCoverage());
    else
        comp_func_Darken_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
   Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
   Da'  = Sa + Da - Sa.Da
*/
static inline int lighten_op(int dst, int src, int da, int sa)
{
    return (qMax(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa)) >> 8;
}

template <typename T>
static inline void comp_func_solid_Lighten_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Lighten(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Lighten_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Lighten_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Lighten_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Lighten(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Lighten_impl(dest, src, length, FullCoverage());
    else
        comp_func_Lighten_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa >= Sa.Da
       Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Dca.Sa/(1-Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_dodge_op(int dst, int src, int da, int sa)
{
    const int sa_da = sa * da;
    const int dst_sa = dst * sa;
    const int src_da = src * da;

    const int temp = src * (255 - da) + dst * (255 - sa);
    if (src_da + dst_sa >= sa_da)
        return (sa_da + temp) >> 8;
    else
        return (255 * dst_sa / (255 - 255 * src / sa) + temp) >> 8;
}

template <typename T>
static inline void comp_func_solid_ColorDodge_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a,b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorDodge(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorDodge_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_ColorDodge_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_ColorDodge_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorDodge(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorDodge_impl(dest, src, length, FullCoverage());
    else
        comp_func_ColorDodge_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa <= Sa.Da
       Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_burn_op(int dst, int src, int da, int sa)
{
    const int src_da = src * da;
    const int dst_sa = dst * sa;
    const int sa_da = sa * da;

    const int temp = src * (255 - da) + dst * (255 - sa);

    if (src == 0 || src_da + dst_sa <= sa_da)
        return temp >> 8;
    else
        return (sa * (src_da + dst_sa - sa_da) / src + temp) >> 8;
}

template <typename T>
static inline void comp_func_solid_ColorBurn_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorBurn(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorBurn_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_ColorBurn_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_ColorBurn_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorBurn(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorBurn_impl(dest, src, length, FullCoverage());
    else
        comp_func_ColorBurn_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
    if 2.Sca < Sa
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline uint hardlight_op(int dst, int src, int da, int sa)
{
    const uint temp = src * (255 - da) + dst * (255 - sa);

    if (2 * src < sa)
        return (2 * src * dst + temp) >> 8;
    else
        return (sa * da - 2 * (da - dst) * (sa - src) + temp) >> 8;
}

template <typename T>
static inline void comp_func_solid_HardLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_HardLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_HardLight_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_HardLight_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_HardLight_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_HardLight(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_HardLight_impl(dest, src, length, FullCoverage());
    else
        comp_func_HardLight_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
   if 2.Sca < Sa
       Dca' = Dca.(Sa - (1 - Dca/Da).(2.Sca - Sa)) + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise if 8.Dca <= Da
       Dca' = Dca.(Sa - (1 - Dca/Da).(2.Sca - Sa).(3 - 8.Dca/Da)) + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = (Dca.Sa + ((Dca/Da)^(0.5).Da - Dca).(2.Sca - Sa)) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int soft_light_op(int dst, int src, int da, int sa)
{
    const int src2 = src + src;
    const int dst_np = da != 0 ? (255 * dst) / da : 0;
    const int temp = (src * (255 - da) + dst * (255 - sa)) << 8;

    if (src2 < sa)
        return (dst * ((sa << 8) - (255 - dst_np) * (src2 - sa)) + temp) >> 16;
    else if (8 * dst <= da)
        return (dst * ((sa << 8) - ((255 - dst_np) * (src2 - sa) * ((3 << 8) - 8 * dst_np) >> 8)) + temp) >> 16;
    else {
        // sqrt is too expensive to do three times per pixel, so skipping it for now
        // a future possibility is to use a LUT
        return ((dst * sa << 8) + (int(dst_np) * da - (dst << 8)) * (src2 - sa) + temp) >> 16;
    }
}

template <typename T>
static inline void comp_func_solid_SoftLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_SoftLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_SoftLight_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_SoftLight_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_SoftLight_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_SoftLight(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_SoftLight_impl(dest, src, length, FullCoverage());
    else
        comp_func_SoftLight_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
   Dca' = abs(Dca.Sa - Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
        = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
*/
static inline int difference_op(int dst, int src, int da, int sa)
{
    return src + dst - (qMin(src * da, dst * sa) >> 7);
}

template <typename T>
static inline void comp_func_solid_Difference_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) difference_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Difference(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Difference_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Difference_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Difference_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) difference_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Difference(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Difference_impl(dest, src, length, FullCoverage());
    else
        comp_func_Difference_impl(dest, src, length, PartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
template <typename T>
static inline void QT_FASTCALL comp_func_solid_Exclusion_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) (a + b - ((a*b) >> 7))
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Exclusion(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Exclusion_impl(dest, length, color, FullCoverage());
    else
        comp_func_solid_Exclusion_impl(dest, length, color, PartialCoverage(const_alpha));
}

template <typename T>
static inline void comp_func_Exclusion_impl(uint *dest, const uint *src, int length, const T &coverage)
{
    for (int i = 0; i < length; ++i) {
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) (a + b - ((a*b) >> 7))
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Exclusion(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Exclusion_impl(dest, src, length, FullCoverage());
    else
        comp_func_Exclusion_impl(dest, src, length, PartialCoverage(const_alpha));
}

static const CompositionFunctionSolid functionForModeSolid_C[] = {
        comp_func_solid_SourceOver,
        comp_func_solid_DestinationOver,
        comp_func_solid_Clear,
        comp_func_solid_Source,
        0,
        comp_func_solid_SourceIn,
        comp_func_solid_DestinationIn,
        comp_func_solid_SourceOut,
        comp_func_solid_DestinationOut,
        comp_func_solid_SourceAtop,
        comp_func_solid_DestinationAtop,
        comp_func_solid_XOR,
        comp_func_solid_Plus,
        comp_func_solid_Multiply,
        comp_func_solid_Screen,
        comp_func_solid_Overlay,
        comp_func_solid_Darken,
        comp_func_solid_Lighten,
        comp_func_solid_ColorDodge,
        comp_func_solid_ColorBurn,
        comp_func_solid_HardLight,
        comp_func_solid_SoftLight,
        comp_func_solid_Difference,
        comp_func_solid_Exclusion
};

static const CompositionFunctionSolid *functionForModeSolid = functionForModeSolid_C;

static const CompositionFunction functionForMode_C[] = {
        comp_func_SourceOver,
        comp_func_DestinationOver,
        comp_func_Clear,
        comp_func_Source,
        0,
        comp_func_SourceIn,
        comp_func_DestinationIn,
        comp_func_SourceOut,
        comp_func_DestinationOut,
        comp_func_SourceAtop,
        comp_func_DestinationAtop,
        comp_func_XOR,
        comp_func_Plus,
        comp_func_Multiply,
        comp_func_Screen,
        comp_func_Overlay,
        comp_func_Darken,
        comp_func_Lighten,
        comp_func_ColorDodge,
        comp_func_ColorBurn,
        comp_func_HardLight,
        comp_func_SoftLight,
        comp_func_Difference,
        comp_func_Exclusion
};

static const CompositionFunction *functionForMode = functionForMode_C;

static TextureBlendType getBlendType(const QSpanData *data)
{
    TextureBlendType ft;
    if (data->txop <= QTransform::TxTranslate)
        if (data->texture.type == TextureData::Tiled)
            ft = BlendTiled;
        else
            ft = BlendUntransformed;
    else if (data->bilinear)
        if (data->texture.type == TextureData::Tiled)
            ft = BlendTransformedBilinearTiled;
        else
            ft = BlendTransformedBilinear;
    else
        if (data->texture.type == TextureData::Tiled)
            ft = BlendTransformedTiled;
        else
            ft = BlendTransformed;
    return ft;
}

static inline Operator getOperator(const QSpanData *data, const QSpan *spans, int spanCount)
{
    Operator op;
    bool solidSource = false;

    switch(data->type) {
    case QSpanData::Solid:
        solidSource = (qAlpha(data->solid.color) == 255);
        break;
    case QSpanData::LinearGradient:
        solidSource = !data->gradient.alphaColor;
        getLinearGradientValues(&op.linear, data);
        op.src_fetch = fetchLinearGradient;
        break;
    case QSpanData::RadialGradient:
        solidSource = !data->gradient.alphaColor;
        getRadialGradientValues(&op.radial, data);
        op.src_fetch = fetchRadialGradient;
        break;
    case QSpanData::ConicalGradient:
        solidSource = !data->gradient.alphaColor;
        op.src_fetch = fetchConicalGradient;
        break;
    case QSpanData::Texture:
        op.src_fetch = sourceFetch[getBlendType(data)][data->texture.format];
        solidSource = !data->texture.hasAlpha;
    default:
        break;
    }

    op.mode = data->rasterBuffer->compositionMode;
    if (op.mode == QPainter::CompositionMode_SourceOver && solidSource)
        op.mode = QPainter::CompositionMode_Source;

    op.dest_fetch = destFetchProc[data->rasterBuffer->format];
    if (op.mode == QPainter::CompositionMode_Source) {
        switch (data->rasterBuffer->format) {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            // this one sets up the pointer correctly so we save one copy
            op.dest_fetch = destFetchProc[QImage::Format_ARGB32_Premultiplied];
            break;
        default: {
            const QSpan *lastSpan = spans + spanCount;
            bool alphaSpans = false;
            while (spans < lastSpan) {
                if (spans->coverage != 255) {
                    alphaSpans = true;
                    break;
                }
                ++spans;
            }
            if (!alphaSpans)
                op.dest_fetch = 0;
        }
        }
    }

    op.dest_store = destStoreProc[data->rasterBuffer->format];

    op.funcSolid = functionForModeSolid[op.mode];
    op.func = functionForMode[op.mode];

    return op;
}



// -------------------- blend methods ---------------------


static void blend_color_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    uint buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.funcSolid)
        return;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.funcSolid(dest, l, data->solid.color, spans->coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            length -= l;
            x += l;
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_color_generic_callback(int count, const QSpan *spans,
                                         void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    data->rasterEngine->drawColorSpans(spans, count, data->solid.color);
}
#endif

static void blend_color_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    if (!op.funcSolid)
        return;

    if (op.mode == QPainter::CompositionMode_Source) {
        // inline for performance
        while (count--) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_UINT(target, spans->len, data->solid.color);
            } else {
                uint c = BYTE_MUL(data->solid.color, spans->coverage);
                int ialpha = 255 - spans->coverage;
                for (int i = 0; i < spans->len; ++i)
                    target[i] = c + BYTE_MUL(target[i], ialpha);
            }
            ++spans;
        }
        return;
    }

    while (count--) {
        uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
        op.funcSolid(target, spans->len, data->solid.color, spans->coverage);
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_color_argb_callback(int count, const QSpan *spans,
                                      void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    data->rasterEngine->drawColorSpans(spans, count, data->solid.color);
}
#endif // Q_WS_QWS

static inline uint BYTE_MUL_RGB16(uint x, uint a) {
    a += 1;
    uint t = (((x & 0x07e0)*a) >> 8) & 0x07e0;
    t |= (((x & 0xf81f)*(a>>2)) >> 6) & 0xf81f;
    return t;
}

static inline uint BYTE_MUL_RGB16_32(uint x, uint a) {
    uint t = (((x & 0xf81f07e0) >> 5)*a) & 0xf81f07e0;
    t |= (((x & 0x07e0f81f)*a) >> 5) & 0x07e0f81f;
    return t;
}

static void blend_color_rgb16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);
    if (!op.funcSolid)
        return;

    if (op.mode == QPainter::CompositionMode_Source) {
        // inline for performance
        ushort c = qConvertRgb32To16(data->solid.color);
        while (count--) {
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_USHORT(target, spans->len, c);
            } else {
                ushort color = BYTE_MUL_RGB16(c, spans->coverage);
                int ialpha = 255 - spans->coverage;
                const ushort *end = target + spans->len;
                while (target < end) {
                    *target = color + BYTE_MUL_RGB16(*target, ialpha);
                    ++target;
                }
            }
            ++spans;
        }
        return;
    }

    Q_ASSERT(op.mode == QPainter::CompositionMode_SourceOver);

    while (count--) {
        uint color = BYTE_MUL(data->solid.color, spans->coverage);
        int ialpha = qAlpha(~color);
        ushort c = qConvertRgb32To16(color);
        ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
#if 1
        int len = spans->len;
        bool pre = (((unsigned long)target) & 0x3) != 0;
        bool post = false;
        if (pre) {
            // skip to word boundary
            *target = c + BYTE_MUL_RGB16(*target, ialpha);
            ++target;
            --len;
        }
        if (len & 0x1) {
            post = true;
            --len;
        }
        uint *target32 = (uint*)target;
        uint c32 = c | (c<<16);
        len >>= 1;
        uint salpha = (ialpha+1) >> 3; // calculate here rather than in loop
        while (len--) {
            // blend full words
            *target32 = c32 + BYTE_MUL_RGB16_32(*target32, salpha);
            ++target32;
            target += 2;
        }
        if (post) {
            // one last pixel beyond a full word
            *target = c + BYTE_MUL_RGB16(*target, ialpha);
        }
#else
        const ushort *end = target + spans->len;
        while (target != end) {
            *target = c + BYTE_MUL_RGB16(*target, ialpha);
            ++target;
        }
#endif
        ++spans;
    }
}

static void blend_src_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            const uint *src = op.src_fetch(src_buffer, &op, data, spans->y, x, l);
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.func(dest, src, l, spans->coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            x += l;
            length -= l;
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_src_generic_callback(int count, const QSpan *spans,
                                       void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            const uint *src = op.src_fetch(src_buffer, &op, data, spans->y, x, l);
            data->rasterEngine->drawBufferSpan(src, l, x, spans->y, l,
                                               spans->coverage);
            x += l;
            length -= l;
        }
        ++spans;
    }
}
#endif // Q_WS_QWS

static void blend_src_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    while (count--) {
        uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y));
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(length, buffer_size);
            const uint *src = op.src_fetch(src_buffer, &op, data, spans->y, x, l);
            op.func(target + x, src, l, spans->coverage);
            x += l;
            length -= l;
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_src_argb_callback(int count, const QSpan *spans,
                                    void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(length, buffer_size);
            const uint *src = op.src_fetch(src_buffer, &op, data, spans->y, x, l);
            data->rasterEngine->drawBufferSpan(src, buffer_size,
                                               x, spans->y, l,
                                               spans->coverage);
            x += l;
            length -= l;
        }
        ++spans;
    }
}
#endif // Q_WS_QWS

static void blend_untransformed_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx);
    int yoff = qRound(data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                while (length) {
                    int l = qMin(buffer_size, length);
                    const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
                    uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
                    op.func(dest, src, l, coverage);
                    if (op.dest_store)
                        op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_untransformed_generic_callback(int count, const QSpan *spans,
                                                 void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx);
    int yoff = qRound(data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                while (length) {
                    int l = qMin(buffer_size, length);
                    const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
                    data->rasterEngine->drawBufferSpan(src, l, x, spans->y,
                                                       l, coverage);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
        ++spans;
    }
}
#endif // Q_WS_QWS

static void blend_untransformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx);
    int yoff = qRound(data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
                op.func(dest, src, length, coverage);
            }
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_untransformed_argb_callback(int count, const QSpan *spans,
                                              void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_untransformed_generic_callback(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx);
    int yoff = qRound(data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                data->rasterEngine->drawBufferSpan(src, length, x,
                                                   spans->y, length, coverage);
            }
        }
        ++spans;
    }
}
#endif // Q_WS_QWS

#if defined(Q_WS_QWS) || (QT_VERSION >= 0x040400)
static inline quint16 interpolate_pixel_rgb16_255(quint16 x, quint8 a,
                                                  quint16 y, quint8 b)
{
    quint16 t = ((((x & 0x07e0) * a) + ((y & 0x07e0) * b)) >> 5) & 0x07e0;
    t |= ((((x & 0xf81f) * a) + ((y & 0xf81f) * b)) >> 5) & 0xf81f;

    return t;
}

static inline quint32 interpolate_pixel_rgb16x2_255(quint32 x, quint8 a,
                                                    quint32 y, quint8 b)
{
    uint t;
    t = ((((x & 0xf81f07e0) >> 5) * a) + (((y & 0xf81f07e0) >> 5) * b)) & 0xf81f07e0;
    t |= ((((x & 0x07e0f81f) * a) + ((y & 0x07e0f81f) * b)) >> 5) & 0x07e0f81f;
    return t;
}

static inline void blend_sourceOver_rgb16_rgb16(quint16 *dest,
                                                const quint16 *src,
                                                int length,
                                                const quint8 alpha,
                                                const quint8 ialpha)
{
    const int dstAlign = ((long)dest) & 0x3;
    if (dstAlign) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha,
                                            *dest, ialpha);
        ++dest;
        ++src;
        --length;
    }
    const int srcAlign = ((long)src) & 0x3;
    int length32 = length >> 1;
    if (length32 && srcAlign == 0) {
        while (length32--) {
            const quint32 *src32 = reinterpret_cast<const quint32*>(src);
            quint32 *dest32 = reinterpret_cast<quint32*>(dest);
            *dest32 = interpolate_pixel_rgb16x2_255(*src32,
                                                    alpha,
                                                    *dest32,
                                                    ialpha);
            dest += 2;
            src += 2;
        }
        length &= 0x1;
    }
    while (length--) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha,
                                            *dest, ialpha);
        ++dest;
        ++src;
    }
}
#endif

static void blend_untransformed_rgb16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_RGB16 &&
        data->texture.format != QImage::Format_ARGB32_Premultiplied) {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx);
    int yoff = qRound(data->dy);

    if (data->texture.format == QImage::Format_ARGB32_Premultiplied) {
        while (count--) {
            int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            int x = spans->x;
            int length = spans->len;
            int sx = xoff + x;
            int sy = yoff + spans->y;
            if (sy >= 0 && sy < image_height && sx < image_width) {
                if (sx < 0) {
                    x -= sx;
                    length += sx;
                    sx = 0;
                }
                if (sx + length > image_width)
                    length = image_width - sx;
                if (length > 0) {
                    ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                    const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                    if (coverage == 255) {
#if defined(Q_WS_QWS) || (QT_VERSION >= 0x040400)
                        const int dstAlign = ((long)dest) & 0x3;
                        if (dstAlign) {
                            const quint8 alpha = qAlpha(*src);
                            if (alpha) {
                                quint16 s = qConvertRgb32To16(*src);
                                if (alpha < 255)
                                    s += BYTE_MUL_RGB16(*dest, 255 - alpha);
                                *dest = s;
                            }
                            ++dest;
                            ++src;
                            --length;
                        }
                        const int length32 = length >> 1;
                        const int srcAlign = ((long)src) & 0x3;
                        if (length32) {
                            for (int i = 0; i < length32; ++i) {
                                quint32 *dest32 = reinterpret_cast<quint32*>(dest);
                                const quint8 a1 = qAlpha(src[0]);
                                const quint8 a2 = qAlpha(src[1]);
                                quint32 s;

                                if (!a1 && !a2) {
                                    src += 2;
                                    dest +=2;
                                    continue;
                                }

                                if (srcAlign) {
                                    s = qConvertRgb32To16(src[0])
                                        | (qConvertRgb32To16(src[1]) << 16);
                                } else {
                                    const quint64 *src64 =
                                        reinterpret_cast<const quint64*>(src);
                                    s = qConvertRgb32To16x2(*src64);
                                }

                                if (a1 == a2) {
                                    if (a1 < 255) {
                                        const quint8 sa = ((255 - a1)+1) >> 3;
                                        s += BYTE_MUL_RGB16_32(*dest32, sa);
                                    }
                                } else {
                                    if (a1 < 255)
                                        s += BYTE_MUL_RGB16(dest[0], 255 - a1);
                                    if (a2 < 255)
                                        s += BYTE_MUL_RGB16(dest[1], 255 - a2) << 16;
                                }

                                *dest32 = s;
                                src += 2;
                                dest += 2;
                            }
                        }
                        const int tail = length & 0x1;
                        if (tail) {
                            const quint8 alpha = qAlpha(*src);
                            if (alpha) {
                                quint16 s = qConvertRgb32To16(*src);
                                if (alpha < 255)
                                    s += BYTE_MUL_RGB16(*dest, 255 - alpha);
                                *dest = s;
                            }
                        }
#else
                        for (int i = 0; i < length; ++i) {
                            uint s = src[i];
                            int alpha = qAlpha(s);
                            s = qConvertRgb32To16(s);
                            if (alpha != 255)
                                s += BYTE_MUL_RGB16(dest[i], 255 - alpha);
                            dest[i] = s;
                        }
#endif
                    } else {
                        for (int i = 0; i < length; ++i) {
                            uint s = src[i];
                            MASK(s, coverage);
                            int alpha = qAlpha(s);
                            s = qConvertRgb32To16(s);
                            Q_ASSERT(alpha < 255);
                            s += BYTE_MUL_RGB16(dest[i], 255 - alpha);
                            dest[i] = s;
                        }
                    }
                }
            }
            ++spans;
        }
        return;
    }

    // texture is RGB16
    while (count--) {
        const int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
#if defined(Q_WS_QWS) || (QT_VERSION >= 0x040400)
        const int alpha = (coverage + 1) >> 3;
        const int ialpha = 0x20 - alpha;
#endif
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                const ushort *src = (ushort *)data->texture.scanLine(sy) + sx;
#if defined(Q_WS_QWS) || (QT_VERSION >= 0x040400)
                if (alpha == 0x20) {
                    memcpy(dest, src, length*sizeof(quint16));
                } else if (alpha > 0) {
                    blend_sourceOver_rgb16_rgb16(dest, src, length,
                                                 alpha, ialpha);
                }
#else
                if (coverage == 255) {
                    memcpy(dest, src, length*sizeof(quint16));
                } else if (coverage > 0) {
                    const int ialpha = 255 - coverage;
                    for (int i = 0; i < length; ++i)
                        dest[i] = qConvertRgb32To16(INTERPOLATE_PIXEL_255(qConvertRgb16To32(src[i]), coverage,
                                                                         qConvertRgb16To32(dest[i]), ialpha));
                }
#endif
            }
        }
        ++spans;
    }
}

static void blend_tiled_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx) % image_width;
    int yoff = qRound(data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.func(dest, src, l, coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_tiled_generic_callback(int count, const QSpan *spans,
                                         void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = qRound(data->dx) % image_width;
    int yoff = qRound(data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
            data->rasterEngine->drawBufferSpan(src, l, x, spans->y, l,
                                               coverage);
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}
#endif // Q_WS_QWS

static void blend_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    int xoff = qRound(data->dx) % image_width;
    int yoff = qRound(data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = (uint *)data->texture.scanLine(sy) + sx;
            uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
            op.func(dest, src, l, coverage);
            x += l;
            length -= l;
            sx = 0;
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_tiled_argb_callback(int count, const QSpan *spans,
                                      void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    int xoff = qRound(data->dx) % image_width;
    int yoff = qRound(data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = (uint *)data->texture.scanLine(sy) + sx;
            data->rasterEngine->drawBufferSpan(src, buffer_size,
                                               x, spans->y, l, coverage);
            x += l;
            length -= l;
            sx = 0;
        }
        ++spans;
    }
}
#endif

static void blend_tiled_rgb16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_RGB16
        && data->texture.format != QImage::Format_ARGB32_Premultiplied) {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    int xoff = qRound(data->dx) % image_width;
    int yoff = qRound(data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    if (data->texture.format == QImage::Format_ARGB32_Premultiplied) {
        while (count--) {
            int x = spans->x;
            int length = spans->len;
            int sx = (xoff + spans->x) % image_width;
            int sy = (spans->y + yoff) % image_height;
            if (sx < 0)
                sx += image_width;
            if (sy < 0)
                sy += image_height;

            if (spans->coverage == 255) {
                while (length) {
                    int l = qMin(image_width - sx, length);
                    if (buffer_size < l)
                        l = buffer_size;
                    ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                    const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                    for (int i = 0; i < l; ++i) {
                        uint s = src[i];
                        int alpha = qAlpha(s);
                        s = qConvertRgb32To16(s);
                        if (alpha != 255)
                            s += BYTE_MUL_RGB16(dest[i], 255 - alpha);
                        dest[i] = s;
                    }
                    x += l;
                    length -= l;
                    sx = 0;
                }
            } else {
                while (length) {
                    int l = qMin(image_width - sx, length);
                    if (buffer_size < l)
                        l = buffer_size;
                    ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                    const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                    for (int i = 0; i < l; ++i) {
                        uint s = src[i];
                        MASK(s, spans->coverage);
                        int alpha = qAlpha(s);
                        s = qConvertRgb32To16(s);
                        if (alpha != 255)
                            s += BYTE_MUL_RGB16(dest[i], 255 - alpha);
                        dest[i] = s;
                    }
                    x += l;
                    length -= l;
                    sx = 0;
                }
            }
            ++spans;
        }
        return;
    }

    // texture is RGB16
    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
#if defined(Q_WS_QWS) || (QT_VERSION >= 0x040400)
        const int alpha = (coverage + 1) >> 3;

        if (alpha == 0x20) {
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                const ushort *src = (ushort *)data->texture.scanLine(sy) + sx;
                memcpy(dest, src, l*sizeof(quint16));
                x += l;
                length -= l;
                sx = 0;
            }
        } else if (alpha > 0) {
            const int ialpha = 0x20 - alpha;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                const ushort *src = (ushort *)data->texture.scanLine(sy) + sx;
                blend_sourceOver_rgb16_rgb16(dest, src, l, alpha, ialpha);
                x += l;
                length -= l;
                sx = 0;
            }
        }
#else
        if (coverage == 255) {
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                const ushort *src = (ushort *)data->texture.scanLine(sy) + sx;
                memcpy(dest, src, l*sizeof(quint16));
                x += l;
                length -= l;
                sx = 0;
            }
        } else {
            int ialpha = 255 - coverage;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                ushort *dest = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + x;
                const ushort *src = (ushort *)data->texture.scanLine(sy) + sx;
                for (int i = 0; i < l; ++i)
                    dest[i] = qConvertRgb32To16(INTERPOLATE_PIXEL_255(qConvertRgb16To32(src[i]), coverage,
                                                                     qConvertRgb16To32(dest[i]), ialpha));
                x += l;
                length -= l;
                sx = 0;
            }
        }
#endif
        ++spans;
    }
}

static void blend_texture_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            const uint *src = op.src_fetch(src_buffer, &op, data, spans->y, x, l);
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.func(dest, src, l, coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);

            x += l;
            length -= l;
        }
        ++spans;
    }
}

#ifdef Q_WS_QWS
static void blend_texture_generic_callback(int count, const QSpan *spans,
                                           void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);
    if (!op.func)
        return;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            const uint *src = op.src_fetch(src_buffer, &op, data, spans->y, x, l);
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            data->rasterEngine->drawBufferSpan(src, buffer_size,
                                               x, spans->y, l, coverage);

            x += l;
            length -= l;
        }
        ++spans;
    }
}
#endif // Q_WS_QWS

static void blend_transformed_bilinear_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    bool affine = !data->m13 && !data->m23;

    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;

            int length = spans->len;
            const int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = (x >> 16);
                    int x2 = x1 + 1;
                    int y1 = (y >> 16);
                    int y2 = y1 + 1;

                    int distx = ((x - (x1 << 16)) >> 8);
                    int disty = ((y - (y1 << 16)) >> 8);
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 = qBound(0, x1, image_width - 1);
                    x2 = qBound(0, x2, image_width - 1);
                    y1 = qBound(0, y1, image_height - 1);
                    y2 = qBound(0, y2, image_height - 1);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;

                    x += fdx;
                    y += fdy;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            int length = spans->len;
            const int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = x/w;
                    int x2 = x1 + 1;
                    int y1 = y/w;
                    int y2 = y1 + 1;

                    int distx = ((x -(x1*w)) >> 8) & 0xff;
                    int disty = ((y -(y1*w)) >> 8) & 0xff;
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 = qBound(0, x1, image_width - 1);
                    x2 = qBound(0, x2, image_width - 1);
                    y1 = qBound(0, y1, image_height - 1);
                    y2 = qBound(0, y2, image_height - 1);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

#ifdef Q_WS_QWS
static void blend_transformed_bilinear_argb_callback(int count,
                                                     const QSpan *spans,
                                                     void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    bool affine = !data->m13 && !data->m23;

    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;

            int length = spans->len;
            const int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = (x >> 16);
                    int x2 = x1 + 1;
                    int y1 = (y >> 16);
                    int y2 = y1 + 1;

                    int distx = ((x - (x1 << 16)) >> 8);
                    int disty = ((y - (y1 << 16)) >> 8);
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 = qBound(0, x1, image_width - 1);
                    x2 = qBound(0, x2, image_width - 1);
                    y1 = qBound(0, y1, image_height - 1);
                    y2 = qBound(0, y2, image_height - 1);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;

                    x += fdx;
                    y += fdy;
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            int length = spans->len;
            const int coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = x/w;
                    int x2 = x1 + 1;
                    int y1 = y/w;
                    int y2 = y1 + 1;

                    int distx = ((x -(x1*w)) >> 8) & 0xff;
                    int disty = ((y -(y1*w)) >> 8) & 0xff;
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 = qBound(0, x1, image_width - 1);
                    x2 = qBound(0, x2, image_width - 1);
                    y1 = qBound(0, y1, image_height - 1);
                    y2 = qBound(0, y2, image_height - 1);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}
#endif // Q_WS_QWS

static void blend_transformed_bilinear_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    bool affine = !data->m13 && !data->m23;
    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = (x >> 16);
                    int x2 = (x1 + 1);
                    int y1 = (y >> 16);
                    int y2 = (y1 + 1);

                    int distx = ((x - (x1 << 16)) >> 8);
                    int disty = ((y - (y1 << 16)) >> 8);
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 %= image_width;
                    x2 %= image_width;
                    y1 %= image_height;
                    y2 %= image_height;

                    if (x1 < 0) x1 += image_width;
                    if (x2 < 0) x2 += image_width;
                    if (y1 < 0) y1 += image_height;
                    if (y2 < 0) y2 += image_height;

                    Q_ASSERT(x1 >= 0 && x1 < image_width);
                    Q_ASSERT(x2 >= 0 && x2 < image_width);
                    Q_ASSERT(y1 >= 0 && y1 < image_height);
                    Q_ASSERT(y2 >= 0 && y2 < image_height);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;
                    x += fdx;
                    y += fdy;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = x/w;
                    int x2 = (x1 + 1);
                    int y1 = y/w;
                    int y2 = (y1 + 1);

                    int distx = ((x -(x1*w)) >> 8) & 0xff;
                    int disty = ((y -(y1*w)) >> 8) & 0xff;
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 %= image_width;
                    x2 %= image_width;
                    y1 %= image_height;
                    y2 %= image_height;

                    if (x1 < 0) x1 += image_width;
                    if (x2 < 0) x2 += image_width;
                    if (y1 < 0) y1 += image_height;
                    if (y2 < 0) y2 += image_height;

                    Q_ASSERT(x1 >= 0 && x1 < image_width);
                    Q_ASSERT(x2 >= 0 && x2 < image_width);
                    Q_ASSERT(y1 >= 0 && y1 < image_height);
                    Q_ASSERT(y2 >= 0 && y2 < image_height);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

#ifdef Q_WS_QWS
static void blend_transformed_bilinear_tiled_argb_callback(int count,
                                                           const QSpan *spans,
                                                           void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    bool affine = !data->m13 && !data->m23;
    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = (x >> 16);
                    int x2 = (x1 + 1);
                    int y1 = (y >> 16);
                    int y2 = (y1 + 1);

                    int distx = ((x - (x1 << 16)) >> 8);
                    int disty = ((y - (y1 << 16)) >> 8);
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 %= image_width;
                    x2 %= image_width;
                    y1 %= image_height;
                    y2 %= image_height;

                    if (x1 < 0) x1 += image_width;
                    if (x2 < 0) x2 += image_width;
                    if (y1 < 0) y1 += image_height;
                    if (y2 < 0) y2 += image_height;

                    Q_ASSERT(x1 >= 0 && x1 < image_width);
                    Q_ASSERT(x2 >= 0 && x2 < image_width);
                    Q_ASSERT(y1 >= 0 && y1 < image_height);
                    Q_ASSERT(y2 >= 0 && y2 < image_height);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;
                    x += fdx;
                    y += fdy;
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale) - half_point;
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int x1 = x/w;
                    int x2 = (x1 + 1);
                    int y1 = y/w;
                    int y2 = (y1 + 1);

                    int distx = ((x -(x1*w)) >> 8) & 0xff;
                    int disty = ((y -(y1*w)) >> 8) & 0xff;
                    int idistx = 256 - distx;
                    int idisty = 256 - disty;

                    x1 %= image_width;
                    x2 %= image_width;
                    y1 %= image_height;
                    y2 %= image_height;

                    if (x1 < 0) x1 += image_width;
                    if (x2 < 0) x2 += image_width;
                    if (y1 < 0) y1 += image_height;
                    if (y2 < 0) y2 += image_height;

                    Q_ASSERT(x1 >= 0 && x1 < image_width);
                    Q_ASSERT(x2 >= 0 && x2 < image_width);
                    Q_ASSERT(y1 >= 0 && y1 < image_height);
                    Q_ASSERT(y2 >= 0 && y2 < image_height);

                    int y1_offset = y1 * image_width;
                    int y2_offset = y2 * image_width;

#if defined(Q_IRIX_GCC3_3_WORKAROUND)
                    uint tl = gccBug(image_bits[y1_offset + x1]);
                    uint tr = gccBug(image_bits[y1_offset + x2]);
                    uint bl = gccBug(image_bits[y2_offset + x1]);
                    uint br = gccBug(image_bits[y2_offset + x2]);
#else
                    uint tl = image_bits[y1_offset + x1];
                    uint tr = image_bits[y1_offset + x2];
                    uint bl = image_bits[y2_offset + x1];
                    uint br = image_bits[y2_offset + x2];
#endif

                    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
                    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
                    *b = INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
                    ++b;
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}
#endif // Q_WS_QWS

static void blend_transformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    bool affine = !data->m13 && !data->m23;

    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x >> 16;
                    int py = y >> 16;

                    bool out = (px < 0) || (px >= image_width)
                               || (py < 0) || (py >= image_height);

                    int y_offset = py * image_width;
                    *b = out ? uint(0) : image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x/w;
                    int py = y/w;

                    bool out = (px < 0) || (px >= image_width)
                               || (py < 0) || (py >= image_height);

                    int y_offset = py * image_width;
                    *b = out ? uint(0) : image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }

                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

#ifdef Q_WS_QWS
static void blend_transformed_argb_callback(int count, const QSpan *spans,
                                            void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);

    bool affine = !data->m13 && !data->m23;

    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x >> 16;
                    int py = y >> 16;

                    bool out = (px < 0) || (px >= image_width)
                               || (py < 0) || (py >= image_height);

                    int y_offset = py * image_width;
                    *b = out ? uint(0) : image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    ++b;
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x/w;
                    int py = y/w;

                    bool out = (px < 0) || (px >= image_width)
                               || (py < 0) || (py >= image_height);

                    int y_offset = py * image_width;
                    *b = out ? uint(0) : image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }

                    ++b;
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}
#endif // Q_WS_QWS

static void blend_transformed_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);
    bool affine = !data->m13 && !data->m23;

    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x >> 16;
                    int py = y >> 16;
                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * image_width;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x/w;
                    int py = y/w;
                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * image_width;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

#ifdef Q_WS_QWS
static void blend_transformed_tiled_argb_callback(int count,
                                                  const QSpan *spans,
                                                  void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    if (!func)
        return;
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    // The increment pr x in the scanline
    int fdx = (int)(data->m11 * fixed_scale);
    int fdy = (int)(data->m12 * fixed_scale);
    bool affine = !data->m13 && !data->m23;

    if (affine) {
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x >> 16;
                    int py = y >> 16;
                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * image_width;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    ++b;
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        int fdw = (int)(data->m13 * fixed_scale);
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;
            int x = int((data->m21 * (spans->y + 0.5)
                         + data->m11 * (spans->x + 0.5) + data->dx) * fixed_scale);
            int y = int((data->m22 * (spans->y + 0.5)
                         + data->m12 * (spans->x + 0.5) + data->dy) * fixed_scale);
            int w = int((data->m13 * (spans->x + 0.5)
                         + data->m23 * (spans->y + 0.5) + 1.) * fixed_scale);
            if (!w)
                w = 1;

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = x/w;
                    int py = y/w;
                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * image_width;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                    ++b;
                }
                data->rasterEngine->drawBufferSpan(buffer, buffer_size,
                                                   spans->x + spans->len - length,
                                                   spans->y, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}
#endif

/* Image formats here are target formats */
static const ProcessSpans processTextureSpans[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        blend_untransformed_generic,   // Mono
        blend_untransformed_generic,   // MonoLsb
        blend_untransformed_generic,   // Indexed8
        blend_untransformed_generic,   // RGB32
        blend_untransformed_generic,   // ARGB32
        blend_untransformed_argb,    // ARGB32_Premultiplied
        blend_untransformed_rgb16    // RGB16
    },
    // Tiled
    {
        0, // Invalid
        blend_tiled_generic,   // Mono
        blend_tiled_generic,   // MonoLsb
        blend_tiled_generic,   // Indexed8
        blend_tiled_generic,   // RGB32
        blend_tiled_generic,   // ARGB32
        blend_tiled_argb,    // ARGB32_Premultiplied
        blend_tiled_rgb16 // RGB16
    },
    // Transformed
    {
        0, // Invalid
        blend_texture_generic,   // Mono
        blend_texture_generic,   // MonoLsb
        blend_texture_generic,   // Indexed8
        blend_texture_generic,   // RGB32
        blend_texture_generic,   // ARGB32
        blend_transformed_argb, // ARGB32_Premultiplied
        blend_src_generic // RGB16
    },
     // TransformedTiled
    {
        0,
        blend_texture_generic,   // Mono
        blend_texture_generic,   // MonoLsb
        blend_texture_generic,   // Indexed8
        blend_texture_generic,   // RGB32
        blend_texture_generic,   // ARGB32
        blend_transformed_tiled_argb, // ARGB32_Premultiplied
        blend_src_generic // RGB16
    },
    // Bilinear
    {
        0,
        blend_texture_generic,   // Mono
        blend_texture_generic,   // MonoLsb
        blend_texture_generic,   // Indexed8
        blend_texture_generic,   // RGB32
        blend_texture_generic,   // ARGB32
        blend_transformed_bilinear_argb, // ARGB32_Premultiplied
        blend_src_generic // RGB16
    },
    // BilinearTiled
    {
        0,
        blend_texture_generic,   // Mono
        blend_texture_generic,   // MonoLsb
        blend_texture_generic,   // Indexed8
        blend_texture_generic,   // RGB32
        blend_texture_generic,   // ARGB32
        blend_transformed_bilinear_tiled_argb, // ARGB32_Premultiplied
        blend_src_generic // RGB16
    }
};

#ifdef Q_WS_QWS
static const ProcessSpans processTextureSpansCallback[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        blend_untransformed_generic_callback,   // Mono
        blend_untransformed_generic_callback,   // MonoLsb
        blend_untransformed_generic_callback,   // Indexed8
        blend_untransformed_generic_callback,   // RGB32
        blend_untransformed_generic_callback,   // ARGB32
        blend_untransformed_argb_callback, // ARGB32_Premultiplied
        blend_untransformed_generic_callback // RGB16
    },
    // Tiled
    {
        0, // Invalid
        blend_tiled_generic_callback,   // Mono
        blend_tiled_generic_callback,   // MonoLsb
        blend_tiled_generic_callback,   // Indexed8
        blend_tiled_generic_callback,   // RGB32
        blend_tiled_generic_callback,   // ARGB32
        blend_tiled_argb_callback, // ARGB32_Premultiplied
        blend_tiled_generic_callback, // RGB16
    },
    // Transformed
    {
        0, // Invalid
        blend_texture_generic_callback,   // Mono
        blend_texture_generic_callback,   // MonoLsb
        blend_texture_generic_callback,   // Indexed8
        blend_texture_generic_callback,   // RGB32
        blend_texture_generic_callback,   // ARGB32
        blend_transformed_argb_callback, // ARGB32_Premultiplied
        blend_src_generic_callback // RGB16
    },
     // TransformedTiled
    {
        0,
        blend_texture_generic_callback,   // Mono
        blend_texture_generic_callback,   // MonoLsb
        blend_texture_generic_callback,   // Indexed8
        blend_texture_generic_callback,   // RGB32
        blend_texture_generic_callback,   // ARGB32
        blend_transformed_tiled_argb_callback, // ARGB32_Premultiplied
        blend_src_generic_callback // RGB16
    },
    // Bilinear
    {
        0,
        blend_texture_generic_callback,   // Mono
        blend_texture_generic_callback,   // MonoLsb
        blend_texture_generic_callback,   // Indexed8
        blend_texture_generic_callback,   // RGB32
        blend_texture_generic_callback,   // ARGB32
        blend_transformed_bilinear_argb_callback, // ARGB32_Premultiplied
        blend_src_generic_callback // RGB16
    },
    // BilinearTiled
    {
        0,
        blend_texture_generic_callback,   // Mono
        blend_texture_generic_callback,   // MonoLsb
        blend_texture_generic_callback,   // Indexed8
        blend_texture_generic_callback,   // RGB32
        blend_texture_generic_callback,   // ARGB32
        blend_transformed_bilinear_tiled_argb_callback, // ARGB32_Premultiplied
        blend_src_generic_callback // RGB16
    }
};
#endif // Q_WS_QWS

void qBlendTexture(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    ProcessSpans proc = processTextureSpans[getBlendType(data)][data->rasterBuffer->format];
    proc(count, spans, userData);
}

#ifdef Q_WS_QWS
void qBlendTextureCallback(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    ProcessSpans proc = processTextureSpansCallback[getBlendType(data)][data->rasterBuffer->format];
    proc(count, spans, userData);
}
#endif

template <class DST>
inline void qt_bitmapblit_template(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride,
                                   DST dummy = 0)
{
    Q_UNUSED(dummy);
    const DST c = qt_colorConvert<DST, quint32>(color, 0);
    DST *dest = reinterpret_cast<DST*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(DST);

    if (mapWidth > 8) {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (int x = 0; x < mapWidth; x += 8) {
                uchar s = map[x >> 3];
                for (int i = 0; i < 8; ++i) {
                    if (s & 0x80) {
                        ++n;
                    } else {
                        if (n) {
                            qt_memfill(dest + x0, c, n);
                            x0 += n + 1;
                            n = 0;
                        } else {
                            ++x0;
                        }
                        if (!s) {
                            x0 += 8 - 1 - i;
                            break;
                        }
                    }
                    s <<= 1;
                }
            }
            if (n)
                qt_memfill(dest + x0, c, n);
            dest += destStride;
            map += mapStride;
        }
    } else {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (uchar s = *map; s; s <<= 1) {
                if (s & 0x80) {
                    ++n;
                } else if (n) {
                    qt_memfill(dest + x0, c, n);
                    x0 += n + 1;
                    n = 0;
                } else {
                    ++x0;
                }
            }
            if (n)
                qt_memfill(dest + x0, c, n);
            dest += destStride;
            map += mapStride;
        }
    }
}

inline static void qt_bitmapblit_quint32(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x,  y,  color,
                                    map, mapWidth, mapHeight, mapStride);
}

inline static void qt_bitmapblit_quint16(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint16>(rasterBuffer, x,  y,  color,
                                    map, mapWidth, mapHeight, mapStride);
}


static void qt_alphamapblit_quint32(QRasterBuffer *rasterBuffer,
                                    int x, int y, quint32 color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride)
{
    const quint32 c = color;
    quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint32);

    while (mapHeight--) {
        for (int i = 0; i < mapWidth; ++i) {
            const int coverage = map[i];

            if (coverage == 0) {
                // nothing
            } else if (coverage == 255) {
                dest[i] = c;
            } else {
                int ialpha = 255 - coverage;
                dest[i] = BYTE_MUL(c, coverage) + BYTE_MUL(dest[i], ialpha);
            }
        }
        dest += destStride;
        map += mapStride;
    }
}

static void qt_alphamapblit_quint16(QRasterBuffer *rasterBuffer,
                                    int x, int y, quint32 color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride)
{
    const quint16 c = qt_colorConvert<quint16, quint32>(color, 0);
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint16);

    while (mapHeight--) {
        for (int i = 0; i < mapWidth; ++i) {
            const int coverage = map[i];

            if (coverage == 0) {
                // nothing
            } else if (coverage == 255) {
                dest[i] = c;
            } else {
                int ialpha = 255 - coverage;
                dest[i] = BYTE_MUL_RGB16(c, coverage)
                          + BYTE_MUL_RGB16(dest[i], ialpha);
            }
        }
        dest += destStride;
        map += mapStride;
    }
}

template <class T>
inline void qt_rectfill_template(QRasterBuffer *rasterBuffer,
                                 int x, int y, int width, int height,
                                 quint32 color, T dummy = 0)
{
    Q_UNUSED(dummy);
    qt_rectfill<T>(reinterpret_cast<T*>(rasterBuffer->buffer()),
                   qt_colorConvert<T, quint32>(color, 0),
                   x, y, width, height, rasterBuffer->bytesPerLine());
}

inline static void qt_rectfill_quint32(QRasterBuffer *rasterBuffer,
                                 int x, int y, int width, int height,
                                 quint32 color)
{
    qt_rectfill_template<quint32>(rasterBuffer, x, y, width, height, color);
}

inline static void qt_rectfill_quint16(QRasterBuffer *rasterBuffer,
                                 int x, int y, int width, int height,
                                 quint32 color)
{
    qt_rectfill_template<quint16>(rasterBuffer, x, y, width, height, color);
}


DrawHelper qDrawHelper[QImage::NImageFormats] =
{
    // Format_Invalid,
    { 0, 0, 0, 0, 0 },
    // Format_Mono,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0
    },
    // Format_MonoLSB,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0
    },
    // Format_Indexed8,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0
    },
    // Format_RGB32,
    {
        blend_color_argb,
        blend_src_argb,
        qt_bitmapblit_quint32,
        qt_alphamapblit_quint32,
        qt_rectfill_quint32
    },
    // Format_ARGB32,
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_quint32,
        qt_alphamapblit_quint32,
        qt_rectfill_quint32
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_argb,
        blend_src_argb,
        qt_bitmapblit_quint32,
        qt_alphamapblit_quint32,
        qt_rectfill_quint32
    },
    // Format_RGB16
    {
        blend_color_rgb16,
        blend_src_generic,
        qt_bitmapblit_quint16,
        qt_alphamapblit_quint16,
        qt_rectfill_quint16
    }
};

#ifdef Q_WS_QWS
DrawHelper qDrawHelperCallback[QImage::NImageFormats] =
{
    // Format_Invalid,
    { 0, 0, 0, 0, 0 },
    // Format_Mono,
    {
        blend_color_generic_callback,
        blend_src_generic_callback,
        0, 0, 0
    },
    // Format_MonoLSB,
    {
        blend_color_generic_callback,
        blend_src_generic_callback,
        0, 0, 0
    },
    // Format_Indexed8,
    {
        blend_color_generic_callback,
        blend_src_generic_callback,
        0, 0, 0
    },
    // Format_RGB32,
    {
        blend_color_generic_callback,
        blend_src_generic_callback,
        0, 0, 0
    },
    // Format_ARGB32,
    {
        blend_color_generic_callback,
        blend_src_generic_callback,
        0, 0, 0
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_argb_callback,
        blend_src_argb_callback,
        0, 0, 0
    },
    // Format_RGB16
    {
        blend_color_generic_callback,
        blend_src_generic_callback,
        0, 0, 0
    }
};
#endif // Q_WS_QWS

template <class DST, class SRC>
inline void qt_memfill_template(DST *dest, SRC color, int count)
{
    const DST c = qt_colorConvert<DST, SRC>(color, 0);
    int n = (count + 7) / 8;
    switch (count & 0x07)
    {
    case 0: do { *dest++ = c;
    case 7:      *dest++ = c;
    case 6:      *dest++ = c;
    case 5:      *dest++ = c;
    case 4:      *dest++ = c;
    case 3:      *dest++ = c;
    case 2:      *dest++ = c;
    case 1:      *dest++ = c;
    } while (--n > 0);
    }
}

template <>
inline void qt_memfill_template(quint16 *dest, quint16 value, int count)
{
    if (count < 3) {
        switch (count) {
        case 2: *dest++ = value;
        case 1: *dest = value;
        }
        return;
    }

    const int align = (long)(dest) & 0x3;
    switch (align) {
    case 2: *dest++ = value; --count;
    }

    const quint32 value32 = (value << 16) | value;
    qt_memfill(reinterpret_cast<quint32*>(dest), value32, count / 2);
    if (count & 0x1)
        dest[count - 1] = value;
}

static void qt_memfill_quint16(quint16 *dest, quint16 color, int count)
{
    qt_memfill_template<quint16, quint16>(dest, color, count);
}

typedef void (*qt_memfill32_func)(quint32 *dest, quint32 value, int count);
typedef void (*qt_memfill16_func)(quint16 *dest, quint16 value, int count);
static void qt_memfill32_setup(quint32 *dest, quint32 value, int count);
static void qt_memfill16_setup(quint16 *dest, quint16 value, int count);

qt_memfill32_func qt_memfill32 = qt_memfill32_setup;
qt_memfill16_func qt_memfill16 = qt_memfill16_setup;

enum CPUFeatures {
    None        = 0,
    MMX         = 0x1,
    MMXEXT      = 0x2,
    MMX3DNOW    = 0x4,
    MMX3DNOWEXT = 0x8,
    SSE         = 0x10,
    SSE2        = 0x20,
    CMOV        = 0x40,
    IWMMXT      = 0x80
};

static uint detectCPUFeatures() {
#if defined(__x86_64__) || defined(Q_OS_WIN64)
    return MMX|SSE|SSE2|CMOV;
#elif defined(__ia64__)
    return MMX|SSE|SSE2;
#elif defined(QT_HAVE_IWMMXT)
    // runtime detection only available when running as a previlegied process
    static const bool doIWMMXT = !qgetenv("QT_NO_IWMMXT").toInt();
    return doIWMMXT ? IWMMXT : 0;
#elif defined(__i386__) || defined(_M_IX86)
    unsigned int extended_result = 0;
    uint result = 0;
    /* see p. 118 of amd64 instruction set manual Vol3 */
#if defined(Q_CC_GNU)
    asm ("push %%ebx\n"
         "pushf\n"
         "pop %%eax\n"
         "mov %%eax, %%ebx\n"
         "xor $0x00200000, %%eax\n"
         "push %%eax\n"
         "popf\n"
         "pushf\n"
         "pop %%eax\n"
         "xor %%edx, %%edx\n"
         "xor %%ebx, %%eax\n"
         "jz 1f\n"

         "mov $0x00000001, %%eax\n"
         "cpuid\n"
         "1:\n"
         "pop %%ebx\n"
         "mov %%edx, %0\n"
        : "=r" (result)
        :
        : "%eax", "%ecx", "%edx"
        );

    asm ("push %%ebx\n"
         "pushf\n"
         "pop %%eax\n"
         "mov %%eax, %%ebx\n"
         "xor $0x00200000, %%eax\n"
         "push %%eax\n"
         "popf\n"
         "pushf\n"
         "pop %%eax\n"
         "xor %%edx, %%edx\n"
         "xor %%ebx, %%eax\n"
         "jz 2f\n"

         "mov $0x80000000, %%eax\n"
         "cpuid\n"
	 "cmp $0x80000000, %%eax\n"
	 "jbe 2f\n"
	 "mov $0x80000001, %%eax\n"
	 "cpuid\n"
         "2:\n"
         "pop %%ebx\n"
         "mov %%edx, %0\n"
        : "=r" (extended_result)
        :
        : "%eax", "%ecx", "%edx"
        );
#elif defined (Q_OS_WIN)
    _asm {
	push eax
	push ebx
	push ecx
	push edx
	pushfd
	pop eax
	mov ebx, eax
	xor eax, 00200000h
	push eax
	popfd
	pushfd
        pop eax
	mov edx, 0
	xor eax, ebx
	jz skip

	mov eax, 1
	cpuid
	mov result, edx
    skip:
        pop edx
	pop ecx
	pop ebx
	pop eax
    }

    _asm {
	push eax
	push ebx
	push ecx
	push edx
	pushfd
	pop eax
	mov ebx, eax
	xor eax, 00200000h
	push eax
	popfd
	pushfd
        pop eax
	mov edx, 0
	xor eax, ebx
	jz skip2

	mov eax, 80000000h
	cpuid
        cmp eax, 80000000h
        jbe skip2
        mov eax, 80000001h
        cpuid
	mov extended_result, edx
    skip2:
        pop edx
	pop ecx
	pop ebx
	pop eax
    }
#endif

    static const bool doMMX = !qgetenv("QT_NO_MMX").toInt();
    static const bool doMMXEXT = !qgetenv("QT_NO_MMXEXT").toInt();
    static const bool do3DNOW = !qgetenv("QT_NO_3DNOW").toInt();
    static const bool do3DNOWEXT = !qgetenv("QT_NO_3DNOWEXT").toInt();
    static const bool doSSE = !qgetenv("QT_NO_SSE").toInt();
    static const bool doSSE2 = !qgetenv("QT_NO_SSE2").toInt();

    uint features = 0;
    // result now contains the standard feature bits
    if (result & (1 << 15))
        features |= CMOV;
    if (doMMX && (result & (1 << 23)))
        features |= MMX;
    if (doMMXEXT && (extended_result & (1 << 22)))
        features |= MMXEXT;
    if (do3DNOW && (extended_result & (1 << 31)))
        features |= MMX3DNOW;
    if (do3DNOWEXT && (extended_result & (1 << 30)))
        features |= MMX3DNOWEXT;
    if (doSSE && (result & (1 << 25)))
        features |= SSE;
    if (doSSE2 && (result & (1 << 26)))
        features |= SSE2;
    return features;
#else
    return 0;
#endif
}

void qInitDrawhelperAsm()
{
    static uint features = 0xffffffff;
    if (features != 0xffffffff)
        return;
    features = detectCPUFeatures();

    qt_memfill32 = qt_memfill_template<quint32, quint32>;
    qt_memfill16 = qt_memfill_quint16; //qt_memfill_template<quint16, quint16>;

#ifdef QT_NO_DEBUG
    if (false) {
#ifdef QT_HAVE_SSE2
    } else if (features & SSE2) {
        qt_memfill32 = qt_memfill32_sse2;
        qt_memfill16 = qt_memfill16_sse2;
        qDrawHelper[QImage::Format_RGB32].bitmapBlit = qt_bitmapblit32_sse2;
        qDrawHelper[QImage::Format_ARGB32].bitmapBlit = qt_bitmapblit32_sse2;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].bitmapBlit = qt_bitmapblit32_sse2;
        qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse2;
#endif
#ifdef QT_HAVE_SSE
    } else if (features & SSE) {
//        qt_memfill32 = qt_memfill32_sse;
        qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse;
#ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            qt_memfill32 = qt_memfill32_sse3dnow;
            qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse3dnow;
        }
#endif
#endif // SSE
#if defined(QT_HAVE_MMXEXT) && defined(QT_HAVE_SSE)
    } else if (features & MMXEXT) {
        qt_memfill32 = qt_memfill32_sse;
        qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse;
# ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            qt_memfill32 = qt_memfill32_sse3dnow;
            qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse3dnow;
        }
# endif // 3DNOW
#endif // MMXEXT
    }
#ifdef QT_HAVE_MMX
    if (features & MMX) {
        functionForMode = qt_functionForMode_MMX;
        functionForModeSolid = qt_functionForModeSolid_MMX;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_mmx;
#ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            functionForMode = qt_functionForMode_MMX3DNOW;
            functionForModeSolid = qt_functionForModeSolid_MMX3DNOW;
            qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_mmx3dnow;
        }
#endif // 3DNOW
    }
#endif // MMX

#ifdef QT_HAVE_SSE
    if (features & SSE) {
        functionForMode = qt_functionForMode_SSE;
        functionForModeSolid = qt_functionForModeSolid_SSE;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_sse;
#ifdef QT_HAVE_3DNOW
        if (features & MMX3DNOW) {
            functionForMode = qt_functionForMode_SSE3DNOW;
            functionForModeSolid = qt_functionForModeSolid_SSE3DNOW;
            qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_sse3dnow;
        }
#endif // 3DNOW
    }
#endif // SSE

#ifdef QT_HAVE_IWMMXT
    if (features & IWMMXT) {
        functionForMode = qt_functionForMode_IWMMXT;
        functionForModeSolid = qt_functionForModeSolid_IWMMXT;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_iwmmxt;
    }
#endif // IWMMXT

#endif // QT_NO_DEBUG
}

static void qt_memfill32_setup(quint32 *dest, quint32 value, int count)
{
    qInitDrawhelperAsm();
    qt_memfill32(dest, value, count);
}

static void qt_memfill16_setup(quint16 *dest, quint16 value, int count)
{
    qInitDrawhelperAsm();
    qt_memfill16(dest, value, count);
}

#if QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
static const int tileSize = 32;
#endif

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#if QT_ROTATION_ALGORITHM == QT_ROTATION_PACKED || QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
#error Big endian version not implemented for the transformed driver!
#endif
#endif

#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD || defined(QT_QWS_DEPTH_18) || defined(QT_QWS_DEPTH_24)

template <class DST, class SRC>
static inline void qt_memrotate90_cachedRead(const SRC *src, int w, int h,
                                             int sstride,
                                             DST *dest, int dstride)
{
    for (int y = 0; y < h; ++y) {
        for (int x = w - 1; x >= 0; --x) {
            dest[(w - x - 1) * dstride + y] = qt_colorConvert<DST,SRC>(src[x], 0);
        }
        src += sstride;
    }
}

template <class DST, class SRC>
static inline void qt_memrotate270_cachedRead(const SRC *src, int w, int h,
                                              int sstride,
                                              DST *dest, int dstride)
{
    src += (h - 1) * sstride;
    for (int y = h - 1; y >= 0; --y) {
        for (int x = 0; x < w; ++x) {
            dest[x * dstride + h - y - 1] = qt_colorConvert<DST,SRC>(src[x], 0);
        }
        src -= sstride;
    }
}

#endif // QT_ROTATION_CACHEDREAD

#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE

template <class DST, class SRC>
static inline void qt_memrotate90_cachedWrite(const SRC *src, int w, int h,
                                              int sstride,
                                              DST *dest, int dstride)
{
    for (int x = w - 1; x >= 0; --x) {
        DST *d = dest + (w - x - 1) * dstride;
        for (int y = 0; y < h; ++y) {
            *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
        }
    }

}

template <class DST, class SRC>
static inline void qt_memrotate270_cachedWrite(const SRC *src, int w, int h,
                                               int sstride,
                                               DST *dest, int dstride)
{
    for (int x = 0; x < w; ++x) {
        DST *d = dest + x * dstride;
        for (int y = h - 1; y >= 0; --y) {
            *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
        }
    }
}

#endif // QT_ROTATION_CACHEDWRITE

#if QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING

// TODO: packing algorithms should probably be modified on 64-bit architectures

template <class DST, class SRC>
static inline void qt_memrotate90_packing(const SRC *src, int w, int h,
                                          int sstride,
                                          DST *dest, int dstride)
{
    const int pack = sizeof(quint32) / sizeof(DST);
    const int unaligned = int((long(dest) & (sizeof(quint32)-1))) / sizeof(DST);

    for (int x = w - 1; x >= 0; --x) {
        int y = 0;

        for (int i = 0; i < unaligned; ++i) {
            dest[(w - x - 1) * dstride + y]
                = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            ++y;
        }

        quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride
                                                + unaligned);
        const int rest = (h - unaligned) % pack;
        while (y < h - rest) {
            quint32 c = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            for (int i = 1; i < pack; ++i) {
                c |= qt_colorConvert<DST,SRC>(src[(y + i) * sstride + x], 0)
                     << (sizeof(int) * 8 / pack * i);
            }
            *d++ = c;
            y += pack;
        }

        while (y < h) {
            dest[(w - x - 1) * dstride + y]
                = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            ++y;
        }
    }
}

template <class DST, class SRC>
static inline void qt_memrotate270_packing(const SRC *src, int w, int h,
                                           int sstride,
                                           DST *dest, int dstride)
{
    const int pack = sizeof(quint32) / sizeof(DST);
    const int unaligned = int((long(dest) & (sizeof(quint32)-1))) / sizeof(DST);

    for (int x = 0; x < w; ++x) {
        int y = h - 1;

        for (int i = 0; i < unaligned; ++i) {
            dest[x * dstride + h - y - 1]
                = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            --y;
        }

        quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                                                + unaligned);
        const int rest = (h - unaligned) % pack;
        while (y > rest) {
            quint32 c = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            for (int i = 1; i < pack; ++i) {
                c |= qt_colorConvert<DST,SRC>(src[(y - i) * sstride + x], 0)
                     << (sizeof(int) * 8 / pack * i);
            }
            *d++ = c;
            y -= pack;
        }
        while (y >= 0) {
            dest[x * dstride + h - y - 1]
                = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            --y;
        }
    }
}

#endif // QT_ROTATION_PACKING

#if QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
template <class DST, class SRC>
static inline void qt_memrotate90_tiled(const SRC *src, int w, int h,
                                        int sstride,
                                        DST *dest, int dstride)
{
    const int pack = sizeof(quint32) / sizeof(DST);
    const int unaligned =
        qMin(uint((long(dest) & (sizeof(quint32)-1)) / sizeof(DST)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        if (unaligned) {
            for (int x = startx; x >= stopx; --x) {
                DST *d = dest + (w - x - 1) * dstride;
                for (int y = 0; y < unaligned; ++y) {
                    *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize + unaligned;
            const int stopy = qMin(starty + tileSize, h - unoptimizedY);

            for (int x = startx; x >= stopx; --x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride + starty);
                for (int y = starty; y < stopy; y += pack) {
                    quint32 c = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(int) * 8 / pack * i);
                        const DST color = qt_colorConvert<DST,SRC>(src[(y + i) * sstride + x], 0);
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }

        if (unoptimizedY) {
            const int starty = h - unoptimizedY;
            for (int x = startx; x >= stopx; --x) {
                DST *d = dest + (w - x - 1) * dstride + starty;
                for (int y = starty; y < h; ++y) {
                    *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
                }
            }
        }
    }
}

template <class DST, class SRC>
static inline void qt_memrotate90_tiled_unpacked(const SRC *src, int w, int h,
                                                 int sstride,
                                                 DST *dest, int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize;
            const int stopy = qMin(starty + tileSize, h);

            for (int x = startx; x >= stopx; --x) {
                DST *d = dest + (w - x - 1) * dstride + starty;
                for (int y = starty; y < stopy; ++y)
                    *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            }
        }
    }
}

template <class DST, class SRC>
static inline void qt_memrotate270_tiled(const SRC *src, int w, int h,
                                         int sstride,
                                         DST *dest, int dstride)
{
    const int pack = sizeof(quint32) / sizeof(DST);
    const int unaligned =
        qMin(uint((long(dest) & (sizeof(quint32)-1)) / sizeof(DST)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        if (unaligned) {
            for (int x = startx; x < stopx; ++x) {
                DST *d = dest + x * dstride;
                for (int y = h - 1; y >= h - unaligned; --y) {
                    *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - unaligned - ty * tileSize;
            const int stopy = qMax(starty - tileSize, unoptimizedY);

            for (int x = startx; x < stopx; ++x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                                                        + h - 1 - starty);
                for (int y = starty; y > stopy; y -= pack) {
                    quint32 c = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(int) * 8 / pack * i);
                        const DST color = qt_colorConvert<DST,SRC>(src[(y - i) * sstride + x], 0);
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }
        if (unoptimizedY) {
            const int starty = unoptimizedY - 1;
            for (int x = startx; x < stopx; ++x) {
                DST *d = dest + x * dstride + h - 1 - starty;
                for (int y = starty; y >= 0; --y) {
                    *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
                }
            }
        }
    }
}

template <class DST, class SRC>
static inline void qt_memrotate270_tiled_unpacked(const SRC *src, int w, int h,
                                                  int sstride,
                                                  DST *dest, int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - ty * tileSize;
            const int stopy = qMax(starty - tileSize, 0);

            for (int x = startx; x < stopx; ++x) {
                DST *d = dest + x * dstride + h - 1 - starty;
                for (int y = starty; y >= stopy; --y)
                    *d++ = qt_colorConvert<DST,SRC>(src[y * sstride + x], 0);
            }
        }
    }
}

#endif // QT_ROTATION_ALFORITHM

template <class DST, class SRC>
static inline void qt_memrotate90_template(const SRC *src,
                                           int srcWidth, int srcHeight, int srcStride,
                                           DST *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate90_cachedRead<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                       dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate90_cachedWrite<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                        dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    qt_memrotate90_packing<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                    dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    qt_memrotate90_tiled<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                  dest, dstStride);
#endif
}

template <class DST, class SRC>
static inline void qt_memrotate180_template(const SRC *src,
                                            int w, int h, int sstride,
                                            DST *dest, int dstride)
{
    src += (h - 1) * sstride;
    for (int y = h - 1; y >= 0; --y) {
        for (int x = w - 1; x >= 0; --x) {
            dest[(h - y - 1) * dstride + w - x - 1] = qt_colorConvert<DST,SRC>(src[x], 0);
        }
        src -= sstride;
    }
}

template <class DST, class SRC>
static inline void qt_memrotate270_template(const SRC *src,
                                            int srcWidth, int srcHeight, int srcStride,
                                            DST *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate270_cachedRead<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                        dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate270_cachedWrite<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                         dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    qt_memrotate270_packing<DST,SRC>(src, srcWidth, srcHeight, srcStride,
                                     dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    qt_memrotate270_tiled_unpacked<DST,SRC>(src, srcWidth, srcHeight,
                                            srcStride,
                                            dest, dstStride);
#endif
}

#ifdef QT_QWS_DEPTH_24
template <>
static inline void qt_memrotate90_template<quint24, quint32>(const quint32 *src,
                                                             int srcWidth, int srcHeight, int srcStride,
                                                             quint24 *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate90_cachedRead<quint24,quint32>(src, srcWidth, srcHeight,
                                               srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate90_cachedWrite<quint24,quint32>(src, srcWidth, srcHeight,
                                                srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    // packed algorithm not implemented
    qt_memrotate90_cachedRead<quint24,quint32>(src, srcWidth, srcHeight,
                                               srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    // packed algorithm not implemented
    qt_memrotate90_tiled_unpacked<quint24,quint32>(src, srcWidth, srcHeight,
                                                   srcStride, dest, dstStride);
#endif
}
#endif // QT_QWS_DEPTH_24

#ifdef QT_QWS_DEPTH_18
template <>
static inline void qt_memrotate90_template<quint18, quint32>(const quint32 *src,
                                                             int srcWidth, int srcHeight, int srcStride,
                                                             quint18 *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate90_cachedRead<quint18,quint32>(src, srcWidth, srcHeight,
                                               srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate90_cachedWrite<quint18,quint32>(src, srcWidth, srcHeight,
                                                srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    // packed algorithm not implemented
    qt_memrotate90_cachedRead<quint18,quint32>(src, srcWidth, srcHeight,
                                               srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    // packed algorithm not implemented
    qt_memrotate90_tiled_unpacked<quint18,quint32>(src, srcWidth, srcHeight,
                                                   srcStride, dest, dstStride);
#endif
}
#endif // QT_QWS_DEPTH_24

#define QT_IMPL_MEMROTATE(srctype, desttype)                        \
void qt_memrotate90(const srctype *src, int w, int h, int sstride,  \
                    desttype *dest, int dstride)                    \
{                                                                   \
    qt_memrotate90_template(src, w, h, sstride, dest, dstride);     \
}                                                                   \
void qt_memrotate180(const srctype *src, int w, int h, int sstride, \
                     desttype *dest, int dstride)                   \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
void qt_memrotate270(const srctype *src, int w, int h, int sstride, \
                     desttype *dest, int dstride)                   \
{                                                                   \
    qt_memrotate270_template(src, w, h, sstride, dest, dstride);    \
}

QT_IMPL_MEMROTATE(quint32, quint32)
QT_IMPL_MEMROTATE(quint32, quint16)
QT_IMPL_MEMROTATE(quint16, quint32)
QT_IMPL_MEMROTATE(quint16, quint16)
#ifdef QT_QWS_DEPTH_24
QT_IMPL_MEMROTATE(quint32, quint24)
#endif
#ifdef QT_QWS_DEPTH_18
QT_IMPL_MEMROTATE(quint32, quint18)
#endif
QT_IMPL_MEMROTATE(quint32, quint8)
QT_IMPL_MEMROTATE(quint16, quint8)
QT_IMPL_MEMROTATE(quint8, quint8)

#ifdef QT_QWS_DEPTH_GENERIC
QT_IMPL_MEMROTATE(quint32, qrgb_generic16)
QT_IMPL_MEMROTATE(quint16, qrgb_generic16)
#endif

#ifdef QT_QWS_DEPTH_GENERIC

int qrgb::bpp = 0;
int qrgb::len_red = 0;
int qrgb::len_green = 0;
int qrgb::len_blue = 0;
int qrgb::len_alpha = 0;
int qrgb::off_red = 0;
int qrgb::off_green = 0;
int qrgb::off_blue = 0;
int qrgb::off_alpha = 0;

template <typename SRC>
static inline void qt_rectconvert_rgb(qrgb *dest, const SRC *src,
                                      int x, int y, int width, int height,
                                      int dstStride, int srcStride)
{
    quint8 *dest8 = reinterpret_cast<quint8*>(dest)
                    + y * dstStride + x * qrgb::bpp;

    srcStride = srcStride / sizeof(SRC) - width;
    dstStride -= (width * qrgb::bpp);

    for (int j = 0;  j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const quint32 v = qt_convertToRgb<SRC>(*src++);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            for (int j = qrgb::bpp - 1; j >= 0; --j)
                *dest8++ = (v >> (8 * j)) & 0xff;
#else
            for (int j = 0; j < qrgb::bpp; ++j)
                *dest8++ = (v >> (8 * j)) & 0xff;
#endif
        }

        dest8 += dstStride;
        src += srcStride;
    }
}

template <>
void qt_rectconvert(qrgb *dest, const quint32 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_rgb<quint32>(dest, src, x, y, width, height,
                                dstStride, srcStride);
}

template <>
void qt_rectconvert(qrgb *dest, const quint16 *src,
                    int x, int y, int width, int height,
                    int dstStride, int srcStride)
{
    qt_rectconvert_rgb<quint16>(dest, src, x, y, width, height,
                                dstStride, srcStride);
}

#endif // QT_QWS_DEPTH_GENERIC
