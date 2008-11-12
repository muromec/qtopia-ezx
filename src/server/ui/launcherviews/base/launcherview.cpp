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

#include "launcherview.h"
#include <QtopiaApplication>
#include <QResizeEvent>
#include <QSoftMenuBar>
#include <QtopiaServiceRequest>
#include <QtopiaServiceDescription>
#include <QMenu>
#include <QDesktopWidget>
#include <QSpeedDial>
#include <QPainter>
#include <QSet>
#include <QPixmap>
#include <QtopiaItemDelegate>

#include <QContentFilter>
#include <QKeyEvent>
#include <QAbstractProxyModel>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QMimeType>

////////////////////////////////////////////////////////////////
//
// LauncherViewListView implementation

void LauncherViewListView::currentChanged( const QModelIndex &current, const QModelIndex &previous ) {
    QListView::currentChanged( current, previous );
    emit currentIndexChanged( current, previous );
}

void LauncherViewListView::focusOutEvent(QFocusEvent *)
{
    // Don't need an update.
}

void LauncherViewListView::focusInEvent(QFocusEvent *)
{
    if (!Qtopia::mousePreferred())
        ensureSelected();
    // Don't need an update.
}

bool LauncherViewListView::viewportEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
        // suppress unneeded viewport update
        return true;
    default:
        break;
    }

    return QListView::viewportEvent(e);
}

void LauncherViewListView::rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    QModelIndex index = currentIndex();
    
    int scrollValue = verticalScrollBar()->value();

    QListView::rowsAboutToBeRemoved(parent, start, end);

    if (index.row() >= start && index.row() <= end && end + 1 < model()->rowCount(parent)) {
        selectionModel()->setCurrentIndex(
                model()->index( end + 1, index.column(), parent ),
                QItemSelectionModel::ClearAndSelect);
    }

    if (index.row() >= start) {
        int adjustedValue = index.row() > end
                ? scrollValue - end + start - 1
                : scrollValue - index.row() + start;

        verticalScrollBar()->setValue(adjustedValue > 0 ? adjustedValue : 0);
    }
}

void LauncherViewListView::rowsInserted(const QModelIndex &parent, int start, int end)
{
    QListView::rowsInserted(parent, start, end);
    if (!Qtopia::mousePreferred())
        ensureSelected();
    else
        selectionModel()->clearSelection();

    scrollTo(currentIndex());
}

// Makes sure that an item is selected.
void LauncherViewListView::ensureSelected()
{
    if (selectionModel() && !currentIndex().isValid()) {
        selectionModel()->setCurrentIndex(
                moveCursor(MoveHome, Qt::NoModifier), // first visible index
                QItemSelectionModel::ClearAndSelect);
    }
}

//===========================================================================

QVariant QLauncherProxyModel::data ( const QModelIndex & index, int role ) const
{
    if (role == QLauncherProxyModel::BusyRole)
        return QVariant(hasBusy() && items.contains(index));
    else
        return QSortFilterProxyModel::data(index, role);
}

void QLauncherProxyModel::setBusyItem(const QModelIndex& index)
{
    clearBusyItems();
    items.append(index);
    emit dataChanged(index, index);
}

void QLauncherProxyModel::clearBusyItems()
{
    QList<QModelIndex> olditems=items;
    items.clear();
    foreach(QModelIndex item, olditems)
        emit dataChanged(item, item);
}

void QLauncherProxyModel::setSorting(LauncherView::SortingStyle style)
{
    sortingStyle=style;
    if(style==LauncherView::LanguageAwareSorting)
    {
        sort(0);
        setDynamicSortFilter(true);
    }
    else
        setDynamicSortFilter(false);
}

//===========================================================================

class LauncherViewDelegate : public QtopiaItemDelegate
{
public:
    LauncherViewDelegate(QObject *parent=0)
        : QtopiaItemDelegate(parent)
    {
    }

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index) const
    {
        QtopiaItemDelegate::paint(painter, opt, index);

        QVariant value = index.data(QLauncherProxyModel::BusyRole);
        if (value.toBool()) {
            int size = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
            QPixmap pm = QIcon(":icon/wait").pixmap(size,size);
            painter->drawPixmap(opt.rect.right()-size, opt.rect.top()+((opt.rect.height() - size))/2, pm);
        }
    }
};


////////////////////////////////////////////////////////////////
//
// LauncherView implementation
#include <QStringListModel>
LauncherView::LauncherView( QWidget* parent, Qt::WFlags fl )
    : QWidget(parent, fl)
        , icons(NULL)
#ifdef ENABLE_SMOOTHLIST
        , smoothicons(NULL)
#endif
        , contentSet(NULL)
        , model(NULL)
        , nColumns(1)
        , mainLayout(NULL)
        , mNeedGridSize(false)
{
    init();
}

