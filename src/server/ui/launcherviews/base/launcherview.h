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
#ifndef LAUNCHERVIEW_H
#define LAUNCHERVIEW_H

#include <QWidget>
#include <QListView>
#include <QList>
#include <QContentSet>
#include <QSortFilterProxyModel>

#ifndef QTOPIA_NO_QSMOOTHLIST
#include <private/qsmoothlist.h>
#endif

class QContentSetMultiColumnProxyModel;
class QAbstractMessageBox;
class TypeDialog;
class QCategoryDialog;
class QLauncherProxyModel;
class QAbstractProxyModel;
class QVBoxLayout;
//class QSortFilterProxyModel;

class LauncherViewListView : public QListView
{
    Q_OBJECT
public:
    LauncherViewListView( QWidget *parent ) : QListView(parent) {}

signals:
    void currentIndexChanged( const QModelIndex &current, const QModelIndex &previous );

protected slots:
    virtual void currentChanged( const QModelIndex &current, const QModelIndex &previous );
    virtual void rowsAboutToBeRemoved(const QModelIndex &parent, int start, int end);
    virtual void rowsInserted(const QModelIndex &parent,int start, int end);
protected:
    virtual void focusOutEvent(QFocusEvent *);
    virtual void focusInEvent(QFocusEvent *);
    virtual bool viewportEvent(QEvent *e);
    void ensureSelected();
};

class LauncherView : public QWidget
{
    Q_OBJECT

    public:
        LauncherView( QWidget* parent = 0, Qt::WFlags fl = 0);
        virtual ~LauncherView() {};
        virtual void resetSelection();
        void setBusy(bool);
        void setBusy(const QModelIndex &, bool);
        void setItemDelegate(QAbstractItemDelegate *);
        void setViewMode( QListView::ViewMode m );
        QListView::ViewMode viewMode() const;

    void removeAllItems();
    void addItem(QContent* app, bool resort=true);
    void removeItem(const QContent &);

    void setColumns(int);
    virtual void setFilter(const QContentFilter &filter);
    const QContent currentItem() const;

    enum SortingStyle { NoSorting, LanguageAwareSorting };

        QAbstractItemView *view() const { return icons; }

    protected:
        friend class QLauncherProxyModel;

        LauncherViewListView *icons;
#ifndef QTOPIA_NO_QSMOOTHLIST
        QSmoothList *smoothicons;
#endif
        QContentSet *contentSet;
        QContentSetModel *model;
        QContentFilter mainFilter;
        QContentFilter categoryFilter;
        QContentFilter typeFilter;
        QContentFilter auxiliaryFilter;
        int nColumns;
        int busyTimer;
        QVBoxLayout *mainLayout;
        bool mNeedGridSize;

        virtual void changeEvent(QEvent *e);
        virtual void showEvent(QShowEvent *e);
        virtual void calculateGridSize(bool force = false);
        virtual void timerEvent( QTimerEvent * event );

        virtual void handleReturnPressed(const QModelIndex &item);
        virtual void handleItemClicked(const QModelIndex &item, bool setCurrentIndex);
        virtual void handleItemPressed(const QModelIndex &item);
signals:
        void clicked( QContent );
        void rightPressed( QContent );

    protected slots:
        void returnPressed(const QModelIndex &item) { handleReturnPressed(item); }
        void itemClicked(const QModelIndex &item) { handleItemClicked(item, true); }
        void itemPressed(const QModelIndex &item){ handleItemPressed(item); }
        void resizeEvent(QResizeEvent *);

    public slots:
        void showType( const QContentFilter & );
        void showCategory( const QContentFilter & );
        void setAuxiliaryFilter( const QContentFilter & );

    private:
        void init();
};

class ApplicationLauncherView : public LauncherView {
    Q_OBJECT

    public:
        ApplicationLauncherView(QWidget * = 0);
        ApplicationLauncherView(const QString &, QWidget * = 0);
    private slots:
        void addSpeedDial();
        void launcherRightPressed(QContent);
    private:
        QMenu *rightMenu;
};

class QLauncherProxyModel : public QSortFilterProxyModel
{
    public:
        QLauncherProxyModel(QObject *parent=0) : QSortFilterProxyModel(parent), sortingStyle(LauncherView::NoSorting) { }
        virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

        void setBusyItem(const QModelIndex&);
        void clearBusyItems();
        bool hasBusy() const {return items.count() != 0;}
        void setSorting(LauncherView::SortingStyle);
        enum Roles { BusyRole = Qt::UserRole+2 };

    private:
        QList<QModelIndex> items;
        LauncherView::SortingStyle sortingStyle;
};

#endif // LAUNCHERVIEW_H
