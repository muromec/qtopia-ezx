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

#define QT_FATAL_ASSERT

#include "qplatformdefs.h"

#include "qfont.h"
#include "qapplication.h"
#include "qfontinfo.h"
#include "qfontdatabase.h"
#include "qfontmetrics.h"
#include "qpaintdevice.h"
#include "qtextcodec.h"
#include "qiodevice.h"
#include "qhash.h"

#include <private/qunicodetables_p.h>
#include "qfont_p.h"
#include "qfontengine_p.h"
#include "qfontengine_x11_p.h"
#include "qtextengine_p.h"

#include <private/qt_x11_p.h>
#include "qx11info_x11.h"

#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define QFONTLOADER_DEBUG
#define QFONTLOADER_DEBUG_VERBOSE

double qt_pixelSize(double pointSize, int dpi)
{
    if (pointSize < 0)
        return -1.;
    if (dpi == 75) // the stupid 75 dpi setting on X11
        dpi = 72;
    return (pointSize * dpi) /72.;
}

double qt_pointSize(double pixelSize, int dpi)
{
    if (pixelSize < 0)
        return -1.;
    if (dpi == 75) // the stupid 75 dpi setting on X11
        dpi = 72;
    return pixelSize * 72. / ((double) dpi);
}

/*
  Removes wildcards from an XLFD.

  Returns \a xlfd with all wildcards removed if a match for \a xlfd is
  found, otherwise it returns \a xlfd.
*/
static QByteArray qt_fixXLFD(const QByteArray &xlfd)
{
    QByteArray ret = xlfd;
    int count = 0;
    char **fontNames =
        XListFonts(QX11Info::display(), xlfd, 32768, &count);
    if (count > 0)
        ret = fontNames[0];
    XFreeFontNames(fontNames);
    return ret ;
}

typedef QHash<int, QString> FallBackHash;
Q_GLOBAL_STATIC(FallBackHash, fallBackHash)

// Returns the user-configured fallback family for the specified script.
QString qt_fallback_font_family(int script)
{
    FallBackHash *hash = fallBackHash();
    return hash->value(script);
}

// Sets the fallback family for the specified script.
Q_GUI_EXPORT void qt_x11_set_fallback_font_family(int script, const QString &family)
{
    FallBackHash *hash = fallBackHash();
    if (!family.isEmpty())
        hash->insert(script, family);
    else
        hash->remove(script);
}

int QFontPrivate::defaultEncodingID = -1;

/*!
  Internal function that initializes the font system.

  \internal
  The font cache and font dict do not alloc the keys. The key is a QString
  which is shared between QFontPrivate and QXFontName.
*/
void QFont::initialize()
{
    // create global font cache
    if (! QFontCache::instance) (void) new QFontCache;

    extern int qt_encoding_id_for_mib(int mib); // from qfontdatabase_x11.cpp
    QTextCodec *codec = QTextCodec::codecForLocale();
    // determine the default encoding id using the locale, otherwise
    // fallback to latin1 (mib == 4)
    int mib = codec ? codec->mibEnum() : 4;

    // for asian locales, use the mib for the font codec instead of the locale codec
    switch (mib) {
    case 38: // eucKR
        mib = 36;
        break;

    case 2025: // GB2312
        mib = 57;
        break;

    case 113: // GBK
        mib = -113;
        break;

    case 114: // GB18030
        mib = -114;
        break;

    case 2026: // Big5
        mib = -2026;
        break;

    case 2101: // Big5-HKSCS
        mib = -2101;
        break;

    case 16: // JIS7
        mib = 15;
        break;

    case 17: // SJIS
    case 18: // eucJP
        mib = 63;
        break;
    }

    // get the default encoding id for the locale encoding...
    QFontPrivate::defaultEncodingID = qt_encoding_id_for_mib(mib);
}

/*! \internal

  Internal function that cleans up the font system.
*/
void QFont::cleanup()
{
    // delete the global font cache
    delete QFontCache::instance;
}

/*!
  \internal
  X11 Only: Returns the screen with which this font is associated.
*/
int QFont::x11Screen() const
{
    return d->screen;
}

/*! \internal
    X11 Only: Associate the font with the specified \a screen.
*/
void QFont::x11SetScreen(int screen)
{
    if (screen < 0) // assume default
        screen = QX11Info::appScreen();

    if (screen == d->screen)
        return; // nothing to do

    detach();
    d->screen = screen;
}

/*!
    Returns the window system handle to the font, for low-level
    access. Using this function is \e not portable.
*/
Qt::HANDLE QFont::handle() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    if (engine->type() == QFontEngine::Multi)
        engine = static_cast<QFontEngineMulti *>(engine)->engine(0);
    if (engine->type() == QFontEngine::XLFD)
        return static_cast<QFontEngineXLFD *>(engine)->fontStruct()->fid;
    return 0;
}


/*!
  Returns the handle to the primary freetype face of the font. I font merging is not disabled a
  QFont can contain several physical fonts.

  Returns 0 if the font does not contains a freetype face.
*/
FT_Face QFont::freetypeFace() const
{
#ifndef QT_NO_FREETYPE
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    if (engine->type() == QFontEngine::Multi)
        engine = static_cast<QFontEngineMulti *>(engine)->engine(0);
#ifndef QT_NO_FONTCONFIG
    if (engine->type() == QFontEngine::Freetype) {
        const QFontEngineFT *ft = static_cast<const QFontEngineFT *>(engine);
        return ft->non_locked_face();
    } else
#endif
    if (engine->type() == QFontEngine::XLFD) {
        const QFontEngineXLFD *xlfd = static_cast<const QFontEngineXLFD *>(engine);
        return xlfd->non_locked_face();
    }
#endif
    return 0;
}

