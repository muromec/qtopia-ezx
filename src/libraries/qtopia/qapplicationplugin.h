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
#ifndef APPLICATIONPLUGIN_H
#define APPLICATIONPLUGIN_H

#include <qfactoryinterface.h>
#include <qtopiaglobal.h>

struct QTOPIA_EXPORT QApplicationFactoryInterface : public QFactoryInterface
{
    virtual void setProcessKey( const QString &appName ) = 0;
    virtual QWidget *createMainWindow(const QString &key, QWidget *parent=0, Qt::WFlags f=0) = 0;
};

#define QApplicationFactoryInterface_iid "com.trolltech.Qtopia.QApplicationFactoryInterface"
Q_DECLARE_INTERFACE(QApplicationFactoryInterface, QApplicationFactoryInterface_iid)

class QTOPIA_EXPORT QApplicationPlugin : public QObject, public QApplicationFactoryInterface
{
    Q_OBJECT
    Q_INTERFACES(QApplicationFactoryInterface:QFactoryInterface)
public:
    explicit QApplicationPlugin(QObject *parent = 0);
    ~QApplicationPlugin();

    virtual QStringList keys() const = 0;
    virtual void setProcessKey( const QString &appName ) = 0;
    virtual QWidget *createMainWindow(const QString &key, QWidget *parent=0, Qt::WFlags f=0) = 0;
};

#endif
