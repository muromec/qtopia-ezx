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

#include "themedhomescreen.h"
#include "serverthemeview.h"
#include "themecontrol.h"
#include "themebackground_p.h"
#include "homescreenwidgets.h"
#include "pressholdgate.h"
#include <themedview.h>
#include <qvaluespace.h>
#include "touchscreenlockdlg.h"

#include <QtopiaServiceRequest>

#ifdef QTOPIA_PHONEUI
#include "dialercontrol.h"
#endif

#include <QVBoxLayout>

/*!
  \class ThemedHomeScreen
  \brief The ThemedHomeScreen class provides the home screen for Qtopia Phone.
  \ingroup QtopiaServer::PhoneUI

  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface, QAbstractHomeScreen

*/

/*!
  Constructs a new ThemedHomeScreen instance with the specified \a parent
  and widget \a flags
  */
ThemedHomeScreen::ThemedHomeScreen(QWidget *parent, Qt::WFlags flags)
    : QAbstractHomeScreen(parent, flags),
      themedView(new PhoneThemedView(parent, flags)),
      bgIface(0)
{
    // Try to avoid double layouting.
    if (parent)
        setGeometry(0, 0, parent->width(), parent->height());

    vsObject = new QValueSpaceObject("/UI/HomeScreen", this);
    themeBackground = new ThemeBackground(0, themedView);

    connect(themedView, SIGNAL(loaded()), this, SLOT(themeLoaded()));
    connect(themedView, SIGNAL(itemClicked(ThemeItem*)),
            this, SLOT(themeItemClicked(ThemeItem*)));

    QVBoxLayout *vbox = new QVBoxLayout;
    setLayout(vbox);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->addWidget(themedView);
    ThemeControl::instance()->registerThemedView(themedView, "Home");
}

/*!
  \internal
 */
ThemedHomeScreen::~ThemedHomeScreen()
{
    delete themedView;
}

/*!
  \internal
 */
void ThemedHomeScreen::updateBackground()
{
    themeBackground->updateBackground(themedView);
}

/*!
  \internal
 */
void ThemedHomeScreen::applyHomeScreenImage()
{
    ThemePluginItem *ip = (ThemePluginItem *)themedView->findItem("bgplugin", ThemedView::Plugin);
    if (ip) {
        QSettings cfg("Trolltech", "Launcher");
        cfg.beginGroup("HomeScreen");
        QString pname = cfg.value("Plugin", "Background").toString();
        if (pname == "Background") {
            // DON'T delete the existing object here - it causes it to crash. Something else
            // must be deleting it, such as "ip".
            bgIface = new ThemeBackgroundImagePlugin;
            ip->setBuiltin(bgIface);
        } else {
            ip->setPlugin(pname);
        }
    }
}

// bool ThemedHomeScreen::eventFilter(QObject *object, QEvent *e)
// {
// #ifdef QTOPIA_PHONEUI
//     bool calls = DialerControl::instance()->allCalls().count() != 0;
// #else
//     bool calls = false;
// #endif
// 
// #ifdef QTOPIA_CELL
//     bool locked = !calls && (keyLock->locked() || !simLock->open());
// #else
//     bool locked = !calls && (keyLock->locked());
// #endif
//     if (locked && (e->type() == QEvent::MouseButtonPress ||
//                    e->type() == QEvent::MouseButtonRelease ||
//                    e->type() == QEvent::MouseButtonDblClick ||
//                    e->type() == QEvent::MouseMove)) {
//         QMouseEvent *me = (QMouseEvent *)e;
//         ThemeItem *item = themedView->itemAt(me->pos());
//         if (item && item->itemName() == "star")
//             return false;
//         else
//             return true;
//     }
//     return QAbstractHomeScreen::eventFilter(object, e);
// }

/*!
  \internal
 */
void ThemedHomeScreen::themeLoaded()
{
    ThemeTextItem *textItem = (ThemeTextItem *)themedView->findItem("infobox", ThemedView::Text);
    if (textItem)
        textItem->setTextFormat(Qt::RichText);

    updateHomeScreenInfo();
}

/*!
  \internal
 */
void ThemedHomeScreen::updateHomeScreenInfo()
{
    if (!vsObject)
        return;

    QSettings config("Trolltech", "qpe");
    config.beginGroup("HomeScreen");
    vsObject->setAttribute("ShowOperator", config.value("ShowOperator", "true").toBool());
    vsObject->setAttribute("ShowProfile", config.value("ShowProfile", "true").toBool());
    vsObject->setAttribute("ShowDate", config.value("ShowDate", "true").toBool());
    vsObject->setAttribute("ShowTime", config.value("ShowTime", "true").toBool());
    vsObject->setAttribute("ShowLocation", config.value("ShowLocation", "true").toBool());
}

