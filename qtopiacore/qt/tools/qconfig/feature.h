/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef FEATURE_H
#define FEATURE_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QSet>
#include <QList>

class Feature;

class FeaturePrivate
{
public:
    FeaturePrivate(const QString &k)
	: key(k), enabled(true), selectable(true) {};
    
    const QString key;
    QString section;
    QString title;
    QString description;
    QSet<Feature*> dependencies;
    QSet<Feature*> supports; // features who depends on this one
    QSet<Feature*> relations;
    bool enabled;
    bool selectable;
};

class Feature : public QObject
{
    Q_OBJECT    

public:
    static Feature* getInstance(const QString &key);
    static void clear();

public:
    QString key() const { return d->key; }

    void setTitle(const QString &title);
    QString title() const { return d->title; }

    void setSection(const QString &section);
    QString section() const { return d->section; }

    void setDescription(const QString &description);    
    QString description() const { return d->description; };

    void addRelation(const QString &key);
    void setRelations(const QStringList &keys);
    QList<Feature*> relations() const;    

    void addDependency(const QString &dependency);
    void setDependencies(const QStringList &dependencies);
    QList<Feature*> dependencies() const;

    QList<Feature*> supports() const;    
    QString getDocumentation() const;

    void setEnabled(bool on);
    bool enabled() const { return d->enabled; };

    bool selectable() const { return d->selectable; }
    
    QString toHtml() const;    
    
    ~Feature();    

signals:
    void changed();
    
private:
    Feature(const QString &key);
    void updateSelectable();
    
    static QMap<QString, Feature*> instances;
    FeaturePrivate *d;
};

#endif // FEATURE_H
