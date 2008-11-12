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

#include "contextlabel.h"

#include <qtopiaipcenvelope.h>
#include <qtopialog.h>

#include <QSettings>
#include <QStyle>
#include <QPixmapCache>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include "qsoftmenubarprovider.h"
#include "qtopiainputevents.h"

/*!
  \class ContextLabel
  \ingroup QtopiaServer::PhoneUI
  \brief The ContextLabel class provides a themeable, dockable soft key bar for phones.

  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*!
  Create a new ContextLabel widget, with the appropriate \a parent and \a flags.
  */
ContextLabel::ContextLabel( QWidget *parent, Qt::WFlags flags )
    : PhoneThemedView(parent, flags), buttons(0), blockUpdates(false),
    pressedBtn(-1), loadedTheme(false), themeInit(false), menuProvider(0)

{
    if(!parent)
        setWindowTitle("_decoration_");

    menuProvider = new QSoftMenuBarProvider(this);
    QObject::connect(menuProvider, SIGNAL(keyChanged(QSoftMenuBarProvider::MenuButton)), this, SLOT(keyChanged(QSoftMenuBarProvider::MenuButton)));
    buttonCount = menuProvider->keyCount();
    qLog(UI) << "ContextLabel: generating" << buttonCount <<  "buttons";
    if(menuProvider->keyCount()) {
        buttons = new Button [buttonCount];
        for(int ii = 0; ii < menuProvider->keyCount(); ++ii) {
            buttons[ii].key = menuProvider->key(ii).key();
            qLog(UI) << "Add contextbutton:" << buttons[ii].key;
            buttons[ii].imgItem = 0;
            buttons[ii].txtItem = 0;
            buttons[ii].changed = false;
        }
    }

    connect(this, SIGNAL(itemPressed(ThemeItem*)),
            this, SLOT(itemPressed(ThemeItem*)));
    connect(this, SIGNAL(itemReleased(ThemeItem*)),
            this, SLOT(itemReleased(ThemeItem*)));

    QtopiaInputEvents::addKeyboardFilter(this);
}

/*!
  Destroys the widget.
 */
ContextLabel::~ContextLabel()
{
    delete [] buttons;
}

/*!
  Returns the size of the ContextLabel widget.
  */
QSize ContextLabel::reservedSize() const
{
    int rh = -1;
    ThemeItem *reserved = ((ThemedView *)this)->findItem("reserved", Item);
    if (reserved)
        rh = reserved->rect().height();

    return QSize(sizeHint().width(), rh);
}

/*! \internal */
void ContextLabel::themeLoaded(const QString &)
{
    loadedTheme = true;
    themeInit = false;
    QTimer::singleShot(0, this, SLOT(initializeButtons()));
}

void ContextLabel::initializeButtons()
{
    if (loadedTheme && !themeInit) {
        int availBtns = 0;
        int maxbuttons = buttonCount >= 3 ? 3 : buttonCount;
        ThemeImageItem *img[3] = { NULL, NULL, NULL };
        ThemeTextItem *txt[3] = { NULL, NULL, NULL };

        if (buttonCount) {
            for (int i = 0; i < maxbuttons; i++) {
                buttons[i].imgItem = 0;
                buttons[i].txtItem = 0;
                buttons[i].changed = true;
                ThemeImageItem *ii = (ThemeImageItem *)findItem("button"+QString::number(i),
                        Image);
                ThemeTextItem *ti = (ThemeTextItem *)findItem("button"+QString::number(i),
                        Text);

                if (ii || ti) {
                    img[availBtns] = ii;
                    txt[availBtns] = ti;
                    availBtns++;
                }
            }
        }

        if (buttonCount) {
            buttons[0].imgItem = img[0];
            buttons[0].txtItem = txt[0];
        }

        if (availBtns == buttonCount) {
            for (int i = 1; i < availBtns; i++) {
                buttons[i].imgItem = img[i];
                buttons[i].txtItem = txt[i];
            }
        } else if (availBtns < buttonCount) {
            if (availBtns == 2) {
                buttons[2].imgItem = img[1];
                buttons[2].txtItem = txt[1];
            }
        } else {
            if (buttonCount == 2) {
                buttons[1].imgItem = img[2];
                buttons[1].txtItem = txt[2];
            }
        }
        themeInit = true;

        updateLabels();
    }
}

int ContextLabel::buttonForItem(ThemeItem *item) const
{
    int pressed = -1;
    for( int i = 0 ; i < buttonCount ; ++i )
    {
        if( buttons[i].imgItem == item )
        {
            pressed = i;
            break;
        }
        if( buttons[i].txtItem == item )
        {
            pressed = i;
            break;
        }
    }

    return pressed;
}

/*! \internal */
void ContextLabel::itemPressed(ThemeItem *item)
{
    initializeButtons();
    pressedBtn = buttonForItem(item);
    if (pressedBtn >= 0) {
        int keycode = buttons[pressedBtn].key;
        //we have to swap the keycode because the Keyfilter in ServerApplication
        //swaps these two keys by default
        if ( QApplication::layoutDirection() == Qt::RightToLeft ) {
            if ( keycode == Qt::Key_Context1 )
                keycode = Qt::Key_Back;
            else if ( keycode == Qt::Key_Back )
                keycode = Qt::Key_Context1;
        }
        QtopiaInputEvents::processKeyEvent(0xffff, keycode, 0, true, false);
    }
}

/*! \internal */
void ContextLabel::itemReleased( ThemeItem *item )
{
    Q_UNUSED(item);
    initializeButtons();
    if (pressedBtn >= 0) {
        int keycode = buttons[pressedBtn].key;
        //we have to swap the keycode because the Keyfilter in ServerApplication
        //swaps these two keys by default
        if ( QApplication::layoutDirection() == Qt::RightToLeft ) {
            if ( keycode == Qt::Key_Context1 )
                keycode = Qt::Key_Back;
            else if ( keycode == Qt::Key_Back )
                keycode = Qt::Key_Context1;
        }
        QtopiaInputEvents::processKeyEvent(0xffff, keycode, 0, false, false);
        pressedBtn = -1;
    }
}

/*! \internal */
bool ContextLabel::filter(int unicode, int keycode, int modifiers, bool press,
                          bool autoRepeat)
{
    if(isHidden())
        return false;

    //we have to change Key_Context1 to Key_Back and vice versa
    bool rtl = QApplication::layoutDirection() == Qt::RightToLeft;
    if ( rtl ) {
        int code = -1;
        if ( keycode == Qt::Key_Context1 )
            code = Qt::Key_Back;
        if ( keycode == Qt::Key_Back )
            code = Qt::Key_Context1;
        if ( code >= 0 ) {
            QtopiaInputEvents::sendKeyEvent( unicode, code, (Qt::KeyboardModifiers) modifiers, press, autoRepeat );
            return true;
        }
    }
    return false;
}

/*! \internal */
void ContextLabel::keyChanged(const QSoftMenuBarProvider::MenuButton &button)
{
    Q_ASSERT(button.index() < buttonCount);
    initializeButtons();
    buttons[button.index()].changed = true;
    updateLabels();
}

/*! \internal */
void ContextLabel::updateLabels()
{
    initializeButtons();
    for (int idx = 0; idx < buttonCount; idx++) {
        if (buttons[idx].changed) {
            if (buttons[idx].txtItem)
                buttons[idx].txtItem->setText(menuProvider->key(idx).text());
            if (buttons[idx].imgItem)
                buttons[idx].imgItem->setImage(menuProvider->key(idx).pixmap());

            buttons[idx].changed = false;
        }
    }
}

