/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtXML module of the Qt Toolkit.
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

#ifndef QXMLSTREAM_H
#define QXMLSTREAM_H

#include <QtCore/QIODevice>
#include <QtCore/QString>
#include <QtCore/QVector>

QT_BEGIN_HEADER

QT_MODULE(Xml)

class Q_XML_EXPORT QXmlStreamStringRef {
    QString m_string;
    int m_position, m_size;
public:
    inline QXmlStreamStringRef():m_position(0), m_size(0){}
    inline QXmlStreamStringRef(const QStringRef &string)
        :m_string(string.string()?*string.string():QString()), m_position(string.position()), m_size(string.size()){}
    inline ~QXmlStreamStringRef(){}
    inline void clear() { m_string.clear(); m_position = m_size = 0; }
    inline operator QStringRef() const { return QStringRef(&m_string, m_position, m_size); }
};


class QXmlStreamReaderPrivate;
class QXmlStreamAttributes;
class Q_XML_EXPORT QXmlStreamAttribute {
    QXmlStreamStringRef m_name, m_namespaceUri, m_qualifiedName, m_value;
    void *reserved;
    uint m_isDefault : 1;
    friend class QXmlStreamReaderPrivate;
    friend class QXmlStreamAttributes;
public:
    QXmlStreamAttribute();
    QXmlStreamAttribute(const QString &qualifiedName, const QString &value);
    QXmlStreamAttribute(const QString &namespaceUri, const QString &name, const QString &value);
    QXmlStreamAttribute(const QXmlStreamAttribute &);
    QXmlStreamAttribute& operator=(const QXmlStreamAttribute &);
    ~QXmlStreamAttribute();
    inline QStringRef namespaceUri() const { return m_namespaceUri; }
    inline QStringRef name() const { return m_name; }
    inline QStringRef qualifiedName() const { return m_qualifiedName; }
    inline QStringRef value() const { return m_value; }
    inline bool isDefault() const { return m_isDefault; }
    inline bool operator==(const QXmlStreamAttribute &other) const {
        return (value() == other.value()
                && (namespaceUri().isNull() ? (qualifiedName() == other.qualifiedName())
                    : (namespaceUri() == other.namespaceUri() && name() == other.name())));
    }
    inline bool operator!=(const QXmlStreamAttribute &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamAttribute, Q_MOVABLE_TYPE);

class Q_XML_EXPORT QXmlStreamAttributes : public QVector<QXmlStreamAttribute>
{
public:
    QStringRef value(const QString &namespaceUri, const QString &name) const;
    QStringRef value(const QString &namespaceUri, const QLatin1String &name) const;
    QStringRef value(const QLatin1String &namespaceUri, const QLatin1String &name) const;
    QStringRef value(const QString &qualifiedName) const;
    QStringRef value(const QLatin1String &qualifiedName) const;
    void append(const QString &namespaceUri, const QString &name, const QString &value);
    void append(const QString &qualifiedName, const QString &value);
#if !defined(Q_NO_USING_KEYWORD)
    using QVector<QXmlStreamAttribute>::append;
#else
    inline void append(const QXmlStreamAttribute &attribute)
        { QVector<QXmlStreamAttribute>::append(attribute); }
#endif
};

class Q_XML_EXPORT QXmlStreamNamespaceDeclaration {
    QXmlStreamStringRef m_prefix, m_namespaceUri;
    void *reserved;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamNamespaceDeclaration();
    QXmlStreamNamespaceDeclaration(const QXmlStreamNamespaceDeclaration &);
    QXmlStreamNamespaceDeclaration& operator=(const QXmlStreamNamespaceDeclaration &);
    ~QXmlStreamNamespaceDeclaration();
    inline QStringRef prefix() const { return m_prefix; }
    inline QStringRef namespaceUri() const { return m_namespaceUri; }
    inline bool operator==(const QXmlStreamNamespaceDeclaration &other) const {
        return (prefix() == other.prefix() && namespaceUri() == other.namespaceUri());
    }
    inline bool operator!=(const QXmlStreamNamespaceDeclaration &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamNamespaceDeclaration, Q_MOVABLE_TYPE);
typedef QVector<QXmlStreamNamespaceDeclaration> QXmlStreamNamespaceDeclarations;

class Q_XML_EXPORT QXmlStreamNotationDeclaration {
    QXmlStreamStringRef m_name, m_systemId, m_publicId;
    void *reserved;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamNotationDeclaration();
    ~QXmlStreamNotationDeclaration();
    QXmlStreamNotationDeclaration(const QXmlStreamNotationDeclaration &);
    QXmlStreamNotationDeclaration& operator=(const QXmlStreamNotationDeclaration &);
    inline QStringRef name() const { return m_name; }
    inline QStringRef systemId() const { return m_systemId; }
    inline QStringRef publicId() const { return m_publicId; }
    inline bool operator==(const QXmlStreamNotationDeclaration &other) const {
        return (name() == other.name() && systemId() == other.systemId()
                && publicId() == other.publicId());
    }
    inline bool operator!=(const QXmlStreamNotationDeclaration &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamNotationDeclaration, Q_MOVABLE_TYPE);
typedef QVector<QXmlStreamNotationDeclaration> QXmlStreamNotationDeclarations;

class Q_XML_EXPORT QXmlStreamEntityDeclaration {
    QXmlStreamStringRef m_name, m_notationName, m_systemId, m_publicId, m_value;
    void *reserved;

