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

#include "contactlistpane.h"
#include "addressbook.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QKeyEvent>

#include "qcontactmodel.h"
#include "qcontactview.h"
#include "qtopiaapplication.h"
#include "qcategoryselector.h"
#include "qsoftmenubar.h"
#include <QTextEntryProxy>
#include <QtopiaItemDelegate>

ContactListPane::ContactListPane(QWidget *w, QContactModel* model)
    : QWidget(w), mTextProxy(0), mCategoryLabel(0)
{
    mModel = model;
    connect(mModel, SIGNAL(modelReset()), this, SLOT(contactsChanged()));

    mLayout = new QVBoxLayout();

    mLayout->setMargin(0);
    mLayout->setSpacing(2);

    //
    //  Create the contact list itself.
    //

    mListView = new QContactListView(0);

#ifndef GREENPHONE_EFFECTS
    if (style()->inherits("Series60Style"))
        mDelegate = new QtopiaItemDelegate(mListView);
    else
        mDelegate = new QContactDelegate(mListView);
#else
    mSmoothListView = new QSmoothList(this);
    if (style()->inherits("Series60Style"))
        mDelegate = new QtopiaItemDelegate(mSmoothListView);
    else
        mDelegate = new QContactDelegate(mSmoothListView);
    mSmoothListView->setItemDelegate(mDelegate);
    mSmoothListView->setModel(mModel);
    mLayout->addWidget(mSmoothListView);
    QtopiaApplication::setInputMethodHint( mSmoothListView, QtopiaApplication::Named, "text nohandwriting" );
#endif

    mListView->setFrameStyle(QFrame::NoFrame);
    mListView->setItemDelegate(mDelegate);
    mListView->setModel(mModel);
    mListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    mLayout->addWidget(mListView);

    // Don't show find bar for QThumbStyle
    if (!style()->inherits("QThumbStyle")) {
#ifndef GREENPHONE_EFFECTS
        mTextProxy = new QTextEntryProxy(this, mListView);
#else
        mTextProxy = new QTextEntryProxy(this, mSmoothListView);
#endif
        int mFindHeight = mTextProxy->sizeHint().height();
        mFindIcon = new QLabel;
        mFindIcon->setPixmap(QIcon(":icon/find").pixmap(mFindHeight-2, mFindHeight-2));
        mFindIcon->setMargin(2);

        QHBoxLayout *findLayout = new QHBoxLayout;
        findLayout->addWidget(mFindIcon);
        findLayout->addWidget(mTextProxy);
        mLayout->addLayout(findLayout);

        //
        //  If this is a touch screen phone, tie the search box into the contact list.
        //
        connect( mTextProxy, SIGNAL(textChanged(QString)),
                this, SLOT(search(QString)) );

        QtopiaApplication::setInputMethodHint( mListView, QtopiaApplication::Text );
    }
#ifdef GREENPHONE_EFFECTS
    mListView->setVisible(false);
    connect(mSmoothListView, SIGNAL(activated(QModelIndex)),
            this, SLOT(contactActivated(QModelIndex)));
#endif

#ifndef GREENPHONE_EFFECTS
    mListView->installEventFilter(this);
#endif

    mListView->setSelectionMode(QListView::SingleSelection);
    connect(mListView, SIGNAL(clicked(QModelIndex)),
            this, SLOT(contactActivated(QModelIndex)));
    connect(mListView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SIGNAL(currentChanged(QModelIndex,QModelIndex)));
    connect(mListView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateIcons()));
    mListView->setWhatsThis( tr("List of contacts in the selected category.  Click to view"
            " detailed information.") );


#ifdef QTOPIA_CELL
    mLoadingLabel = 0;
#endif

    setLayout(mLayout);
}

void ContactListPane::resetSearchText()
{
    if (mTextProxy)
        mTextProxy->clear();
}

