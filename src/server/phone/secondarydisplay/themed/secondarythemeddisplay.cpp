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

#include "secondarythemeddisplay.h"
#include "serverthemeview.h"
#include "qtopiaserverapplication.h"
#include "themecontrol.h"
#include "windowmanagement.h"
#include "themebackground_p.h"

#include <custom.h>
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
#include <qexportedbackground.h>
#endif

#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QPainter>


class SecondaryHomeScreen : public PhoneThemedView
{
    Q_OBJECT
public:
    SecondaryHomeScreen(QWidget *parent=0, Qt::WFlags f=0);

    ~SecondaryHomeScreen();

    void applyBackgroundImage();

public slots:

    void updateBackground();

protected:

    void themeLoaded(const QString &);

private:
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
    QExportedBackground *exportedBg;
#endif
    ThemedItemPlugin *bgIface;
    ThemeBackground *themeBackground;
};

SecondaryHomeScreen::SecondaryHomeScreen(QWidget *parent, Qt::WFlags f)
    : PhoneThemedView(parent, f)
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
    , exportedBg(0)
#endif
    , bgIface(0)
{
    ThemeControl::instance()->registerThemedView(this, "SecondaryHome");

#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
    QDesktopWidget *desktop = QApplication::desktop();
    QRect desktopRect = desktop->screenGeometry(ThemeBackground::SecondaryScreen);
    QExportedBackground::initExportedBackground(desktopRect.width(),
                                                desktopRect.height(),
                                                ThemeBackground::SecondaryScreen);

    if (ThemeControl::instance()->exportBackground())
        exportedBg = new QExportedBackground(ThemeBackground::SecondaryScreen, this);
#endif

    connect(ThemeControl::instance(), SIGNAL(themeChanged()),
            this, SLOT(updateBackground()));

    themeBackground = new ThemeBackground(ThemeBackground::SecondaryScreen, this);
}

SecondaryHomeScreen::~SecondaryHomeScreen()
{
    if ( bgIface ) {
        delete bgIface;
    }
}

void SecondaryHomeScreen::applyBackgroundImage()
{
    ThemePluginItem *ip = (ThemePluginItem *)findItem("bgplugin", Plugin);
    if (ip) {
        QSettings cfg("Trolltech","Launcher");
        cfg.beginGroup("SecondaryHomeScreen");
        QString pname = cfg.value("Plugin", "Background").toString();
        if (pname == "Background") {
            // DON'T delete the existing object here - it causes it to crash. Something else
            // must be deleting it, "ip" presumably <sigh>.
            bgIface = new ThemeBackgroundImagePlugin(ThemeBackground::SecondaryScreen);
            ip->setBuiltin(bgIface);
        } else {
            ip->setPlugin(pname);
        }
    }
}

void SecondaryHomeScreen::updateBackground()
{
    themeBackground->updateBackground(this);
}

void SecondaryHomeScreen::themeLoaded(const QString&)
{
    applyBackgroundImage();

    ThemeTextItem *textItem = (ThemeTextItem *)findItem("infobox", Text);
    if (textItem)
        textItem->setTextFormat(Qt::RichText);
}

//===========================================================================

class SecondaryTitle : public PhoneThemedView
{
public:
    SecondaryTitle(QWidget *parent=0, Qt::WFlags f=0);
};

SecondaryTitle::SecondaryTitle(QWidget *parent, Qt::WFlags f)
    : PhoneThemedView(parent, f)
{
    ThemeControl::instance()->registerThemedView(this, "SecondaryTitle");
    WindowManagement::dockWindow(this, WindowManagement::Top, ThemeBackground::SecondaryScreen);
}

//===========================================================================

/*!
  \class ThemedSecondaryDisplay
  \brief The ThemedSecondaryDisplay class implements the secondary display for Qtopia Phone.
  \ingroup QtopiaServer::PhoneUI

  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface, QAbstractSecondaryDisplay
  */

/*!
  Constructs a ThemedSecondaryDisplay instance with the specified
  \a parent and widget flags \a f.
  */
ThemedSecondaryDisplay::ThemedSecondaryDisplay(QWidget *parent, Qt::WFlags f)
    : QAbstractSecondaryDisplay(parent, f)
{
    title = new SecondaryTitle(0, Qt::FramelessWindowHint
                                    | Qt::Tool
                                    | Qt::WindowStaysOnTopHint);
    title->show();

    QVBoxLayout *vbox = new QVBoxLayout;

    // ensure home screen image stretches to edges of screen
    vbox->setContentsMargins(0, 0, 0, 0);

    setLayout(vbox);
    home = new SecondaryHomeScreen;

    vbox->addWidget(home);
}

/*!
  \reimp
  */
void ThemedSecondaryDisplay::applyBackgroundImage()
{
    home->applyBackgroundImage();
}

/*!
  \reimp
  */
void ThemedSecondaryDisplay::updateBackground()
{
    home->updateBackground();
}

QTOPIA_REPLACE_WIDGET(QAbstractSecondaryDisplay, ThemedSecondaryDisplay);

#include "secondarythemeddisplay.moc"