    friend class QXmlStreamReaderPrivate;
public:
    QXmlStreamEntityDeclaration();
    ~QXmlStreamEntityDeclaration();
    QXmlStreamEntityDeclaration(const QXmlStreamEntityDeclaration &);
    QXmlStreamEntityDeclaration& operator=(const QXmlStreamEntityDeclaration &);
    inline QStringRef name() const { return m_name; }
    inline QStringRef notationName() const { return m_notationName; }
    inline QStringRef systemId() const { return m_systemId; }
    inline QStringRef publicId() const { return m_publicId; }
    inline QStringRef value() const { return m_value; }
    inline bool operator==(const QXmlStreamEntityDeclaration &other) const {
        return (name() == other.name()
                && notationName() == other.notationName()
                && systemId() == other.systemId()
                && publicId() == other.publicId()
                && value() == other.value());
    }
    inline bool operator!=(const QXmlStreamEntityDeclaration &other) const
        { return !operator==(other); }
};

Q_DECLARE_TYPEINFO(QXmlStreamEntityDeclaration, Q_MOVABLE_TYPE);
typedef QVector<QXmlStreamEntityDeclaration> QXmlStreamEntityDeclarations;



class Q_XML_EXPORT QXmlStreamReader {
    QDOC_PROPERTY(bool namespaceProcessing READ namespaceProcessing WRITE setNamespaceProcessing)
public:
    enum TokenType {
        NoToken = 0,
        Invalid,
        StartDocument,
        EndDocument,
        StartElement,
        EndElement,
        Characters,
        Comment,
        DTD,
        EntityReference,
        ProcessingInstruction
    };


    QXmlStreamReader();
    QXmlStreamReader(QIODevice *device);
    QXmlStreamReader(const QByteArray &data);
    QXmlStreamReader(const QString &data);
    QXmlStreamReader(const char * data);
    ~QXmlStreamReader();

    void setDevice(QIODevice *device);
    QIODevice *device() const;
    void addData(const QByteArray &data);
    void addData(const QString &data);
    void addData(const char *data);
    void clear();


    bool atEnd() const;
    TokenType readNext();

    TokenType tokenType() const;
    QString tokenString() const;

    void setNamespaceProcessing(bool);
    bool namespaceProcessing() const;

