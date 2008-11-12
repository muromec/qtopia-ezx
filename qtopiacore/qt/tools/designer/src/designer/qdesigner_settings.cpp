/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qdesigner.h"
#include "preferences.h"
#include "qdesigner_settings.h"
#include "qdesigner_widgetbox.h"
#include "qdesigner_workbench.h"
#include "qdesigner_propertyeditor.h"
#include "qdesigner_objectinspector.h"

#include <qdesigner_utils_p.h>

#include <QtCore/QVariant>
#include <QtCore/QDir>

#include <QtGui/QDesktopWidget>
#include <QtGui/QStyle>

#include <QtCore/qdebug.h>

static const char *designerPath = "/.designer";

static bool checkTemplatePath(const QString &path, bool create)
{
    QDir current(QDir::current());
    if (current.exists(path))
        return true;

    if (!create)
        return false;

    if (current.mkpath(path))
        return true;

    qdesigner_internal::designerWarning(QObject::tr("The template path %1 could not be created.").arg(path));
    return false;
}

QDesignerSettings::QDesignerSettings()
{
}

const QStringList &QDesignerSettings::defaultFormTemplatePaths()
{
    static QStringList rc;
    if (rc.empty()) {
        // Ensure default form template paths
        const QString templatePath = QLatin1String("/templates");
        // home
        QString path = QDir::homePath();
        path += QLatin1String(designerPath);
        path += templatePath;
        if (checkTemplatePath(path, true))
            rc += path;

        // designer/bin: Might be owned by root in some installations, do not force it.
        path = qDesigner->applicationDirPath();
        path += templatePath;
        if (checkTemplatePath(path, false))
            rc += path;
    }
    return rc;
}

QStringList QDesignerSettings::formTemplatePaths() const
{
    return value(QLatin1String("FormTemplatePaths"),defaultFormTemplatePaths()).toStringList();
}

void QDesignerSettings::setFormTemplatePaths(const QStringList &paths)
{
    setValue(QLatin1String("FormTemplatePaths"), paths);
}

QString QDesignerSettings::defaultUserWidgetBoxXml() const
{
    QString rc = QDir::homePath();
    rc += QLatin1String(designerPath);
    rc += QLatin1String("/widgetbox.xml");
    return rc;
}

void QDesignerSettings::saveGeometryFor(const QWidget *w)
{
    Q_ASSERT(w && !w->objectName().isEmpty());
    saveGeometryHelper(w, w->objectName());
}

void QDesignerSettings::setGeometryFor(QWidget *w, const QRect &fallBack) const
{
    Q_ASSERT(w && !w->objectName().isEmpty());
    setGeometryHelper(w, w->objectName(),
                      fallBack.isNull() ? QRect(QPoint(0, 0), w->sizeHint()) : fallBack);
}

void QDesignerSettings::saveGeometryHelper(const QWidget *w, const QString &key)
{
    beginGroup(key);
    setValue(QLatin1String("visible"), w->isVisible());
    setValue(QLatin1String("geometry"), w->saveGeometry());
    endGroup();
}

void QDesignerSettings::setGeometryHelper(QWidget *w, const QString &key,
                                          const QRect &fallBack) const
{
    QByteArray ba(value(key + QLatin1String("/geometry")).toByteArray());

    if (ba.isEmpty()) {
        w->move(fallBack.topLeft());
        w->resize(fallBack.size());
    } else {
        w->restoreGeometry(ba);
    }

    if (value(key + QLatin1String("/visible"), true).toBool())
        w->show();
}

QStringList QDesignerSettings::recentFilesList() const
{
    return value(QLatin1String("recentFilesList")).toStringList();
}

void QDesignerSettings::setRecentFilesList(const QStringList &sl)
{
    setValue(QLatin1String("recentFilesList"), sl);
}

void QDesignerSettings::setShowNewFormOnStartup(bool showIt)
{
    setValue(QLatin1String("newFormDialog/ShowOnStartup"), showIt);
}

bool QDesignerSettings::showNewFormOnStartup() const
{
    return value(QLatin1String("newFormDialog/ShowOnStartup"), true).toBool();
}

QByteArray QDesignerSettings::mainWindowState() const
{
    return value(QLatin1String("MainWindowState")).toByteArray();
}

void QDesignerSettings::setMainWindowState(const QByteArray &mainWindowState)
{
    setValue(QLatin1String("MainWindowState"), mainWindowState);
}

QByteArray QDesignerSettings::toolBoxState() const
{
    return value(QLatin1String("ToolBoxState")).toByteArray();
}

void QDesignerSettings::setToolBoxState(const QByteArray &state)
{
    setValue(QLatin1String("ToolBoxState"), state);
}

void QDesignerSettings::clearBackup()
{
    remove(QLatin1String("backup/fileListOrg"));
    remove(QLatin1String("backup/fileListBak"));
}

void QDesignerSettings::setBackup(const QMap<QString, QString> &map)
{
    const QStringList org = map.keys();
    const QStringList bak = map.values();

    setValue(QLatin1String("backup/fileListOrg"), org);
    setValue(QLatin1String("backup/fileListBak"), bak);
}

QMap<QString, QString> QDesignerSettings::backup() const
{
    const QStringList org = value(QLatin1String("backup/fileListOrg"), QStringList()).toStringList();
    const QStringList bak = value(QLatin1String("backup/fileListBak"), QStringList()).toStringList();

    QMap<QString, QString> map;
    for (int i = 0; i < org.count(); ++i)
        map.insert(org.at(i), bak.at(i));

    return map;
}

void QDesignerSettings::setPreferences(const Preferences& p)
{
    beginGroup(QLatin1String("UI"));
    setValue(QLatin1String("currentMode"), p.m_uiMode);
    setValue(QLatin1String("font"), p.m_font);
    setValue(QLatin1String("useFont"), p.m_useFont);
    setValue(QLatin1String("writingSystem"), p.m_writingSystem);
    endGroup();
    // grid
    setValue(QLatin1String("defaultGrid"), p.m_defaultGrid.toVariantMap());
    // merge template paths
    QStringList templatePaths = defaultFormTemplatePaths();
    templatePaths += p.m_additionalTemplatePaths;
    setFormTemplatePaths(templatePaths);
}

Preferences QDesignerSettings::preferences() const
{
    Preferences rc;
#ifdef Q_WS_WIN
    const UIMode defaultMode = DockedMode;
#else
    const UIMode defaultMode = TopLevelMode;
#endif
    rc.m_uiMode = static_cast<UIMode>(value(QLatin1String("UI/currentMode"), defaultMode).toInt());
    rc.m_writingSystem = static_cast<QFontDatabase::WritingSystem>(value(QLatin1String("UI/writingSystem"), QFontDatabase::Any).toInt());
    rc.m_font = qVariantValue<QFont>(value(QLatin1String("UI/font")));
    rc.m_useFont = value(QLatin1String("UI/useFont"), QVariant(false)).toBool();
    const QVariantMap defaultGridMap = value(QLatin1String("defaultGrid"), QVariantMap()).toMap();
    if (!defaultGridMap.empty())
        rc.m_defaultGrid.fromVariantMap(defaultGridMap);
    rc.m_additionalTemplatePaths = additionalFormTemplatePaths();
    return rc;
}

QStringList QDesignerSettings::additionalFormTemplatePaths() const
{
    // get template paths excluding internal ones
    QStringList rc = formTemplatePaths();
    foreach (QString internalTemplatePath, defaultFormTemplatePaths()) {
        const int index = rc.indexOf(internalTemplatePath);
        if (index != -1)
            rc.removeAt(index);
    }
    return rc;
}
