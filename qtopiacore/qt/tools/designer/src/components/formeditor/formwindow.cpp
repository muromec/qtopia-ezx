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

/*
TRANSLATOR qdesigner_internal::FormWindow
*/

#include "formwindow.h"
#include "formeditor.h"
#include "formwindow_dnditem.h"
#include "formwindow_widgetstack.h"
#include "formwindowcursor.h"
#include "formwindowmanager.h"
#include "tool_widgeteditor.h"
#include "widgetselection.h"

// shared
#include <metadatabase_p.h>
#include <qdesigner_tabwidget_p.h>
#include <qdesigner_toolbox_p.h>
#include <qdesigner_stackedbox_p.h>
#include <qdesigner_resource.h>
#include <qdesigner_command_p.h>
#include <qdesigner_propertycommand_p.h>
#include <qdesigner_widget_p.h>
#include <qdesigner_utils_p.h>
#include <qlayout_widget_p.h>
#include <spacer_widget_p.h>
#include <invisible_widget_p.h>
#include <layoutinfo_p.h>
#include <connectionedit_p.h>
#include <actionprovider_p.h>

#include <QtDesigner/QExtensionManager>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerWidgetFactoryInterface>
#include <QtDesigner/QDesignerContainerExtension>
#include <QtDesigner/QDesignerTaskMenuExtension>
#include <QtDesigner/QDesignerWidgetBoxInterface>

#include <QtCore/QtDebug>
#include <QtCore/QBuffer>
#include <QtCore/QTimer>
#include <QtGui/QMenu>
#include <QtGui/QClipboard>
#include <QtGui/QUndoGroup>
#include <QtGui/QScrollArea>
#include <QtGui/QRubberBand>
#include <QtGui/QApplication>
#include <QtGui/QSplitter>
#include <QtGui/QMessageBox>
#include <QtGui/QPainter>
#include <QtGui/QGroupBox>
#include <QtGui/QDockWidget>

namespace {
class BlockSelection
{
public:
    BlockSelection(qdesigner_internal::FormWindow *fw)
        : m_formWindow(fw),
          m_blocked(m_formWindow->blockSelectionChanged(true))
    {
    }

    ~BlockSelection()
    {
        if (m_formWindow)
            m_formWindow->blockSelectionChanged(m_blocked);
    }

private:
    QPointer<qdesigner_internal::FormWindow> m_formWindow;
    const bool m_blocked;
};

enum { debugFormWindow = 0 };
}

namespace qdesigner_internal {

// ------------------------ FormWindow::Selection
// Maintains a pool of WidgetSelections to be used for selected widgets.

class FormWindow::Selection
{
public:
    Selection();
    ~Selection();

    // Clear
    void clear();

    // Also clear out the pool. Call if reparenting of the main container occurs.
    void  clearSelectionPool();

    void repaintSelection(QWidget *w);
    void repaintSelection();

    bool isWidgetSelected(QWidget *w) const;
    QWidgetList selectedWidgets() const;

    WidgetSelection *addWidget(FormWindow* fw, QWidget *w);
    // remove widget, return new current widget or 0
    QWidget* removeWidget(QWidget *w);

    void raiseList(const QWidgetList& l);
    void raiseWidget(QWidget *w);

    void updateGeometry(QWidget *w);

private:

    typedef QList<WidgetSelection *> SelectionPool;
    SelectionPool m_selectionPool;

    typedef QHash<QWidget *, WidgetSelection *> SelectionHash;
    SelectionHash m_usedSelections;
};

FormWindow::Selection::Selection()
{
}

FormWindow::Selection::~Selection()
{
    clearSelectionPool();
}

void FormWindow::Selection::clear()
{
    if (!m_usedSelections.empty()) {
        const SelectionHash::iterator mend = m_usedSelections.end();
        for (SelectionHash::iterator it = m_usedSelections.begin(); it != mend; ++it) {
            it.value()->setWidget(0);
        }
        m_usedSelections.clear();
    }
}

void  FormWindow::Selection::clearSelectionPool()
{
    clear();
    qDeleteAll(m_selectionPool);
    m_selectionPool.clear();
}

WidgetSelection *FormWindow::Selection::addWidget(FormWindow* fw, QWidget *w)
{
    WidgetSelection *rc = m_usedSelections.value(w);
    if (rc != 0) {
        rc->show();
        return rc;
    }
    // find a free one in the pool
    const SelectionPool::iterator pend = m_selectionPool.end();
    for (SelectionPool::iterator it = m_selectionPool.begin(); it != pend; ++it) {
        if (! (*it)->isUsed()) {
            rc = *it;
            break;
        }
    }

    if (rc == 0) {
        rc = new WidgetSelection(fw);
        m_selectionPool.push_back(rc);
    }

    m_usedSelections.insert(w, rc);
    rc->setWidget(w);
    return rc;
}

QWidget* FormWindow::Selection::removeWidget(QWidget *w)
{
    WidgetSelection *s = m_usedSelections.value(w);
    if (!s)
        return w;

    s->setWidget(0);
    m_usedSelections.remove(w);

    if (m_usedSelections.isEmpty())
        return 0;

    return (*m_usedSelections.begin())->widget();
}

void FormWindow::Selection::repaintSelection(QWidget *w)
{
    if (WidgetSelection *s = m_usedSelections.value(w))
        s->update();
}

void FormWindow::Selection::repaintSelection()
{
    const SelectionHash::iterator mend = m_usedSelections.end();
    for (SelectionHash::iterator it = m_usedSelections.begin(); it != mend; ++it) {
        it.value()->update();
    }
}

bool FormWindow::Selection::isWidgetSelected(QWidget *w) const{
    return  m_usedSelections.contains(w);
}

QWidgetList FormWindow::Selection::selectedWidgets() const
{
    return m_usedSelections.keys();
}

void FormWindow::Selection::raiseList(const QWidgetList& l)
{
    const SelectionHash::iterator mend = m_usedSelections.end();
    for (SelectionHash::iterator it = m_usedSelections.begin(); it != mend; ++it) {
        WidgetSelection *w = it.value();
        if (l.contains(w->widget()))
            w->show();
    }
}

void FormWindow::Selection::raiseWidget(QWidget *w)
{
    if (WidgetSelection *s = m_usedSelections.value(w))
        s->show();
}

void FormWindow::Selection::updateGeometry(QWidget *w)
{
    if (WidgetSelection *s = m_usedSelections.value(w)) {
        s->updateGeometry();
    }
}

// ------------------------ FormWindow
FormWindow::FormWindow(FormEditor *core, QWidget *parent, Qt::WindowFlags flags)
    : FormWindowBase(parent, flags),
      m_core(core), m_selection(new Selection()), m_widgetStack(0), m_dblClicked(false)
{
    init();

    m_cursor = new FormWindowCursor(this, this);

    core->formWindowManager()->addFormWindow(this);

    setDirty(false);
    setAcceptDrops(true);
}

FormWindow::~FormWindow()
{
    Q_ASSERT(core() != 0);
    Q_ASSERT(core()->metaDataBase() != 0);
    Q_ASSERT(core()->formWindowManager() != 0);

    core()->formWindowManager()->removeFormWindow(this);
    core()->metaDataBase()->remove(this);

    QWidgetList l = widgets();
    foreach (QWidget *w, l)
        core()->metaDataBase()->remove(w);

    m_widgetStack = 0;
    m_rubberBand = 0;
    delete m_selection;
}

QDesignerFormEditorInterface *FormWindow::core() const
{
    return m_core;
}

QDesignerFormWindowCursorInterface *FormWindow::cursor() const
{
    return m_cursor;
}

void FormWindow::updateWidgets()
{
    if (!m_mainContainer)
        return;
}

int FormWindow::widgetDepth(const QWidget *w)
{
    int d = -1;
    while (w && !w->isWindow()) {
        d++;
        w = w->parentWidget();
    }

    return d;
}

bool FormWindow::isChildOf(const QWidget *c, const QWidget *p)
{
    while (c) {
        if (c == p)
            return true;
        c = c->parentWidget();
    }
    return false;
}

void FormWindow::setCursorToAll(const QCursor &c, QWidget *start)
{
    start->setCursor(c);
    const QWidgetList widgets = qFindChildren<QWidget*>(start);
    foreach (QWidget *widget, widgets) {
        if (!qobject_cast<WidgetHandle*>(widget)) {
            widget->setCursor(c);
        }
    }
}

void FormWindow::init()
{
    if (FormWindowManager *manager = qobject_cast<FormWindowManager*> (core()->formWindowManager())) {
        m_commandHistory = new QUndoStack(this);
        manager->undoGroup()->addStack(m_commandHistory);
    }

    m_blockSelectionChanged = false;

    m_defaultMargin = INT_MIN;
    m_defaultSpacing = INT_MIN;

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    m_widgetStack = new FormWindowWidgetStack(this);
    connect(m_widgetStack, SIGNAL(currentToolChanged(int)), this, SIGNAL(toolChanged(int)));
    layout->addWidget(m_widgetStack);

    m_selectionChangedTimer = new QTimer(this);
    m_selectionChangedTimer->setSingleShot(true);
    connect(m_selectionChangedTimer, SIGNAL(timeout()), this, SLOT(selectionChangedTimerDone()));

    m_checkSelectionTimer = new QTimer(this);
    m_checkSelectionTimer->setSingleShot(true);
    connect(m_checkSelectionTimer, SIGNAL(timeout()), this, SLOT(checkSelectionNow()));

    m_geometryChangedTimer = new QTimer(this);
    m_geometryChangedTimer->setSingleShot(true);
    connect(m_geometryChangedTimer, SIGNAL(timeout()), this, SIGNAL(geometryChanged()));

    m_rubberBand = 0;

    setFocusPolicy(Qt::StrongFocus);

    m_mainContainer = 0;
    m_currentWidget = 0;
    m_drawRubber = false;

    connect(m_commandHistory, SIGNAL(indexChanged(int)), this, SLOT(updateDirty()));
    connect(m_commandHistory, SIGNAL(indexChanged(int)), this, SIGNAL(changed()));
    connect(m_commandHistory, SIGNAL(indexChanged(int)), this, SLOT(checkSelection()));

    core()->metaDataBase()->add(this);

    initializeCoreTools();

    QAction *a = new QAction(this);
    a->setText(tr("Edit contents"));
    a->setShortcut(tr("F2"));
    connect(a, SIGNAL(triggered()), this, SLOT(editContents()));
    addAction(a);
}

QWidget *FormWindow::mainContainer() const
{
    return m_mainContainer;
}


void FormWindow::clearMainContainer()
{
    if (m_mainContainer) {
        setCurrentTool(0);
        core()->metaDataBase()->remove(m_mainContainer);
        unmanageWidget(m_mainContainer);
        delete m_mainContainer;
        m_mainContainer = 0;
    }
}

void FormWindow::setMainContainer(QWidget *w)
{
    if (w == m_mainContainer) {
        // nothing to do
        return;
    }

    clearMainContainer();

    m_mainContainer = w;
    const QSize sz = m_mainContainer->size();

    m_mainContainer->setParent(m_widgetStack, 0);
    m_mainContainer->raise();
    m_mainContainer->show();

    m_widgetStack->setCurrentTool(m_widgetEditor);

    setCurrentWidget(m_mainContainer);
    manageWidget(m_mainContainer);

    if (QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core()->extensionManager(), m_mainContainer)) {
        sheet->setVisible(sheet->indexOf(QLatin1String("windowTitle")), true);
        sheet->setVisible(sheet->indexOf(QLatin1String("windowIcon")), true);
        // ### generalize
    }

