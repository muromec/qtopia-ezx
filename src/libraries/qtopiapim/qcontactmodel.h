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

#ifndef QTOPIA_CONTACTMODEL_H
#define QTOPIA_CONTACTMODEL_H

#include <QStyleOptionViewItem>
#include <QAbstractListModel>
#include <QAbstractItemDelegate>
#include <QSet>
#include <QSharedDataPointer>
#include <qtopia/pim/qcontact.h>
#include <qtopia/pim/qpimsource.h>

#include <QPimModel>

class QContactModelData;
class QTOPIAPIM_EXPORT QContactModel : public QPimModel
{
    Q_OBJECT

public:
    explicit QContactModel(QObject *parent = 0);
    virtual ~QContactModel();

    enum Field {
        Invalid = -1,

        Label, // read only

        NameTitle,
        FirstName,
        MiddleName,
        LastName,
        Suffix,

        JobTitle,
        Department,
        Company,
        BusinessPhone,
        BusinessFax,
        BusinessMobile,

        // email
        DefaultEmail,
        Emails,

        DefaultPhone,

        OtherPhone,
        OtherFax,
        OtherMobile,
        OtherPager,

        HomePhone,
        HomeFax,
        HomeMobile,
        HomePager,

        // business
        BusinessStreet,
        BusinessCity,
        BusinessState,
        BusinessZip,
        BusinessCountry,
        BusinessPager,
        BusinessWebPage,

        Office,
        Profession,
        Assistant,
        Manager,

        // home
        HomeStreet,
        HomeCity,
        HomeState,
        HomeZip,
        HomeCountry,
        HomeWebPage,

        //personal
        Spouse,
        Gender,
        Birthday,
        Anniversary,
        Nickname,
        Children,

        // other
        Portrait,
        Notes,

        LastNamePronunciation,
        FirstNamePronunciation,
        CompanyPronunciation,

        Identifier,
        Categories,
        OtherVOIP,
        HomeVOIP,
        BusinessVOIP
    };

    enum QContactModelRole {
        LabelRole = Qt::UserRole,
        SubLabelRole = Qt::UserRole+1,
        PortraitRole = Qt::UserRole+2,
        StatusIconRole = Qt::UserRole+3
    };

    QPimSource phoneSource() const;
    QPimSource simSource() const;

    bool mirrorToSource(const QPimSource &source, const QUniqueId &);
    bool mirrorAll(const QPimSource &source, const QPimSource &dest);

    static QList<Field> phoneFields();
    static QList<Field> labelFields();
    static QStringList localeNameTitles();
    static QStringList localeSuffixes();

    static QString fieldLabel(Field);
    static QIcon fieldIcon(Field k);

    static QString fieldIdentifier(Field);
    static Field identifierField(const QString &);

    int columnCount(const QModelIndex & = QModelIndex()) const;

    // overridden so can change later and provide drag-n-drop (via vcard)
    // later without breaking API compatibility.
    QMimeData * mimeData(const QModelIndexList &) const;
    QStringList mimeTypes() const;

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &, const QVariant &, int);
    bool setItemData(const QModelIndex &, const QMap<int,QVariant> &);
    QMap<int,QVariant> itemData(const QModelIndex &) const;

    QVariant headerData(int section, Qt::Orientation orientation,
            int role = Qt::DisplayRole) const;

    Qt::ItemFlags flags(const QModelIndex &index) const;

    QContact contact(const QModelIndex &index) const;
    QContact contact(int) const;
    QContact contact(const QUniqueId &) const;

    static QVariant contactField(const QContact &c, QContactModel::Field k);
    static bool setContactField(QContact &c, QContactModel::Field k,  const QVariant &);

    bool updateContact(const QContact& contact);
    bool removeContact(const QContact& contact);
    bool removeContact(const QUniqueId& contact);
    QUniqueId addContact(const QContact& contact, const QPimSource &source = QPimSource());
    bool removeList(const QList<QUniqueId> &);

    QUniqueId addRecord(const QByteArray &, const QPimSource &, const QString &format = QString());
    bool updateRecord(const QUniqueId &id, const QByteArray &, const QString &format = QString());
    QByteArray record(const QUniqueId &id, const QString &format = QString()) const;

    bool removeRecord(const QUniqueId &id) { return removeContact(id); }

    QUniqueId personalID() const;
    QContact personalDetails() const;
    bool hasPersonalDetails() const;

    void setPersonalDetails(const QUniqueId &);
    void clearPersonalDetails();
    bool isPersonalDetails(const QModelIndex &) const;
    bool isPersonalDetails(int r) const { return isPersonalDetails(index(r, 0, QModelIndex())); }
    bool isPersonalDetails(const QUniqueId &) const;

    // starting at 0x0100 so can later maybe |' with
    // Qt::MatchFlags.
    enum FilterFlags {
        ContainsPhoneNumber = 0x00100,
        ContainsEmail = 0x0200,
        ContainsMailing = 0x0400,
    };

    void setFilter(const QString &, int flags = 0);
    QString filterText() const;
    int filterFlags() const;
    void clearFilter();

    /* used in phone a lot */
#ifdef QTOPIA_PHONE
    QContact matchPhoneNumber(const QString &);
#endif
    QContact matchEmailAddress(const QString &);

    /* need a match function since otherwise QAbstractItemModel will do it poorly */
    QModelIndexList match(const QModelIndex &start, int role, const QVariant &,
            int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const;


    bool writeVCard( const QString &filename );

#ifdef QTOPIA_PHONE
    bool isSIMCardContact(const QModelIndex &) const;
    bool isSIMCardContact(const QUniqueId &) const;
#endif

    // missing function to aid in rendering searched for contact.
    // QContact renderedSearchItem(QContact &)?
private slots:
    void pimMessage(const QString& message, const QByteArray& data);

private:
    void setSortField(Field);
    Field sortField() const;

    static void initMaps();
    static QMap<Field, QString> k2t;
    static QMap<Field, QString> k2i;
    static QMap<QString, Field> i2k;

    void updateBusinessCard(const QContact &cnt);
    void removeBusinessCard();

    QContactModelData *d;
};

#endif // QTOPIA_CONTACTMODEL_H
