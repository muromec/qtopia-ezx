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

#ifndef PLUGINLOADER_H
#define PLUGINLOADER_H

#include <qtopiaglobal.h>
#include <QObject>

class QPluginManagerPrivate;
class QPluginLoader;

class QTOPIA_EXPORT QPluginManager : public QObject
{
    Q_OBJECT
public:
    explicit QPluginManager(const QString &type, QObject *parent=0);
    ~QPluginManager();

    void clear();

    const QStringList &list() const;
    const QStringList &disabledList() const;
    QObject *instance(const QString &key);

    void setEnabled(const QString &name, bool enabled=true);
    bool isEnabled(const QString &name) const;

    static bool inSafeMode();
    static void init();

private slots:
    void instanceDestroyed();

private:
    void initType();
    QStringList languageList() const;
    QString stripSystem(const QString &libFile) const;
    void loaded( QObject *iface, QPluginLoader *lib, QString name );

private:
    QPluginManagerPrivate *d;
};

#endif // PLUGINLOADER_H
