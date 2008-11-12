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

#include "qfont.h"
#include "qfont_p.h"
#include "qfontengine_p.h"
#include "qfontinfo.h"
#include "qfontmetrics.h"
#include "qpaintdevice.h"
#include "qstring.h"
#include <private/qt_mac_p.h>
#include <private/qtextengine_p.h>
#include <private/qunicodetables_p.h>
#include <qapplication.h>
#include "qfontdatabase.h"
#include <qpainter.h>
#include "qtextengine_p.h"
#include <stdlib.h>

extern float qt_mac_defaultDpi_x(); //qpaintdevice_mac.cpp

int qt_mac_pixelsize(const QFontDef &def, int dpi)
{
    float ret;
    if(def.pixelSize == -1)
        ret = def.pointSize *  dpi / qt_mac_defaultDpi_x();
    else
        ret = def.pixelSize;
    return qRound(ret);
}
int qt_mac_pointsize(const QFontDef &def, int dpi)
{
    float ret;
    if(def.pointSize < 0)
        ret = def.pixelSize * qt_mac_defaultDpi_x() / float(dpi);
    else
        ret = def.pointSize;
    return qRound(ret);
}

QString QFont::rawName() const
{
    return family();
}

void QFont::setRawName(const QString &name)
{
    setFamily(name);
}

void QFont::cleanup()
{
    delete QFontCache::instance;
}

/*!
  Returns an ATSUFontID
*/
quint32 QFont::macFontID() const
{
#if 1
    QFontEngine *fe = d->engineForScript(QUnicodeTables::Common);
    if (fe && fe->type() == QFontEngine::Mac)
        return static_cast<QFontEngineMacMulti*>(fe)->macFontID();
#else
    Str255 name;
    if(FMGetFontFamilyName((FMFontFamily)((UInt32)handle()), name) == noErr) {
        short fnum;
        GetFNum(name, &fnum);
        return fnum;
    }
#endif
    return 0;
}

// Returns an ATSUFonFamilyRef
Qt::HANDLE QFont::handle() const
{
#if 0
    QFontEngine *fe = d->engineForScript(QUnicodeTables::Common);
    if (fe && fe->type() == QFontEngine::Mac)
        return (Qt::HANDLE)static_cast<QFontEngineMacMulti*>(fe)->fontFamilyRef();
#endif
    return 0;
}

void QFont::initialize()
{
    if(!QFontCache::instance)
        new QFontCache();
}

QString QFont::defaultFamily() const
{
    switch(d->request.styleHint) {
        case QFont::Times:
            return QString::fromLatin1("Times New Roman");
        case QFont::Courier:
            return QString::fromLatin1("Courier New");
        case QFont::Decorative:
            return QString::fromLatin1("Bookman Old Style");
        case QFont::Helvetica:
        case QFont::System:
        default:
            return QString::fromLatin1("Helvetica");
    }
}

QString QFont::lastResortFamily() const
{
    return QString::fromLatin1("Helvetica");
}

QString QFont::lastResortFont() const
{
    return QString::fromLatin1("Geneva");
}