    m_mainContainer->setFocusPolicy(Qt::StrongFocus);
    m_mainContainer->resize(sz);

    emit mainContainerChanged(m_mainContainer);
}

QWidget *FormWindow::findTargetContainer(QWidget *widget) const
{
    Q_ASSERT(widget);

    while (QWidget *parentWidget = widget->parentWidget()) {
        if (LayoutInfo::layoutType(m_core, parentWidget) == LayoutInfo::NoLayout && isManaged(widget))
            return widget;

        widget = parentWidget;
    }

    return mainContainer();
}

bool FormWindow::handleMousePressEvent(QWidget * widget, QWidget *managedWidget, QMouseEvent *e)
{
    m_startPos = QPoint();
    e->accept();

    BlockSelection blocker(this);

    if (core()->formWindowManager()->activeFormWindow() != this)
        core()->formWindowManager()->setActiveFormWindow(this);

    if (e->buttons() != Qt::LeftButton)
        return true;

    m_startPos = mapFromGlobal(e->globalPos());

    const bool inLayout = LayoutInfo::isWidgetLaidout(m_core, managedWidget);

    const bool selected = isWidgetSelected(managedWidget);

    if (debugFormWindow)
        qDebug() << "handleMousePressEvent:" <<  widget << ',' << managedWidget << " inLayout=" << inLayout << " selected=" << selected;
    // if the dragged widget is not in a layout, raise it
    if (inLayout == false) {
        managedWidget->raise();
        if (selected)
            selectWidget(managedWidget, true);
    }

    if (isMainContainer(managedWidget) == true) { // press was on the formwindow
        clearSelection(false);

        m_drawRubber = true;
        m_currRect = QRect();
        startRectDraw(mapFromGlobal(e->globalPos()), this, Rubber);
        return true;
    }

    if (e->modifiers() & Qt::ShiftModifier) {
        // shift-click - toggle selection state of widget
        selectWidget(managedWidget, !selected);
        return true;
    }

    QWidget *current = managedWidget;

    if (!selected)
        clearSelection(false);

    selectWidget(current);
    raiseChildSelections(current);

    return true;
}

bool FormWindow::isPageOfContainerWidget(const QWidget *widget) const
{
    QDesignerContainerExtension *c = qt_extension<QDesignerContainerExtension*>(core()->extensionManager(),
                widget->parentWidget());

    if (c != 0) {
        for (int i = 0; i<c->count(); ++i) {
            if (widget == c->widget(i))
                return true;
        }
    }

    return false;
}

bool FormWindow::handleMouseMoveEvent(QWidget *, QWidget *, QMouseEvent *e)
{
    e->accept();

    if (e->buttons() != Qt::LeftButton || m_startPos.isNull())
        return true;

    const QPoint pos = mapFromGlobal(e->globalPos());

    if (m_drawRubber == true) {
        continueRectDraw(pos, this, Rubber);
        return true;
    }

    const bool canStartDrag = (m_startPos - pos).manhattanLength() > QApplication::startDragDistance();

    if (canStartDrag == false) {
        // nothing to do
        return true;
    }

    const bool blocked = blockSelectionChanged(true);

    QWidgetList sel = selectedWidgets();
    simplifySelection(&sel);

    QSet<QWidget*> widget_set;

    foreach (QWidget *child, sel) {
        QWidget *current = child;

        bool done = false;
        while (!isMainContainer(current) && !done) {
            if (!isManaged(current)) {
                current = current->parentWidget();
                continue;
            } else if (LayoutInfo::isWidgetLaidout(core(), current)) {
                current = current->parentWidget();
                continue;
            } else if (isPageOfContainerWidget(current)) {
                current = current->parentWidget();
                continue;
            } else if (current->parentWidget()) {
                QScrollArea *area = qobject_cast<QScrollArea*>(current->parentWidget()->parentWidget());
                if (area && area->widget() == current) {
                    current = area;
                    continue;
                }
            }

            done = true;
        }

        if (current == mainContainer())
            continue;

        widget_set.insert(current);
    }

    sel = widget_set.toList();
    QDesignerFormWindowCursorInterface *c = cursor();
    QWidget *current = c->current();
    if (sel.contains(current)) {
        sel.removeAll(current);
        sel.prepend(current);
    }

    QList<QDesignerDnDItemInterface*> item_list;
    const QPoint globalPos = mapToGlobal(m_startPos);
    const QDesignerDnDItemInterface::DropType dropType = (e->modifiers()
#ifndef Q_WS_MAC
        & Qt::ControlModifier)
#else
        & Qt::AltModifier)
#endif
        ? QDesignerDnDItemInterface::CopyDrop : QDesignerDnDItemInterface::MoveDrop;
    foreach (QWidget *widget, sel) {
        item_list.append(new FormWindowDnDItem(dropType,  this, widget, globalPos));
        if (dropType == QDesignerDnDItemInterface::MoveDrop)
            widget->hide();
        }

    blockSelectionChanged(blocked);

    if (sel.count())
        core()->formWindowManager()->dragItems(item_list);

    m_startPos = QPoint();

    return true;
}

