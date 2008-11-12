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

#ifndef SPEEDDIAL_H
#define SPEEDDIAL_H

#include <qtopiaserviceselector.h>
#include <QListView>
#include <QList>

class QSpeedDial;
class QSpeedDialListPrivate;
class QSpeedDialList;


class QTOPIA_EXPORT QSpeedDialList : public QListView
{
    Q_OBJECT
    Q_PROPERTY(int count READ count)
    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow)
    Q_PROPERTY(QString currentInput READ currentInput)
    friend class QSpeedDialDialog;

public:
    explicit QSpeedDialList(QWidget* parent=0);
    ~QSpeedDialList();

    QString currentInput() const;
    void setCurrentInput(const QString&);

    QString rowInput(int row) const;

    void setCurrentRow(int row);
    int currentRow() const;

    int count() const;

    void setBlankSetEnabled(bool);
    bool isBlankSetEnabled() const;

public slots:
    void reload(const QString& sd);
    void editItem(int row);
    void editItem();
    void clearItem(int row);
    void clearItem();

signals:
    void currentRowChanged(int row);
    void rowClicked(int row);
    void itemSelected(QString);

protected:
    void keyPressEvent(QKeyEvent*);
    void timerEvent(QTimerEvent*);
    void scrollContentsBy(int dx, int dy);

private slots:
    void select(const QModelIndex& index);
    void click(const QModelIndex& index);
    void sendRowChanged();

private:
    void init(const QString&);
    QSpeedDialList(const QString& label, const QString& icon, QWidget* parent);
    void setActionChooserEnabled(bool);

    QSpeedDialListPrivate* d;
};



class QTOPIA_EXPORT QSpeedDial
{
public:
    // QSpeedDial is very similar to QDeviceButtonManager

    static QString addWithDialog(const QString& label, const QString& icon,
        const QtopiaServiceRequest& action, QWidget* parent);
    static QString selectWithDialog(QWidget* parent);
    static QList<QString> assignedInputs();
    static QList<QString> possibleInputs();
    static QtopiaServiceDescription* find(const QString& input);
    static void remove(const QString& input);
    static void set(const QString& input, const QtopiaServiceDescription&);
};

#endif
