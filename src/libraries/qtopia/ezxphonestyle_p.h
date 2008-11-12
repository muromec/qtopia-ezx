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

#ifdef EZX_A780
#ifndef _HAVE_EZXPHONESTYLE_H_
#define _HAVE_EZXPHONESTYLE_H_

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#include <QPhoneStyle>
class EzXPhoneStyle : public QPhoneStyle
{
public:
    EzXPhoneStyle();
    void drawControl(ControlElement ce, const QStyleOption *opt, QPainter *p,
                                const QWidget *widget) const;
    int pixelMetric(PixelMetric metric, const QStyleOption *option,
                            const QWidget *widget) const;
    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex *opt,
                                SubControl sc, const QWidget *w) const;

    virtual int scrollLineHeight() const;
private:
    void loadPixmaps() const;
};

#endif
#endif
