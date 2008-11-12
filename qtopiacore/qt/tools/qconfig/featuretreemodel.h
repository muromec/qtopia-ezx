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

#ifndef FEATURETREEMODEL_H
#define FEATURETREEMODEL_H

#include <QAbstractItemModel>
#include <QMap>
#include <QHash>
#include <QTextStream>

class Feature;
class Node;

uint qHash(const QModelIndex&);

class FeatureTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FeatureTreeModel(QObject *parent = 0);
    ~FeatureTreeModel();

    void clear();
    
    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(const Feature *feature) const;    
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    void addFeature(Feature *feature);    
    Feature* getFeature(const QModelIndex &index) const;

    void readConfig(QTextStream &stream);
    void writeConfig(QTextStream &stream) const;
    
public slots:
    void featureChanged();
    
private:
    QModelIndex createIndex(int row, int column,
                            const QModelIndex &parent,
                            const Node *feature) const;    
    QModelIndex index(const QModelIndex &parent, const Feature *feature) const;
    bool contains(const QString &section, const Feature *f) const;
    Node* find(const QString &section, const Feature *f) const;
    QStringList findDisabled(const QModelIndex &parent) const;    
    
    QMap<QString, QList<Node*> > sections;
    mutable QHash<QModelIndex, QModelIndex> parentMap;
    mutable QHash<const Feature*, QModelIndex> featureIndexMap;    
};

#endif // FEATURETREEMODEL_H