void LauncherView::init() {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(2);

    icons = new LauncherViewListView(this);
    icons->setItemDelegate(new LauncherViewDelegate(icons));
    mainLayout->addWidget(icons);
    setFocusProxy(icons);
    
#ifdef ENABLE_SMOOTHLIST
    icons->setVisible(false);
    smoothicons = new QSmoothList(this);
    smoothicons->setItemDelegate(new LauncherViewDelegate(smoothicons));
    mainLayout->addWidget(smoothicons);
    setFocusProxy(smoothicons);
    QSoftMenuBar::setLabel(smoothicons, Qt::Key_Select, QSoftMenuBar::Select);
#endif
    
    QtopiaApplication::setStylusOperation( icons->viewport(), QtopiaApplication::RightOnHold );

    icons->setFrameStyle( QFrame::NoFrame );
    icons->setResizeMode( QListView::Fixed );
    icons->setSelectionMode( QAbstractItemView::SingleSelection );
    icons->setSelectionBehavior( QAbstractItemView::SelectItems );
    icons->setUniformItemSizes( true );
//    icons->setWordWrap( true );

    contentSet = new QContentSet( QContentSet::Asynchronous, this);
    contentSet->setSortOrder(QStringList() << "name");
    model = new QContentSetModel(contentSet, this);

//    setViewMode(QListView::IconMode);
    setViewMode(QListView::ListMode);

    connect( icons, SIGNAL(clicked(QModelIndex)),
             SLOT(itemClicked(QModelIndex)));
    connect( icons, SIGNAL(activated(QModelIndex)),
             SLOT(returnPressed(QModelIndex)) );
    connect( icons, SIGNAL(pressed(QModelIndex)),
             SLOT(itemPressed(QModelIndex)));

    icons->setModel(model);
    
#ifdef ENABLE_SMOOTHLIST
    connect( smoothicons, SIGNAL(activated(QModelIndex)),
             SLOT(returnPressed(QModelIndex)) );
    smoothicons->setModel(model);
#endif
}

void LauncherView::setBusy(bool on)
{
    setBusy(icons->currentIndex(), on);
}

void LauncherView::setBusy(const QModelIndex &index, bool on)
{
    Q_UNUSED(index);
    Q_UNUSED(on);
/*
    // Enable this code to display a wait icon next to the busy item.
    if ( on )
        bpModel->setBusyItem(index);
    else
        bpModel->clearBusyItems();
*/
}

void LauncherView::timerEvent ( QTimerEvent * event )
{
    QWidget::timerEvent( event );
}

void LauncherView::setItemDelegate(QAbstractItemDelegate *delegate)
{
    icons->setItemDelegate(delegate);
#ifdef ENABLE_SMOOTHLIST
    smoothicons->setItemDelegate(delegate);
#endif
}

void LauncherView::setViewMode( QListView::ViewMode m )
{
    Q_ASSERT(icons);
    if(m==QListView::ListMode) {
        icons->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifdef ENABLE_SMOOTHLIST
        icons->setVisible(false);
        smoothicons->setVisible(true);
        setFocusProxy(smoothicons);
#endif
    } else {
        icons->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
#ifdef ENABLE_SMOOTHLIST
        icons->setVisible(true);
        smoothicons->setVisible(false);
        setFocusProxy(icons);
#endif
    }
    icons->setViewMode(m);
    calculateGridSize();
}

QListView::ViewMode LauncherView::viewMode() const
{
    Q_ASSERT(icons);
    return icons->viewMode();
}

void LauncherView::removeAllItems()
{
    contentSet->clear();
}

void LauncherView::addItem(QContent* app, bool resort)
{
    Q_UNUSED(resort);
    if(app == NULL)
        return;
    contentSet->add( *app );

}

void LauncherView::removeItem(const QContent &app) {
    contentSet->remove(app);
}

void LauncherView::handleReturnPressed(const QModelIndex &item) {
    emit clicked(model->content(item));
}

void LauncherView::handleItemClicked(const QModelIndex & item, bool setCurrentIndex)
{
    if(QApplication::mouseButtons () == Qt::LeftButton) {
        if (setCurrentIndex)
            icons->setCurrentIndex( item );
        if (Qtopia::mousePreferred())
            icons->selectionModel()->clearSelection();
        emit clicked(model->content(item));
    }
}

void LauncherView::handleItemPressed(const QModelIndex & index)
{
    if(QApplication::mouseButtons () & Qt::RightButton)
    {
        icons->setCurrentIndex( index );
        emit rightPressed(model->content(index));
    }
}

void LauncherView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent( e );
    calculateGridSize();
}

void LauncherView::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::PaletteChange) {
        QPalette pal(palette());
        //pal.setColor(QPalette::Text, textCol);    // todo later, include text color setting. Not necessary for now.
        icons->setPalette(pal);
    }
    if (e->type() == QEvent::StyleChange)
        calculateGridSize();
    QWidget::changeEvent(e);
}

void LauncherView::showEvent(QShowEvent *e)
{
    if(mNeedGridSize)
        calculateGridSize(true);

    QWidget::showEvent(e);
}

void LauncherView::setColumns(int columns)
{
    nColumns = columns;
    calculateGridSize();
}