/*!
  \internal
 */
void ThemedHomeScreen::updateInformation()
{
    bool hideInfo = true;
    ThemeTextItem *textItem;
    if (infoData.count()) {
        InfoData info = infoData[0];

        textItem = (ThemeTextItem *)themedView->findItem("infobox", ThemedView::Text);
        if (textItem)
            textItem->setText(info.text);
        ThemeImageItem *imgItem = (ThemeImageItem *)themedView->findItem("infobox", ThemedView::Image);
        if (imgItem)
            imgItem->setImage(info.pixmap);
        hideInfo = info.text.isEmpty() && info.pixmap.isNull();
    }

    ThemeItem *item = themedView->findItem("infobox", ThemedView::Item);
    if (item)
        item->setActive(!hideInfo);
}

struct PhoneKeyDescription
{
    const char *name;
    int key;
};

PhoneKeyDescription keyMap [] = {
    {"zero", Qt::Key_0},
    {"one", Qt::Key_1},
    {"two", Qt::Key_2},
    {"three", Qt::Key_3},
    {"four", Qt::Key_4},
    {"five", Qt::Key_5},
    {"six", Qt::Key_6},
    {"seven", Qt::Key_7},
    {"eight", Qt::Key_8},
    {"nine", Qt::Key_9},
    {"hash", Qt::Key_NumberSign},
    {"star", Qt::Key_Asterisk},
    {0, 0}
};

/*!
  \internal
 */
void ThemedHomeScreen::themeItemClicked(ThemeItem *item)
{
    if (!item)
        return;

    if (!item->isInteractive())
        return;

    QString in = item->itemName();
#ifdef QTOPIA_CELL
    {
        int i = 0;
        while (keyMap[i].name != 0) {
            if (in == keyMap[i].name) {
                if (!emLock->processKeyEvent(new QKeyEvent(QEvent::KeyPress, keyMap[i].key, Qt::NoModifier)))
                    simLock->processKeyEvent(new QKeyEvent(QEvent::KeyPress, keyMap[i].key, Qt::NoModifier));
                break;
            }
            ++i;
        }
    }
    if (in == "star")
        QtopiaInputEvents::processKeyEvent('*', Qt::Key_Asterisk, Qt::NoModifier, true, false);
    if (keyLock->locked() || !simLock->open())
        return;
#endif
    if (in == "LauncherHSWidget") {
        ThemeWidgetItem* wItem = static_cast<ThemeWidgetItem*>(item);
        if (wItem != 0) {
            LauncherHSWidget* launcher = qobject_cast<LauncherHSWidget*>(wItem->widget());
            if (launcher != 0)
                launcher->launch();
        }
    } else if (in == "WorldmapHSWidget") {
        ThemeWidgetItem* wItem = static_cast<ThemeWidgetItem*>(item);
        if (wItem != 0) {
            WorldmapHSWidget* worldmap = qobject_cast<WorldmapHSWidget*>(wItem->widget());
            if (worldmap != 0)
                worldmap->showCity();
        }
    } else if (in == "lock") {
        TouchScreenLockDialog *dialog;
        dialog = new TouchScreenLockDialog(0, Qt::WindowStaysOnTopHint);
    } else if (in == "mainmenu") {
        emit showPhoneBrowser();
    } else if (in == "speeddial") {
        emit speedDial(QString());
    } else if( in == "dialer" ) {
        QtopiaServiceRequest e( "Dialer", "showDialer(QString)" );
        e << QString();
        e.send();
    }
}

/*!
  \internal
 */
void ThemedHomeScreen::showPinboxInformation(const QString &pix, const QString &text)
{
    ThemeTextItem *textItem = (ThemeTextItem*)themedView->findItem("pinbox", ThemedView::Text);
    if (textItem)
        textItem->setText(text);
    ThemeImageItem *imgItem = (ThemeImageItem*)themedView->findItem("pinbox", ThemedView::Image);
    if (imgItem) {
        imgItem->setImage(QPixmap(pix));
    }
    themedView->findItem("pinbox", ThemedView::Layout)->setActive(true);
}

/*!
  \internal
 */
void ThemedHomeScreen::activatePinbox(bool enable)
{
    themedView->findItem("pinbox", ThemedView::Layout)->setActive(enable);
}

QTOPIA_REPLACE_WIDGET(QAbstractHomeScreen, ThemedHomeScreen);
