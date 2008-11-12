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

#include "contactdetails.h"
#include "contactbrowser.h"
#include "contactoverview.h"
#if defined(QTOPIA_TELEPHONY)
#include "contactcallhistorylist.h"
#endif
#include "contactmessagehistorylist.h"

#include "qsoftmenubar.h"
#include "qcontactmodel.h"
#include "qtopiaapplication.h"
#include "qpimdelegate.h"

#include <QApplication>
#include <QTextDocument>
#include <QTextFrame>
#include <QTabWidget>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QIcon>
#include <QSize>
#include <QVBoxLayout>

// -------------------------------------------------------------
// ContactDetails
// -------------------------------------------------------------
ContactDetails::ContactDetails( QWidget *parent )
    : QWidget( parent ), mTabs(0), mModel(0)
{
    setObjectName("viewing");

    QSoftMenuBar::setLabel(this, Qt::Key_Back,
        QSoftMenuBar::Back, QSoftMenuBar::AnyFocus);
    QSoftMenuBar::setLabel(this, Qt::Key_Select,
        QSoftMenuBar::NoLabel, QSoftMenuBar::AnyFocus);
}

ContactDetails::~ContactDetails()
{
}

void ContactDetails::init( const QContact &entry )
{
    // If we redisplay the same entry, don't shift our view around too much
    bool sameEntry = (entry.uid() == ent.uid());

    ent = entry;
    mLink.clear();

    /* Create our members, if we haven't */
    if ( !mModel ) {
        mModel = new QContactModel(this);
        connect(mModel, SIGNAL(modelReset()), this, SLOT(modelChanged()));

        mTabs = new QTabWidget();

        mQuickTab = new ContactOverview(0);
        mDetailsTab = new ContactBrowser(0);
#if defined(QTOPIA_TELEPHONY)
        mCallHistoryTab = new ContactCallHistoryList(0);
#endif
        mMessageHistoryTab = new ContactMessageHistoryList(0);

        mTabs->addTab(mQuickTab, QIcon(":icon/contactdetails"), tr("Overview"));
        mTabs->addTab(mDetailsTab, QIcon(":icon/details"), tr("Details"));
#if defined(QTOPIA_TELEPHONY)
        mTabs->addTab(mCallHistoryTab, QIcon(":icon/phone/calls"), tr("Calls"));
#endif
        mTabs->addTab(mMessageHistoryTab, QIcon(":icon/email"), tr("Messages"));

        connect(mQuickTab, SIGNAL(externalLinkActivated()), this, SIGNAL(externalLinkActivated()));
        connect(mQuickTab, SIGNAL(backClicked()), this, SIGNAL(backClicked()));
        connect(mDetailsTab, SIGNAL(externalLinkActivated()), this, SIGNAL(externalLinkActivated()));
        connect(mDetailsTab, SIGNAL(backClicked()), this, SIGNAL(backClicked()));
#if defined(QTOPIA_TELEPHONY)
        connect(mCallHistoryTab, SIGNAL(externalLinkActivated()), this, SIGNAL(externalLinkActivated()));
        connect(mCallHistoryTab, SIGNAL(backClicked()), this, SIGNAL(backClicked()));
#endif
        connect(mMessageHistoryTab, SIGNAL(externalLinkActivated()), this, SIGNAL(externalLinkActivated()));
        connect(mMessageHistoryTab, SIGNAL(backClicked()), this, SIGNAL(backClicked()));

        connect(mQuickTab, SIGNAL(callContact()), this, SIGNAL(callContact()));
        connect(mQuickTab, SIGNAL(textContact()), this, SIGNAL(textContact()));
        connect(mQuickTab, SIGNAL(emailContact()), this, SIGNAL(emailContact()));
        connect(mQuickTab, SIGNAL(editContact()), this, SIGNAL(editContact()));

        connect(mDetailsTab, SIGNAL(highlighted(QString)), this, SIGNAL(highlighted(QString)));

        QVBoxLayout *v = new QVBoxLayout();
        v->addWidget(mTabs);
        v->setMargin(0);
        setLayout(v);
    }

    modelChanged();

    if (!sameEntry)
        mTabs->setCurrentIndex(0);
    mTabs->currentWidget()->setFocus();
}

void ContactDetails::modelChanged()
{
    if (mTabs && mModel) {
        mQuickTab->setModel(mModel);
        mDetailsTab->setModel(mModel);
#if defined(QTOPIA_TELEPHONY)
        mCallHistoryTab->setModel(mModel);
#endif
        mMessageHistoryTab->setModel(mModel);

        mQuickTab->init(ent);
        mDetailsTab->init(ent, ContactDocument::Details);
#if defined(QTOPIA_TELEPHONY)
        mCallHistoryTab->init(ent);
#endif
        mMessageHistoryTab->init(ent);
    }
}

// -------------------------------------------------------------
// ContactHistoryDelegate
// -------------------------------------------------------------

ContactHistoryDelegate::ContactHistoryDelegate(QObject *parent)
    : QPimDelegate(parent)
{
    int dim = qApp->style()->pixelMetric(QStyle::PM_ListViewIconSize);
    mPrimarySize = QSize(dim,dim);
    dim = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
    mSecondarySize = QSize(dim,dim);
}

ContactHistoryDelegate::~ContactHistoryDelegate()
{

}

int ContactHistoryDelegate::subTextsCountHint(const QStyleOptionViewItem& , const QModelIndex& ) const
{
    // Always one line of subtexts
    return 1;
}

QList<StringPair> ContactHistoryDelegate::subTexts(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    QList< StringPair > subTexts;

    subTexts.append(qMakePair(QString(),index.model()->data(index, SubLabelRole).toString()));

    return subTexts;
}

void ContactHistoryDelegate::drawDecorations(QPainter* p, bool rtl, const QStyleOptionViewItem &option, const QModelIndex& index,
                                  QList<QRect>& leadingFloats, QList<QRect>& trailingFloats) const
{
    Q_UNUSED(option);

    QIcon pi = qvariant_cast<QIcon>(index.model()->data(index, Qt::DecorationRole));
    QIcon si = qvariant_cast<QIcon>(index.model()->data(index, SecondaryDecorationRole));
    QRect drawRect = option.rect;
    QPoint drawOffset;

    // draw the primary icon
    // 8px padding, 4 on either side
    if (rtl)
        drawRect.setLeft(drawRect.right() - mPrimarySize.width() - 8);
    else
        drawRect.setRight(mPrimarySize.width() + 8);

    drawOffset = QPoint(drawRect.left() + ((drawRect.width() - mPrimarySize.width())/2), drawRect.top() + ((drawRect.height() - mPrimarySize.height()) / 2));
    p->drawPixmap(drawOffset, pi.pixmap(mPrimarySize));

    leadingFloats.append(drawRect);

    // secondary icon now
    drawRect = option.rect;
    if (rtl)
        drawRect.setRight(mSecondarySize.width());
    else
        drawRect.setLeft(drawRect.right() - mSecondarySize.width() - 8);

    drawOffset = QPoint(drawRect.left() + ((drawRect.width() - mSecondarySize.width())/2), drawRect.top() + ((drawRect.height() - mSecondarySize.height()) / 2));
    p->drawPixmap(drawOffset, si.pixmap(mSecondarySize));

    trailingFloats.append(drawRect);
}

QSize ContactHistoryDelegate::decorationsSizeHint(const QStyleOptionViewItem& option, const QModelIndex& index, const QSize& s) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(mPrimarySize.width() + mSecondarySize.width() + s.width(), qMax(qMax(mPrimarySize.height(), mSecondarySize.height()) + 2, s.height()));
}

