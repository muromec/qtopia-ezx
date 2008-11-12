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

#include "themecontrol.h"
#include "qabstractthemewidgetfactory.h"
#include <QSettings>
#include <QDebug>
#include <themedview.h>
#include <qtopianamespace.h>
#include <qtopialog.h>

/*!
  \class ThemeControl
  \ingroup QtopiaServer::Task
  \brief The ThemeControl class manages the registered theme views.
  
  This class is part of the Qtopia server and cannot be used by other Qtopia applications.
 */

/*!
  Returns the ThemeControl instance.
  */
ThemeControl *ThemeControl::instance()
{
    static ThemeControl *control = 0;
    if(!control)
        control = new ThemeControl;
    return control;
}

/*! \internal */
ThemeControl::ThemeControl()
: m_exportBackground(false), m_widgetFactory(0)
{
    refresh();
}

/*!
  Register a theme \a view with the given \a name.
  */
void ThemeControl::registerThemedView(ThemedView *view,
                                      const QString &name)
{
    m_themes.append(qMakePair(view, name));
    doTheme(view, name);
}

/*!
  Returns true if the theme configuration indicates that the background should
  be exported.
 */
bool ThemeControl::exportBackground() const
{
    return m_exportBackground;
}

/*!
  Update themed views.
  */
void ThemeControl::refresh()
{
    emit themeChanging();

    QSettings qpeCfg("Trolltech","qpe");
    qpeCfg.beginGroup("Appearance");
    m_exportBackground = qpeCfg.value("ExportBackground", true).toBool();

    QString themeFile = findFile(qpeCfg.value("Theme").toString()); // The server ensures this value is present and correct
    QSettings cfg(themeFile, QSettings::IniFormat);
    cfg.beginGroup("Theme");
    m_themeName = cfg.value("Name[]", "Unnamed").toString(); //we must use the untranslated theme name
    if ( m_themeName == "Unnamed" )
        qLog(I18n) << "Invalid theme name: Cannot load theme translations";

    // XXX hack around broken QSettings
    QString str = cfg.value("ContextConfig").toString();
    QStringList keys = cfg.childKeys();

    m_themeFiles.clear();
    foreach(QString screen, keys)
        m_themeFiles.insert(screen, cfg.value(screen).toString());

    for(int ii = 0; ii < m_themes.count(); ++ii) {
        doTheme(m_themes.at(ii).first, m_themes.at(ii).second);
    }

    emit themeChanged();
}

/*!
    Sets the factory used to create theme widgets displayed in themes
    to \a factory.
*/
void ThemeControl::setThemeWidgetFactory(QAbstractThemeWidgetFactory *factory)
{
    delete m_widgetFactory;
    m_widgetFactory = factory;
    for (int ii = 0; ii < m_themes.count(); ++ii)
        doThemeWidgets(m_themes.at(ii).first);
}

/*!
  \fn ThemeControl::themeChanging()

  Emitted just before the theme is changed.
  */

/*!
  \fn ThemeControl::themeChanged()

  Emitted immediately after the theme changes.
 */

QString ThemeControl::findFile(const QString &file) const
{
    QStringList instPaths = Qtopia::installPaths();
    foreach (QString path, instPaths) {
        QString themeDataPath(path + QLatin1String("etc/themes/") + file);
        if (QFile::exists(themeDataPath)) {
            return themeDataPath;
        }
    }

    return QString();
}

void ThemeControl::doTheme(ThemedView *view, const QString &name)
{
    QString path = m_themeFiles[name + "Config"];
    if(!path.isEmpty()) {
        view->setThemeName(m_themeName);
        view->loadSource(findFile(path));
        doThemeWidgets(view);
    } else {
        qWarning("Invalid %s theme.", name.toAscii().constData());
    }
}

void ThemeControl::doThemeWidgets(ThemedView *view)
{
    if (m_widgetFactory) {
        QList<ThemeItem*> items = view->findItems(QString(), ThemedView::Widget);
        QList<ThemeItem*>::ConstIterator it;
        for (it = items.begin(); it != items.end(); ++it)
            m_widgetFactory->createWidget((ThemeWidgetItem*)*it);
    }
}

