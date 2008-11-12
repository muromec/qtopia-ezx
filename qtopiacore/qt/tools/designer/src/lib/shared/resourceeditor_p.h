/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef RESOURCEEDITOR_H
#define RESOURCEEDITOR_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "shared_global_p.h"

#include <QtGui/QWidget>

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;
class QPushButton;
class QToolButton;
class QComboBox;
class QStackedWidget;
class QTreeView;
class QModelIndex;

namespace qdesigner_internal {

class ResourceModel;

class QDESIGNER_SHARED_EXPORT ResourceEditor : public QWidget
{
    Q_OBJECT

public:
    ResourceEditor(QDesignerFormEditorInterface *core,
                   bool dragEnabled,
                   QWidget *parent = 0);
    
    QDesignerFormWindowInterface *form() const { return m_form; }
    int qrcCount() const;
    void setCurrentFile(const QString &qrc_path, const QString &file_path);
    bool isIcon(const QString &qrc_path, const QString &file_path) const;

signals:
    void fileActivated(const QString &qrc_path, const QString &file_path);
    void currentFileChanged(const QString &qrc_path, const QString &file_path);

public slots:
    void saveCurrentView();
    void removeCurrentView();
    void reloadCurrentView();
    void newView();
    void openView();

    void setActiveForm(QDesignerFormWindowInterface *form);

private slots:
    void updateQrcPaths();
    void updateQrcStack();
    void updateUi();
    void addPrefix();
    void addFiles();
    void deleteItem();
    void setCurrentIndex(int i);
    void addView(const QString &file_name);
    void itemActivated(const QModelIndex &index);
    void itemChanged(const QModelIndex &index);

private:
    void getCurrentItem(QString &prefix, QString &file);
    QTreeView *currentView() const;
    ResourceModel *currentModel() const;
    QTreeView *view(int i) const;
    ResourceModel *model(int i) const;
    int indexOfView(QTreeView *view);
    QString qrcName(const QString &path) const;
    int currentIndex() const;

    void insertEmptyComboItem();
    void removeEmptyComboItem();

    QDesignerFormWindowInterface *m_form;
    QComboBox *m_qrc_combo;
    QStackedWidget *m_qrc_stack;
    QToolButton *m_add_button;
    QToolButton *m_remove_button;
    QPushButton *m_add_files_button;
    QToolButton *m_remove_qrc_button;
    bool m_ignore_update;
    const bool m_dragEnabled;
};

} // namespace qdesigner_internal

#endif // RESOURCEEDITOR_H