bool ContactListPane::eventFilter( QObject *o, QEvent *e )
{
    if(o == mListView && e->type() == QEvent::KeyPress) {
        QKeyEvent *ke = (QKeyEvent *)e;
        if ( ke->key() == Qt::Key_Select ) {
            emit contactActivated(mModel->contact(mListView->currentIndex()));
            return true;
        } else if (ke->key() == Qt::Key_Back && (mTextProxy == NULL || mTextProxy->text().length() == 0)) {
            emit backClicked();
            return true;
        }
    }
    return false;
}

#ifdef QTOPIA_CELL
void ContactListPane::showLoadLabel(bool show)
{
    if (show) {
        if( !mLoadingLabel ) {
            mLoadingLabel = new QLabel(tr("Loading SIM..."), 0);
            mLoadingLabel->setAlignment(Qt::AlignCenter);
            // Put this above the filter bar
            mLayout->insertWidget(mLayout->count() - 1, mLoadingLabel);
        }
        mLoadingLabel->show();
    } else {
        if (mLoadingLabel)
            mLoadingLabel->hide();
    }
}
#endif

void ContactListPane::search( const QString &text )
{
    if (text.isEmpty()) {
        mModel->clearFilter();
    } else {
        mModel->setFilter( text );
    }
}

void ContactListPane::contactActivated(const QModelIndex &idx)
{
    /* We should get a selection changed event before this, so don't do the rest of the processing here */
    emit contactActivated(mModel->contact(idx));
}

/*
   Called when the ContactModel is reset (e.g. after refreshing
   cache from SIM or changing filter).  If we have any contacts, select the first,
   otherwise we show the "new" option.
*/
void ContactListPane::contactsChanged()
{
#ifdef GREENPHONE_EFFECTS
    mSmoothListView->reset();
#endif
    if (!mListView->currentIndex().isValid()) {
        QModelIndex newSel = mModel->index(0,0);

        if(newSel.isValid()) {
            mListView->setCurrentIndex(newSel);
            mListView->selectionModel()->setCurrentIndex(newSel, QItemSelectionModel::ClearAndSelect);
            mListView->scrollTo(newSel, QAbstractItemView::PositionAtCenter);
        } else {
            // view doesn't emit selection changed over a model reset
            emit currentChanged(QModelIndex(), QModelIndex());
        }
    }
    updateIcons();
}

void ContactListPane::updateIcons()
{
    if(mModel->rowCount() > 0)
        QSoftMenuBar::setLabel( this, Qt::Key_Select, QSoftMenuBar::View);
    else
        QSoftMenuBar::setLabel( this, Qt::Key_Select, "new" /* <- icon filename */, tr("New") );
}

QContact ContactListPane::currentContact() const
{
    return mListView->currentContact();
}

QList<QUniqueId> ContactListPane::selectedContactIds()
{
    return mListView->selectedContactIds();
}

void ContactListPane::closeEvent( QCloseEvent *e)
{
#ifdef QTOPIA_CELL
    if( mLoadingLabel )
        mLoadingLabel->hide();
#endif
    QWidget::closeEvent(e);
}

void ContactListPane::showCategory( const QCategoryFilter &c )
{
    setWindowTitle( tr("Contacts") + " - " + c.label("Address Book") );

    // The model should already be filtered.

    /* XXX for non S60 UI we may want this back */
    /*
    if(c == QCategoryFilter(QCategoryFilter::All)) {
        if (mCategoryLabel)
            mCategoryLabel->hide();
    } else {
        if (!mCategoryLabel) {
            mCategoryLabel = new QLabel();
            mLayout->addWidget(mCategoryLabel);
        }
        mCategoryLabel->setText(tr("Category: %1").arg(c.label("Address Book")));
        mCategoryLabel->show();
    }
    */
}

void ContactListPane::setCurrentContact( const QContact& contact)
{
    QModelIndex idx = mModel->index(contact.uid());
    if ( !idx.isValid())
        idx = mModel->index(0,0);
    if(idx.isValid() && idx != mListView->currentIndex()) {
        mListView->setCurrentIndex(idx);
        mListView->selectionModel()->setCurrentIndex(idx, QItemSelectionModel::ClearAndSelect);
        mListView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}
