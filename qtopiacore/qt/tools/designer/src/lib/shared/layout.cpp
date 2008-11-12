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

#include "layout_p.h"
#include "qdesigner_utils_p.h"
#include "qlayout_widget_p.h"
#include "spacer_widget_p.h"
#include "layoutdecoration.h"
#include "widgetfactory_p.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>

#include <QtCore/qdebug.h>
#include <QtCore/QVector>

#include <QtGui/qevent.h>
#include <QtGui/QGridLayout>
#include <QtGui/QPainter>
#include <QtGui/QBitmap>
#include <QtGui/QSplitter>
#include <QtGui/QMainWindow>
#include <QtGui/QApplication>
#include <QtGui/QScrollArea>

namespace qdesigner_internal {

class FriendlyBoxLayout: public QBoxLayout
{
public:
    inline FriendlyBoxLayout(Direction d) : QBoxLayout(d) { Q_ASSERT(0); }

    friend void insert_into_box_layout(QBoxLayout *box, int index, QWidget *widget);
};

void add_to_box_layout(QBoxLayout *box, QWidget *widget)
{
    if (QLayoutWidget *layoutWidget = qobject_cast<QLayoutWidget*>(widget)) {
        QLayoutWidgetItem *item = new QLayoutWidgetItem(layoutWidget);
        item->addTo(box);
        box->addItem(item);
    } else {
        box->addWidget(widget);
    }
}

void insert_into_box_layout(QBoxLayout *box, int index, QWidget *widget)
{
    if (QLayoutWidget *layoutWidget = qobject_cast<QLayoutWidget*>(widget)) {
        QLayoutWidgetItem *item = new QLayoutWidgetItem(layoutWidget);
        item->addTo(box);
        static_cast<FriendlyBoxLayout*>(box)->insertItem(index, item);
    } else if (QSplitter *splitter = qobject_cast<QSplitter *>(widget->parent())) {
        splitter->insertWidget(index, widget);
    } else {
        box->insertWidget(index, widget);
    }
}

void add_to_grid_layout(QGridLayout *grid, QWidget *widget, int r, int c, int rs, int cs, Qt::Alignment align)
{
    if (QLayoutWidget *layoutWidget = qobject_cast<QLayoutWidget*>(widget)) {
        QLayoutWidgetItem *item = new QLayoutWidgetItem(layoutWidget);
        item->addTo(grid);
        grid->addItem(item, r, c, rs, cs, align);
    } else {
        grid->addWidget(widget, r, c, rs, cs, align);
    }
}


/*!
  \class Layout layout.h
  \brief Baseclass for layouting widgets in the Designer

  Classes derived from this abstract base class are used for layouting
  operations in the Designer.

*/

/*!  \a p specifies the parent of the layoutBase \a lb. The parent
  might be changed in setup(). If the layoutBase is a
  container, the parent and the layoutBase are the same. Also they
  always have to be a widget known to the designer (e.g. in the case
  of the tabwidget parent and layoutBase are the tabwidget and not the
  page which actually gets laid out. For actual usage the correct
  widget is found later by Layout.)
 */

Layout::Layout(const QList<QWidget*> &wl, QWidget *p, QDesignerFormWindowInterface *fw, QWidget *lb, bool splitter) :
    m_widgets(wl), 
    m_parentWidget(p), 
    m_layoutBase(lb), 
    m_formWindow(fw), 
    m_useSplitter(splitter), 
    m_isBreak(false)
{
    if (m_layoutBase)
        m_oldGeometry = m_layoutBase->geometry();
}

Layout::~Layout()
{
}

/*!  The widget list we got in the constructor might contain too much
  widgets (like widgets with different parents, already laid out
  widgets, etc.). Here we set up the list and so the only the "best"
  widgets get laid out.
*/

void Layout::setup()
{
    m_startPoint = QPoint(32767, 32767);

    // Go through all widgets of the list we got. As we can only
    // layout widgets which have the same parent, we first do some
    // sorting which means create a list for each parent containing
    // its child here. After that we keep working on the list of
    // children which has the most entries.
    // Widgets which are already laid out are thrown away here too

    QMultiMap<QWidget*, QWidget*> lists;
    foreach (QWidget *w, m_widgets) {
        QWidget *p = w->parentWidget();

        if (p && LayoutInfo::layoutType(m_formWindow->core(), p) != LayoutInfo::NoLayout
                && m_formWindow->core()->metaDataBase()->item(p->layout()) != 0)
            continue;

        lists.insert(p, w);
    }

    QList<QWidget*> lastList;
    QList<QWidget*> parents = lists.keys();
    foreach (QWidget *p, parents) {
        QList<QWidget*> children = lists.values(p);

        if (children.count() > lastList.count())
            lastList = children;
    }


    // If we found no list (because no widget did fit at all) or the
    // best list has only one entry and we do not layout a container,
    // we leave here.
    QDesignerWidgetDataBaseInterface *widgetDataBase = m_formWindow->core()->widgetDataBase();
    if (lastList.count() < 2 &&
                        (!m_layoutBase ||
                          (!widgetDataBase->isContainer(m_layoutBase, false) &&
                            m_layoutBase != m_formWindow->mainContainer()))
                       ) {
        m_widgets.clear();
        m_startPoint = QPoint(0, 0);
        return;
    }

    // Now we have a new and clean widget list, which makes sense
    // to layout
    m_widgets = lastList;
    // Also use the only correct parent later, so store it

    Q_ASSERT(m_widgets.isEmpty() == false);

    m_parentWidget = m_formWindow->core()->widgetFactory()->widgetOfContainer(m_widgets.first()->parentWidget());
    // Now calculate the position where the layout-meta-widget should
    // be placed and connect to widgetDestroyed() signals of the
    // widgets to get informed if one gets deleted to be able to
    // handle that and do not crash in this case
    foreach (QWidget *w, m_widgets) {
        connect(w, SIGNAL(destroyed()), this, SLOT(widgetDestroyed()));
        m_startPoint = QPoint(qMin(m_startPoint.x(), w->x()), qMin(m_startPoint.y(), w->y()));
        const QRect rc(w->geometry());

        m_geometries.insert(w, rc);
        // Change the Z-order, as saving/loading uses the Z-order for
        // writing/creating widgets and this has to be the same as in
        // the layout. Else saving + loading will give different results
        w->raise();
    }

    sort();
}

void Layout::widgetDestroyed()
{
    if (QWidget *w = qobject_cast<QWidget *>(sender())) {
        m_widgets.removeAt(m_widgets.indexOf(w));
        m_geometries.remove(w);
    }
}

bool Layout::prepareLayout(bool &needMove, bool &needReparent)
{
    foreach (QWidget *widget, m_widgets) {
        widget->raise();
    }

    needMove = !m_layoutBase;
    needReparent = needMove || qobject_cast<QLayoutWidget*>(m_layoutBase) || qobject_cast<QSplitter*>(m_layoutBase);

    QDesignerWidgetFactoryInterface *widgetFactory = m_formWindow->core()->widgetFactory();
    QDesignerMetaDataBaseInterface *metaDataBase = m_formWindow->core()->metaDataBase();

    if (m_layoutBase == 0) {
        QString baseWidgetClassName = QLatin1String("QLayoutWidget");

        if (m_useSplitter)
            baseWidgetClassName = QLatin1String("QSplitter");

        m_layoutBase = widgetFactory->createWidget(baseWidgetClassName, widgetFactory->containerOfWidget(m_parentWidget));
        if (m_useSplitter) {
            m_layoutBase->setObjectName(QLatin1String("splitter"));
            m_formWindow->ensureUniqueObjectName(m_layoutBase);
        }
    } else {
        LayoutInfo::deleteLayout(m_formWindow->core(), m_layoutBase);
    }

    metaDataBase->add(m_layoutBase);

    Q_ASSERT(m_layoutBase->layout() == 0 || metaDataBase->item(m_layoutBase->layout()) == 0);

    return true;
}

static bool isMainContainer(QDesignerFormWindowInterface *fw, const QWidget *w)
{
    return w && (w == fw || w == fw->mainContainer());
}

static bool isPageOfContainerWidget(QDesignerFormWindowInterface *fw, QWidget *widget)
{
    QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(
            fw->core()->extensionManager(), widget->parentWidget());

    if (c != 0) {
        for (int i = 0; i<c->count(); ++i) {
            if (widget == c->widget(i))
                return true;
        }
    }

    return false;
}
void Layout::finishLayout(bool needMove, QLayout *layout)
{
    if (m_parentWidget == m_layoutBase) {
        QWidget *widget = m_layoutBase;
        m_oldGeometry = widget->geometry();

        bool done = false;
        while (!isMainContainer(m_formWindow, widget) && !done) {
            if (!m_formWindow->isManaged(widget)) {
                widget = widget->parentWidget();
                continue;
            } else if (LayoutInfo::isWidgetLaidout(m_formWindow->core(), widget)) {
                widget = widget->parentWidget();
                continue;
            } else if (isPageOfContainerWidget(m_formWindow, widget)) {
                widget = widget->parentWidget();
                continue;
            } else if (widget->parentWidget()) {
                QScrollArea *area = qobject_cast<QScrollArea*>(widget->parentWidget()->parentWidget());
                if (area && area->widget() == widget) {
                    widget = area;
                    continue;
                }
            }

            done = true;
        }

        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        // We don't want to resize the form window
        if (!Utils::isCentralWidget(m_formWindow, widget))
            widget->adjustSize();

        return;
    }

    if (needMove)
        m_layoutBase->move(m_startPoint);

    const QRect g(m_layoutBase->pos(), m_layoutBase->size());

    if (LayoutInfo::layoutType(m_formWindow->core(), m_layoutBase->parentWidget()) == LayoutInfo::NoLayout && !m_isBreak)
        m_layoutBase->adjustSize();
    else if (m_isBreak)
        m_layoutBase->setGeometry(m_oldGeometry);

    m_oldGeometry = g;
    layout->invalidate();
    m_layoutBase->show();

    if (qobject_cast<QLayoutWidget*>(m_layoutBase) || qobject_cast<QSplitter*>(m_layoutBase)) {
        m_formWindow->manageWidget(m_layoutBase);
        m_formWindow->selectWidget(m_layoutBase);
    }
}

void Layout::undoLayout()
{
    if (!m_widgets.count())
        return;

    m_formWindow->selectWidget(m_layoutBase, false);

    QDesignerWidgetFactoryInterface *widgetFactory = m_formWindow->core()->widgetFactory();
    QHashIterator<QWidget *, QRect> it(m_geometries);
    while (it.hasNext()) {
        it.next();

        if (!it.key())
            continue;

        QWidget* w = it.key();
        const QRect rc = it.value();

        const bool showIt = w->isVisibleTo(m_formWindow);
        QWidget *container = widgetFactory->containerOfWidget(m_parentWidget);

        // ### remove widget here
        QWidget *parentWidget = w->parentWidget();
        QDesignerFormEditorInterface *core = m_formWindow->core();
        QDesignerLayoutDecorationExtension *deco = qt_extension<QDesignerLayoutDecorationExtension*>(core->extensionManager(), parentWidget);

        if (deco)
            deco->removeWidget(w);

        w->setParent(container);
        w->setGeometry(rc);

        if (showIt)
            w->show();
    }

    LayoutInfo::deleteLayout(m_formWindow->core(), m_layoutBase);

    if (m_parentWidget != m_layoutBase && !qobject_cast<QMainWindow*>(m_layoutBase)) {
        m_formWindow->unmanageWidget(m_layoutBase);
        m_layoutBase->hide();
    } else {
        QMainWindow *mw = qobject_cast<QMainWindow*>(m_formWindow->mainContainer());
        if (m_layoutBase != m_formWindow->mainContainer() &&
                    (!mw || mw->centralWidget() != m_layoutBase))
            m_layoutBase->setGeometry(m_oldGeometry);
    }

    QWidget *ww = m_widgets.size() ? m_widgets.front() : m_formWindow;
    m_formWindow->selectWidget(ww);
}

void Layout::breakLayout()
{
    QMap<QWidget *, QRect> rects;
    foreach (QWidget *w, m_widgets) {
        rects.insert(w, w->geometry());
    }

    const QPoint m_layoutBasePos = m_layoutBase->pos();
    QDesignerWidgetDataBaseInterface *widgetDataBase = m_formWindow->core()->widgetDataBase();

    LayoutInfo::deleteLayout(m_formWindow->core(), m_layoutBase);

    const bool needReparent = qobject_cast<QLayoutWidget*>(m_layoutBase) ||
                        qobject_cast<QSplitter*>(m_layoutBase)     ||
                        (!widgetDataBase->isContainer(m_layoutBase, false) &&
                          m_layoutBase != m_formWindow->mainContainer());
    const bool needResize = qobject_cast<QSplitter*>(m_layoutBase);
    const bool add = m_geometries.isEmpty();

    QMapIterator<QWidget*, QRect> it(rects);
    while (it.hasNext()) {
        it.next();

        QWidget *w = it.key();
        if (needReparent) {
            w->setParent(m_layoutBase->parentWidget(), 0);
            w->move(m_layoutBasePos + it.value().topLeft());
            w->show();
        }

        if (needResize)
            w->resize(it.value().size());

        if (add)
            m_geometries.insert(w, QRect(w->pos(), w->size()));
    }

    if (needReparent) {
        m_layoutBase->hide();
        m_parentWidget = m_layoutBase->parentWidget();
        m_formWindow->unmanageWidget(m_layoutBase);
    } else {
        m_parentWidget = m_layoutBase;
    }

    if (!m_widgets.isEmpty() && m_widgets.first() && m_widgets.first()->isVisibleTo(m_formWindow))
        m_formWindow->selectWidget(m_widgets.first());
    else
        m_formWindow->selectWidget(m_formWindow);
}
 
    
QLayout *Layout::createLayout(int type)
{
    if (m_useSplitter)
        return WidgetFactory::createUnmanagedLayout(m_layoutBase, type);

    QLayout *layout = m_formWindow->core()->widgetFactory()->createLayout(m_layoutBase, 0, type);
    if (qobject_cast<QLayoutWidget*>(m_layoutBase)) {
        QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(m_formWindow->core()->extensionManager(), layout);
        if (sheet) {
            sheet->setProperty(sheet->indexOf(QLatin1String("leftMargin")), 0);
            sheet->setProperty(sheet->indexOf(QLatin1String("topMargin")), 0);
            sheet->setProperty(sheet->indexOf(QLatin1String("rightMargin")), 0);
            sheet->setProperty(sheet->indexOf(QLatin1String("bottomMargin")), 0);
        }
    }
    return layout;
}

HorizontalLayout::HorizontalLayout(const QList<QWidget*> &wl, QWidget *p, QDesignerFormWindowInterface *fw, QWidget *lb, bool splitter)
    : Layout(wl, p, fw, lb, splitter)
{
}

void HorizontalLayout::sort()
{
    HorizontalLayoutList l(m_widgets);
    l.sort();
    m_widgets = l;
}

void HorizontalLayout::doLayout()
{
    bool needMove, needReparent;
    if (!prepareLayout(needMove, needReparent))
        return;

    QHBoxLayout *layout = static_cast<QHBoxLayout *>(createLayout(LayoutInfo::HBox));

    foreach (QWidget *w, m_widgets) {
        if (needReparent && w->parent() != m_layoutBase) {
            w->setParent(m_layoutBase, 0);
            w->move(QPoint(0,0));
        }

        if (m_useSplitter) {
            QSplitter *splitter = qobject_cast<QSplitter*>(m_layoutBase);
            Q_ASSERT(splitter != 0);
            splitter->addWidget(w);
        } else {
            if (Spacer *spacer = qobject_cast<Spacer*>(w))
                layout->addWidget(w, 0, spacer->alignment());
            else
                add_to_box_layout(layout, w);
        }
        w->show();
    }

    if (QSplitter *splitter = qobject_cast<QSplitter*>(m_layoutBase))
        splitter->setOrientation(Qt::Horizontal);

    finishLayout(needMove, layout);
}

VerticalLayout::VerticalLayout(const QList<QWidget*> &wl, QWidget *p, QDesignerFormWindowInterface *fw, QWidget *lb, bool splitter)
    : Layout(wl, p, fw, lb, splitter)
{
}

void VerticalLayout::sort()
{
    VerticalLayoutList l(m_widgets);
    l.sort();
    m_widgets = l;
}

void VerticalLayout::doLayout()
{
    bool needMove, needReparent;
    if (!prepareLayout(needMove, needReparent))
        return;

    QVBoxLayout *layout = static_cast<QVBoxLayout *>(createLayout(LayoutInfo::VBox));
    Q_ASSERT(layout != 0);

    foreach (QWidget *w, m_widgets) {
        if (needReparent && w->parent() != m_layoutBase) {
            w->setParent(m_layoutBase, 0);
            w->move(QPoint(0,0));
        }

        if (m_useSplitter) {
            QSplitter *splitter = qobject_cast<QSplitter*>(m_layoutBase);
            Q_ASSERT(splitter != 0);
            splitter->addWidget(w);
        } else {
            if (Spacer *spacer = qobject_cast<Spacer*>(w))
                layout->addWidget(w, 0, spacer->alignment());
            else
                add_to_box_layout(layout, w);
        }
        w->show();
    }

    if (QSplitter *splitter = qobject_cast<QSplitter*>(m_layoutBase)) { // ### m_useSplitter??
        splitter->setOrientation(Qt::Vertical);
    }

    finishLayout(needMove, layout);
}

class Grid
{
public:
    Grid(int rows, int cols);
    ~Grid();