    inline bool isStartDocument() const { return tokenType() == StartDocument; }
    inline bool isEndDocument() const { return tokenType() == EndDocument; }
    inline bool isStartElement() const { return tokenType() == StartElement; }
    inline bool isEndElement() const { return tokenType() == EndElement; }
    inline bool isCharacters() const { return tokenType() == Characters; }
    bool isWhitespace() const;
    bool isCDATA() const;
    inline bool isComment() const { return tokenType() == Comment; }
    inline bool isDTD() const { return tokenType() == DTD; }
    inline bool isEntityReference() const { return tokenType() == EntityReference; }
    inline bool isProcessingInstruction() const { return tokenType() == ProcessingInstruction; }

    bool isStandaloneDocument() const;

    qint64 lineNumber() const;
    qint64 columnNumber() const;
    qint64 characterOffset() const;

    QXmlStreamAttributes attributes() const;
    QString readElementText();

    QStringRef name() const;
    QStringRef namespaceUri() const;
    QStringRef qualifiedName() const;

    QStringRef processingInstructionTarget() const;
    QStringRef processingInstructionData() const;

    QStringRef text() const;

    QXmlStreamNamespaceDeclarations namespaceDeclarations() const;
    QXmlStreamNotationDeclarations notationDeclarations() const;
    QXmlStreamEntityDeclarations entityDeclarations() const;


    enum Error {
        NoError,
        UnexpectedElementError,
        CustomError,
        NotWellFormedError,
        PrematureEndOfDocumentError
    };
    void raiseError(const QString& message = QString());
    QString errorString() const;
    Error error() const;

    inline bool hasError() const
    {
        return error() != NoError;
    }

private:
    Q_DISABLE_COPY(QXmlStreamReader)
    Q_DECLARE_PRIVATE(QXmlStreamReader)
    QXmlStreamReaderPrivate *d_ptr;

};



class QXmlStreamWriterPrivate;

class Q_XML_EXPORT QXmlStreamWriter
{
    QDOC_PROPERTY(bool autoFormatting READ autoFormatting WRITE setAutoFormatting)
public:
    QXmlStreamWriter();
    QXmlStreamWriter(QIODevice *device);
    QXmlStreamWriter(QByteArray *array);
    QXmlStreamWriter(QString *string);
    ~QXmlStreamWriter();

    void setDevice(QIODevice *device);
    QIODevice *device() const;

#ifndef QT_NO_TEXTCODEC
    void setCodec(QTextCodec *codec);
    void setCodec(const char *codecName);
    QTextCodec *codec() const;
#endif

    void setAutoFormatting(bool);
    bool autoFormatting() const;

    void writeAttribute(const QString &qualifiedName, const QString &value);
    void writeAttribute(const QString &namespaceUri, const QString &name, const QString &value);
    void writeAttribute(const QXmlStreamAttribute& attribute);
    void writeAttributes(const QXmlStreamAttributes& attributes);

    void writeCDATA(const QString &text);
    void writeCharacters(const QString &text);
    void writeComment(const QString &text);

    void writeDTD(const QString &dtd);

    void writeEmptyElement(const QString &qualifiedName);
    void writeEmptyElement(const QString &namespaceUri, const QString &name);

    void writeTextElement(const QString &qualifiedName, const QString &text);
    void writeTextElement(const QString &namespaceUri, const QString &name, const QString &text);

    void writeEndDocument();
    void writeEndElement();

    void writeEntityReference(const QString &name);
    void writeNamespace(const QString &namespaceUri, const QString &prefix = QString());
    void writeDefaultNamespace(const QString &namespaceUri);
    void writeProcessingInstruction(const QString &target, const QString &data = QString());

    void writeStartDocument();
    void writeStartDocument(const QString &version);
    void writeStartElement(const QString &qualifiedName);
    void writeStartElement(const QString &namespaceUri, const QString &name);

    void writeCurrentToken(const QXmlStreamReader &reader);

private:
    Q_DISABLE_COPY(QXmlStreamWriter)
    Q_DECLARE_PRIVATE(QXmlStreamWriter)
    QXmlStreamWriterPrivate *d_ptr;
};

QT_END_HEADER

#endif // QXMLSTREAM_H
