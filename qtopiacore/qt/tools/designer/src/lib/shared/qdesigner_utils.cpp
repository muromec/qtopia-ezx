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

#include "qdesigner_utils_p.h"
#include "resourcemimedata_p.h"

#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerIconCacheInterface>

#include <QtGui/QIcon>
#include <QtGui/QPixmap>
#include <QtCore/QDir>

#include <QtGui/QApplication>
#include <QtCore/QProcess>
#include <QtCore/QLibraryInfo>

namespace qdesigner_internal
{
    QDESIGNER_SHARED_EXPORT void designerWarning(const QString &message)
    {
        qWarning("Designer: %s", qPrintable(message));
    }

    QString EnumType::id() const
    {
        const int v = value.toInt();
        const ItemMap::const_iterator cend = items.constEnd();
        for (ItemMap::const_iterator it = items.constBegin();it != cend; ++it )  {
            if (it.value().toInt() == v)
                return it.key();
        }
        return QString();
    }

    // ------- FlagType
    QStringList FlagType::flags() const
    {
        QStringList rc;
        const uint v = value.toUInt();
        const ItemMap::const_iterator cend = items.constEnd();
        for (ItemMap::const_iterator it = items.constBegin();it != cend; ++it )  {
            const uint itemValue = it.value().toUInt();
            // Check for equality first as flag values can be 0 or -1, too. Takes preference over a bitwise flag
            if (v == itemValue) {
                rc.clear();
                rc.push_back(it.key());
                return rc;
            }
            if ((v & itemValue) == itemValue)
                rc.push_back(it.key());
        }
        return rc;
    }

    QString FlagType::flagString() const
    {
        const QStringList flagIds = flags();
        switch (flagIds.size()) {
        case 0:
            return QString();
        case 1:
            return flagIds.front();
        default:
            break;
        }
        static const QString delimiter = QString(QLatin1Char('|'));
        return flagIds.join(delimiter);
    }

    // Convenience to return an icon normalized to form directory
    QDESIGNER_SHARED_EXPORT QIcon resourceMimeDataToIcon(const ResourceMimeData &rmd, QDesignerFormWindowInterface *fw)
    {
        if (rmd.type() != ResourceMimeData::Image)
            return QIcon();

        const QString normalizedQrcPath = fw->absoluteDir().absoluteFilePath(rmd.qrcPath());
        const QIcon rc =  fw->core()->iconCache()->nameToIcon(rmd.filePath(), normalizedQrcPath);
        return rc;
    }
    // Convenience to return an icon normalized to form directory
    QDESIGNER_SHARED_EXPORT QPixmap resourceMimeDataToPixmap(const ResourceMimeData &rmd, QDesignerFormWindowInterface *fw)
    {
        if (rmd.type() != ResourceMimeData::Image)
            return QPixmap();

        const QString normalizedQrcPath = fw->absoluteDir().absoluteFilePath(rmd.qrcPath());
        const QPixmap rc =  fw->core()->iconCache()->nameToPixmap(rmd.filePath(), normalizedQrcPath);
        return rc;
    }


    QDESIGNER_SHARED_EXPORT bool runUIC(const QString &fileName, UIC_Mode mode, QByteArray& ba, QString &errorMessage)
    {
        QStringList argv;
        QString binary = QLibraryInfo::location(QLibraryInfo::BinariesPath);
        binary += QDir::separator();
        switch (mode) {
        case UIC_GenerateCode:
            binary += QLatin1String("uic");
            break;
        case UIC_ConvertV3:
            binary += QLatin1String("uic3");
            argv += QLatin1String("-convert");
            break;
        }
        argv += fileName;
        QProcess uic;
        uic.start(binary, argv);
        if (!uic.waitForStarted()) {
            errorMessage = QApplication::translate("Designer", "Unable to launch %1.").arg(binary);
            return false;
        }
        if (!uic.waitForFinished()) {
            errorMessage = QApplication::translate("Designer", "%1 timed out.").arg(binary);
            return false;
        }
        if (uic.exitCode()) {
            errorMessage =  uic.readAllStandardError();
            return false;
        }
        ba = uic.readAllStandardOutput();
        return true;
    }
} // namespace qdesigner_internal