bool FormWindow::handleMouseReleaseEvent(QWidget *w, QWidget *mw, QMouseEvent *e)
{
    if (m_dblClicked) {
        m_dblClicked = false;
        return true;
    }
    if (debugFormWindow)
        qDebug() << "handleMousePressEvent:" << w << ',' << mw;

    e->accept();

    if (m_drawRubber) { // we were drawing a rubber selection
        endRectDraw(); // get rid of the rectangle

        const bool blocked = blockSelectionChanged(true);
        selectWidgets(); // select widgets which intersect the rect
        blockSelectionChanged(blocked);

        m_drawRubber = false;
    }

    m_startPos = QPoint();

    emitSelectionChanged(); // inform about selection changes

    return true;
}

void FormWindow::checkPreviewGeometry(QRect &r)
{
    if (!rect().contains(r)) {
        if (r.left() < rect().left())
            r.moveTopLeft(QPoint(0, r.top()));
        if (r.right() > rect().right())
            r.moveBottomRight(QPoint(rect().right(), r.bottom()));
        if (r.top() < rect().top())
            r.moveTopLeft(QPoint(r.left(), rect().top()));
        if (r.bottom() > rect().bottom())
            r.moveBottomRight(QPoint(r.right(), rect().bottom()));
    }
}

void FormWindow::startRectDraw(const QPoint &pos, QWidget *, RectType t)
{
    m_rectAnchor = (t == Insert) ? designerGrid().snapPoint(pos) : pos;

    m_currRect = QRect(m_rectAnchor, QSize(0, 0));
    if (!m_rubberBand)
        m_rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
    m_rubberBand->setGeometry(m_currRect);
    m_rubberBand->show();
}

void FormWindow::continueRectDraw(const QPoint &pos, QWidget *, RectType t)
{
    const QPoint p2 = (t == Insert) ? designerGrid().snapPoint(pos) : pos;

    QRect r(m_rectAnchor, p2);
    r = r.normalized();

    if (m_currRect == r)
        return;

    if (r.width() > 1 || r.height() > 1) {
        m_currRect = r;
        if (m_rubberBand)
            m_rubberBand->setGeometry(m_currRect);
    }
}

void FormWindow::endRectDraw()
{
    if (m_rubberBand) {
        delete m_rubberBand;
        m_rubberBand = 0;
    }
}

QWidget *FormWindow::currentWidget() const
{
    return m_currentWidget;
}

bool FormWindow::setCurrentWidget(QWidget *currentWidget)
{
     if (debugFormWindow)
        qDebug() << "setCurrentWidget:" <<  m_currentWidget << " --> " << currentWidget;
     if (currentWidget == m_currentWidget)
         return false;
     // repaint the old widget unless it is the main window
     if (m_currentWidget && m_currentWidget != mainContainer()) {
         m_selection->repaintSelection(m_currentWidget);
     }
     // set new and repaint
     m_currentWidget = currentWidget;
     if (m_currentWidget && m_currentWidget != mainContainer()) {
         m_selection->repaintSelection(m_currentWidget);
     }
     return true;
}

QSize FormWindow::sizeHint() const
{
    QWidget *w = mainContainer();
    if (!w)
        return QSize(400, 300);

    QWidget *centralWidget = w;

    QMainWindow *mw = qobject_cast<QMainWindow*>(w);
    if (mw)
        centralWidget = mw->centralWidget();

    if (centralWidget->layout())
        return w->sizeHint();

    return w->sizeHint().expandedTo(QSize(400, 300));
}

void FormWindow::selectWidget(QWidget* w, bool select)
{
    if (trySelectWidget(w, select))
        emitSelectionChanged();
}

// Selects a widget and determines the new current one. Returns true if a change occurs.
bool FormWindow::trySelectWidget(QWidget *w, bool select)
{
    if (debugFormWindow)
        qDebug() << "trySelectWidget:" << w << select;
    if (!isManaged(w) && !isCentralWidget(w))
        return false;

    if (!select && !isWidgetSelected(w))
        return false;

    if (!mainContainer())
        return false;

    if (isMainContainer(w) || isCentralWidget(w)) {
        setCurrentWidget(mainContainer());
        return true;
    }

    if (select) {
        setCurrentWidget(w);
        m_selection->addWidget(this, w);
    } else {
        QWidget *newCurrent = m_selection->removeWidget(w);
        if (!newCurrent)
            newCurrent = mainContainer();
        setCurrentWidget(newCurrent);
    }
    return true;
}

void FormWindow::hideSelection(QWidget *w)
{
    selectWidget(w, false);
}

void FormWindow::clearSelection(bool changePropertyDisplay)
{
    if (debugFormWindow)
        qDebug() << "clearSelection(" <<  changePropertyDisplay << ')';
    // At all events, we need a current widget.
    m_selection->clear();
    setCurrentWidget(mainContainer());

    if (changePropertyDisplay)
        emitSelectionChanged();
}

void FormWindow::emitSelectionChanged()
{
    if (m_blockSelectionChanged == true) {
        // nothing to do
        return;
    }

    m_selectionChangedTimer->start(0);
}

void FormWindow::selectionChangedTimerDone()
{
    emit selectionChanged();
}

bool FormWindow::isWidgetSelected(QWidget *w) const
{
    return m_selection->isWidgetSelected(w);
}

bool FormWindow::isMainContainer(const QWidget *w) const
{
    return w && (w == this || w == mainContainer());
}

void FormWindow::updateChildSelections(QWidget *w)
{
    const QWidgetList l = qFindChildren<QWidget*>(w);

    QListIterator<QWidget*> it(l);
    while (it.hasNext()) {
        QWidget *w = it.next();
        if (isManaged(w)) {
            updateSelection(w);
        }
    }
}

void FormWindow::repaintSelection()
{
    m_selection->repaintSelection();
}

void FormWindow::raiseSelection(QWidget *w)
{
    m_selection->raiseWidget(w);
}

void FormWindow::updateSelection(QWidget *w)
{
    if (!w->isVisibleTo(this)) {
        selectWidget(w, false);
    } else {
        m_selection->updateGeometry(w);
    }
}

QWidget *FormWindow::designerWidget(QWidget *w) const
{
    while (w && !isMainContainer(w) && !isManaged(w) || isCentralWidget(w))
        w = w->parentWidget();

    return w;
}

bool FormWindow::isCentralWidget(QWidget *w) const
{
    if (QMainWindow *mainWindow = qobject_cast<QMainWindow*>(mainContainer()))
        return w == mainWindow->centralWidget();

    return false;
}

void FormWindow::ensureUniqueObjectName(QObject *object)
{
    QString name = object->objectName();
    if (name.isEmpty()) {
        QDesignerWidgetDataBaseInterface *db = core()->widgetDataBase();
        if (QDesignerWidgetDataBaseItemInterface *item = db->item(db->indexOfObject(object)))
            name = QDesignerResource::qtify(item->name());
    }

    unify(object, name, true);
    object->setObjectName(name);
}

template <class T, class P>
void merge(QList<T> *target, const QList<P> &source)
{
    foreach (P item, source) {
        target->append(item);
    }
}

bool FormWindow::unify(QObject *w, QString &s, bool changeIt)
{
    bool found = !isMainContainer(static_cast<QWidget*>(w)) && objectName() == s;

    if (!found) {
        QList<QObject*> objects;

        if (mainContainer()) {
            objects.append(mainContainer());
            merge(&objects, qFindChildren<QWidget*>(mainContainer()));
            merge(&objects, qFindChildren<QAction*>(mainContainer()));
        }

        QMutableListIterator<QObject*> mit(objects);
        while (mit.hasNext()) {
            if (!core()->metaDataBase()->item(mit.next())) {
                mit.remove();
            }
        }

        QListIterator<QObject*> it(objects);
        while (it.hasNext()) {
            QObject *child = it.next();

            if (child != w && child->objectName() == s) {
                found = true;

                if (!changeIt)
                    break;

                qlonglong num = 0;
                qlonglong factor = 1;
                int idx = s.length()-1;
                for ( ; (idx > 0) && s.at(idx).isDigit(); --idx) {
                    num += (s.at(idx).unicode() - QLatin1Char('0').unicode()) * factor;
                    factor *= 10;
                }

                if ((idx >= 0) && (QLatin1Char('_') == s.at(idx)))
                    s = s.left(idx+1) + QString::number(num+1);
                else
                    s = s + QLatin1String("_2");

                it.toFront();
            }
        }
    }

    return !found;
}

