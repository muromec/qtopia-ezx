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

#include <qimage.h>
#include <qpainter.h>

#include "omegacamera.h"

#define	VIDEO_DEVICE	    "/dev/video0"


namespace camera
{

bool OmegaCamera::hasCamera() const
{
    return ( fd != -1 );
}

QSize OmegaCamera::captureSize() const
{
    return QSize( width, height );
}

uint OmegaCamera::refocusDelay() const
{
    return 250;
}

int OmegaCamera::minimumFramePeriod() const
{
    return (int)(1000/20); // milliseconds
}

OmegaCamera::OmegaCamera():
    m_imageBuf(0)
{
    setupCamera( QSize( 0, 0 ) );
}

OmegaCamera::~OmegaCamera()
{
    shutdown();
}

void OmegaCamera::setupCamera( QSize size )
{
    // Clear important variables.
    frames = 0;
    currentFrame = 0;
    caps.minwidth = 144;
    caps.minheight = 176;
    caps.maxwidth = 1024;
    caps.maxheight = 1280;

    // Open the video device.
    fd = open( VIDEO_DEVICE, O_RDWR );
    if ( fd == -1 ) {
        qWarning( "%s: %s", VIDEO_DEVICE, strerror( errno ) );
        return;
    }

    // Set palette mode
    struct { int r1; int r2; } _pm = { 15, 15 };
    ioctl(fd, 215, &_pm);

    // Determine the capture size to use.  Zero indicates "preview mode".
    if ( size.width() == 0 ) {
        size = QSize(144, 176);
    }

    width = size.height();
    height = size.width();

    // Set the new capture window.
    struct video_window wind;
    memset(&wind, 0, sizeof(wind));

    wind.width = width;
    wind.height = height;

    if ( ioctl( fd, VIDIOCSWIN, &wind ) < 0 ) {
        qWarning("%s: could not set the capture window", VIDEO_DEVICE);
    }

    m_imageBuf = (quint16*) malloc(width * height * 2);

    // Enable mmap-based access to the camera.
    memset( &mbuf, 0, sizeof( mbuf ) );
    if ( ioctl( fd, VIDIOCGMBUF, &mbuf ) < 0 ) {
        qWarning( "%s: mmap-based camera access is not available", VIDEO_DEVICE );
        close( fd );
        fd = -1;
        return;
    }

    // Mmap the designated memory region.
    frames = (unsigned char *)mmap( 0, mbuf.size, PROT_READ | PROT_WRITE,
				    MAP_SHARED, fd, 0 );
    if ( !frames || frames == (unsigned char *)(long)(-1) ) {
        qWarning( "%s: could not mmap the device", VIDEO_DEVICE );
        close( fd );
        fd = -1;
        return;
    }

    // Start capturing of the first frame.
    ioctl(fd, VIDIOCCAPTURE, 0);
}

void OmegaCamera::shutdown()
{
    if ( frames != 0 ) {
        munmap( frames, mbuf.size );
        frames = 0;
    }

    if ( fd != -1 ) {
        ioctl(fd, VIDIOCCAPTURE, -1);
        close( fd );
        fd = -1;
    }

    if (m_imageBuf)
    {
        free(m_imageBuf);
        m_imageBuf = 0;
    }
}


static const signed short redAdjust[] = {
-161,-160,-159,-158,-157,-156,-155,-153,
-152,-151,-150,-149,-148,-147,-145,-144,
-143,-142,-141,-140,-139,-137,-136,-135,
-134,-133,-132,-131,-129,-128,-127,-126,
-125,-124,-123,-122,-120,-119,-118,-117,
-116,-115,-114,-112,-111,-110,-109,-108,
-107,-106,-104,-103,-102,-101,-100, -99,
 -98, -96, -95, -94, -93, -92, -91, -90,
 -88, -87, -86, -85, -84, -83, -82, -80,
 -79, -78, -77, -76, -75, -74, -72, -71,
 -70, -69, -68, -67, -66, -65, -63, -62,
 -61, -60, -59, -58, -57, -55, -54, -53,
 -52, -51, -50, -49, -47, -46, -45, -44,
 -43, -42, -41, -39, -38, -37, -36, -35,
 -34, -33, -31, -30, -29, -28, -27, -26,
 -25, -23, -22, -21, -20, -19, -18, -17,
 -16, -14, -13, -12, -11, -10,  -9,  -8,
  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,
   2,   3,   4,   5,   6,   7,   9,  10,
  11,  12,  13,  14,  15,  17,  18,  19,
  20,  21,  22,  23,  25,  26,  27,  28,
  29,  30,  31,  33,  34,  35,  36,  37,
  38,  39,  40,  42,  43,  44,  45,  46,
  47,  48,  50,  51,  52,  53,  54,  55,
  56,  58,  59,  60,  61,  62,  63,  64,
  66,  67,  68,  69,  70,  71,  72,  74,
  75,  76,  77,  78,  79,  80,  82,  83,
  84,  85,  86,  87,  88,  90,  91,  92,
  93,  94,  95,  96,  97,  99, 100, 101,
 102, 103, 104, 105, 107, 108, 109, 110,
 111, 112, 113, 115, 116, 117, 118, 119,
 120, 121, 123, 124, 125, 126, 127, 128,
};

static const signed short greenAdjust1[] = {
  34,  34,  33,  33,  32,  32,  32,  31,
  31,  30,  30,  30,  29,  29,  28,  28,
  28,  27,  27,  27,  26,  26,  25,  25,
  25,  24,  24,  23,  23,  23,  22,  22,
  21,  21,  21,  20,  20,  19,  19,  19,
  18,  18,  17,  17,  17,  16,  16,  15,
  15,  15,  14,  14,  13,  13,  13,  12,
  12,  12,  11,  11,  10,  10,  10,   9,
   9,   8,   8,   8,   7,   7,   6,   6,
   6,   5,   5,   4,   4,   4,   3,   3,
   2,   2,   2,   1,   1,   0,   0,   0,
   0,   0,  -1,  -1,  -1,  -2,  -2,  -2,
  -3,  -3,  -4,  -4,  -4,  -5,  -5,  -6,
  -6,  -6,  -7,  -7,  -8,  -8,  -8,  -9,
  -9, -10, -10, -10, -11, -11, -12, -12,
 -12, -13, -13, -14, -14, -14, -15, -15,
 -16, -16, -16, -17, -17, -17, -18, -18,
 -19, -19, -19, -20, -20, -21, -21, -21,
 -22, -22, -23, -23, -23, -24, -24, -25,
 -25, -25, -26, -26, -27, -27, -27, -28,
 -28, -29, -29, -29, -30, -30, -30, -31,
 -31, -32, -32, -32, -33, -33, -34, -34,
 -34, -35, -35, -36, -36, -36, -37, -37,
 -38, -38, -38, -39, -39, -40, -40, -40,
 -41, -41, -42, -42, -42, -43, -43, -44,
 -44, -44, -45, -45, -45, -46, -46, -47,
 -47, -47, -48, -48, -49, -49, -49, -50,
 -50, -51, -51, -51, -52, -52, -53, -53,
 -53, -54, -54, -55, -55, -55, -56, -56,
 -57, -57, -57, -58, -58, -59, -59, -59,
 -60, -60, -60, -61, -61, -62, -62, -62,
 -63, -63, -64, -64, -64, -65, -65, -66,
};

static const signed short greenAdjust2[] = {
  74,  73,  73,  72,  71,  71,  70,  70,
  69,  69,  68,  67,  67,  66,  66,  65,
  65,  64,  63,  63,  62,  62,  61,  60,
  60,  59,  59,  58,  58,  57,  56,  56,
  55,  55,  54,  53,  53,  52,  52,  51,
  51,  50,  49,  49,  48,  48,  47,  47,
  46,  45,  45,  44,  44,  43,  42,  42,
  41,  41,  40,  40,  39,  38,  38,  37,
  37,  36,  35,  35,  34,  34,  33,  33,
  32,  31,  31,  30,  30,  29,  29,  28,
  27,  27,  26,  26,  25,  24,  24,  23,
  23,  22,  22,  21,  20,  20,  19,  19,
  18,  17,  17,  16,  16,  15,  15,  14,
  13,  13,  12,  12,  11,  11,  10,   9,
   9,   8,   8,   7,   6,   6,   5,   5,
   4,   4,   3,   2,   2,   1,   1,   0,
   0,   0,  -1,  -1,  -2,  -2,  -3,  -4,
  -4,  -5,  -5,  -6,  -6,  -7,  -8,  -8,
  -9,  -9, -10, -11, -11, -12, -12, -13,
 -13, -14, -15, -15, -16, -16, -17, -17,
 -18, -19, -19, -20, -20, -21, -22, -22,
 -23, -23, -24, -24, -25, -26, -26, -27,
 -27, -28, -29, -29, -30, -30, -31, -31,
 -32, -33, -33, -34, -34, -35, -35, -36,
 -37, -37, -38, -38, -39, -40, -40, -41,
 -41, -42, -42, -43, -44, -44, -45, -45,
 -46, -47, -47, -48, -48, -49, -49, -50,
 -51, -51, -52, -52, -53, -53, -54, -55,
 -55, -56, -56, -57, -58, -58, -59, -59,
 -60, -60, -61, -62, -62, -63, -63, -64,
 -65, -65, -66, -66, -67, -67, -68, -69,
 -69, -70, -70, -71, -71, -72, -73, -73,
};

static const signed short blueAdjust[] = {
-276,-274,-272,-270,-267,-265,-263,-261,
-259,-257,-255,-253,-251,-249,-247,-245,
-243,-241,-239,-237,-235,-233,-231,-229,
-227,-225,-223,-221,-219,-217,-215,-213,
-211,-209,-207,-204,-202,-200,-198,-196,
-194,-192,-190,-188,-186,-184,-182,-180,
-178,-176,-174,-172,-170,-168,-166,-164,
-162,-160,-158,-156,-154,-152,-150,-148,
-146,-144,-141,-139,-137,-135,-133,-131,
-129,-127,-125,-123,-121,-119,-117,-115,
-113,-111,-109,-107,-105,-103,-101, -99,
 -97, -95, -93, -91, -89, -87, -85, -83,
 -81, -78, -76, -74, -72, -70, -68, -66,
 -64, -62, -60, -58, -56, -54, -52, -50,
 -48, -46, -44, -42, -40, -38, -36, -34,
 -32, -30, -28, -26, -24, -22, -20, -18,
 -16, -13, -11,  -9,  -7,  -5,  -3,  -1,
   0,   2,   4,   6,   8,  10,  12,  14,
  16,  18,  20,  22,  24,  26,  28,  30,
  32,  34,  36,  38,  40,  42,  44,  46,
  49,  51,  53,  55,  57,  59,  61,  63,
  65,  67,  69,  71,  73,  75,  77,  79,
  81,  83,  85,  87,  89,  91,  93,  95,
  97,  99, 101, 103, 105, 107, 109, 112,
 114, 116, 118, 120, 122, 124, 126, 128,
 130, 132, 134, 136, 138, 140, 142, 144,
 146, 148, 150, 152, 154, 156, 158, 160,
 162, 164, 166, 168, 170, 172, 175, 177,
 179, 181, 183, 185, 187, 189, 191, 193,
 195, 197, 199, 201, 203, 205, 207, 209,
 211, 213, 215, 217, 219, 221, 223, 225,
 227, 229, 231, 233, 235, 238, 240, 242,
};


#define CLAMP(x) x < 0 ? 0 : x & 0xff

inline void yuv2rgb(int y, int u, int v, quint16 *rgb)
{
    register  int r, g, b;

    r = y + redAdjust[v];
    g = y + greenAdjust1[u] + greenAdjust2[v];
    b = y + blueAdjust[u];
    
    //r = CLAMP(( 298 * y           + 409 * v + 128) >> 8);
    //g = CLAMP(( 298 * y - 100 * u - 208 * v + 128) >> 8);
    //b = CLAMP(( 298 * y + 516 * u           + 128) >> 8);
    
    r = CLAMP(r);
    g = CLAMP(g);
    b = CLAMP(b);

    *rgb = (quint16)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}


void OmegaCamera::getCameraImage( QImage& img, bool copy )
{
    Q_UNUSED(copy);

    if ( fd == -1 ) {
        if ( img.isNull() ) {
            img = QImage(height, width, QImage::Format_RGB16);
        }
        return;
    }

    currentFrame = ++currentFrame % mbuf.frames;
    
    unsigned char*  buf = frames + mbuf.offsets[currentFrame];
    quint16 *dest;
    for (int x = height  - 1; x >= 0; --x)
    { 
        dest = m_imageBuf + x;
        
        for (int y = 0; y < width; y+=2)
        {
            int u = buf[0];
            int v = buf[2];
            yuv2rgb(buf[1], u, v, dest);
            dest += height;
            yuv2rgb(buf[3], u, v, dest);
            dest += height;
            buf += 4;
        }
        
     }
    img = QImage((uchar*) m_imageBuf, height, width, QImage::Format_RGB16);

}

QList<QSize> OmegaCamera::photoSizes() const
{
    QList<QSize> list;

    /*
    list << QSize(176, 144) << QSize(160, 120) <<
            QSize(320, 240) << QSize(352, 288) <<
            QSize(640, 480) << QSize(1280, 1024);
    */
    list << QSize(480, 640) << QSize(240, 320) << QSize(144, 176);

    return list;
}

QList<QSize> OmegaCamera::videoSizes() const
{
    // We use the same sizes for both.
    return photoSizes();
}

QSize OmegaCamera::recommendedPhotoSize() const
{
    return QSize(480, 640);
}

QSize OmegaCamera::recommendedVideoSize() const
{
    return QSize(480, 640);
}

QSize OmegaCamera::recommendedPreviewSize() const
{
    return QSize(144, 176);
}

void OmegaCamera::setCaptureSize( QSize size )
{
    if ( size.width() != height || size.height() != width) {
        shutdown();
        setupCamera( size );
    }
}

} // ns camera

