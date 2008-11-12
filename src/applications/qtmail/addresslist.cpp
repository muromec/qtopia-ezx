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

#include "addresslist.h"
#include <qsoftmenubar.h>
#include <qtopiaapplication.h>
#include <QMenu>
#include <QSettings>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QLabel>
#include <QtopiaItemDelegate>


// This class, or something equivalent, should be part of the PIM library...
class AddressTypeSelectorPrivate;
class AddressTypeSelector : public QDialog
{
    Q_OBJECT
public:
    AddressTypeSelector( const QContact &cnt, bool allowPhones, QWidget *parent = 0);
    //AddressTypeSelector( const QContact &cnt, QList<QContact::PhoneType> allowedTypes, QWidget *parent = 0);

    QString selectedAddress() const;

protected slots:
    void setSelected(QListWidgetItem*);

signals:
    void selected(const QContact&, const QString&);

protected:
    void resizeEvent(QResizeEvent *resizeEvent);

private:
    void init();

    AddressTypeSelectorPrivate *d;

    QString verboseIfEmpty( const QString &number );
};

class AddressTypeSelectorPrivate
{
public:
    AddressTypeSelectorPrivate(const QContact &cnt, QList<QContact::PhoneType> allowedTypes)
        : mToolTip(0), mContact( cnt ), mAllowedPhoneTypes(allowedTypes) {}

    QLabel *mLabel, *mToolTip;
    QListWidget *mAddressList;
    QMap<QListWidgetItem *, QString> mItemToAddress;
    const QContact mContact;
    QList<QContact::PhoneType> mAllowedPhoneTypes;
};

AddressTypeSelector::AddressTypeSelector( const QContact &contact, bool allowPhones, QWidget *parent )
    : QDialog( parent )
{
    d = new AddressTypeSelectorPrivate(contact, (allowPhones ? QContact::phoneTypes() : QList<QContact::PhoneType>()));

    init();
}

/* I think the allowedTypes option will be removed from QPhoneTypeSelector...
AddressTypeSelector::AddressTypeSelector( const QContact &contact, QList<QContact::PhoneType> allowedTypes, QWidget *parent)
    : QDialog( parent )
{
    d = new AddressTypeSelectorPrivate(contact, allowedTypes);

    init();
}
*/

QString AddressTypeSelector::verboseIfEmpty( const QString &number )
{
    if( number.isEmpty() )
        return tr("(empty)");
    return number;
}

QString AddressTypeSelector::selectedAddress() const
{
    QListWidgetItem *item = d->mAddressList->currentItem();
    if( !item )
        return QString();
    return item->text();
}

void AddressTypeSelector::init()
{
    QVBoxLayout *l = new QVBoxLayout( this );
    d->mLabel = new QLabel( this );
    d->mLabel->setWordWrap( true );
    l->addWidget(d->mLabel);
    d->mAddressList = new QListWidget( this );
    l->addWidget(d->mAddressList);
    d->mAddressList->setItemDelegate(new QtopiaItemDelegate(this));
    d->mAddressList->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    setWindowTitle(tr("Addresses"));

    QListWidgetItem *item = 0;

    bool haveAddress = false;
    bool first = true;

    d->mLabel->setText( "<qt>"+tr("Choose address:")+"</qt>" );

    // Add relevant phone numbers
    foreach(QContact::PhoneType type, d->mAllowedPhoneTypes) {
        QString number = d->mContact.phoneNumber(type);
        if (!number.isEmpty()) {
            item = new QListWidgetItem( number, d->mAddressList );
            item->setIcon(QContact::phoneIcon(type));
            d->mItemToAddress[item] = number;
            haveAddress = true;
            if (first) {
                d->mAddressList->setCurrentItem( item );
                first = false;
            }
        }
    }

    // Add email addresses
    foreach(const QString& emailAddress, d->mContact.emailList()) {
        if (!emailAddress.isEmpty()) {
            item = new QListWidgetItem( emailAddress, d->mAddressList );
            item->setIcon(QIcon(":icon/email"));
            d->mItemToAddress[item] = emailAddress;
            haveAddress = true;
            if (first) {
                d->mAddressList->setCurrentItem( item );
                first = false;
            }
        }
    }

    // Any others? Not yet...

    if (!haveAddress) {
        d->mToolTip = new QLabel(tr("No suitable address for this contact"), this);
        d->mToolTip->setWordWrap( true );
        d->mToolTip->setAlignment(Qt::AlignCenter);
        d->mToolTip->show();
    }

    connect( d->mAddressList, SIGNAL(itemActivated(QListWidgetItem*)), 
             this, SLOT(setSelected(QListWidgetItem*)) );
    
#ifdef QTOPIA_PHONE
    QtopiaApplication::setMenuLike(this, true);
#endif
}

void AddressTypeSelector::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    if (d->mToolTip)
        d->mToolTip->move((width()-d->mToolTip->width())/2, (height()-d->mToolTip->height())/2);
}

void AddressTypeSelector::setSelected(QListWidgetItem* item)
{
    if( item )
        emit selected( d->mContact, d->mItemToAddress[item] );

    accept();
}


class AddressPickerData
{
public:
    AddressPickerData() : f(0), addAction(0), removeAction(0) {}

    int f;
    QContact contact;

    QAction *addAction, *removeAction;
};

class AddressPickerItem : public QContactItem
{
public:
    AddressPickerItem(const QString& address)
        : QContactItem(), address(address)
    {
        setData(address, Qt::DisplayRole);
    }

    AddressPickerItem(const QContact& contact, const QString& address)
        : QContactItem(contact, address), address(address)
    {
        setData(contact.label(), Qt::DisplayRole);
    }