/* already_in_form is true when we are moving a widget from one parent to another inside the same
 * form. All this means is that InsertWidgetCommand::undo() must not unmanage it. */

void FormWindow::insertWidget(QWidget *w, const QRect &rect, QWidget *container, bool already_in_form)
{
    clearSelection(false);

    beginCommand(tr("Insert widget '%1").arg(QString::fromUtf8(w->metaObject()->className()))); // ### use the WidgetDatabaseItem

    /* Reparenting into a QSplitter automatically adjusts child's geometry. We create the geometry
     * command before we push the reparent command, so that the geometry command has the original
     * geometry of the widget. */
    QRect r = rect;
    Q_ASSERT(r.isValid());
    SetPropertyCommand *geom_cmd = new SetPropertyCommand(this);
    geom_cmd->init(w, QLatin1String("geometry"), r); // ### use rc.size()

    if (w->parentWidget() != container) {
        ReparentWidgetCommand *cmd = new ReparentWidgetCommand(this);
        cmd->init(w, container);
        m_commandHistory->push(cmd);
    }

    m_commandHistory->push(geom_cmd);

    InsertWidgetCommand *cmd = new InsertWidgetCommand(this);
    cmd->init(w, already_in_form);
    m_commandHistory->push(cmd);

    endCommand();

    w->show();
}

QWidget *FormWindow::createWidget(DomUI *ui, const QRect &rc, QWidget *target)
{
    QWidget *container = findContainer(target, false);
    if (!container)
        return 0;
    if (isMainContainer(container)) {
        if (QMainWindow *mw = qobject_cast<QMainWindow*>(container)) {
            Q_ASSERT(mw->centralWidget() != 0);
            container = mw->centralWidget();
        }
    }
    QDesignerResource resource(this);
    const QWidgetList widgets = resource.paste(ui, container);
    Q_ASSERT(widgets.size() == 1); // multiple-paste from DomUI not supported yet

    insertWidget(widgets.first(), rc, container);
    return widgets.first();
}

#ifndef QT_NO_DEBUG
static bool isDescendant(const QWidget *parent, const QWidget *child)
{
    for (; child != 0; child = child->parentWidget()) {
        if (child == parent)
            return true;
    }
    return false;
}
#endif

void FormWindow::resizeWidget(QWidget *widget, const QRect &geometry)
{
    Q_ASSERT(isDescendant(this, widget));

    QRect r = geometry;
    SetPropertyCommand *cmd = new SetPropertyCommand(this);
    cmd->init(widget, QLatin1String("geometry"), r);
    cmd->setText(tr("Resize"));
    m_commandHistory->push(cmd);
}

void FormWindow::raiseChildSelections(QWidget *w)
{
    const QWidgetList l = qFindChildren<QWidget*>(w);
    if (l.isEmpty())
        return;
    m_selection->raiseList(l);
}

QWidget *FormWindow::containerAt(const QPoint &pos, QWidget *notParentOf)
{
    QWidget *container = 0;
    int depth = -1;
    const QWidgetList selected = selectedWidgets();
    if (rect().contains(mapFromGlobal(pos))) {
        container = mainContainer();
        depth = widgetDepth(container);
    }

    QListIterator<QWidget*> it(m_widgets);
    while (it.hasNext()) {
        QWidget *wit = it.next();
        if (qobject_cast<QLayoutWidget*>(wit) || qobject_cast<QSplitter*>(wit))
            continue;
        if (!wit->isVisibleTo(this))
            continue;
        if (selected.indexOf(wit) != -1)
            continue;
        if (!core()->widgetDataBase()->isContainer(wit) &&
             wit != mainContainer())
            continue;

        // the rectangles of all ancestors of the container must contain the insert position
        QWidget *w = wit;
        while (w && !w->isWindow()) {
            if (!w->rect().contains((w->mapFromGlobal(pos))))
                break;
            w = w->parentWidget();
        }
        if (!(w == 0 || w->isWindow()))
            continue; // we did not get through the full while loop

        int wd = widgetDepth(wit);
        if (wd == depth && container) {
            if (wit->parentWidget()->children().indexOf(wit) >
                 container->parentWidget()->children().indexOf(container))
                wd++;
        }
        if (wd > depth && !isChildOf(wit, notParentOf)) {
            depth = wd;
            container = wit;
        }
    }
    return container;
}

QWidgetList FormWindow::selectedWidgets() const
{
    return m_selection->selectedWidgets();
}

void FormWindow::selectWidgets()
{
    bool selectionChanged = false;
    const QWidgetList l = qFindChildren<QWidget*>(mainContainer());
    QListIterator <QWidget*> it(l);
    const QRect selRect(mapToGlobal(m_currRect.topLeft()), m_currRect.size());
    while (it.hasNext()) {
        QWidget *w = it.next();
        if (w->isVisibleTo(this) && isManaged(w)) {
            const QPoint p = w->mapToGlobal(QPoint(0,0));
            const QRect r(p, w->size());
            if (r.intersects(selRect) && !r.contains(selRect) && trySelectWidget(w, true))
                selectionChanged = true;
        }
    }

    if (selectionChanged)
        emitSelectionChanged();
}

bool FormWindow::handleKeyPressEvent(QWidget *widget, QWidget *, QKeyEvent *e)
{
    if (qobject_cast<const FormWindow*>(widget) || qobject_cast<const QMenu*>(widget))
        return false;

    e->accept(); // we always accept!

    switch (e->key()) {
        default: break; // we don't care about the other keys

        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            deleteWidgets();
            break;

        case Qt::Key_Tab:
            cursor()->movePosition(QDesignerFormWindowCursorInterface::Next);
            break;

        case Qt::Key_Backtab:
            cursor()->movePosition(QDesignerFormWindowCursorInterface::Prev);
            break;

        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Up:
        case Qt::Key_Down:
            handleArrowKeyEvent(e->key(), e->modifiers());
            break;
    }

    return true;
}

int FormWindow::getValue(const QRect &rect, int key, bool size) const
{
    if (size) {
        if (key == Qt::Key_Left || key == Qt::Key_Right)
            return rect.width();
        return rect.height();
    }
    if (key == Qt::Key_Left || key == Qt::Key_Right)
        return rect.x();
    return rect.y();
}

int FormWindow::calcValue(int val, bool forward, bool snap, int snapOffset) const
{
    if (snap) {
        const int rest = val % snapOffset;
        if (rest) {
            const int offset = forward ? snapOffset : 0;
            const int newOffset = rest < 0 ? offset - snapOffset : offset;
            return val + newOffset - rest;
        }
        return (forward ? val + snapOffset : val - snapOffset);
    }
    return (forward ? val + 1 : val - 1);
}

QRect FormWindow::applyValue(const QRect &rect, int val, int key, bool size) const
{
    QRect r = rect;
    if (size) {
        if (key == Qt::Key_Left || key == Qt::Key_Right)
            r.setWidth(val);
        else
            r.setHeight(val);
    } else {
        if (key == Qt::Key_Left || key == Qt::Key_Right)
            r.moveLeft(val);
        else
            r.moveTop(val);
    }
    return r;
}

void FormWindow::handleArrowKeyEvent(int key, Qt::KeyboardModifiers modifiers)
{
    bool startMacro = false;
    QDesignerFormWindowCursorInterface *c = cursor();
    if (!c->hasSelection())
        return;

    QList<QWidget *> selection;

    // check if a laid out widget is selected
    for (int index = 0; index < c->selectedWidgetCount(); ++index) {
        QWidget *w = c->selectedWidget(index);
        if (!LayoutInfo::isWidgetLaidout(m_core, w))
            selection.append(w);
    }

    if (selection.isEmpty())
        return;

    QWidget *current = c->current();
    if (!current || LayoutInfo::isWidgetLaidout(m_core, current)) {
        current = selection.first();
    }

    const bool size = modifiers & Qt::ShiftModifier;

    const bool snap = !(modifiers & Qt::ControlModifier);
    const bool forward = (key == Qt::Key_Right || key == Qt::Key_Down);
    const int snapPoint = (key == Qt::Key_Left || key == Qt::Key_Right) ? grid().x() : grid().y();

    const int oldValue = getValue(current->geometry(), key, size);

    const int newValue = calcValue(oldValue, forward, snap, snapPoint);

    const int offset = newValue - oldValue;

    const int selCount = selection.count();
    // check if selection is the same as last time
    if (selCount != m_moveSelection.count() ||
        m_lastUndoIndex != m_commandHistory->index()) {
        m_moveSelection.clear();
        startMacro = true;
    } else {
        for (int index = 0; index < selCount; ++index) {
            if (m_moveSelection[index]->object() != selection.at(index)) {
                m_moveSelection.clear();
                startMacro = true;
                break;
            }
        }
    }

    if (startMacro)
        beginCommand(tr("Key Move"));

    for (int index = 0; index < selCount; ++index) {
        QWidget *w = selection.at(index);
        const QRect oldGeom = w->geometry();
        const QRect geom = applyValue(oldGeom, getValue(oldGeom, key, size) + offset, key, size);

        SetPropertyCommand *cmd = 0;

        if (m_moveSelection.count() > index)
            cmd = m_moveSelection[index];

        if (!cmd) {
            cmd = new SetPropertyCommand(this);
            cmd->init(w, QLatin1String("geometry"), geom);
            cmd->setText(tr("Key Move"));
            m_commandHistory->push(cmd);

            if (m_moveSelection.count() > index)
                m_moveSelection.replace(index, cmd);
            else
                m_moveSelection.append(cmd);
        } else {
            cmd->setNewValue(geom);
            cmd->redo();
        }
    }

    if (startMacro) {
        endCommand();
        m_lastUndoIndex = m_commandHistory->index();
    }
}

