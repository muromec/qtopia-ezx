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
#ifndef __QTOPIA_QEXPORTEDBACKGROUND_H__
#define __QTOPIA_QEXPORTEDBACKGROUND_H__

#include <qtopiaglobal.h>
#include <qobject.h>
class QExportedBackgroundPrivate;
class QPixmap;
class QColor;

class QTOPIA_EXPORT QExportedBackground : public QObject
{
    Q_OBJECT
public:
    explicit QExportedBackground(QObject *parent = 0);
    explicit QExportedBackground(int screen, QObject *parent = 0);
    ~QExportedBackground();

    QPixmap wallpaper() const;
    const QPixmap &background() const;
    bool isAvailable() const;

    static void initExportedBackground(int width, int height, int screen=0 );
    static void clearExportedBackground(int screen=0);
    static void setExportedBackgroundTint(int);
    static void setExportedBackground(const QPixmap &image, int screen=0);

signals:
    void wallpaperChanged();
    void changed();
    void changed(const QPixmap &);

private slots:
    void sysMessage(const QString&,const QByteArray&);

private:
    void getPixmaps();
    static void colorize(QPixmap &, const QPixmap &, const QColor &);

private:
    QExportedBackgroundPrivate *d;
    friend class QExportedBackgroundPrivate;
};
#endif //__QTOPIA_QEXPORTEDBACKGROUND_H__
