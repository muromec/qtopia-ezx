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

#include "resourcemimedata_p.h"

#include <QtCore/QStringList>
#include <QtCore/QMimeData>
#include <QtXml/QDomDocument>

static const char *elementResourceData = "resource";
static const char *typeAttribute = "type";
static const char *typeImage = "image";
static const char *typeFile = "file";
static const char *qrcAttribute = "qrc";
static const char *fileAttribute = "file";

static bool readResourceMimeData(const QMimeData *md,
                                 qdesigner_internal::ResourceMimeData::Type *t = 0,
                                 QString *qrc = 0, QString *file = 0)
{
    if (!md->hasText())
        return false;

    const QString docElementName = QLatin1String(elementResourceData);
    static const QString docElementString = QLatin1Char('<') + docElementName;

    const QString text = md->text();
    if (text.isEmpty() || text.indexOf(docElementString) == -1)
        return false;

    QDomDocument doc;
    if (!doc.setContent(text))
        return false;

    const QDomElement domElement = doc.documentElement();
    if (domElement.tagName() != docElementName)
        return false;

    if (t) {
        const QString typeAttr = QLatin1String(typeAttribute);
        if (domElement.hasAttribute (typeAttr)) {
            const QString fileTypeValue = QLatin1String(typeFile);
            *t = domElement.attribute(typeAttr, fileTypeValue) == fileTypeValue ?
            qdesigner_internal::ResourceMimeData::File : qdesigner_internal::ResourceMimeData::Image;
        } else {
            *t = qdesigner_internal::ResourceMimeData::File;
        }
    }

    if (qrc) {
        const QString qrcAttr = QLatin1String(qrcAttribute);
        if (domElement.hasAttribute(qrcAttr)) {
            *qrc = domElement.attribute(qrcAttr, QString());
        } else {
            qrc->clear();
        }
    }

    if (file) {
        const QString fileAttr = QLatin1String(fileAttribute);
        if (domElement.hasAttribute(fileAttr)) {
            *file = domElement.attribute(fileAttr, QString());
        } else {
            file->clear();
        }
    }

    return true;
}

namespace qdesigner_internal {

ResourceMimeData::ResourceMimeData(Type t) :
    m_type(t)
{
}

QMimeData *ResourceMimeData::toMimeData() const
{
    QDomDocument doc;
    QDomElement elem = doc.createElement(QLatin1String(elementResourceData));
    elem.setAttribute(QLatin1String(typeAttribute), QLatin1String(m_type == Image ? typeImage : typeFile));
    if (!m_qrcPath.isEmpty())
        elem.setAttribute(QLatin1String(qrcAttribute), m_qrcPath);
    if (!m_filePath.isEmpty())
        elem.setAttribute(QLatin1String(fileAttribute), m_filePath);

    doc.appendChild(elem);

    QMimeData *rc = new QMimeData;
    rc->setText(doc.toString());
    return rc;
}

bool ResourceMimeData::isResourceMimeData(const QMimeData *md)
{
    return readResourceMimeData(md);
}

bool  ResourceMimeData::isResourceMimeData(const QMimeData *md, Type desiredType)
{
    Type t;
    return readResourceMimeData(md, &t) && t == desiredType;
}

bool  ResourceMimeData::fromMimeData(const QMimeData *md)
{
    Type type;
    QString file;
    QString qrc;
    if (!readResourceMimeData(md, &type, &qrc, &file))
        return false;
    m_type = type;
    m_qrcPath = qrc;
    m_filePath = file;
    return true;
}

}