bool FormWindow::handleKeyReleaseEvent(QWidget *, QWidget *, QKeyEvent *e)
{
    e->accept();
    return true;
}

void FormWindow::selectAll()
{
    bool selectionChanged = false;
    foreach (QWidget *widget, m_widgets) {
        if (widget->isVisibleTo(this) && trySelectWidget(widget, true))
            selectionChanged = true;
    }
    if (selectionChanged)
        emitSelectionChanged();
}

void FormWindow::layoutHorizontal()
{
    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), selectedWidgets(), LayoutInfo::HBox);
    clearSelection(false);
    commandHistory()->push(cmd);
}

void FormWindow::layoutVertical()
{
    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), selectedWidgets(), LayoutInfo::VBox);
    clearSelection(false);
    commandHistory()->push(cmd);
}

void FormWindow::layoutGrid()
{
    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), selectedWidgets(), LayoutInfo::Grid);
    clearSelection(false);
    commandHistory()->push(cmd);
}

void FormWindow::deleteWidgets()
{
    QWidgetList selection = selectedWidgets();
    simplifySelection(&selection);

    deleteWidgetList(selection);
}

QString FormWindow::fileName() const
{
    return m_fileName;
}

void FormWindow::setFileName(const QString &fileName)
{
    if (m_fileName == fileName)
        return;

    m_fileName = fileName;
    emit fileNameChanged(fileName);
}

QString FormWindow::contents() const
{
    QBuffer b;
    if (!mainContainer() || !b.open(QIODevice::WriteOnly))
        return QString();

    QDesignerResource resource(const_cast<FormWindow*>(this));
    resource.save(&b, mainContainer());

    return QString::fromUtf8(b.buffer());
}

void FormWindow::copy()
{
    QBuffer b;
    if (!b.open(QIODevice::WriteOnly))
        return;

    QDesignerResource resource(this);
    QWidgetList sel = selectedWidgets();
    simplifySelection(&sel);
    resource.copy(&b, sel);

    qApp->clipboard()->setText(QString::fromUtf8(b.buffer()), QClipboard::Clipboard);
}

void FormWindow::cut()
{
    copy();
    deleteWidgets();
}

void FormWindow::paste()
{
    QWidget *w = mainContainer();
    const QWidgetList l(selectedWidgets());
    if (l.count() == 1) {
        w = l.first();
        w = m_core->widgetFactory()->containerOfWidget(w);
        if (LayoutInfo::layoutType(m_core, w) != LayoutInfo::NoLayout ||
             (!core()->widgetDataBase()->isContainer(w) &&
               w != mainContainer()))
            w = mainContainer();
    }

    if (w && LayoutInfo::layoutType(m_core, w) == LayoutInfo::NoLayout) {
        clearSelection(true);

        QByteArray code = qApp->clipboard()->text().toUtf8();
        QBuffer b(&code);
        b.open(QIODevice::ReadOnly);

        QDesignerResource resource(this);
        QWidget *widget = core()->widgetFactory()->containerOfWidget(w);
       const  QWidgetList widgets = resource.paste(&b, widget);

        beginCommand(tr("Paste"));
        foreach (QWidget *w, widgets) {
            InsertWidgetCommand *cmd = new InsertWidgetCommand(this);
            cmd->init(w);
            m_commandHistory->push(cmd);
            selectWidget(w);
        }
        endCommand();

        /* This will put the freshly pasted widgets into the clipboard, replacing the original.
         *  The point here is that the copied widgets are shifted a little with respect to the original.
         *  If the user presses paste again, the pasted widgets will be shifted again, rather than
         *  appearing on top of the previously pasted widgets. */
        copy();

    } else {
        QMessageBox::information(this, tr("Paste error"),
                                  tr("Can't paste widgets. Designer couldn't find a container\n"
                                      "to paste into which does not contain a layout. Break the layout\n"
                                      "of the container you want to paste into and select this container\n"
                                      "and then paste again."));
    }

}

// Draw a dotted frame around containers
bool FormWindow::frameNeeded(QWidget *w) const
{
    if (!core()->widgetDataBase()->isContainer(w))
        return false;
    if (qobject_cast<QGroupBox *>(w))
        return false;
    if (qobject_cast<QToolBox *>(w))
        return false;
    if (qobject_cast<QTabWidget *>(w))
        return false;
    if (qobject_cast<QStackedWidget *>(w))
        return false;
    if (qobject_cast<QDockWidget *>(w))
        return false;
    if (qobject_cast<QDesignerWidget *>(w))
        return false;
    if (qobject_cast<QMainWindow *>(w))
        return false;
    if (qobject_cast<QDialog *>(w))
        return false;
    if (qobject_cast<QLayoutWidget *>(w))
        return false;
    return true;
}

bool FormWindow::eventFilter(QObject *watched, QEvent *event)
{
    const bool ret = FormWindowBase::eventFilter(watched, event);
    if (event->type() != QEvent::Paint)
        return ret;

    Q_ASSERT(watched->isWidgetType());
    QWidget *w = static_cast<QWidget *>(watched);
    QPaintEvent *pe = static_cast<QPaintEvent*>(event);
    const QRect widgetRect = w->rect();
    const QRect paintRect =  pe->rect();
    // Does the paint rectangle touch the borders of the widget rectangle
    if (paintRect.x()     > widgetRect.x()     && paintRect.y()      > widgetRect.y() &&
        paintRect.right() < widgetRect.right() && paintRect.bottom() < paintRect.bottom())
        return ret;
    QPainter p(w);
    const QPen pen(QColor(0, 0, 0, 32), 0, Qt::DotLine);
    p.setPen(pen);
    p.setBrush(QBrush(Qt::NoBrush));
    p.drawRect(widgetRect.adjusted(0, 0, -1, -1));
    return ret;
}

void FormWindow::manageWidget(QWidget *w)
{
    if (isManaged(w))
        return;

    Q_ASSERT(qobject_cast<QMenu*>(w) == 0);

    if (w->hasFocus())
        setFocus();

    core()->metaDataBase()->add(w);

    m_insertedWidgets.insert(w);
    m_widgets.append(w);

    setCursorToAll(Qt::ArrowCursor, w);

    emit changed();
    emit widgetManaged(w);

    if (frameNeeded(w))
        w->installEventFilter(this);
}

void FormWindow::unmanageWidget(QWidget *w)
{
    if (!isManaged(w))
        return;

    m_selection->removeWidget(w);

    emit aboutToUnmanageWidget(w);

    if (w == m_currentWidget)
        setCurrentWidget(mainContainer());

    core()->metaDataBase()->remove(w);

    m_insertedWidgets.remove(w);
    m_widgets.removeAt(m_widgets.indexOf(w));

    emit changed();
    emit widgetUnmanaged(w);

    if (frameNeeded(w))
        w->removeEventFilter(this);
}

bool FormWindow::isManaged(QWidget *w) const
{
    return m_insertedWidgets.contains(w);
}

