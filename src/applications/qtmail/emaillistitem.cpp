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


#include "emaillistitem.h"
#include "maillistview.h"

#include <qmailaddress.h>
#include <qtimestring.h>

#include <qstring.h>
#include <qfont.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qicon.h>
#include <qpixmapcache.h>

#include <stdlib.h>


static QPixmap* pm_normal = 0;
static QPixmap* pm_toget = 0;
static QPixmap* pm_tosend = 0;
static QPixmap* pm_unread = 0;
static QPixmap* pm_unfinished = 0;
static QPixmap* pm_removed = 0;

static QPixmap* pm_mms = 0;
static QPixmap* pm_ems = 0;
static QPixmap* pm_sms = 0;
static QPixmap* pm_email = 0;

static QFont* nameFont = 0;
static QFont* subjectFont = 0;

static void ensurePixmaps()
{
    if ( !pm_normal ) {
        pm_normal = new QPixmap(":image/flag_normal");
        pm_unread = new QPixmap(":image/flag_unread");
        pm_toget = new QPixmap(":image/flag_toget");
        pm_tosend = new QPixmap(":image/flag_tosend");
        pm_unfinished = new QPixmap(":image/flag_unfinished");
        pm_removed = new QPixmap(":image/flag_removed");

        int extent = qApp->style()->pixelMetric(QStyle::PM_SmallIconSize);
        pm_mms = new QPixmap(QIcon(":icon/multimedia").pixmap(extent));
        pm_ems = new QPixmap(*pm_mms);   // Same as MMS icon
        pm_sms = new QPixmap(QIcon(":icon/txt").pixmap(extent));
        pm_email = new QPixmap(QIcon(":icon/email").pixmap(extent));
    }
}

/*  This method must only be called from the EmailClient destructor (otherwise
    nasty things will happen    */
void EmailListItem::deletePixmaps()
{
    delete pm_normal;
    delete pm_unread;
    delete pm_toget;
    delete pm_tosend;
    delete pm_unfinished;
    delete pm_removed;
    delete pm_mms;
    delete pm_ems;
    delete pm_sms;
    delete pm_email;
}

EmailListItem::EmailListItem(MailListView *parent, const QMailId& id, int col)
    : QTableWidgetItem(), mId(id), columnsSet(false), typePm(0), alt(0), mCol( col ), messageType(QMailMessage::None)
{
    ensurePixmaps();
    setFlags( flags() & ~Qt::ItemIsEditable );

    Q_UNUSED(parent)
}

EmailListItem::~EmailListItem()
{
}

bool EmailListItem::operator<(const QTableWidgetItem &other) const
{
    EmailListItem *otherItem = const_cast<EmailListItem*>(static_cast<const EmailListItem *>(&other));

    return mId < otherItem->mId;
}

void EmailListItem::updateState()
{
    setColumns();
}

QString EmailListItem::dateToString( QDateTime dateTime )
{
    QDate endWeek = QDate::currentDate();
    endWeek.addDays( 7 - endWeek.dayOfWeek() );

    int daysTo = abs( dateTime.date().daysTo(endWeek) );
    if ( daysTo < 7 ) {
        if ( QDate::currentDate() == dateTime.date() ) {
            QString day = MailListView::tr("Today %1").arg( QTimeString::localHM( dateTime.time() ) );
            return day;
        } else if ( dateTime.daysTo(QDateTime::currentDateTime()) == 1 ) {
            return MailListView::tr("Yesterday");
        } else {
            return QTimeString::localDayOfWeek(dateTime.date());
        }
    } else {
        return QTimeString::localYMD( dateTime.date() );
    }
}

