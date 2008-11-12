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

/*
 * KAsteroids - Copyright (c) Martin R. Jones 1997
 *
 * Part of the KDE project
 */

#ifndef __LEDMETER_H__
#define __LEDMETER_H__

#include <qframe.h>

class KALedMeter : public QFrame
{
    Q_OBJECT

 public:
    KALedMeter(QWidget* parent);

    int maxRawValue() const { return maxRawValue_; }
    void setMaxRawValue(int max);

    int meterLevels() const { return meterLevels_; }
    void setMeterLevels(int count);

    int rawValue () const { return rawValue_; }

 public slots:
    void setValue( int v );

 protected:
    virtual void resizeEvent(QResizeEvent* );
    virtual void paintEvent(QPaintEvent* event);

 private:
    void drawContents();

 protected:
    int maxRawValue_;
    int meterLevels_;
    int currentMeterLevel_;
    int rawValue_;
};

#endif