    QString address;
};

AddressPicker::AddressPicker(QWidget *parent)
    : QContactListView(parent)
{
    setFrameStyle(NoFrame);

    QContactItemModel *model = new QContactItemModel(this);
    setModel(model);

    d = new AddressPickerData;

    QMenu *menu = QSoftMenuBar::menuFor( this );

    d->addAction = menu->addAction( QIcon(":icon/new"), tr("New recipient"), this, SLOT(addAddress()) );
    d->removeAction = menu->addAction( QIcon(":icon/trash"), tr("Remove recipient"), this, SLOT(removeAddress()) );
}

AddressPicker::~AddressPicker()
{
}

void AddressPicker::clear()
{
    contactItemModel()->clear();
}

void AddressPicker::setFilterFlags(QContactModel::FilterFlags flags)
{
    d->f = flags;
}

void AddressPicker::resetFilterFlags()
{
    d->f = 0;
}

bool AddressPicker::isEmpty() const
{
    return (contactItemModel()->rowCount() == 0);
}

QStringList AddressPicker::addressList() const
{
    QStringList result;

    for (int i = 0; i < contactItemModel()->rowCount(); ++i)
        result.append(static_cast<AddressPickerItem*>(contactItemModel()->item(i))->address);

    return result;
}

QContactItemModel *AddressPicker::contactItemModel() const
{
    return qobject_cast<QContactItemModel *>(model());
}

void AddressPicker::addItem(AddressPickerItem* item)
{
    contactItemModel()->appendRow(item);
    
    emit selectionChanged();
}

void AddressPicker::addressSelected(const QContact& contact, const QString& address)
{
    addItem(new AddressPickerItem(contact, address));
}

void AddressPicker::phoneTypeSelected(QContact::PhoneType type)
{
    addressSelected(d->contact, d->contact.phoneNumber(type));
}

void AddressPicker::contactSelected(const QContact& contact)
{
    AddressPickerItem* item = 0;
    AddressTypeSelector* addressSelector = 0;
    QPhoneTypeSelector* phoneSelector = 0;

    if (d->f == 0) {
        if (contact.phoneNumbers().count() == 1 && contact.emailList().count() == 0) {
            item = new AddressPickerItem(contact, contact.defaultPhoneNumber());
        } else if (contact.phoneNumbers().count() == 0 && contact.emailList().count() == 1) {
            item = new AddressPickerItem(contact, contact.defaultEmail());
        } else {
            addressSelector = new AddressTypeSelector(contact, true, 0);
        }
    } else if (d->f == QContactModel::ContainsPhoneNumber) {
        if (contact.phoneNumbers().count() == 1) {
            item = new AddressPickerItem(contact, contact.defaultPhoneNumber());
        } else {
            phoneSelector = new QPhoneTypeSelector(contact, QString(), this);
        }
    } else if (d->f == QContactModel::ContainsEmail) {
        if (contact.emailList().count() == 1) {
            item = new AddressPickerItem(contact, contact.defaultEmail());
        } else {
            addressSelector = new AddressTypeSelector(contact, false, 0);
        }
    }

    if (item) {
        addItem(item);
    } else {
        QDialog* dialog = 0;

        if (addressSelector) {
            connect(addressSelector, SIGNAL(selected(QContact, QString)),
                    this, SLOT(addressSelected(QContact, QString)));
            dialog = addressSelector;
        } else if (phoneSelector) {
            connect(phoneSelector, SIGNAL(selected(QContact::PhoneType)),
                    this, SLOT(phoneTypeSelected(QContact::PhoneType)));
            dialog = phoneSelector;
        }

        if (dialog) {
            d->contact = contact;

            connect(dialog, SIGNAL(rejected()), this, SLOT(selectionCanceled()));

            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->showMaximized();
            QtopiaApplication::showDialog(dialog);
        }
    }
}

void AddressPicker::addTextAddress(const QString& text)
{
    if (!text.isEmpty())
        addItem(new AddressPickerItem(text));
}

void AddressPicker::selectionCanceled()
{
    emit selectionChanged();
}

void AddressPicker::addAddress()
{
    QContactSelector* selector = new QContactSelector;
    selector->setObjectName("select-contact");

    QContactModel *m = new QContactModel(selector);
    if (d->f)
        m->setFilter(QString(), d->f);

    QSettings config( "Trolltech", "Contacts" );
    config.beginGroup( "default" );
    if (config.contains("SelectedSources/size")) {
        int count = config.beginReadArray("SelectedSources");
        QSet<QPimSource> set;
        for(int i = 0; i < count; ++i) {
            config.setArrayIndex(i);
            QPimSource s;
            s.context = QUuid(config.value("context").toString());
            s.identity = config.value("identity").toString();
            set.insert(s);
        }
        config.endArray();
        m->setVisibleSources(set);
    }

    selector->setModel(m);
    selector->setAcceptTextEnabled(true);

    connect(selector, SIGNAL(contactSelected(QContact)),
            this, SLOT(contactSelected(QContact)));
    connect(selector, SIGNAL(textSelected(QString)),
            this, SLOT(addTextAddress(QString)));
    connect(selector, SIGNAL(rejected()),
            this, SLOT(selectionCanceled()));

    selector->setAttribute(Qt::WA_DeleteOnClose);
    selector->showMaximized();
    QtopiaApplication::showDialog(selector);
}

void AddressPicker::removeAddress()
{
    QModelIndex ind = currentIndex();
    if (ind.isValid()) {
        contactItemModel()->removeRow(ind.row());
    }
}

#include "addresslist.moc"

