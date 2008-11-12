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
TRANSLATOR qdesigner_internal::QtBrushEditor
*/

#include "qtbrusheditor.h"
#include "qtbrushpatterndialog.h"
#include "qtgradientdialog.h"
#include "ui_qtbrusheditor.h"
#include "abstractbrushmanager.h"
#include <qitemdelegate.h>
#include <qlineedit.h>
#include <qvalidator.h>
#include <QContextMenuEvent>
#include <QMenu>

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtBrushEditorPrivate
{
    QtBrushEditor *q_ptr;
    Q_DECLARE_PUBLIC(QtBrushEditor)
public:
    void slotPatternChooserClicked();
    void slotTextureChooserClicked();
    void slotGradientChooserClicked();
    void slotChooserClicked();
    void slotApplyClicked();
    void slotAddToCustomClicked();
    void slotRemoveClicked();
    void slotItemActivated(QListWidgetItem *item);
    void slotCurrentItemChanged(QListWidgetItem *item);
    void slotItemRenamed(QListWidgetItem *item);
    void slotBrushAdded(const QString &name, const QBrush &brush);
    void slotBrushRemoved(const QString &name);
    void slotCurrentBrushChanged(const QString &name, const QBrush &brush);

    QBrush m_brush;
    QDesignerBrushManagerInterface *m_brushManager;

    QMap<QString, QListWidgetItem *> m_brushToItem;
    QMap<QListWidgetItem *, QString> m_itemToBrush;

    Ui::QtBrushEditor m_ui;
};

}

void QtBrushEditorPrivate::slotTextureChooserClicked()
{
    QBrush br;
    if (m_brush.style() == Qt::TexturePattern)
        br = m_brush;
    emit q_ptr->textureChooserActivated(m_ui.textureToolButton, br);
}

void QtBrushEditorPrivate::slotPatternChooserClicked()
{
    QBrush br;
    if (m_brush.style() != Qt::LinearGradientPattern &&
            m_brush.style() != Qt::RadialGradientPattern &&
            m_brush.style() != Qt::ConicalGradientPattern &&
            m_brush.style() != Qt::TexturePattern) {
        br = m_brush;
    } else {
        br = QBrush(QColor::fromRgb(162, 197, 17));
    }
    QtBrushPatternDialog dlg(m_ui.patternToolButton);
    dlg.setBrush(br);
    if (dlg.exec() == QDialog::Accepted) {
        q_ptr->setBrush(dlg.brush());
    }
}

void QtBrushEditorPrivate::slotGradientChooserClicked()
{
    QtGradientDialog dlg(m_ui.gradientToolButton);
    const QGradient *gradient = m_ui.brushWidget->brush().gradient();
    if (gradient)
        dlg.setGradient(*gradient);
    else {
        QLinearGradient gr(0, 0, 1, 0);
        gr.setColorAt(0, Qt::red);
        gr.setColorAt(1.0 / 6, Qt::yellow);
        gr.setColorAt(2.0 / 6, Qt::green);
        gr.setColorAt(3.0 / 6, Qt::cyan);
        gr.setColorAt(4.0 / 6, Qt::blue);
        gr.setColorAt(5.0 / 6, Qt::magenta);
        gr.setColorAt(1, Qt::red);
        dlg.setGradient(gr);
    }
    if (dlg.exec() == QDialog::Accepted) {
        QBrush brush(dlg.gradient());
        q_ptr->setBrush(brush);
    }
}

void QtBrushEditorPrivate::slotChooserClicked()
{
    if (m_brush.style() == Qt::LinearGradientPattern ||
            m_brush.style() == Qt::RadialGradientPattern ||
            m_brush.style() == Qt::ConicalGradientPattern) {
        m_ui.gradientToolButton->animateClick();
    } else if (m_brush.style() == Qt::TexturePattern) {
        m_ui.textureToolButton->animateClick();
    } else {
        m_ui.patternToolButton->animateClick();
    }
}

void QtBrushEditorPrivate::slotApplyClicked()
{
    QString name = m_brushManager->currentBrush();
    if (name.isNull())
        return;

    QBrush brush = m_brushManager->brush(name);
    q_ptr->setBrush(brush);
}

void QtBrushEditorPrivate::slotAddToCustomClicked()
{
    QString name = m_brushManager->addBrush(QApplication::translate("qdesigner_internal::QtBrushEditor", "New Brush", 0, QApplication::UnicodeUTF8), m_brush);
    m_brushManager->setCurrentBrush(name);

    if (m_brushToItem.contains(name))
        m_ui.listWidget->editItem(m_brushToItem[name]);
}

void QtBrushEditorPrivate::slotRemoveClicked()
{
    QString name = m_brushManager->currentBrush();
    if (name.isNull())
        return;

    QString newCurrent;

    QMap<QString, QBrush> brushes = m_brushManager->brushes();
    QMap<QString, QBrush>::ConstIterator it = brushes.constFind(name);
    if (it == brushes.constEnd())
        return; // not possible

    QMap<QString, QBrush>::ConstIterator itCurrent = it;
    it++;
    if (it == brushes.constEnd()) {
        if (itCurrent != brushes.constBegin())
            newCurrent = (--itCurrent).key();
    } else {
        newCurrent = it.key();
    }

    m_brushManager->removeBrush(name);
    m_brushManager->setCurrentBrush(newCurrent);
}

