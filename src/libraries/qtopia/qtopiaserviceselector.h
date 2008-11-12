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
#ifndef QTOPIASERVICESELECTOR_H
#define QTOPIASERVICESELECTOR_H

#include <qdialog.h>
#include <qtopiaservices.h>

class QtopiaServiceDescriptionPrivate;
class QTOPIA_EXPORT QtopiaServiceDescription
{
public:
    QtopiaServiceDescription();
    QtopiaServiceDescription(const QtopiaServiceRequest& r, const QString& l, const QString& ic, const QVariantMap& p = QVariantMap());
    ~QtopiaServiceDescription();

    QtopiaServiceDescription(const QtopiaServiceDescription& other);
    QtopiaServiceDescription& operator=(const QtopiaServiceDescription& other);

    QtopiaServiceRequest request() const;
    QString label() const;
    QString iconName() const;

    void setRequest(const QtopiaServiceRequest& r);
    void setLabel(const QString& l);
    void setIconName(const QString& i);

    QVariant optionalProperty(const QString& name) const;
    void setOptionalProperty(const QString& name, const QVariant &value);
    void removeOptionalProperty(const QString& name);

    QVariantMap optionalProperties() const;
    void setOptionalProperties(QVariantMap properties);

private:
    QtopiaServiceDescriptionPrivate* d;
};

class QListWidgetItem;
class QTranslatableSettings;
class QLabel;
class QListWidget;

class QTOPIA_EXPORT QtopiaServiceSelector : public QDialog
{
    Q_OBJECT
public:
    explicit QtopiaServiceSelector(QWidget* parent);

    void addApplications();

    QtopiaServiceDescription descriptionFor(const QtopiaServiceRequest& req) const;

protected:
    void closeEvent(QCloseEvent *e);
    void keyPressEvent(QKeyEvent* e);

public slots:
    bool edit(const QString& targetlabel, QtopiaServiceDescription& item);

private slots:
    void selectAction(int a);
    void selectAction(QListWidgetItem *i);

private:
    QtopiaServiceDescription descFor(QListWidgetItem* item) const;
    void populateActionsList();
    void populateActionsList(const QString& srv, QTranslatableSettings &cfg);

    QLabel *label;
    QListWidget *actionlist;
    int selection;
};

#endif