void FormWindow::breakLayout(QWidget *w)
{
    if (w == this)
        w = mainContainer();

    w = core()->widgetFactory()->containerOfWidget(w);

    beginCommand(tr("Break layout"));

    for (;;) {
        if (!w || w == this)
            break;

        if (LayoutInfo::layoutType(m_core, core()->widgetFactory()->containerOfWidget(w)) != LayoutInfo::NoLayout
                && core()->widgetDataBase()->isContainer(w, false)) {

            if (BreakLayoutCommand *cmd = breakLayoutCommand(w)) {
                commandHistory()->push(cmd);
                break;
            }
        }

        w = w->parentWidget();
    }

    clearSelection(false);
    endCommand();
}

BreakLayoutCommand *FormWindow::breakLayoutCommand(QWidget *w)
{
    QWidgetList widgets;

    QListIterator<QObject*> it(w->children());
    while (it.hasNext()) {
        QObject *obj = it.next();

        if (!obj->isWidgetType()
                || !core()->metaDataBase()->item(obj))
            continue;

        widgets.append(static_cast<QWidget*>(obj));
    }

    BreakLayoutCommand *cmd = new BreakLayoutCommand(this);
    cmd->init(widgets, core()->widgetFactory()->widgetOfContainer(w));
    return cmd;
}

void FormWindow::beginCommand(const QString &description)
{
    m_commandHistory->beginMacro(description);
}

void FormWindow::endCommand()
{
    m_commandHistory->endMacro();
}

void FormWindow::raiseWidgets()
{
    QWidgetList widgets = selectedWidgets();
    simplifySelection(&widgets);

    foreach (QWidget *widget, widgets) {
        widget->raise();
    }
}

void FormWindow::lowerWidgets()
{
    QWidgetList widgets = selectedWidgets();
    simplifySelection(&widgets);

    foreach (QWidget *widget, widgets) {
        widget->lower();
    }
}

bool FormWindow::handleMouseButtonDblClickEvent(QWidget *, QWidget *managedWidget, QMouseEvent *e)
{
    e->accept();

    emit activated(managedWidget);
    m_dblClicked = true;
    return true;
}


QMenu *FormWindow::initializePopupMenu(QWidget *managedWidget)
{
    if (!isManaged(managedWidget) || currentTool())
        return 0;

    // Make sure the managedWidget is selected and current since
    // the SetPropertyCommands must use the right reference
    // object obtained from the property editor for the property group
    // of a multiselection to be correct.
    const bool selected = isWidgetSelected(managedWidget);
    bool update = false;
    if (selected == false) {
        clearSelection(false);
        update = trySelectWidget(managedWidget, true);
        raiseChildSelections(managedWidget); // raise selections and select widget
    } else {
        update = setCurrentWidget(managedWidget);
    }

    if (update) {
        emitSelectionChanged();
        QMetaObject::invokeMethod(core()->formWindowManager(), "slotUpdateActions");
    }

    QWidget *contextMenuWidget = 0;

    if (isMainContainer(managedWidget)) { // press on a child widget
        contextMenuWidget = mainContainer();
    } else {  // press on a child widget
        // if widget is laid out, find the first non-laid out super-widget
        QWidget *realWidget = managedWidget; // but store the original one
        QMainWindow *mw = qobject_cast<QMainWindow*>(mainContainer());

        if (mw && mw->centralWidget() == realWidget) {
            contextMenuWidget = managedWidget;
        } else {
            contextMenuWidget = realWidget;
        }
    }

    if (!contextMenuWidget)
        return 0;

    QMenu *contextMenu = createPopupMenu(contextMenuWidget);
    if (!contextMenu)
        return 0;

    emit contextMenuRequested(contextMenu, contextMenuWidget);
    return contextMenu;
}

bool FormWindow::handleContextMenu(QWidget *, QWidget *managedWidget, QContextMenuEvent *e)
{
    QMenu *contextMenu = initializePopupMenu(managedWidget);
    if (!contextMenu)
        return false;
    contextMenu->exec(e->globalPos());
    delete contextMenu;
    e->accept();
    return true;
}

void FormWindow::setContents(QIODevice *dev)
{
    const bool saved = updatesEnabled();

    setUpdatesEnabled(false);
    clearSelection();
    m_selection->clearSelectionPool();
    m_insertedWidgets.clear();
    m_widgets.clear();
    // The main container is cleared as otherwise
    // the names of the newly loaded objects will be unified.
    clearMainContainer();
    emit changed();

    QDesignerResource r(this);
    QWidget *w = r.load(dev, this);
    setMainContainer(w);
    emit changed();

    setUpdatesEnabled(saved);
}

void FormWindow::setContents(const QString &contents)
{
    QByteArray data = contents.toUtf8();
    QBuffer b(&data);
    if (b.open(QIODevice::ReadOnly))
        setContents(&b);
}

void FormWindow::layoutHorizontalContainer(QWidget *w)
{
    if (w == this)
        w = mainContainer();

    w = core()->widgetFactory()->containerOfWidget(w);

    QList<QObject*> l = w->children();
    if (l.isEmpty())
        return;

    QWidgetList widgets;
    QListIterator<QObject*> it(l);
    while (it.hasNext()) {
        QObject* o = it.next();
        if (!o->isWidgetType())
            continue;

        QWidget *widget = static_cast<QWidget*>(o);
        if (widget->isVisibleTo(this) && isManaged(widget))
            widgets.append(widget);
    }

    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), widgets, LayoutInfo::HBox, w);
    clearSelection(false);
    commandHistory()->push(cmd);
}

void FormWindow::layoutVerticalContainer(QWidget *w)
{
    if (w == this)
        w = mainContainer();

    w = core()->widgetFactory()->containerOfWidget(w);

    const QList<QObject*> l = w->children();
    if (l.isEmpty())
        return;

    QListIterator<QObject*> it(l);
    QWidgetList widgets;
    while (it.hasNext()) {
        QObject* o = it.next();
        if (!o->isWidgetType())
            continue;

        QWidget *widget = static_cast<QWidget*>(o);
        if (widget->isVisibleTo(this) && isManaged(widget))
            widgets.append(widget);
    }

    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), widgets, LayoutInfo::VBox, w);
    clearSelection(false);
    commandHistory()->push(cmd);
}

void FormWindow::layoutGridContainer(QWidget *w)
{
    if (w == this)
        w = mainContainer();

    w = core()->widgetFactory()->containerOfWidget(w);

    const QList<QObject*> l = w->children();
    if (l.isEmpty())
        return;

    QWidgetList widgets;
    QListIterator<QObject*> it(l);
    while (it.hasNext()) {
        QObject* o = it.next();

        if (!o->isWidgetType())
            continue;

        QWidget *widget = static_cast<QWidget*>(o);
        if (widget->isVisibleTo(this) && isManaged(widget))
            widgets.append(widget);
    }

    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), widgets, LayoutInfo::Grid, w);
    clearSelection(false);
    commandHistory()->push(cmd);
}

bool FormWindow::hasInsertedChildren(QWidget *widget) const // ### move
{
    if (QDesignerContainerExtension *container = qt_extension<QDesignerContainerExtension*>(core()->extensionManager(), widget)) {
        widget = container->widget(container->currentIndex());
    }

    const QWidgetList l = widgets(widget);

    foreach (QWidget *child, l) {
        if (isManaged(child) && !LayoutInfo::isWidgetLaidout(core(), child) && child->isVisibleTo(const_cast<FormWindow*>(this)))
            return true;
    }

    return false;
}

void FormWindow::layoutHorizontalSplit()
{
    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), selectedWidgets(), LayoutInfo::HBox, /*layoutBase=*/ 0, /*splitter=*/ true);
    clearSelection(false);
    commandHistory()->push(cmd);
}

void FormWindow::layoutVerticalSplit()
{
    LayoutCommand *cmd = new LayoutCommand(this);
    cmd->init(mainContainer(), selectedWidgets(), LayoutInfo::VBox, /*layoutBase=*/ 0, /*splitter=*/ true);
    clearSelection(false);
    commandHistory()->push(cmd);
}

