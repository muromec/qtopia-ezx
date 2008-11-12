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

#include "themebackground_p.h"
#include "themecontrol.h"

#include <custom.h>
#include <QPainter>
#ifndef QTOPIA_NO_SVG
#include <QSvgRenderer>
#endif
#include <qtopiachannel.h>
#include <QSettings>
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
#include <qexportedbackground.h>
#endif
#include <qtopialog.h>
#include <themedview.h>
#include <QDesktopWidget>
#include <qtopiaapplication.h>

//XXX TODO!! Should probably get secondary screen number from config.
const int ThemeBackground::PrimaryScreen = 0;
const int ThemeBackground::SecondaryScreen = 1;


// Theme background is responsible for extracting the background
// specified by the theme and exporting it for use as application
// background.

ThemeBackground::ThemeBackground(int _screen, QObject *parent)
    : QObject(parent), screen(_screen), exportedBackground(0)
{
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
    // The exported background thing is responsible for persisting the
    // Theme's background.
    exportedBackground = new QExportedBackground(screen, this);
#endif
}

// Added to enable the background to be persisted.
void ThemeBackground::updateBackground(ThemedView *themedView)
{
    if (ThemeControl::instance()->exportBackground()) {

        ThemeImageItem *item = (ThemeImageItem *)themedView->findItem("background", ThemedView::Image);
        ThemeItem *plugin = themedView->findItem("bgplugin", ThemedView::Item);
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
        bool wasExported = exportedBackground->isAvailable();
#endif
        if (item) {
            item->updateDefaultImage();
            QDesktopWidget *desktop = QApplication::desktop();
            QRect desktopRect = desktop->screenGeometry(screen); //desktop->primaryScreen());
            // Create a 16bpp pixmap is possible
            QImage::Format fmt = desktop->depth() <= 16 ? QImage::Format_RGB16 : QImage::Format_ARGB32_Premultiplied;
            QImage img(desktopRect.width(),
                       desktopRect.height(), fmt);
            QPixmap pm = QPixmap::fromImage(img);
            QPainter p(&pm);
            QRect rect(QPoint(0,0), desktopRect.size());
            // We want the wallpaper to be visible on the home screen only,
            // so we don't export it
            if (plugin)
                plugin->setVisible(false);
            themedView->paint(&p, rect, item);
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
            QExportedBackground::setExportedBackground(pm,screen);
#endif
            // now we can set the "plugin" visible again
            if (plugin)
                plugin->setVisible(true);
        } else {
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
            QExportedBackground::clearExportedBackground(screen);
#endif
        }

#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
        if (!wasExported && exportedBackground->isAvailable()) {
           polishWindows(screen);
        }
#endif
    } else {
#ifdef QTOPIA_ENABLE_EXPORTED_BACKGROUNDS
        QExportedBackground::clearExportedBackground(screen);
#endif
    }
}

void ThemeBackground::polishWindows(int screen)
{
    QApplication::setPalette(QApplication::palette());

    foreach (QWidget *w, QApplication::topLevelWidgets()) {
        if (QApplication::desktop()->screenNumber(w) == screen) {
            QApplication::style()->polish(w);
            foreach (QObject *o, w->children()) {
                QWidget *sw = qobject_cast<QWidget*>(o);
                if (sw) {
                    QApplication::style()->polish(sw);
                }
            }
        }
    }
}

//===========================================================================

// ThemeBackgroundImagePlugin provides user selectable backgrounds.

ThemeBackgroundImagePlugin::ThemeBackgroundImagePlugin(int _screen)
    : ThemedItemPlugin(), width(0), height(0), dpMode(Center), ref(0)
    , screen(_screen)
{
    QSettings cfg("Trolltech","qpe");
    cfg.beginGroup("HomeScreen");
    switch ( screen ) {
    case ThemeBackground::PrimaryScreen:
        imgName = cfg.value("HomeScreenPicture").toString();
        dpMode = (DisplayMode)cfg.value("HomeScreenPictureMode").toInt();
        break;
    case ThemeBackground::SecondaryScreen:
        imgName = cfg.value("SecondaryHomeScreenPicture").toString();
        dpMode = (DisplayMode)cfg.value("SecondaryHomeScreenPictureMode").toInt();
        break;
    }

    if( !imgName.isEmpty() )
    {
        imgContent.setPermission( QDrmRights::Display );
        imgContent.setLicenseOptions( QDrmContent::Handover | QDrmContent::Reactivate );

        connect( &imgContent, SIGNAL(rightsExpired(QDrmContent)),
                    this,       SLOT  (rightsExpired(QDrmContent)) );

        if( !imgContent.requestLicense( QContent( imgName, false ) ) )
            return;

        imgContent.renderStarted();
        qLog(UI) << "Loading home screen picture:" << imgName;
    }
}

