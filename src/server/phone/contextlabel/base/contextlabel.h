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

#ifndef CONTEXTLABEL_H
#define CONTEXTLABEL_H

#include <qtopiaglobal.h>
#include <themedview.h>
#include "qtopiainputevents.h"
#include "serverthemeview.h"
#include "qsoftmenubarprovider.h"

class QTimer;
class QSettings;

class ContextLabel : public PhoneThemedView, public QtopiaKeyboardFilter
{
    Q_OBJECT
public:
    ContextLabel( QWidget *parent=0, Qt::WFlags f=0 );
    ~ContextLabel();


    QSize reservedSize() const;

protected:
    void updateLabels();
    virtual bool filter(int unicode, int keycode, int modifiers, bool press,
                        bool autoRepeat);

protected slots:
    void itemPressed(ThemeItem *item);
    void itemReleased(ThemeItem *item);
    void keyChanged(const QSoftMenuBarProvider::MenuButton &);

protected:
    virtual void themeLoaded(const QString &);

private slots:
    void initializeButtons();

private:
    struct Button {
        int key;
        ThemeImageItem *imgItem;
        ThemeTextItem *txtItem;
        bool changed;
    };

    int buttonForItem(ThemeItem *item) const;

    Button *buttons;
    int buttonCount;
    bool blockUpdates;
    int pressedBtn;
    bool loadedTheme;
    bool themeInit;
    QSoftMenuBarProvider *menuProvider;
};

#endif