QMenu *FormWindow::createPopupMenu(QWidget *w)
{
    QMenu *popup = new QMenu;

    // Query extension
    if (const QDesignerTaskMenuExtension *taskMenu = qt_extension<QDesignerTaskMenuExtension*>(core()->extensionManager(), w)) {
        const QList<QAction *> acts = taskMenu->taskActions();
        if (!acts.empty()) {
            popup->addActions( acts);
            popup->addSeparator();
        }
    }

    QDesignerFormWindowManagerInterface *manager = core()->formWindowManager();
    const bool isFormWindow = qobject_cast<const FormWindow*>(w);

    // Check for special containers and obtain the page menu from them to add layout actions.
    if (!isFormWindow) {
        if (QDesignerStackedWidget *stackedWidget  = qobject_cast<QDesignerStackedWidget*>(w)) {
            stackedWidget->addContextMenuActions(popup);
        } else {
            if (QDesignerTabWidget *tabWidget = qobject_cast<QDesignerTabWidget*>(w)) {
                tabWidget->addContextMenuActions(popup);
            }  else {
                if (QDesignerToolBox *toolBox = qobject_cast<QDesignerToolBox*>(w)) {
                    toolBox->addContextMenuActions(popup);
                }
            }
        }

        popup->addAction(manager->actionCut());
        popup->addAction(manager->actionCopy());
    }

    popup->addAction(manager->actionPaste());
    popup->addAction(manager->actionSelectAll());

    if (!isFormWindow) {
        popup->addAction(manager->actionDelete());
    }

    popup->addSeparator();
    QMenu *layoutMenu = popup->addMenu(tr("Lay out"));
    layoutMenu->addAction(manager->actionAdjustSize());
    layoutMenu->addAction(manager->actionHorizontalLayout());
    layoutMenu->addAction(manager->actionVerticalLayout());
    layoutMenu->addAction(manager->actionGridLayout());

    if (!isFormWindow) {
        layoutMenu->addAction(manager->actionSplitHorizontal());
        layoutMenu->addAction(manager->actionSplitVertical());
    }
    layoutMenu->addAction(manager->actionBreakLayout());

    return popup;
}

void FormWindow::resizeEvent(QResizeEvent *e)
{
    m_geometryChangedTimer->start(10);

    QWidget::resizeEvent(e);
}

/*!
  Maps \a pos in \a w's coordinates to the form's coordinate system.

  This is the equivalent to mapFromGlobal(w->mapToGlobal(pos)) but
  avoids the two roundtrips to the X-Server on Unix/X11.
 */
QPoint FormWindow::mapToForm(const QWidget *w, const QPoint &pos) const
{
    QPoint p = pos;
    const QWidget* i = w;
    while (i && !i->isWindow() && !isMainContainer(i)) {
        p = i->mapToParent(p);
        i = i->parentWidget();
    }

    return mapFromGlobal(w->mapToGlobal(pos));
}

bool FormWindow::canBeBuddy(QWidget *w) const // ### rename me.
{
    if (QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(core()->extensionManager(), w)) {
        const int index = sheet->indexOf(QLatin1String("focusPolicy"));
        if (index != -1) {
            bool ok = false;
            const Qt::FocusPolicy q = static_cast<Qt::FocusPolicy>(Utils::valueOf(sheet->property(index), &ok));
            return ok && q != Qt::NoFocus;
        }
    }

    return false;
}

QWidget *FormWindow::findContainer(QWidget *w, bool excludeLayout) const
{
    if (!isChildOf(w, this)
        || const_cast<const QWidget *>(w) == this)
        return 0;

    QDesignerWidgetFactoryInterface *widgetFactory = core()->widgetFactory();
    QDesignerWidgetDataBaseInterface *widgetDataBase = core()->widgetDataBase();
    QDesignerMetaDataBaseInterface *metaDataBase = core()->metaDataBase();

    QWidget *container = widgetFactory->containerOfWidget(mainContainer()); // default parent for new widget is the formwindow
    if (!isMainContainer(w)) { // press was not on formwindow, check if we can find another parent
        while (w) {
            if (qobject_cast<InvisibleWidget*>(w) || !metaDataBase->item(w)) {
                w = w->parentWidget();
                continue;
            }

            bool isContainer =  widgetDataBase->isContainer(w, true) || w == mainContainer();

            if (!isContainer || (excludeLayout && qobject_cast<QLayoutWidget*>(w))) { // ### skip QSplitter
                w = w->parentWidget();
            } else {
                container = w;
                break;
            }
        }
    }

    return container;
}

void FormWindow::simplifySelection(QWidgetList *sel) const
{
    // Figure out which widgets should be removed from selection.
    // We want to remove those whose parent widget is also in the
    // selection (because the child widgets are contained by
    // their parent, they shouldn't be in the selection --
    // they are "implicitly" selected)
    QWidgetList toBeRemoved;
    QListIterator<QWidget*> it(*sel);
    while (it.hasNext()) {
        QWidget *child = it.next();
        QWidget *w = child;

        while (w->parentWidget() && sel->contains(w->parentWidget()))
            w = w->parentWidget();

        if (child != w)
            toBeRemoved.append(child);
    }
    // Now we can actually remove the widgets that were marked
    // for removal in the previous pass.
    while (!toBeRemoved.isEmpty())
        sel->removeAll(toBeRemoved.takeFirst());
}

FormWindow *FormWindow::findFormWindow(QWidget *w)
{
    return qobject_cast<FormWindow*>(QDesignerFormWindowInterface::findFormWindow(w));
}

bool FormWindow::isDirty() const
{
    return m_dirty;
}

void FormWindow::setDirty(bool dirty)
{
    m_dirty = dirty;

    if (!m_dirty)
        m_lastIndex = m_commandHistory->index();
}

void FormWindow::updateDirty()
{
    m_dirty = m_commandHistory->index() != m_lastIndex;
}

QWidget *FormWindow::containerAt(const QPoint &pos)
{
    QWidget *widget = widgetAt(pos);
    return findContainer(widget, true);
}

static QWidget *childAt_SkipDropLine(QWidget *w, QPoint pos)
{
    const QObjectList child_list = w->children();
    for (int i = child_list.size() - 1; i >= 0; --i) {
        QObject *child_obj = child_list[i];
        if (qobject_cast<WidgetHandle*>(child_obj) != 0)
            continue;
        QWidget *child = qobject_cast<QWidget*>(child_obj);
        if (!child || child->isWindow() || !child->isVisible() ||
                !child->geometry().contains(pos) || child->testAttribute(Qt::WA_TransparentForMouseEvents))
            continue;
        const QPoint childPos = child->mapFromParent(pos);
        if (QWidget *res = childAt_SkipDropLine(child, childPos))
            return res;
        if (child->testAttribute(Qt::WA_MouseNoMask) || child->mask().contains(pos)
                || child->mask().isEmpty())
            return child;
    }

    return 0;
}

QWidget *FormWindow::widgetAt(const QPoint &pos)
{
    QWidget *w = childAt(pos);
    if (qobject_cast<const WidgetHandle*>(w) != 0)
        w = childAt_SkipDropLine(this, pos);
    return w == 0 ? this : w;
}

void FormWindow::highlightWidget(QWidget *widget, const QPoint &pos, HighlightMode mode)
{
    Q_ASSERT(widget);

    if (QMainWindow *mainWindow = qobject_cast<QMainWindow*> (widget)) {
        widget = mainWindow->centralWidget();
    }

    QWidget *container = findContainer(widget, false);

    if (container == 0 || core()->metaDataBase()->item(container) == 0)
        return;

    if (QDesignerActionProviderExtension *g = qt_extension<QDesignerActionProviderExtension*>(core()->extensionManager(), container)) {
        if (mode == Restore) {
            g->adjustIndicator(QPoint());
        } else {
            const QPoint pt = widget->mapTo(container, pos);
            g->adjustIndicator(pt);
        }
    } else if (QDesignerLayoutDecorationExtension *g = qt_extension<QDesignerLayoutDecorationExtension*>(core()->extensionManager(), container)) {
        if (mode == Restore) {
            g->adjustIndicator(QPoint(), -1);
        } else {
            const QPoint pt = widget->mapTo(container, pos);
            const int index = g->findItemAt(pt);
            g->adjustIndicator(pt, index);
        }
    }

    QMainWindow *mw = qobject_cast<QMainWindow*> (container);
    if (container == mainContainer() || (mw && mw->centralWidget() && mw->centralWidget() == container))
        return;

    if (mode == Restore) {
        const PaletteAndFill paletteAndFill = m_palettesBeforeHighlight.take(container);
        container->setPalette(paletteAndFill.first);
        container->setAutoFillBackground(paletteAndFill.second);
    } else {
        QPalette p = container->palette();
        if (!m_palettesBeforeHighlight.contains(container)) {
            PaletteAndFill paletteAndFill;
            if (container->testAttribute(Qt::WA_SetPalette))
                paletteAndFill.first = p;
            paletteAndFill.second = container->autoFillBackground();
            m_palettesBeforeHighlight[container] = paletteAndFill;
        }

        p.setColor(backgroundRole(), p.midlight().color());
        container->setPalette(p);
        container->setAutoFillBackground(true);
    }
}

