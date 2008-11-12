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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of Qt Designer.  This header
// file may change from version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef FINDICONDIALOG_H
#define FINDICONDIALOG_H

#include "shared_global_p.h"

#include <QtCore/QDir>
#include <QtGui/QDialog>

class QDesignerFormWindowInterface;
class QListWidgetItem;
class QModelIndex;

class QDesignerResourceBrowserInterface;

namespace qdesigner_internal {

namespace Ui
{
    class FindIconDialog;
} // namespace Ui

class ResourceEditor;

class QDESIGNER_SHARED_EXPORT FindIconDialog : public QDialog
{
    Q_OBJECT

public:
    FindIconDialog(QDesignerFormWindowInterface *form, QWidget *parent);
    virtual ~FindIconDialog();

    void setPaths(const QString &qrcPath, const QString &filePath);
    QString qrcPath() const;
    QString filePath() const;

    virtual void accept();

private slots:
    void setActiveBox(int box);
    void setActiveBox();
    void updateButtons();

    void setFile(const QString &path);
    void setQrc(const QString &qrc, const QString &file);
    void setLanguagePath(const QString &path);
    void cdUp();

    void itemActivated(QListWidgetItem *item); // File page
    void currentItemChanged(QListWidgetItem *item);
    
    void itemActivated(const QString &qrc_path, const QString &file_name); // QRC page
    void itemChanged(const QString &qrc_path, const QString &file_name);
    
    void itemActivated(const QString &file_name); // Language plugin page
    void itemChanged(const QString &file_name);

private:
    enum InputBox { ResourceBox, FileBox, LanguageBox };

    InputBox activeBox() const;
    bool isIconValid(const QString &file) const;

    qdesigner_internal::Ui::FindIconDialog *ui;
    QDesignerFormWindowInterface *m_form;

    void setViewDir(const QString &path);
    QDir m_view_dir;
    struct FileData {
        QString file;
    } m_file_data;
    struct LanguageData {
        QString file;
    } m_language_data;
    struct ResourceData {
        QString file;
        QString qrc;
    } m_resource_data;
    ResourceEditor *m_resource_editor;
    QDesignerResourceBrowserInterface *m_language_editor;

    static QString defaultQrcPath();
    static QString defaultFilePath(QDesignerFormWindowInterface *form);
    static QString defaultLanguagePath();
    static void setDefaultQrcPath(const QString &path);
    static void setDefaultFilePath(const QString &path);
    static void setDefaultLanguagePath(const QString &path);
    static InputBox previousInputBox();
    static void setPreviousInputBox(InputBox box);

#ifdef Q_OS_WIN
    bool isRoot;
    QString rootDir;
#endif
};

} // namespace qdesigner_internal

#endif // FINDICONDIALOG_H
