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

#ifndef _THEMECONTROL_H_
#define _THEMECONTROL_H_

#include <QObject>
#include <QList>
#include <QPair>
#include <QMap>

class ThemedView;
class QAbstractThemeWidgetFactory;

class ThemeControl : public QObject
{
    Q_OBJECT
public:
    static ThemeControl *instance();

    void registerThemedView(ThemedView *, const QString &);
    bool exportBackground() const;

    void refresh();

    void setThemeWidgetFactory(QAbstractThemeWidgetFactory *);

signals:
    void themeChanging();
    void themeChanged();

private:
    QString findFile(const QString &) const;
    void doTheme(ThemedView *, const QString &);
    void doThemeWidgets(ThemedView *view);

    ThemeControl();

    QString m_themeName;
    bool m_exportBackground;

    QMap<QString, QString> m_themeFiles;
    QList<QPair<ThemedView *, QString> > m_themes;

    QAbstractThemeWidgetFactory *m_widgetFactory;
};

#endif // _THEMECONTROL_H_