/*!
    Returns the name of the font within the underlying window system.

    On Windows and Mac OS X, this is usually just the family name of a TrueType
    font.

    On X11, depending on whether Qt was built with FontConfig support, it is an
    XLFD (X Logical Font Description) or a FontConfig pattern. An XLFD may be
    returned even if FontConfig support is enabled.

    Using the return value of this function is usually \e not \e
    portable.

    \sa setRawName()
*/
QString QFont::rawName() const
{
    QFontEngine *engine = d->engineForScript(QUnicodeTables::Common);
    Q_ASSERT(engine != 0);
    return QString::fromLatin1(engine->name());
}

/*!
    Sets a font by its system specific name. The function is
    particularly useful under X, where system font settings (for
    example X resources) are usually available in XLFD (X Logical Font
    Description) form only. You can pass an XLFD as \a name to this
    function.

    A font set with setRawName() is still a full-featured QFont. It can
    be queried (for example with italic()) or modified (for example with
    setItalic()) and is therefore also suitable for rendering rich text.

    If Qt's internal font database cannot resolve the raw name, the
    font becomes a raw font with \a name as its family.

    Note that the present implementation does not handle wildcards in
    XLFDs well, and that font aliases (file \c fonts.alias in the font
    directory on X11) are not supported.

    \sa rawName(), setRawMode(), setFamily()
*/
void QFont::setRawName(const QString &name)
{
    detach();

    // from qfontdatabase_x11.cpp
    extern bool qt_fillFontDef(const QByteArray &xlfd, QFontDef *fd, int dpi);

    if (!qt_fillFontDef(qt_fixXLFD(name.toLatin1()), &d->request, d->dpi)) {
        qWarning("QFont::setRawName: Invalid XLFD: \"%s\"", name.toLatin1().constData());

        setFamily(name);
        setRawMode(true);
    } else {
        resolve_mask = QFontPrivate::Complete;
    }
}

/*!
    Returns the "last resort" font family name.

    The current implementation tries a wide variety of common fonts,
    returning the first one it finds. Is is possible that no family is
    found in which case an empty string is returned.

    \sa lastResortFont()
*/
QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("Helvetica");
}

/*!
    Returns the family name that corresponds to the current style
    hint.

    \sa StyleHint styleHint() setStyleHint()
*/
QString QFont::defaultFamily() const
{
    switch (d->request.styleHint) {
    case QFont::Times:
        return QString::fromLatin1("Times");

    case QFont::Courier:
        return QString::fromLatin1("Courier");

    case QFont::Decorative:
        return QString::fromLatin1("Old English");

    case QFont::Helvetica:
    case QFont::System:
    default:
        return QString::fromLatin1("Helvetica");
    }
}

/*
  Returns a last resort raw font name for the font matching algorithm.
  This is used if even the last resort family is not available. It
  returns \e something, almost no matter what.  The current
  implementation tries a wide variety of common fonts, returning the
  first one it finds. The implementation may change at any time.
*/
static const char * const tryFonts[] = {
    "-*-helvetica-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-courier-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-times-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-lucida-medium-r-*-*-*-120-*-*-*-*-*-*",
    "-*-helvetica-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-courier-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-times-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-lucida-*-*-*-*-*-120-*-*-*-*-*-*",
    "-*-helvetica-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-courier-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-times-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-lucida-*-*-*-*-*-*-*-*-*-*-*-*",
    "-*-fixed-*-*-*-*-*-*-*-*-*-*-*-*",
    "6x13",
    "7x13",
    "8x13",
    "9x15",
    "fixed",
    0
};

// Returns true if the font exists, false otherwise
static bool fontExists(const QString &fontName)
{
    int count;
    char **fontNames = XListFonts(QX11Info::display(), (char*)fontName.toLatin1().constData(), 32768, &count);
    if (fontNames) XFreeFontNames(fontNames);

    return count != 0;
}

/*!
    Returns a "last resort" font name for the font matching algorithm.
    This is used if the last resort family is not available. It will
    always return a name, if necessary returning something like
    "fixed" or "system".

    The current implementation tries a wide variety of common fonts,
    returning the first one it finds. The implementation may change
    at any time, but this function will always return a string
    containing something.

    It is theoretically possible that there really isn't a
    lastResortFont() in which case Qt will abort with an error
    message. We have not been able to identify a case where this
    happens. Please \link bughowto.html report it as a bug\endlink if
    it does, preferably with a list of the fonts you have installed.

    \sa lastResortFamily() rawName()
*/
QString QFont::lastResortFont() const
{
    static QString last;

    // already found
    if (! last.isNull())
        return last;

    int i = 0;
    const char* f;

    while ((f = tryFonts[i])) {
        last = QString::fromLatin1(f);

        if (fontExists(last))
            return last;

        i++;
    }

#if defined(CHECK_NULL)
    qFatal("QFontPrivate::lastResortFont: Cannot find any reasonable font");
#endif

    return last;
}