void EmailListItem::setColumns()
{
    QString temp;

    QMailMessage mail(mId,QMailMessage::Header);

    QMailMessage::Status mailStatus(mail.status());
    if ( mailStatus & QMailMessage::Incoming ) {
        if ( mailStatus & QMailMessage::Removed ) {
            setIcon(*pm_removed);
        } else if ( mailStatus & QMailMessage::Downloaded ) {
            if ( mailStatus & QMailMessage::Read || mailStatus & QMailMessage::ReadElsewhere ) {
                setIcon(*pm_normal);
            } else {
                setIcon(*pm_unread);
            }
        } else {
            setIcon(*pm_toget);
        }
    } else {
        if ( mailStatus & QMailMessage::Sent ) {
            setIcon(*pm_normal);
        } else if ( !mail.hasRecipients() ) {
            setIcon(*pm_unfinished);
        } else {
            setIcon(*pm_tosend);
        }
    }

    messageType = mail.messageType();
    if (messageType == QMailMessage::Mms)
        typePm = pm_mms;
    else if (messageType == QMailMessage::Sms)
        typePm = pm_sms;
    else if (messageType == QMailMessage::Email)
        typePm = pm_email;

    columnsSet = true;
}

QMailId EmailListItem::id() const
{
    return mId;
}

void EmailListItem::setId(const QMailId& id)
{
    mId = id;
    columnsSet = false;
}

QString EmailListItem::cachedName() const
{
    return mCachedName;
}

void EmailListItem::setCachedName(const QString& name)
{
    mCachedName = name;
}

QMailMessage::MessageType EmailListItem::type() const
{
    return messageType;
}


EmailListItemDelegate::EmailListItemDelegate(MailListView *parent)
    : QItemDelegate(parent),
      mParent(parent)
{
}

