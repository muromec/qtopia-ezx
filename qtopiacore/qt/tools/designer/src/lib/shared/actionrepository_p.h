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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef ACTIONREPOSITORY_H
#define ACTIONREPOSITORY_H

#include "shared_global_p.h"
#include <QtCore/QMimeData>
#include <QtGui/QListWidget>

class QPixmap;

namespace qdesigner_internal {

class ResourceMimeData;

class QDESIGNER_SHARED_EXPORT ActionRepository: public QListWidget
{
    Q_OBJECT
public:
    enum   { ActionRole = Qt::UserRole + 1000 };

public:
    ActionRepository(QWidget *parent = 0);
    bool event ( QEvent * event );

signals:
    void contextMenuRequested(QContextMenuEvent *event, QListWidgetItem *item);
    void resourceImageDropped(const ResourceMimeData &data, QAction *action);

public slots:
    void filter(const QString &text);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dragMoveEvent(QDragMoveEvent *event);
    virtual void startDrag(Qt::DropActions supportedActions);
    virtual bool dropMimeData (int index, const QMimeData * data, Qt::DropAction action );
    virtual QMimeData *mimeData(const QList<QListWidgetItem*> items) const;
    virtual void focusInEvent(QFocusEvent *event);
    virtual void contextMenuEvent(QContextMenuEvent *event);
};

class QDESIGNER_SHARED_EXPORT ActionRepositoryMimeData: public QMimeData
{
    Q_OBJECT
public:
    typedef QList<QAction*> ActionList;

    ActionRepositoryMimeData(const ActionList &, Qt::DropAction dropAction);
    ActionRepositoryMimeData(QAction *, Qt::DropAction dropAction);

    const ActionList &actionList() const { return m_actionList; }
    virtual QStringList formats() const;

    static QPixmap actionDragPixmap(const QAction *action);
    
    // Utility to accept with right action
    void accept(QDragMoveEvent *event) const;
private:
    const Qt::DropAction m_dropAction;
    ActionList m_actionList;
};
} // namespace qdesigner_internal

#endif // ACTIONREPOSITORY_H