    QWidget* cell(int row, int col) const { return m_cells[ row * m_ncols + col]; }
    void setCell(int row, int col, QWidget* w) { m_cells[ row * m_ncols + col] = w; }
    void setCells(QRect c, QWidget* w) {
        for (int rows = c.bottom()-c.top(); rows >= 0; rows--)
            for (int cols = c.right()-c.left(); cols >= 0; cols--) {
                setCell(c.top()+rows, c.left()+cols, w);
            }
    }
    int numRows() const { return m_nrows; }
    int numCols() const { return m_ncols; }

    void simplify();
    bool locateWidget(QWidget* w, int& row, int& col, int& rowspan, int& colspan);

private:
    void merge();
    int countRow(int r, int c) const;
    int countCol(int r, int c) const;
    void setRow(int r, int c, QWidget* w, int count);
    void setCol(int r, int c, QWidget* w, int count);
    bool isWidgetStartCol(int c) const;
    bool isWidgetEndCol(int c) const;
    bool isWidgetStartRow(int r) const;
    bool isWidgetEndRow(int r) const;
    bool isWidgetTopLeft(int r, int c) const;
    void extendLeft();
    void extendRight();
    void extendUp();
    void extendDown();
    
    const int m_nrows;
    const int m_ncols;
    
    QWidget** m_cells;
    bool* m_cols;
    bool* m_rows;
};

Grid::Grid(int r, int c) :
    m_nrows(r), 
    m_ncols(c),
    m_cells(new QWidget*[ r * c ]),
    m_cols(new bool[ c ]),
    m_rows(new bool[ r ])
{
    qFill(m_cells, m_cells + r * c,  static_cast<QWidget *>(0));
}

Grid::~Grid()
{
    delete [] m_cells;
    delete [] m_cols;
    delete [] m_rows;
}

int Grid::countRow(int r, int c) const
{
    QWidget* w = cell(r, c);
    int i = c + 1;
    while (i < m_ncols && cell(r, i) == w)
        i++;
    return i - c;
}

int Grid::countCol(int r, int c) const
{
    QWidget* w = cell(r, c);
    int i = r + 1;
    while (i < m_nrows && cell(i, c) == w)
        i++;
    return i - r;
}

void Grid::setCol(int r, int c, QWidget* w, int count)
{
    for (int i = 0; i < count; i++)
        setCell(r + i, c, w);
}

void Grid::setRow(int r, int c, QWidget* w, int count)
{
    for (int i = 0; i < count; i++)
        setCell(r, c + i, w);
}

bool Grid::isWidgetStartCol(int c) const
{
    int r;
    for (r = 0; r < m_nrows; r++) {
        if (cell(r, c) && ((c==0) || (cell(r, c)  != cell(r, c-1)))) {
            return true;
        }
    }
    return false;
}

bool Grid::isWidgetEndCol(int c) const
{
    int r;
    for (r = 0; r < m_nrows; r++) {
        if (cell(r, c) && ((c == m_ncols-1) || (cell(r, c) != cell(r, c+1))))
            return true;
    }
    return false;
}

bool Grid::isWidgetStartRow(int r) const
{
    int c;
    for (c = 0; c < m_ncols; c++) {
        if (cell(r, c) && ((r==0) || (cell(r, c) != cell(r-1, c))))
            return true;
    }
    return false;
}

bool Grid::isWidgetEndRow(int r) const
{
    int c;
    for (c = 0; c < m_ncols; c++) {
        if (cell(r, c) && ((r == m_nrows-1) || (cell(r, c) != cell(r+1, c))))
            return true;
    }
    return false;
}


bool Grid::isWidgetTopLeft(int r, int c) const
{
    QWidget* w = cell(r, c);
    if (!w)
        return false;
    return (!r || cell(r-1, c) != w) && (!c || cell(r, c-1) != w);
}

void Grid::extendLeft()
{
    int r,c,i;
    for (c = 1; c < m_ncols; c++) {
        for (r = 0; r < m_nrows; r++) {
            QWidget* w = cell(r, c);
            if (!w)
                continue;

            int cc = countCol(r, c);
            int stretch = 0;
            for (i = c-1; i >= 0; i--) {
                if (cell(r, i))
                    break;
                if (countCol(r, i) < cc)
                    break;
                if (isWidgetEndCol(i))
                    break;
                if (isWidgetStartCol(i)) {
                    stretch = c - i;
                    break;
                }
            }
            if (stretch) {
                for (i = 0; i < stretch; i++)
                    setCol(r, c-i-1, w, cc);
            }
        }
    }
}


void Grid::extendRight()
{
    int r,c,i;
    for (c = m_ncols - 2; c >= 0; c--) {
        for (r = 0; r < m_nrows; r++) {
            QWidget* w = cell(r, c);
            if (!w)
                continue;
            int cc = countCol(r, c);
            int stretch = 0;
            for (i = c+1; i < m_ncols; i++) {
                if (cell(r, i))
                    break;
                if (countCol(r, i) < cc)
                    break;
                if (isWidgetStartCol(i))
                    break;
                if (isWidgetEndCol(i)) {
                    stretch = i - c;
                    break;
                }
            }
            if (stretch) {
                for (i = 0; i < stretch; i++)
                    setCol(r, c+i+1, w, cc);
            }
        }
    }

}

void Grid::extendUp()
{
    int r,c,i;
    for (r = 1; r < m_nrows; r++) {
        for (c = 0; c < m_ncols; c++) {
            QWidget* w = cell(r, c);
            if (!w)
                continue;
            int cr = countRow(r, c);
            int stretch = 0;
            for (i = r-1; i >= 0; i--) {
                if (cell(i, c))
                    break;
                if (countRow(i, c) < cr)
                    break;
                if (isWidgetEndRow(i))
                    break;
                if (isWidgetStartRow(i)) {
                    stretch = r - i;
                    break;
                }
            }
            if (stretch) {
                for (i = 0; i < stretch; i++)
                    setRow(r-i-1, c, w, cr);
            }
        }
    }
}

void Grid::extendDown()
{
    int r,c,i;
    for (r = m_nrows - 2; r >= 0; r--) {
        for (c = 0; c < m_ncols; c++) {
            QWidget* w = cell(r, c);
            if (!w)
                continue;
            int cr = countRow(r, c);
            int stretch = 0;
            for (i = r+1; i < m_nrows; i++) {
                if (cell(i, c))
                    break;
                if (countRow(i, c) < cr)
                    break;
                if (isWidgetStartRow(i))
                    break;
                if (isWidgetEndRow(i)) {
                    stretch = i - r;
                    break;
                }
            }
            if (stretch) {
                for (i = 0; i < stretch; i++)
                    setRow(r+i+1, c, w, cr);
            }
        }
    }

}

void Grid::simplify()
{
    extendLeft();
    extendRight();
    extendUp();
    extendDown();
    merge();
}


void Grid::merge()
{
    int r,c;
    for (c = 0; c < m_ncols; c++)
        m_cols[c] = false;

    for (r = 0; r < m_nrows; r++)
        m_rows[r] = false;

    for (c = 0; c < m_ncols; c++) {
        for (r = 0; r < m_nrows; r++) {
            if (isWidgetTopLeft(r, c)) {
                m_rows[r] = true;
                m_cols[c] = true;
            }
        }
    }
}

bool Grid::locateWidget(QWidget *w, int &row, int &col, int &rowspan, int &colspan)
{
    int r, c, r2, c2;

    for (c = 0; c < m_ncols; c++) {
        for (r = 0; r < m_nrows; r++) {
            if (cell(r, c) == w) {
                row = 0;
                for (r2 = 1; r2 <= r; r2++) {
                    if (m_rows[r2-1])
                        row++;
                }
                col = 0;
                for (c2 = 1; c2 <= c; c2++) {
                    if (m_cols[c2-1])
                        col++;
                }
                rowspan = 0;
                for (r2 = r ; r2 < m_nrows && cell(r2, c) == w; r2++) {
                    if (m_rows[r2])
                        rowspan++;
                }
                colspan = 0;
                for (c2 = c; c2 < m_ncols && cell(r, c2) == w; c2++) {
                    if (m_cols[c2])
                        colspan++;
                }
                return true;
            }
        }
    }
    return false;
}




GridLayout::GridLayout(const QList<QWidget*> &wl, QWidget *p, QDesignerFormWindowInterface *fw, QWidget *lb, const QSize &res) : 
    Layout(wl, p, fw, lb),
    m_resolution(res),
    m_grid(0)
{
}

GridLayout::~GridLayout()
{
    delete m_grid;
}

QWidget *GridLayout::widgetAt(QGridLayout *layout, int row, int column) const
{
    int index = 0;
    while (QLayoutItem *item = layout->itemAt(index)) {
        if (item->widget()) {
            int r, c, rowspan, colspan;
            layout->getItemPosition(index, &r, &c, &rowspan, &colspan);
            if (row == r && column == c)
                return item->widget();
        }
        ++index;
    }
    return 0;
}

void GridLayout::doLayout()
{
    bool needMove, needReparent;
    if (!prepareLayout(needMove, needReparent))
        return;

    QGridLayout *layout =  static_cast<QGridLayout *>(createLayout(LayoutInfo::Grid));
 
    if (!m_grid)
        buildGrid();

    foreach (QWidget *w, m_widgets) {
        int r = 0, c = 0, rs = 0, cs = 0;

        if (m_grid->locateWidget(w, r, c, rs, cs)) {
            if (needReparent && w->parent() != m_layoutBase) {
                w->setParent(m_layoutBase, 0);
                w->move(QPoint(0,0));
            }

            Qt::Alignment alignment = Qt::Alignment(0);
            if (Spacer *spacer = qobject_cast<Spacer*>(w))
                alignment = spacer->alignment();

            if (rs * cs == 1) {
                add_to_grid_layout(layout, w, r, c, 1, 1, alignment);
            } else {
                add_to_grid_layout(layout, w, r, c, rs, cs, alignment);
            }

            w->show();
        } else {
            qDebug("ooops, widget '%s' does not fit in layout", w->objectName().toUtf8().constData());
        }
    }

    QLayoutSupport::createEmptyCells(layout);

    finishLayout(needMove, layout);
}

void GridLayout::sort()
{
    buildGrid();
}

void GridLayout::buildGrid()
{
    if (!m_widgets.count())
        return;
#if 0
    QMap<int, int> x_dict;
    QMap<int, int> y_dict;

    foreach (QWidget *w, m_widgets) {
        QRect g = w->geometry();

        x_dict.insert(g.left(), g.left());
        x_dict.insert(g.right(), g.right());

        y_dict.insert(g.top(), g.top());
        y_dict.insert(g.bottom(), g.bottom());
    }

    QList<int> x = x_dict.keys();
    QList<int> y = y_dict.keys();
#else
    // Pixel to cell conversion:
    // By keeping a list of start'n'stop values (x & y) for each widget,
    // it is possible to create a very small grid of cells to represent
    // the widget layout.
    // -----------------------------------------------------------------

    // We need a list of both start and stop values for x- & y-axis
    QVector<int> x( m_widgets.count()*2 );
    QVector<int> y( m_widgets.count()*2 );

    // Using push_back would look nicer, but operator[] is much faster
    int index  = 0;
    QWidget* w = 0;
    for (int i = 0; i < m_widgets.size(); ++i) {
        w = m_widgets.at(i);
        QRect widgetPos = w->geometry();
        x[index]   = widgetPos.left();
        x[index+1] = widgetPos.right();
        y[index]   = widgetPos.top();
        y[index+1] = widgetPos.bottom();
        index += 2;
    }

    qSort(x);
    qSort(y);

    // Remove duplicate x enteries (Remove next, if equal to current)
    if ( !x.empty() ) {
        for (QVector<int>::iterator current = x.begin() ;
             (current != x.end()) && ((current+1) != x.end()) ; )
            if ( (*current == *(current+1)) )
                x.erase(current+1);
            else
                current++;
    }

    // Remove duplicate y enteries (Remove next, if equal to current)
    if ( !y.empty() ) {
        for (QVector<int>::iterator current = y.begin() ;
             (current != y.end()) && ((current+1) != y.end()) ; )
            if ( (*current == *(current+1)) )
                y.erase(current+1);
            else
                current++;
    }
#endif

    delete m_grid;
    m_grid = new Grid(y.size() - 1, x.size() - 1);

    // Mark the cells in the grid that contains a widget
    foreach (QWidget *w, m_widgets) {
        QRect widgetPos = w->geometry();

        QRect c(0, 0, 0, 0);

        // From left til right (not including)
        for (int cw=0; cw<x.size(); cw++) {
            if (x[cw] == widgetPos.left())
                c.setLeft(cw);
            if (x[cw] <  widgetPos.right())
                c.setRight(cw);
        }

        // From top til bottom (not including)
        for (int ch=0; ch<y.size(); ch++) {
            if (y[ch] == widgetPos.top()   )
                c.setTop(ch);
            if (y[ch] <  widgetPos.bottom())
                c.setBottom(ch);
        }

        m_grid->setCells(c, w); // Mark cellblock
    }

    m_grid->simplify();

    QList<QWidget *> widgets;
    for (int i = 0; i < m_grid->numRows(); i++)
        for (int j = 0; j < m_grid->numCols(); j++) {
            QWidget *w = m_grid->cell(i, j);
            if (w && !widgets.contains(w))
                widgets.append(w);
        }
    m_widgets = widgets;
}


} // namespace qdesigner_internal
