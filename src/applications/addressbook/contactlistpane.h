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
#ifndef CONTACTLISTPANE_H
#define CONTACTLISTPANE_H

#include <QWidget>
#include "qcontact.h"
#include "qpimsource.h"
#include "quniqueid.h"
#include "qcategorymanager.h"

#ifndef QTOPIA_NO_QSMOOTHLIST
#include <private/qsmoothlist.h>
#endif

class QContactListView;
class QTextEntryProxy;
class QLabel;
class QContactModel;
class QVBoxLayout;
class QModelIndex;
class QAbstractItemDelegate;

class ContactListPane : public QWidget
{
    Q_OBJECT
    public:
        ContactListPane(QWidget *w, QContactModel* model);

        QList<QUniqueId> selectedContactIds();

        QContact currentContact() const;
        void setCurrentContact(const QContact& contact);
        void showCategory(const QCategoryFilter &f);
        void resetSearchText();

        QContactModel* contactModel() const { return mModel; }

    signals:
        void contactActivated( QContact c );
        void backClicked();
        void currentChanged(const QModelIndex &, const QModelIndex &);

    public slots:
        void contactsChanged();

#ifdef QTOPIA_CELL
        void showLoadLabel(bool);
#endif

    protected:
        bool eventFilter( QObject *o, QEvent *e );
        void closeEvent( QCloseEvent *e );

    protected slots:
        void updateIcons();
        void search( const QString &k );
        void contactActivated(const QModelIndex &);

    protected:
        QContactListView *mListView;
#ifndef QTOPIA_NO_QSMOOTHLIST
        QSmoothList *mSmoothListView;
#endif
        QTextEntryProxy *mTextProxy;
#ifdef QTOPIA_CELL
        QLabel *mLoadingLabel;
#endif
        QLabel *mCategoryLabel;
        QContactModel *mModel;
        QVBoxLayout *mLayout;
        QAbstractItemDelegate *mDelegate;
        QLabel *mFindIcon;
};

#endif // CONTACTLISTPANE_H