void QtBrushEditorPrivate::slotItemActivated(QListWidgetItem *item)
{
    if (!m_itemToBrush.contains(item))
        return;

    QString name = m_itemToBrush[item];
    QBrush brush = m_brushManager->brush(name);
    q_ptr->setBrush(brush);
}

void QtBrushEditorPrivate::slotCurrentItemChanged(QListWidgetItem *item)
{
    QString newCurrent;
    if (item) {
        if (!m_itemToBrush.contains(item))
            return;
        newCurrent = m_itemToBrush[item];
    }
    m_brushManager->setCurrentBrush(newCurrent);
}

void QtBrushEditorPrivate::slotItemRenamed(QListWidgetItem *item)
{
    if (!m_itemToBrush.contains(item))
        return;

    QString oldName = m_itemToBrush[item];
    QString newName = item->text();
    if (newName == oldName)
        return;

    QBrush brush = m_brushManager->brush(oldName);
    m_brushManager->removeBrush(oldName);
    QString name = m_brushManager->addBrush(newName, brush);
    m_brushManager->setCurrentBrush(name);
}

void QtBrushEditorPrivate::slotBrushAdded(const QString &name, const QBrush &brush)
{
    QListWidgetItem *item = new QListWidgetItem(m_brushManager->brushPixmap(brush), name, m_ui.listWidget);
    item->setSizeHint(QSize(72, 84));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_itemToBrush[item] = name;
    m_brushToItem[name] = item;
    m_ui.listWidget->sortItems();
}

void QtBrushEditorPrivate::slotBrushRemoved(const QString &name)
{
    if (!m_brushToItem.contains(name))
        return;
    QListWidgetItem *item = m_brushToItem[name];
    delete item;
    m_brushToItem.remove(name);
    m_itemToBrush.remove(item);
}

void QtBrushEditorPrivate::slotCurrentBrushChanged(const QString &name, const QBrush &brush)
{
    Q_UNUSED(brush)
    if (name.isNull()) {
        m_ui.listWidget->clearSelection();
        m_ui.applyButton->setEnabled(false);
        m_ui.removeButton->setEnabled(false);
    }
    if (!m_brushToItem.contains(name))
        return;
    QListWidgetItem *item = m_brushToItem[name];
    m_ui.listWidget->setCurrentItem(item);
    m_ui.applyButton->setEnabled(true);
    m_ui.removeButton->setEnabled(true);
}

class QtBrushDelegate : public QItemDelegate
{
public:
    QtBrushDelegate(QObject *parent = 0)
        : QItemDelegate(parent) {}
    virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

QWidget *QtBrushDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    QLineEdit *le = new QLineEdit(parent);
    le->setFrame(false);
    QRegExpValidator *validator = new QRegExpValidator(QRegExp(QLatin1String("([A-Za-z_])([A-Za-z_ 0-9]*)")), le);
    le->setValidator(validator);
    return le;
}


QtBrushEditor::QtBrushEditor(QWidget *parent)
    : QWidget(parent)
{
    d_ptr = new QtBrushEditorPrivate();
    d_ptr->q_ptr = this;
    d_ptr->m_ui.setupUi(this);

    d_ptr->m_brushManager = 0;

    connect(d_ptr->m_ui.patternToolButton, SIGNAL(clicked()),
                this, SLOT(slotPatternChooserClicked()));
    connect(d_ptr->m_ui.textureToolButton, SIGNAL(clicked()),
                this, SLOT(slotTextureChooserClicked()));
    connect(d_ptr->m_ui.gradientToolButton, SIGNAL(clicked()),
                this, SLOT(slotGradientChooserClicked()));
    connect(d_ptr->m_ui.editToolButton, SIGNAL(clicked()),
                this, SLOT(slotChooserClicked()));

    d_ptr->m_ui.listWidget->setViewMode(QListView::IconMode);
    d_ptr->m_ui.listWidget->setMovement(QListView::Static);
    d_ptr->m_ui.listWidget->setTextElideMode(Qt::ElideRight);
    d_ptr->m_ui.listWidget->setResizeMode(QListWidget::Adjust);
    //d_ptr->m_ui.listWidget->setSpacing(2);
    d_ptr->m_ui.listWidget->setIconSize(QSize(64, 64));
    d_ptr->m_ui.listWidget->setItemDelegate(new QtBrushDelegate(this));
    d_ptr->m_ui.listWidget->setEditTriggers(QAbstractItemView::EditKeyPressed);

    connect(d_ptr->m_ui.listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem *)),
                this, SLOT(slotItemActivated(QListWidgetItem *)));
    connect(d_ptr->m_ui.listWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
                this, SLOT(slotCurrentItemChanged(QListWidgetItem *)));
    connect(d_ptr->m_ui.listWidget, SIGNAL(itemChanged(QListWidgetItem *)),
                this, SLOT(slotItemRenamed(QListWidgetItem *)));

    d_ptr->m_ui.applyButton->setEnabled(false);
    d_ptr->m_ui.addToCustomButton->setEnabled(false);
    d_ptr->m_ui.removeButton->setEnabled(false);

    connect(d_ptr->m_ui.applyButton, SIGNAL(clicked()),
                this, SLOT(slotApplyClicked()));
    connect(d_ptr->m_ui.addToCustomButton, SIGNAL(clicked()),
                this, SLOT(slotAddToCustomClicked()));
    connect(d_ptr->m_ui.removeButton, SIGNAL(clicked()),
                this, SLOT(slotRemoveClicked()));

    d_ptr->m_ui.applyButton->setIcon(QIcon(QLatin1String(":/qtgradienteditor/images/up.png")));
    d_ptr->m_ui.addToCustomButton->setIcon(QIcon(QLatin1String(":/qtgradienteditor/images/down.png")));
    d_ptr->m_ui.removeButton->setIcon(QIcon(QLatin1String(":/qtgradienteditor/images/editdelete.png")));
}