void LauncherView::setFilter(const QContentFilter &filter)
{
    if( filter != mainFilter )
    {
        mainFilter = filter;

        contentSet->setCriteria( mainFilter & typeFilter & categoryFilter & auxiliaryFilter );
    }
}

const QContent LauncherView::currentItem() const
{
    return model->content(icons->currentIndex());
}

void LauncherView::calculateGridSize(bool force)
{
    if(!force && !isVisible()) {
        mNeedGridSize = true;
        return;
    }
    mNeedGridSize = false;

    QSize grSize;
    Q_ASSERT(model);
    Q_ASSERT(icons);

    int dw = width();
    int viewerWidth = dw - style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, icons->verticalScrollBar()) - 1;
    int iconHeight = 0;
    if ( viewMode() == QListView::IconMode ) {
        int width = viewerWidth/nColumns;
        iconHeight = width - fontMetrics().height() * 2;

        grSize = QSize(width, width);
        QSize icoSize = QSize(iconHeight, iconHeight);
        icons->setIconSize(icoSize);
    } else {
        icons->setSpacing(1);

        int viewHeight = geometry().height();
        qreal scalingFactor = 1.65;
        if (Qtopia::mousePreferred())
            scalingFactor = 1.8;

        int nbRow = int(qAbs(viewHeight / (fontMetrics().height() * scalingFactor)));
        iconHeight = qRound(viewHeight / nbRow);

        grSize = QSize((viewerWidth-(nColumns+1)*icons->spacing())/nColumns, iconHeight);
        QSize icoSize = QSize(iconHeight - 4, iconHeight - 4);
        icons->setIconSize(icoSize);
#ifdef ENABLE_SMOOTHLIST
        smoothicons->setIconSize(icoSize);
#endif
    }
    icons->setGridSize(grSize);
}

void LauncherView::showType( const QContentFilter &filter )
{
    if( filter != typeFilter )
    {
        typeFilter = filter;

        contentSet->setCriteria( mainFilter & typeFilter & categoryFilter & auxiliaryFilter );
    }
}

void LauncherView::showCategory( const QContentFilter &filter )
{
    if( filter != categoryFilter )
    {
        categoryFilter = filter;

        contentSet->setCriteria( mainFilter & typeFilter & categoryFilter & auxiliaryFilter );
    }
}

void LauncherView::setAuxiliaryFilter( const QContentFilter &filter )
{
    if( filter !=  auxiliaryFilter )
    {
        auxiliaryFilter = filter;

        contentSet->setCriteria( mainFilter & typeFilter & categoryFilter & auxiliaryFilter );
    }
}

void LauncherView::resetSelection()
{
    if (icons && model && icons->model()->rowCount() && !Qtopia::mousePreferred()) {
        icons->setCurrentIndex(icons->model()->index(0,0));
    }
}

////////////////////////////////////////////////////////////////
//
// ApplicationLauncherView implementation

ApplicationLauncherView::ApplicationLauncherView(QWidget *parent)
    : LauncherView(parent), rightMenu(0)
{
    QMenu * softMenu = QSoftMenuBar::menuFor(this);
    rightMenu = new QMenu(this);
    QAction *a_speed = new QAction(QIcon(":icon/phone/speeddial"),
                                   tr("Add to Speed Dial..."), this );
    connect( a_speed, SIGNAL(triggered()), this, SLOT(addSpeedDial()));

    softMenu->addAction(a_speed);
    rightMenu->addAction(a_speed);

    QObject::connect(this, SIGNAL(rightPressed(QContent)),
                     this, SLOT(launcherRightPressed(QContent)));
    setViewMode(QListView::ListMode);
}


ApplicationLauncherView::ApplicationLauncherView(const QString &category, QWidget *parent)
    : LauncherView(parent), rightMenu(0)
{
    QMenu * softMenu = QSoftMenuBar::menuFor(this);
    rightMenu = new QMenu(this);
    QAction *a_speed = new QAction(QIcon(":icon/phone/speeddial"),
                                   tr("Add to Speed Dial..."), this );
    connect( a_speed, SIGNAL(triggered()), this, SLOT(addSpeedDial()));

    softMenu->addAction(a_speed);
    rightMenu->addAction(a_speed);

    QObject::connect(this, SIGNAL(rightPressed(QContent)),
                     this, SLOT(launcherRightPressed(QContent)));

    QContentFilter filters = (QContentFilter( QContent::Application ) | QContentFilter( QContent::Folder ))
                & QContentFilter( QContentFilter::Category, category );
    contentSet->setCriteria( filters );
}

void ApplicationLauncherView::launcherRightPressed(QContent lnk)
{
    if(!lnk.isValid())
        return;

    rightMenu->popup(QCursor::pos());
}

void ApplicationLauncherView::addSpeedDial()
{
    const QContent lnk(currentItem());
    QtopiaServiceRequest sreq;
    sreq = QtopiaServiceRequest("Launcher","execute(QString)");
    sreq << lnk.executableName();
    QSpeedDial::addWithDialog(Qtopia::dehyphenate(lnk.name()), lnk.iconName(), sreq, this);
}

