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

#ifndef QDESIGNER_TABWIDGET_H
#define QDESIGNER_TABWIDGET_H

#include "shared_global_p.h"

#include <QtGui/QTabWidget>

class QDesignerFormWindowInterface;

namespace qdesigner_internal {
    class PromotionTaskMenu;
}

class QMenu;

class QDESIGNER_SHARED_EXPORT QDesignerTabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY(QString currentTabName READ currentTabName WRITE setCurrentTabName STORED false DESIGNABLE true)
    Q_PROPERTY(QString currentTabText READ currentTabText WRITE setCurrentTabText STORED false DESIGNABLE true)
    Q_PROPERTY(QString currentTabToolTip READ currentTabToolTip WRITE setCurrentTabToolTip STORED false DESIGNABLE true)
    Q_PROPERTY(QIcon currentTabIcon READ currentTabIcon WRITE setCurrentTabIcon STORED false DESIGNABLE true)

public:
    QDesignerTabWidget(QWidget *parent = 0);
    ~QDesignerTabWidget();

    QString currentTabName() const;
    void setCurrentTabName(const QString &tabName);

    QString currentTabText() const;
    void setCurrentTabText(const QString &tabText);

    QString currentTabToolTip() const;
    void setCurrentTabToolTip(const QString &tabToolTip);

    QIcon currentTabIcon() const;
    void setCurrentTabIcon(const QIcon &tabIcon);

    // Add context menu and return page submenu or 0.
    QMenu *addContextMenuActions(QMenu *popup);

    bool eventFilter(QObject *o, QEvent *e);

    QDesignerFormWindowInterface *formWindow() const;

private slots:
    void removeCurrentPage();
    void addPage();
    void addPageAfter();
    void slotCurrentChanged(int index);

protected:
    bool canMove(QMouseEvent *e) const;
    virtual void tabInserted(int index);
    virtual void tabRemoved(int index);

private:

private:
    int pageFromPosition(const QPoint &pos, QRect &rect) const;

    QPoint m_pressPoint;
    QWidget *m_dropIndicator;
    int m_dragIndex;
    QWidget *m_dragPage;
    QString m_dragLabel;
    QIcon m_dragIcon;
    bool m_mousePressed;
    QAction *m_actionDeletePage;
    QAction *m_actionInsertPage;
    QAction *m_actionInsertPageAfter;
    qdesigner_internal::PromotionTaskMenu* m_pagePromotionTaskMenu;
};

#endif // QDESIGNER_TABWIDGET_H