void ThemeBackgroundImagePlugin::renderSvg(int w, int h, Qt::AspectRatioMode mode)
{
#ifndef QTOPIA_NO_SVG
    QSvgRenderer r(imgName);
    QSize svgSize = r.defaultSize();
    svgSize.scale(w, h, mode);
    bg = QPixmap(svgSize);
    bg.fill(QColor(0,0,0,0));
    QPainter p(&bg);
    r.render(&p);
#endif
}

void ThemeBackgroundImagePlugin::resize(int w, int h)
{
    width = w;
    height = h;

    // dpmode
    // ScaleAndCrop - fill viewport keeping aspect ratio by expanding
    // Stretch - stretch image to fill viewport ignoring aspect ratio
    // Center/Tile - no resize
    // Scale - fit all image to viewport
    if (!imgName.isEmpty()) {
        if (dpMode == Center || dpMode == Tile) {
            bg.load(imgName);

#ifndef QTOPIA_NO_SVG
            if (imgName.endsWith(".svg"))
                renderSvg(bg.width(), bg.height(), Qt::KeepAspectRatio);
#endif
        } else {
            // gussing viewport size
            QRect availableRect = QtopiaApplication::desktop()->availableGeometry(screen);
            height = qMin(availableRect.height(), height);
            width = qMin(availableRect.width(), width);

            Qt::AspectRatioMode mode = Qt::IgnoreAspectRatio;
            if (dpMode == ScaleAndCrop) mode = Qt::KeepAspectRatioByExpanding;
            else if (dpMode == Stretch) mode = Qt::IgnoreAspectRatio;
            else if (dpMode == Scale) mode = Qt::KeepAspectRatio;

#ifndef QTOPIA_NO_SVG
            if (imgName.endsWith(".svg")) {
                renderSvg(width, height, mode);
            } else
#endif
            {
                QPixmap p;
                p.load(imgName);
                bg = p.scaled(QSize(width, height), mode);
            }
        }
    }
    qLog(UI) << "Home screen picture display mode:"
        << ((dpMode == ScaleAndCrop) ? "Scale and Crop" : (dpMode == Stretch) ? "Stretch"
                : (dpMode == Center) ? "Center" : (dpMode == Tile) ? "Tile" : "Scale");
    qLog(UI) << "Available screen size:" << width << "x" << height
        << "Homescreen image size:" << bg.size().width() << "x" << bg.size().height();
}

void ThemeBackgroundImagePlugin::paint(QPainter *p, const QRect &r)
{
    Q_UNUSED(r);
    if (!bg.isNull()) {
        QRect cr = QtopiaApplication::desktop()->availableGeometry(screen);

        // ensure image is drawn as if screen origin is at (0,0)
        // (secondary displays may not have (0,0) as screen origin)
        QRect sr = QtopiaApplication::desktop()->screenGeometry(screen);
        cr.moveTopLeft(QPoint(cr.left() - sr.left(), cr.top() - sr.top()));

        if (dpMode == Tile) {
            p->drawTiledPixmap( cr.left(), cr.top(), cr.width(), cr.height(), bg );
        } else {
            QPoint off((cr.width()-bg.width())/2 + cr.x(), (cr.height()-bg.height())/2 + cr.y());
            cr.translate(-off.x(), -off.y());
            cr &= QRect(0, 0, bg.width(), bg.height());
            p->drawPixmap(cr.topLeft()+off, bg, cr);
        }
    }
}

#ifdef QTOPIA_UNPORTED
QRESULT ThemeBackgroundImagePlugin::queryInterface(const QUuid &uuid, QUnknownInterface **iface)
{
    *iface = 0;
    if (uuid == IID_QUnknown)
        *iface = this;
    else if (uuid == IID_ThemedItem)
        *iface = this;
    else
        return QS_false;

    (*iface)->addRef();
    return QS_OK;
}
#endif

void ThemeBackgroundImagePlugin::rightsExpired( const QDrmContent &content )
{
    Q_UNUSED( content );

    QSettings config("Trolltech","qpe");
    config.beginGroup( "HomeScreen" );

    switch ( screen ) {
    case ThemeBackground::PrimaryScreen:
        config.setValue("HomeScreenPicture", QString() );
        break;
    case ThemeBackground::SecondaryScreen:
        config.setValue("SecondaryHomeScreenPicture", QString() );
        break;
    }

    config.sync(); // need to flush the config info first

    QtopiaChannel::send("QPE/System", "applyStyle()");
    QtopiaChannel::send("QPE/System", "applyStyleSplash()");
}
