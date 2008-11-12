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

#include "cityinfo.h"

#include <qtimestring.h>
#include <qtimezone.h>
#include <qtopialog.h>

#include <QStyleOption>
#include <QPalette>
#include <QPainter>

CityInfo::CityInfo(QWidget *parent, Qt::WFlags f)
    : QFrame(parent,f)
{
    mUtc = QTimeZone::utcDateTime();
//    QFontInfo *fontInfo;
//    fontInfo = new QFontInfo(font());
//    qLog(Time)<<"font size"<< fontInfo->pointSize();
//    QFont font = font();
//    font.setPointSize(fontInfo->pointSize()-2);
//    setFont(font);
}

void CityInfo::setZone(const QString &zone)
{
    mZone = zone;
//    qLog(Time) << "CityInfo()::setZone() " <<
        mZone.toLocal8Bit().constData();
    repaint();
}

void CityInfo::setUtcTime(const QDateTime &dt)
{
    mUtc = dt;
    repaint();
}

QString CityInfo::text() const
{
    QString line;
    QDateTime cityTime;

    if ( !mZone.isNull() ) {
        QTimeZone curZone( mZone.toLocal8Bit().constData() );
        if ( curZone.isValid() )
            cityTime = curZone.fromUtc(mUtc);
//        else
        //          qLog(Time) << "CityInfo()::text() curZone Not valid!";
        line = QTimeString::localHMDayOfWeek(cityTime).simplified();
        
//         qLog(Time) << "CityInfo()::text() cityTime=" << cityTime.toString("hh:mm");
//         qLog(Time) << "CityInfo()::text() timestring=" << line.toLocal8Bit().constData();
        return line;
    }
    else {
//        qLog(Time) << "CityInfo()::text() - mZone is NULL!";
        return QString();
    }
}

void CityInfo::paintEvent( QPaintEvent * )
{
    QPainter p(this);
    QStyleOptionHeader opt;

    opt.palette = palette();
    opt.state = QStyle::State_Enabled;
    opt.state |= QStyle::State_Horizontal;

    QRect cr = contentsRect();
    style()->drawItemText( &p, cr, Qt::AlignRight, opt.palette,
            opt.state, text(), QPalette::ButtonText);

    drawFrame(&p);
}


QSize CityInfo::sizeHint() const
{
    QSize res;
    QFontMetrics fm( font() );
    res.setWidth(fm.width(text()) + 2 );
    res.setHeight(fm.height() );
//    qLog(Time) << "CityInfo(QFrame)::sizeHint(w=" << res.width() << ", h=" << res.height();
    return res;
}
