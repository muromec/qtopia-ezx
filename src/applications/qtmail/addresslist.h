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



#ifndef ADDRESSLIST_H
#define ADDRESSLIST_H

#include <qtopia/pim/qcontactview.h>
#include <QDialog>
#include <QContact>

class AddressPickerData;
class AddressPickerItem;

class AddressPicker : public QContactListView
{
    Q_OBJECT
public:
    AddressPicker(QWidget *parent = 0);
    ~AddressPicker();

    void clear();
    void setFilterFlags(QContactModel::FilterFlags);
    void resetFilterFlags();

    bool isEmpty() const;

    QStringList addressList() const;
    QContactItemModel *contactItemModel() const;

signals:
    void selectionChanged();

public slots:
    void addAddress();
    void removeAddress();
    void addressSelected(const QContact&, const QString&);
    void phoneTypeSelected(QContact::PhoneType);
    void contactSelected(const QContact&);
    void addTextAddress(const QString&);
    void selectionCanceled();

private:
    void addItem(AddressPickerItem*);

    AddressPickerData *d;
};

#endif