QList<QWidget *> FormWindow::widgets(QWidget *widget) const
{
    QList<QWidget *> l;

    foreach (QObject *o, widget->children()) {
        QWidget *w = qobject_cast<QWidget*>(o);
        if (w && isManaged(w))
            l.append(w);
    }

    return l;
}

int FormWindow::toolCount() const
{
    return m_widgetStack->count();
}

QDesignerFormWindowToolInterface *FormWindow::tool(int index) const
{
    return m_widgetStack->tool(index);
}

void FormWindow::registerTool(QDesignerFormWindowToolInterface *tool)
{
    Q_ASSERT(tool != 0);

    m_widgetStack->addTool(tool);

    if (m_mainContainer)
        m_mainContainer->update();
}

void FormWindow::setCurrentTool(int index)
{
    m_widgetStack->setCurrentTool(index);
}

int FormWindow::currentTool() const
{
    return m_widgetStack->currentIndex();
}

bool FormWindow::handleEvent(QWidget *widget, QWidget *managedWidget, QEvent *event)
{
    if (m_widgetStack == 0)
        return false;

    QDesignerFormWindowToolInterface *tool = m_widgetStack->currentTool();
    if (tool == 0)
        return false;

    return tool->handleEvent(widget, managedWidget, event);
}

void FormWindow::initializeCoreTools()
{
    m_widgetEditor = new WidgetEditorTool(this);
    registerTool(m_widgetEditor);
}

void FormWindow::checkSelection()
{
    m_checkSelectionTimer->start(0);
}

void FormWindow::checkSelectionNow()
{
    m_checkSelectionTimer->stop();

    foreach (QWidget *widget, selectedWidgets()) {
        updateSelection(widget);

        if (LayoutInfo::layoutType(core(), widget) != LayoutInfo::NoLayout)
            updateChildSelections(widget);
    }
}

QString FormWindow::author() const
{
    return m_author;
}

QString FormWindow::comment() const
{
     return m_comment;
}

void FormWindow::setAuthor(const QString &author)
{
    m_author = author;
}

void FormWindow::setComment(const QString &comment)
{
    m_comment = comment;
}

void FormWindow::editWidgets()
{
    m_widgetEditor->action()->trigger();
}

QStringList FormWindow::resourceFiles() const
{
    return m_resourceFiles;
}

void FormWindow::addResourceFile(const QString &path)
{
    if (!m_resourceFiles.contains(path)) {
        m_resourceFiles.append(path);
        setDirty(true);
        emit resourceFilesChanged();
    }
}

void FormWindow::removeResourceFile(const QString &path)
{
    if (m_resourceFiles.removeAll(path) > 0) {
        setDirty(true);
        emit resourceFilesChanged();
    }
}

bool FormWindow::blockSelectionChanged(bool b)
{
    const bool blocked = m_blockSelectionChanged;
    m_blockSelectionChanged = b;
    return blocked;
}

void FormWindow::editContents()
{
    const QWidgetList sel = selectedWidgets();
    if (sel.count() == 1) {
        QWidget *widget = sel.first();

        if (QDesignerTaskMenuExtension *taskMenu = qt_extension<QDesignerTaskMenuExtension*>(core()->extensionManager(), widget)) {
            if (QAction *a = taskMenu->preferredEditAction()) {
                a->trigger();
            }
        }
    }
}

bool FormWindow::dropWidgets(const QList<QDesignerDnDItemInterface*> &item_list, QWidget *target,
                             const QPoint &global_mouse_pos)
{
    beginCommand(tr("Drop widget"));

    QWidget *parent = target;
    if (parent == 0)
        parent = mainContainer();
    // You can only drop stuff onto the central widget of a QMainWindow
    // ### generalize to use container extension
    if (QMainWindow *main_win = qobject_cast<QMainWindow*>(target)) {
        const QPoint main_win_pos = main_win->mapFromGlobal(global_mouse_pos);
        const QRect central_wgt_geo = main_win->centralWidget()->geometry();
        if (!central_wgt_geo.contains(main_win_pos))
            return false;
    }

    QWidget *container = findContainer(parent, false);
    if (container == 0)
        return false;

    core()->formWindowManager()->setActiveFormWindow(this);
    mainContainer()->activateWindow();
    clearSelection(false);
    highlightWidget(target, target->mapFromGlobal(global_mouse_pos), FormWindow::Restore);


    QPoint offset;
    QDesignerDnDItemInterface *current = 0;
    QDesignerFormWindowCursorInterface *c = cursor();
    foreach (QDesignerDnDItemInterface *item, item_list) {
        QWidget *w = item->widget();
        if (!current)
            current = item;
        if (c->current() == w) {
            current = item;
            break;
        }
    }
    if (current) {
        QRect geom = current->decoration()->geometry();
        QPoint topLeft = container->mapFromGlobal(geom.topLeft());
        offset = designerGrid().snapPoint(topLeft) - topLeft;
    }

    foreach (QDesignerDnDItemInterface *item, item_list) {
        DomUI *dom_ui = item->domUi();
        QRect geometry = item->decoration()->geometry();
        Q_ASSERT(dom_ui != 0);

        geometry.moveTopLeft(container->mapFromGlobal(geometry.topLeft()) + offset);
        if (item->type() == QDesignerDnDItemInterface::CopyDrop) { // from widget box or CTRL + mouse move
            QWidget *widget = createWidget(dom_ui, geometry, parent);
            if (!widget)
                return false;
            selectWidget(widget, true);
            mainContainer()->setFocus(Qt::MouseFocusReason); // in case focus was in e.g. object inspector
        } else { // move
            QWidget *widget = item->widget();
            Q_ASSERT(widget != 0);
            QDesignerFormWindowInterface *dest = findFormWindow(widget);

            QDesignerLayoutDecorationExtension *deco = qt_extension<QDesignerLayoutDecorationExtension*>(core()->extensionManager(), container);
            if (dest == this) { // the same form
                if (deco == 0) { // into container without layout
                    parent = container;
                    if (parent != widget->parent()) { // different parent
                        ReparentWidgetCommand *cmd = new ReparentWidgetCommand(dest);
                        cmd->init(widget, parent);
                        commandHistory()->push(cmd);
                    }

                    resizeWidget(widget, geometry);
                    selectWidget(widget, true);
                    widget->show();
                } else { // into layout
                    insertWidget(widget, geometry, container, true);
                }
            } else { // from other form
                FormWindow *source = qobject_cast<FormWindow*>(item->source());
                Q_ASSERT(source != 0);

                source->deleteWidgetList(QWidgetList() << widget);
                QWidget *new_widget = createWidget(dom_ui, geometry, parent);

                selectWidget(new_widget, true);
            }
        }
    }

    endCommand();
    return true;
}

QDir FormWindow::absoluteDir() const
{
    if (fileName().isEmpty())
        return QDir::current();

    return QFileInfo(fileName()).absoluteDir();
}

void FormWindow::layoutDefault(int *margin, int *spacing)
{
    *margin = m_defaultMargin;
    *spacing = m_defaultSpacing;
}

void FormWindow::setLayoutDefault(int margin, int spacing)
{
    m_defaultMargin = margin;
    m_defaultSpacing = spacing;
}

void FormWindow::layoutFunction(QString *margin, QString *spacing)
{
    *margin = m_marginFunction;
    *spacing = m_spacingFunction;
}

void FormWindow::setLayoutFunction(const QString &margin, const QString &spacing)
{
    m_marginFunction = margin;
    m_spacingFunction = spacing;
}

QString FormWindow::pixmapFunction() const
{
    return m_pixmapFunction;
}

void FormWindow::setPixmapFunction(const QString &pixmapFunction)
{
    m_pixmapFunction = pixmapFunction;
}

QStringList FormWindow::includeHints() const
{
    return m_includeHints;
}

void FormWindow::setIncludeHints(const QStringList &includeHints)
{
    m_includeHints = includeHints;
}

QString FormWindow::exportMacro() const
{
    return m_exportMacro;
}

void FormWindow::setExportMacro(const QString &exportMacro)
{
    m_exportMacro = exportMacro;
}

} // namespace
