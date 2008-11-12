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

#include "resourcefile_p.h"
#include "findicondialog_p.h"
#include "ui_findicondialog.h"
#include "resourceeditor_p.h"

#include <QtDesigner/abstractformwindow.h>
#include <QtDesigner/abstractformeditor.h>
#include <QtDesigner/abstractformwindowmanager.h>
#include <QtDesigner/abstractresourcebrowser.h>
#include <QtDesigner/abstractlanguage.h>
#include <QtDesigner/qextensionmanager.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QMetaObject>
#include <QtCore/QSettings>
#include <QtCore/qdebug.h>
#include <QtGui/QFileDialog>
#include <QtGui/QHeaderView>
#include <QtGui/QImageReader>
#include <QtGui/QLabel>
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtCore/QSignalMapper>

namespace qdesigner_internal {

QStringList extensionList()
{
    static QStringList extension_list;

    if (extension_list.isEmpty()) {
        const QList<QByteArray> _extension_list = QImageReader::supportedImageFormats();
        foreach (const QByteArray &ext, _extension_list) {
            QString filter = QLatin1String("*.");
            filter +=  QString::fromAscii(ext);
            extension_list.append(filter);
        }
    }

    return extension_list;
}

bool FindIconDialog::isIconValid(const QString &file) const
{
    if (!qrcPath().isEmpty())
        return m_resource_editor->isIcon(qrcPath(), file);

    const bool enabled = !file.isEmpty();
    if (enabled) {
        const QStringList ext_list = extensionList();
        foreach (QString ext, ext_list) {
            if (file.endsWith(ext.remove(0, 2), Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

    enum {  g_file_item_id, g_dir_item_id };
    static const char* FindDialogDirSettingsKey="FindIconDialog/RecentDirectories";

QListWidgetItem *createListWidgetItem(const QIcon &icon, const QString &text, int item_id, QListWidget *parent)
{
    QListWidgetItem *result = new QListWidgetItem(icon, text, parent);
    const QSize s = parent->iconSize();
    result->setSizeHint(QSize(s.width()*3, s.height()*2));
    result->setData(Qt::UserRole, item_id);
    return result;
}

bool dirItem(QListWidgetItem *item)
{
    const QVariant v = item->data(Qt::UserRole);
    if (!v.canConvert(QVariant::Int))
        return false;
    return v.toInt() == g_dir_item_id;
}
}

namespace qdesigner_internal {

FindIconDialog::FindIconDialog(QDesignerFormWindowInterface *form, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::FindIconDialog),
      m_form (form),
      m_view_dir(QDir::temp()),
      m_resource_editor(0),
      m_language_editor(0)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    const QSize icon_size(24, 24);
    ui->m_icon_view->setViewMode(QListWidget::IconMode);
    ui->m_icon_view->setMovement(QListWidget::Static);
    ui->m_icon_view->setResizeMode(QListWidget::Adjust);
    ui->m_icon_view->setIconSize(icon_size);
    ui->m_icon_view->setTextElideMode(Qt::ElideRight);

    ui->m_file_input->setMinimumContentsLength(40);
    ui->m_file_input->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);
    QSettings settings;
    const QStringList recent_dir_list = settings.value(QLatin1String(FindDialogDirSettingsKey)).toStringList();
    foreach (const QString &dir, recent_dir_list)
        ui->m_file_input->addItem(dir);

    ui->m_widget_stack->widget(FileBox)->layout()->setMargin(0);
    QWidget *page = ui->m_widget_stack->widget(ResourceBox);
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setMargin(0);
    m_resource_editor = new ResourceEditor(form->core(), false, page);

    disconnect(form->core()->formWindowManager(),
                SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
                m_resource_editor, SLOT(setActiveForm(QDesignerFormWindowInterface*)));
    m_resource_editor->setActiveForm(form);
    layout->addWidget(m_resource_editor);
    m_resource_editor->layout()->setMargin(0);


    QDesignerFormEditorInterface *core = form->core();
    if (QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension*>(core->extensionManager(), core)) {
        m_language_editor = lang->createResourceBrowser(ui->m_widget_stack);
        connect(m_language_editor, SIGNAL( currentPathChanged(const QString&)),
                 this,SLOT(itemChanged(const QString&)));
        connect(m_language_editor, SIGNAL( pathActivated(const QString&)),
                 this,SLOT(itemActivated(const QString&)));
        ui->m_widget_stack->addWidget(m_language_editor);
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Set up switching of modes
    QSignalMapper *mapper = new QSignalMapper(this);
    mapper->setMapping(ui->m_specify_file_input, FileBox);
    mapper->setMapping(ui->m_specify_resource_input, m_language_editor ? LanguageBox : ResourceBox);
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(setActiveBox(int)));

    connect(ui->m_specify_file_input, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->m_specify_resource_input, SIGNAL(clicked()), mapper, SLOT(map()));
    connect(ui->m_icon_view, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(currentItemChanged(QListWidgetItem*)));
    connect(ui->m_icon_view, SIGNAL(itemActivated(QListWidgetItem*)),
            this, SLOT(itemActivated(QListWidgetItem*)));
    connect(ui->m_cd_up_button, SIGNAL(clicked()), this, SLOT(cdUp()));
    connect(ui->m_file_input->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(setFile(QString)));
    connect(ui->m_file_input, SIGNAL(currentIndexChanged(QString)), this, SLOT(setFile(QString)));
    connect(m_resource_editor, SIGNAL(fileActivated(QString, QString)),
            this, SLOT(itemActivated(QString, QString)));
    connect(m_resource_editor, SIGNAL(currentFileChanged(QString, QString)),
            this, SLOT(itemChanged(QString, QString)));

#ifdef Q_OS_WIN
    isRoot = false;
    
    QSettings myComputer(QLatin1String("HKEY_CLASSES_ROOT\\CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}"), QSettings::NativeFormat);
    rootDir = myComputer.value(QString(QLatin1Char('.'))).toString();
#endif

    updateButtons();
}

void FindIconDialog::accept()
{
    if (activeBox() == FileBox && !filePath().isEmpty()) {
        QStringList recent_dir_list;
        const QString new_path = m_view_dir.path();
        recent_dir_list.append(new_path);
        for (int i = 0; i < 15 && i < ui->m_file_input->count(); ++i) {
            const QString path = ui->m_file_input->itemText(i);
            if (path != new_path)
                recent_dir_list.append(path);
        }
        QSettings settings;
        settings.setValue(QLatin1String(FindDialogDirSettingsKey), recent_dir_list);
    }
    if (activeBox() == ResourceBox) {
        setDefaultQrcPath(qrcPath());
    } else if (activeBox() == LanguageBox) {
        setDefaultLanguagePath(filePath());
    } else {
        setDefaultFilePath(QFileInfo(filePath()).absolutePath());
    }
    setPreviousInputBox(activeBox());
    QDialog::accept();
}

void FindIconDialog::cdUp()
{
    QDir dir = m_view_dir;

#ifdef Q_OS_WIN
    if (dir.cdUp() && !isRoot) {
        setFile(dir.canonicalPath());
    } else if (!isRoot)
        setFile(rootDir);
#else
    if (dir.cdUp())
        setFile(dir.path());
#endif
  
    updateButtons();
}

void FindIconDialog::itemActivated(const QString&, const QString &file_name)
{
    if (activeBox() != ResourceBox)
        return;
    if (isIconValid(file_name))
        accept();

    updateButtons();
}

void FindIconDialog::itemActivated(QListWidgetItem *item)
{
    if (!item || activeBox() != FileBox)
        return;
    const QString file = item->text();
    const QString path = m_view_dir.filePath(file);

    if (dirItem(item)) {
#ifdef Q_OS_WIN
        isRoot = false;
#endif
        QMetaObject::invokeMethod(this, "setFile", Qt::QueuedConnection, Q_ARG(QString, path));
    } else
        accept();

    updateButtons();
}

void FindIconDialog::itemChanged(const QString &qrc_path, const QString &file_name)
{
    if (activeBox() != ResourceBox)
        return;

    m_resource_data.file = file_name;
    m_resource_data.qrc = qrc_path;

    updateButtons();
}

void FindIconDialog::currentItemChanged(QListWidgetItem *item)
{
    if (activeBox() != FileBox)
        return;

    if (item == 0)
        return;

    const QString path = m_view_dir.filePath(item->text());
    ui->m_file_input->lineEdit()->setText(path);

    if (dirItem(item))
        m_file_data.file.clear();
    else
        m_file_data.file = path;

    updateButtons();
}

void FindIconDialog::itemChanged( const QString &file_name)
{
    if (activeBox() != LanguageBox)
        return;

    m_language_data.file = file_name;

    updateButtons();
}

void FindIconDialog::itemActivated(const QString &file_name)
{
    if (activeBox() != LanguageBox)
        return;

    itemChanged(file_name);

    if (isIconValid(file_name))
        accept();

    updateButtons();
}

void FindIconDialog::setViewDir(const QString &path)
{
    static const QIcon dir_icon(style()->standardPixmap(QStyle::SP_DirClosedIcon));
#ifdef Q_OS_WIN
    static const QIcon drive_icon(style()->standardPixmap(QStyle::SP_DriveHDIcon));
    if(!isRoot)
#endif
    {
        if (path == m_view_dir.path() || !QFile::exists(path))
            return;
    }

    m_view_dir.setPath(path);
    ui->m_icon_view->clear();

    QStringList subdir_list;
    const QString wildcard = QString(QLatin1Char('*'));
    const QStringList wildcardList = QStringList(wildcard);
    
#ifdef Q_OS_WIN
    if (isRoot) {
        QFileInfoList qFIL = QDir::drives();
        foreach(const QFileInfo &info, qFIL) 
            subdir_list.append(info.path());
    } else
        subdir_list = m_view_dir.entryList(wildcardList, QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (const QString &subdir, subdir_list)
        createListWidgetItem((isRoot ? drive_icon : dir_icon), subdir, g_dir_item_id, ui->m_icon_view);
#else
    subdir_list = m_view_dir.entryList(wildcardList, QDir::Dirs | QDir::NoDotAndDotDot);
    foreach (const QString &subdir, subdir_list)
        createListWidgetItem(dir_icon, subdir, g_dir_item_id, ui->m_icon_view);
#endif

    const QStringList icon_file_list = m_view_dir.entryList(extensionList(), QDir::Files);
    foreach (const QString &icon_file, icon_file_list) {
        QIcon icon(m_view_dir.filePath(icon_file));
        if (!icon.isNull())
            createListWidgetItem(icon, icon_file, g_file_item_id, ui->m_icon_view);
    }
}

void FindIconDialog::setFile(const QString &path)
{
    QString file;
    QString dir = path;
#ifdef Q_OS_WIN
    isRoot = false;
    if (dir.contains(rootDir, Qt::CaseInsensitive))
        isRoot = true;

    if (!isRoot)
#endif
    {
        QFileInfo info(path);

        if (info.isFile()) {
            dir = info.path();
            file = info.fileName();
        }
    }

    setViewDir(dir);

    const int cursorPos = ui->m_file_input->lineEdit()->cursorPosition();
    ui->m_file_input->lineEdit()->setText(path);
    ui->m_file_input->lineEdit()->setCursorPosition(cursorPos);

    m_file_data.file.clear();
    ui->m_icon_view->clearSelection();
    if (!file.isEmpty()) {
        const QList<QListWidgetItem*> item_list = ui->m_icon_view->findItems(file, Qt::MatchExactly);
        if (!item_list.isEmpty()) {
            ui->m_icon_view->setItemSelected(item_list.first(), true);
            m_file_data.file = path;
        }
    }

    updateButtons();
}

FindIconDialog::~FindIconDialog()
{
    delete ui;
    ui = 0;
}

void FindIconDialog::setQrc(const QString &qrc_path, const QString &file_name)
{
    if (!m_resource_editor)
        return;
    m_resource_editor->setCurrentFile(qrc_path, file_name);
    m_resource_data.file = file_name;
    m_resource_data.qrc = qrc_path;
    updateButtons();
}

void FindIconDialog::setLanguagePath(const QString &file_name)
{
    if (!m_language_editor)
        return;
    m_language_editor->setCurrentPath(file_name);
    m_language_data.file = file_name;
    updateButtons();
}

void FindIconDialog::setPaths(const QString &qrcPath, const QString &filePath)
{
    if (!qrcPath.isEmpty()) {
        setFile(defaultFilePath(m_form));
        setActiveBox(ResourceBox);
        setQrc(qrcPath, filePath);
        return;
    }

    if (!filePath.isEmpty()) {
        QDesignerFormEditorInterface *core = m_form->core();
        QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension*>(core->extensionManager(), core);
        if (lang && lang->isLanguageResource(filePath)) {
            setActiveBox(LanguageBox);
            m_language_editor->setCurrentPath(filePath);
        } else {
            setActiveBox(FileBox);
            setFile(filePath);
        }
        return;
    }

    // both empty, default
    const InputBox previousBox = previousInputBox();
    switch (previousBox) {
    case LanguageBox:
    case ResourceBox:
         if (m_language_editor) {
             setLanguagePath(defaultLanguagePath());
             setActiveBox(LanguageBox);
         } else {
             setFile(defaultFilePath(m_form));
             setQrc(defaultQrcPath(), QString());
             setActiveBox(ResourceBox);
         }
         break;
    case FileBox:
         setFile(defaultFilePath(m_form));
         break;
    }
}

void FindIconDialog::updateButtons()
{
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isIconValid(filePath()));
}

void FindIconDialog::setActiveBox()
{
    InputBox inputBox = FileBox;
    if (sender() != ui->m_specify_file_input) {
        if (m_language_editor)
            inputBox = LanguageBox;
        else
            inputBox = ResourceBox;
    }
    setActiveBox(inputBox);
}

void FindIconDialog::setActiveBox(int box)
{
    switch (box) {
    case  FileBox:
        ui->m_specify_file_input->setChecked(true);
        ui->m_widget_stack->setCurrentIndex(1);
         break;
    default:
         ui->m_widget_stack->setCurrentIndex(m_language_editor ? 2 : 0);
         break;
    }
    updateButtons();
}

FindIconDialog::InputBox FindIconDialog::activeBox() const
{
    const int page = ui->m_widget_stack->currentIndex();
    switch (page) {
        case 2:  return LanguageBox;
        case 1:  return FileBox;
        case 0:
        default: return ResourceBox;
    }
    return FileBox;
}

QString FindIconDialog::qrcPath() const
{
    return activeBox() == ResourceBox ? m_resource_data.qrc : QString();
}

QString FindIconDialog::filePath() const
{
    switch (activeBox()) {
        case FileBox:     return m_file_data.file;
        case ResourceBox: return m_resource_data.file;
        case LanguageBox: return m_language_data.file;
    }
    return QString();
}

QString FindIconDialog::defaultQrcPath()
{
    QSettings settings;
    return settings.value(QLatin1String("FindIconDialog/defaultQrcPath")).toString();
}

QString FindIconDialog::defaultFilePath(QDesignerFormWindowInterface *form)
{
    QSettings settings;
    const QString path = settings.value(QLatin1String("FindIconDialog/defaultFilePath")).toString();
    if (path.isEmpty())
         return form->absoluteDir().path();
    return path;
}

QString FindIconDialog::defaultLanguagePath()
{
    QSettings settings;
    return settings.value(QLatin1String("FindIconDialog/defaultLanguagePath")).toString();
}

void FindIconDialog::setDefaultQrcPath(const QString &path)
{
    QSettings settings;
    settings.setValue(QLatin1String("FindIconDialog/defaultQrcPath"), path);
}

void FindIconDialog::setDefaultFilePath(const QString &path)
{
    QSettings settings;
    settings.setValue(QLatin1String("FindIconDialog/defaultFilePath"), path);
}

void FindIconDialog::setDefaultLanguagePath(const QString &path)
{
    QSettings settings;
    settings.setValue(QLatin1String("FindIconDialog/defaultLanguagePath"), path);
}

FindIconDialog::InputBox FindIconDialog::previousInputBox()
{
    QSettings settings;
    const QString box = settings.value(QLatin1String("FindIconDialog/previousInputBox")).toString();
    if (box == QLatin1String("language"))
        return LanguageBox;
    if (box == QLatin1String("file"))
        return FileBox;
    return ResourceBox;
}

void FindIconDialog::setPreviousInputBox(InputBox box)
{
    QSettings settings;
    QString val;
    switch (box) {
        case FileBox:     val = QLatin1String("file"); break;
        case ResourceBox: val = QLatin1String("resource"); break;
        case LanguageBox: val = QLatin1String("language"); break;
    }
    settings.setValue(QLatin1String("FindIconDialog/previousInputBox"), val);
}

} // namespace qdesigner_internal
