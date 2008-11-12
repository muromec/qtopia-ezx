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

#ifndef _THEMEDHOMESCREEN_H
#define _THEMEDHOMESCREEN_H

#include "qabstracthomescreen.h"

class PhoneThemedView;
class ThemedItemPlugin;
class ThemeItem;
class ThemeBackground;
class QValueSpaceObject;

class ThemedHomeScreen : public QAbstractHomeScreen
{
    Q_OBJECT

public:
    ThemedHomeScreen(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~ThemedHomeScreen();

    void updateBackground();
    void updateHomeScreenInfo();
    void updateInformation();
    void activatePinbox(bool enable);
    void showPinboxInformation(const QString &pix, const QString &text);

protected:
//    bool eventFilter(QObject *, QEvent *);

public Q_SLOTS:
    void applyHomeScreenImage();

private Q_SLOTS:
    void themeLoaded();
    void themeItemClicked(ThemeItem *item);

private:
    PhoneThemedView     *themedView;
    ThemedItemPlugin    *bgIface;
    ThemeBackground     *themeBackground;
    QValueSpaceObject   *vsObject;
};

#endif // _THEMEDHOMESCREEN_H