QtBrushEditor::~QtBrushEditor()
{
    delete d_ptr;
}

void QtBrushEditor::setBrush(const QBrush &brush)
{
    if (d_ptr->m_brush == brush)
        return;

    d_ptr->m_brush = brush;
    d_ptr->m_ui.brushWidget->setBrush(brush);

    QString currentType;

    if (d_ptr->m_brush.style() == Qt::LinearGradientPattern ||
            d_ptr->m_brush.style() == Qt::RadialGradientPattern ||
            d_ptr->m_brush.style() == Qt::ConicalGradientPattern) {
        currentType = tr("Gradient");
    } else if (d_ptr->m_brush.style() == Qt::TexturePattern) {
        currentType = tr("Texture");
    } else {
        currentType = tr("Color Pattern");
    }

//    QString preview = tr("%1 Preview").arg(currentType);
//    d_ptr->m_ui.previewBox->setTitle(preview);
}

QBrush QtBrushEditor::brush() const
{
    return d_ptr->m_brush;
}

void QtBrushEditor::setBrushManager(QDesignerBrushManagerInterface *manager)
{
    if (d_ptr->m_brushManager == manager)
        return;

    if (d_ptr->m_brushManager) {
        disconnect(d_ptr->m_brushManager, SIGNAL(brushAdded(const QString &, const QBrush &)),
                    this, SLOT(slotBrushAdded(const QString &, const QBrush &)));
        disconnect(d_ptr->m_brushManager, SIGNAL(brushRemoved(const QString &)),
                    this, SLOT(slotBrushRemoved(const QString &)));
        disconnect(d_ptr->m_brushManager, SIGNAL(currentBrushChanged(const QString &, const QBrush &)),
                    this, SLOT(slotCurrentBrushChanged(const QString &, const QBrush &)));
    }
    d_ptr->m_ui.listWidget->clear();
    d_ptr->m_itemToBrush.clear();
    d_ptr->m_brushToItem.clear();

    d_ptr->m_brushManager = manager;

    d_ptr->m_ui.applyButton->setEnabled(false);
    d_ptr->m_ui.removeButton->setEnabled(false);
    if (!d_ptr->m_brushManager) {
        d_ptr->m_ui.addToCustomButton->setEnabled(false);
        return;
    }

    d_ptr->m_ui.addToCustomButton->setEnabled(true);
    QMap<QString, QBrush> brushes = d_ptr->m_brushManager->brushes();
    QMap<QString, QBrush>::ConstIterator it = brushes.constBegin();
    while (it != brushes.constEnd()) {
        d_ptr->slotBrushAdded(it.key(), it.value());

        it++;
    }
    QString current = d_ptr->m_brushManager->currentBrush();
    d_ptr->slotCurrentBrushChanged(current, d_ptr->m_brushManager->brush(current));

    connect(d_ptr->m_brushManager, SIGNAL(brushAdded(const QString &, const QBrush &)),
            this, SLOT(slotBrushAdded(const QString &, const QBrush &)));
    connect(d_ptr->m_brushManager, SIGNAL(brushRemoved(const QString &)),
            this, SLOT(slotBrushRemoved(const QString &)));
    connect(d_ptr->m_brushManager, SIGNAL(currentBrushChanged(const QString &, const QBrush &)),
            this, SLOT(slotCurrentBrushChanged(const QString &, const QBrush &)));

}

void QtBrushEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QPoint p = d_ptr->m_ui.listWidget->mapFromGlobal(e->globalPos());
    if (!d_ptr->m_ui.listWidget->rect().contains(p))
        return;

    QListWidgetItem *item = d_ptr->m_ui.listWidget->itemAt(p);
    if (!item)
        return;

    QMenu menu(d_ptr->m_ui.listWidget);
    QAction *renameAction = new QAction(tr("Rename\tF2"), &menu);
    menu.addAction(renameAction);
    if (menu.exec(e->globalPos(), renameAction) == renameAction)
        d_ptr->m_ui.listWidget->editItem(item);
}


#include "moc_qtbrusheditor.cpp"