void EmailListItemDelegate::paint(QPainter *painter,
                                  const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    EmailListItem *item = mParent->emailItemFromIndex( index );
    if (item && item->id().isValid()) {
        Q_ASSERT(index.isValid());
        const QAbstractItemModel *model = index.model();
        Q_ASSERT(model);

        QStyleOptionViewItem opt = option;

        // Set color group
        opt.palette.setCurrentColorGroup(option.state & QStyle::State_Enabled
                                         ? QPalette::Active : QPalette::Disabled);

        // set font
        QVariant value = model->data(index, Qt::FontRole);
        if (value.isValid())
            opt.font = qvariant_cast<QFont>(value);

        // set text alignment
        value = model->data(index, Qt::TextAlignmentRole);
        if (value.isValid())
            opt.displayAlignment = QFlag(value.toInt());

        // set text color
        value = model->data(index, Qt::TextColorRole);
        if (value.isValid() && qvariant_cast<QColor>(value).isValid())
            opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));

        // do layout

        // decoration
        if ( !item->stateUpdated() )
            item->updateState();

        value = model->data(index, Qt::DecorationRole);
        QPixmap pixmap1 = decoration(opt, value); // flag (un)read/toget/tosend
        QPixmap pixmap2; // type email/sms/mm/ems
        if ( item->pixmap() ) {
            pixmap2 = *item->pixmap();
        }

        QRect pixmapRect1 = (pixmap1.isNull() ? QRect(0, 0, 0, 0)
                            : QRect(QPoint(0, 0), option.decorationSize));
        QRect pixmapRect2 = (pixmap2.isNull() ? QRect(0, 0, 0, 0)
                            : QRect(QPoint( 0, 0 ),
                                    QPoint( pixmap2.size().width(),
                                            pixmap2.size().height() ) ) );

        // display
        if (!nameFont) {
            nameFont = new QFont( opt.font );
            subjectFont = new QFont( *nameFont );
            nameFont->setBold( true );
            subjectFont->setPointSize( nameFont->pointSize() * 3 / 4 );
        }

        QRect nameRect;

        QMailMessage mail(item->id(),QMailMessage::Header);

        if ( mail.status() & QMailMessage::Incoming ) {
            QMailAddress fromAddress(mail.from());
            if(item->cachedName().isNull())
                item->setCachedName(fromAddress.displayName());
        } else {
            if ( mail.to().count() > 0) {
                QMailAddress firstRecipient(mail.to().first());
                if(item->cachedName().isNull())
                {
                    item->setCachedName(firstRecipient.displayName());

                    int recipientCount = mail.recipients().count();
                    if (recipientCount > 1)
                        item->setCachedName(item->cachedName() + ", ...");
                }
            }
        }

        if (!item->cachedName().isEmpty())
        {
            QRectF result;
            painter->setFont(*nameFont);
            painter->drawText(option.rect,
                              Qt::TextDontPrint|Qt::TextDontClip,
                              item->cachedName(), &result);
            nameRect = result.toRect();
            nameRect.setWidth( nameRect.width() + 6 );//anti-elide fudge
        }

        QRect subjectRect;
        QString subject = mail.subject().simplified();

        if (!subject.isEmpty())
        {
            QRectF result;
            painter->setFont(*subjectFont);
            painter->drawText(option.rect,
                              Qt::TextDontPrint|Qt::TextDontClip,
                              subject, &result);
            subjectRect = result.toRect();
        }

        int margin = 2;
        pixmapRect1.translate( margin, margin + opt.rect.top()  );
        pixmapRect2.translate( pixmapRect1.right() + margin,
                               margin + opt.rect.top() );
        nameRect.translate( pixmapRect2.right() + margin, margin );
        subjectRect.translate( pixmapRect2.right() + margin, nameRect.height() );
        subjectRect.setWidth( subjectRect.width() + 6 );//anti-elide fudge

        // handle rtl drawing
        if (opt.direction == Qt::RightToLeft) {
            pixmapRect1.setRect( opt.rect.left() + opt.rect.width() - pixmapRect1.width(), pixmapRect1.top(),
                                 pixmapRect1.width(), pixmapRect1.height() );
            pixmapRect2.setRect( pixmapRect1.left() - pixmapRect2.width(), opt.rect.top(),
                                 pixmapRect2.width(), pixmapRect2.height() );
            nameRect.setRect( pixmapRect2.left() - nameRect.width(), nameRect.top(),
                              nameRect.width(), nameRect.height() );
            subjectRect.setRect( pixmapRect2.left() - subjectRect.width(), subjectRect.top(),
                                 subjectRect.width(), subjectRect.height() );
        }

        // draw the background color
        if (option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
            QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                ? QPalette::Normal : QPalette::Disabled;

            QString key = QLatin1String("_QTMAIL_");
            key += QString::number(option.rect.width());
            key += QString::number(option.rect.height());
            key += QString::number(int(option.palette.color(cg, QPalette::Highlight).rgba()));

            QPixmap pm;
            if (!QPixmapCache::find(key, pm)) {
                QSize size = option.rect.size();
                QImage img(size, QImage::Format_ARGB32_Premultiplied);
                img.fill(0x00000000);
                QPainter p(&img);
                p.setRenderHint(QPainter::Antialiasing);
                QColor color = option.palette.color(cg, QPalette::Highlight);
                p.setPen(color);
                QLinearGradient bgg(QPoint(0,0), QPoint(0, size.height()));
                bgg.setColorAt(0.0f, color.lighter(175));
                bgg.setColorAt(0.49f, color.lighter(105));
                bgg.setColorAt(0.5f, color);
                p.setBrush(bgg);
                p.drawRoundRect(QRect(QPoint(0,0),size), 800/size.width(),800/size.height());
                pm = QPixmap::fromImage(img);
                QPixmapCache::insert(key, pm);
            }
            painter->drawPixmap(option.rect.topLeft(), pm);
        } else {
            value = model->data(index, Qt::BackgroundColorRole);
            if (value.isValid() && qvariant_cast<QColor>(value).isValid())
                painter->fillRect(option.rect, qvariant_cast<QColor>(value));
         }

        // avoid drawing highlight color twice.
        opt.palette.setColor(QPalette::Highlight, QColor(0,0,0,0));

        if (!item->cachedName().isEmpty()) {
            drawDisplay(painter, opt, nameRect, item->cachedName());
        }
        if (!subject.isEmpty()) {
            QStyleOptionViewItem subjectOpt = opt;
            subjectOpt.font = *subjectFont;
            drawDisplay(painter, subjectOpt, subjectRect, subject);
        }

        // Don't highlight pixmaps.
        opt.state &= ~(QStyle::State_Selected);

        // draw the item
        if (pixmapRect1.isValid())
            drawDecoration(painter, opt, pixmapRect1, pixmap1);
        if (pixmapRect2.isValid())
            drawDecoration(painter, opt, pixmapRect2, pixmap2);
    } else {
        QItemDelegate::paint( painter, option, index );
    }
}

