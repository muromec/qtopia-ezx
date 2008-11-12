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
#ifndef QMAILMESSAGE_H
#define QMAILMESSAGE_H

#include "qmailaddress.h"
#include "qmailid.h"
#include "qmailtimestamp.h"
#include "qprivateimplementation_p.h"

#include <QByteArray>
#include <QFlags>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>


class QMailMessagePart;
class QMailMessagePartContainerPrivate;
class QDataStream;
class QTextStream;
class QFile;

class QMailMessageHeaderFieldPrivate;

class QTOPIAMAIL_EXPORT QMailMessageHeaderField : public QPrivatelyImplemented<QMailMessageHeaderFieldPrivate>
{
public:
    typedef QMailMessageHeaderFieldPrivate ImplementationType;

    typedef QPair<QByteArray, QByteArray> ParameterType;

    enum FieldType
    {
        StructuredField = 1,
        UnstructuredField = 2
    };

    QMailMessageHeaderField();
    QMailMessageHeaderField(const QByteArray& text, FieldType fieldType = StructuredField);
    QMailMessageHeaderField(const QByteArray& name, const QByteArray& text, FieldType fieldType = StructuredField);

    bool isNull() const;

    QByteArray id() const;
    void setId(const QByteArray& text);

    QByteArray content() const;
    void setContent(const QByteArray& text);

    QByteArray parameter(const QByteArray& name) const;
    void setParameter(const QByteArray& name, const QByteArray& value);

    bool isParameterEncoded(const QByteArray& name) const;
    void setParameterEncoded(const QByteArray& name);

    QList<ParameterType> parameters() const;

    virtual QByteArray toString(bool includeName = true, bool presentable = true) const;

    virtual QString decodedContent() const;

    bool operator== (const QMailMessageHeaderField& other) const;

    static QByteArray encodeWord(const QString& input, const QByteArray& charset = "");
    static QString decodeWord(const QByteArray& input);

    static QByteArray encodeParameter(const QString& input, const QByteArray& charset = "", const QByteArray& language = "");
    static QString decodeParameter(const QByteArray& input);

    static QByteArray encodeContent(const QString& input, const QByteArray& charset = "");
    static QString decodeContent(const QByteArray& input);

    static QByteArray removeComments(const QByteArray& input);
    static QByteArray removeWhitespace(const QByteArray& input);

protected:
    void parse(const QByteArray& text, FieldType fieldType);

private:
    friend class QMailMessageHeaderFieldPrivate;
    friend class QMailMessageHeaderPrivate;

    void output(QDataStream& out) const;
};


class QTOPIAMAIL_EXPORT QMailMessageContentType : public QMailMessageHeaderField
{
public:
    QMailMessageContentType();
    QMailMessageContentType(const QByteArray& type);
    QMailMessageContentType(const QMailMessageHeaderField& field);

    QByteArray type() const;
    void setType(const QByteArray& type);

    QByteArray subType() const;
    void setSubType(const QByteArray& subType);

    QByteArray name() const;
    void setName(const QByteArray& name);

    QByteArray boundary() const;
    void setBoundary(const QByteArray& boundary);

    QByteArray charset() const;
    void setCharset(const QByteArray& charset);

private:
    // Don't allow the Id to be changed
    void setId(const QByteArray& text);
};


class QTOPIAMAIL_EXPORT QMailMessageContentDisposition : public QMailMessageHeaderField
{
public:
    enum DispositionType
    {
        None = 0,
        Inline = 1,
        Attachment = 2
    };

    QMailMessageContentDisposition();
    QMailMessageContentDisposition(const QByteArray& type);
    QMailMessageContentDisposition(DispositionType disposition);
    QMailMessageContentDisposition(const QMailMessageHeaderField& field);

    DispositionType type() const;
    void setType(DispositionType disposition);

    QByteArray filename() const;
    void setFilename(const QByteArray& filename);

    QMailTimeStamp creationDate() const;
    void setCreationDate(const QMailTimeStamp& timeStamp);

    QMailTimeStamp modificationDate() const;
    void setModificationDate(const QMailTimeStamp& timeStamp);

    QMailTimeStamp readDate() const;
    void setReadDate(const QMailTimeStamp& timeStamp);

    int size() const;
    void setSize(int size);

private:
    // Don't allow the Id to be changed
    void setId(const QByteArray& text);
};


class QMailMessageHeaderPrivate;

// This class is not exposed to clients:
class QMailMessageHeader : public QPrivatelyImplemented<QMailMessageHeaderPrivate>
{
public:
    typedef QMailMessageHeaderPrivate ImplementationType;

    QMailMessageHeader();
    QMailMessageHeader(const QByteArray& input);

    void update(const QByteArray& id, const QByteArray& content);
    void append(const QByteArray& id, const QByteArray& content);
    void remove(const QByteArray& id);

    QMailMessageHeaderField field(const QByteArray& id) const;
    QList<QMailMessageHeaderField> fields(const QByteArray& id) const;

    QList<const QByteArray*> fieldList() const;

private:
    friend class QMailMessageHeaderPrivate;
    friend class QMailMessagePartContainerPrivate;
    friend class QMailMessagePartPrivate;
    friend class QMailMessagePrivate;

    void output(QDataStream& out, const QList<QByteArray>& exclusions, bool stripInternal) const;
};


class QMailMessageBodyPrivate;

class QTOPIAMAIL_EXPORT QMailMessageBody : public QPrivatelyImplemented<QMailMessageBodyPrivate>
{
public:
    typedef QMailMessageBodyPrivate ImplementationType;

    enum TransferEncoding 
    {
        NoEncoding = 0,
        SevenBit = 1, 
        EightBit = 2, 
        Base64 = 3,
        QuotedPrintable = 4,
        Binary = 5, 
    };

    enum EncodingStatus
    {
        AlreadyEncoded = 1,
        RequiresEncoding = 2
    };

    enum EncodingFormat
    {
        Encoded = 1,
        Decoded = 2
    };

    // Construction functions
    static QMailMessageBody fromFile(const QString& filename, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);

    static QMailMessageBody fromStream(QDataStream& in, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);
    static QMailMessageBody fromData(const QByteArray& input, const QMailMessageContentType& type, TransferEncoding encoding, EncodingStatus status);

    static QMailMessageBody fromStream(QTextStream& in, const QMailMessageContentType& type, TransferEncoding encoding);
    static QMailMessageBody fromData(const QString& input, const QMailMessageContentType& type, TransferEncoding encoding);

    // Output functions
    bool toFile(const QString& filename, EncodingFormat format) const;

    QByteArray data(EncodingFormat format) const;
    bool toStream(QDataStream& out, EncodingFormat format) const;

    QString data() const;
    bool toStream(QTextStream& out) const;

    // Property accessors
    TransferEncoding transferEncoding() const;
    QMailMessageContentType contentType() const;

private:
    friend class QMailMessagePartContainerPrivate;

    QMailMessageBody();

    uint indicativeSize() const;

    void output(QDataStream& out, bool includeAttachments) const;
};

class QTOPIAMAIL_EXPORT QMailMessagePartContainer : public QPrivatelyImplemented<QMailMessagePartContainerPrivate>
{
public:
    typedef QMailMessagePartContainerPrivate ImplementationType;

    enum MultipartType 
    {
        MultipartNone = 0,
        MultipartSigned = 1,
        MultipartEncrypted = 2,
        MultipartMixed = 3,
        MultipartAlternative = 4,
        MultipartDigest = 5,
        MultipartParallel = 6,
        MultipartRelated = 7,
        MultipartFormData = 8,
        MultipartReport = 9
    };

    // Parts management interface:
    MultipartType multipartType() const;
    void setMultipartType(MultipartType type);

    uint partCount() const;
    void appendPart(const QMailMessagePart &part);
    void prependPart(const QMailMessagePart &part);

    const QMailMessagePart& partAt(uint pos) const;
    QMailMessagePart& partAt(uint pos);

    void clearParts();

    QByteArray boundary() const;
    void setBoundary(const QByteArray& text);

    // Body management interface:
    void setBody(const QMailMessageBody& body);
    QMailMessageBody body() const;

    bool hasBody() const;

    // Property accessors
    QMailMessageBody::TransferEncoding transferEncoding() const;
    QMailMessageContentType contentType() const;

    // Header fields describing this part container
    QString headerFieldText( const QString &id ) const;
    QMailMessageHeaderField headerField( const QString &id, QMailMessageHeaderField::FieldType fieldType = QMailMessageHeaderField::StructuredField ) const;

    QStringList headerFieldsText( const QString &id ) const;
    QList<QMailMessageHeaderField> headerFields( const QString &id, QMailMessageHeaderField::FieldType fieldType = QMailMessageHeaderField::StructuredField ) const;

    QList<QMailMessageHeaderField> headerFields() const;

    void setHeaderField( const QString &id, const QString& content );
    void setHeaderField( const QMailMessageHeaderField &field );

    void appendHeaderField( const QString &id, const QString& content );
    void appendHeaderField( const QMailMessageHeaderField &field );

    void removeHeaderField( const QString &id );

protected:
    template<typename Subclass>
    QMailMessagePartContainer(Subclass* p);

private:
    friend class QMailMessagePartContainerPrivate;

    void setHeader(const QMailMessageHeader& header, const QMailMessagePartContainerPrivate* parent = 0);

    uint indicativeSize() const;

    void outputParts(QDataStream& out, bool includePreamble, bool includeAttachments, bool stripInternal) const;
    void outputBody(QDataStream& out, bool includeAttachments) const;
};

class QMailMessagePartPrivate;

class QTOPIAMAIL_EXPORT QMailMessagePart : public QMailMessagePartContainer
{
public:
    typedef QMailMessagePartPrivate ImplementationType;

    QMailMessagePart();

    // Construction functions
    static QMailMessagePart fromFile(const QString& filename, const QMailMessageContentDisposition& disposition, 
                                     const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, 
                                     QMailMessageBody::EncodingStatus status = QMailMessageBody::RequiresEncoding);

    static QMailMessagePart fromStream(QDataStream& in, const QMailMessageContentDisposition& disposition, 
                                       const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, 
                                       QMailMessageBody::EncodingStatus status = QMailMessageBody::RequiresEncoding);
    static QMailMessagePart fromData(const QByteArray& input, const QMailMessageContentDisposition& disposition, 
                                     const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding, 
                                     QMailMessageBody::EncodingStatus status = QMailMessageBody::RequiresEncoding);

    static QMailMessagePart fromStream(QTextStream& in, const QMailMessageContentDisposition& disposition, 
                                       const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);
    static QMailMessagePart fromData(const QString& input, const QMailMessageContentDisposition& disposition, 
                                     const QMailMessageContentType& type, QMailMessageBody::TransferEncoding encoding);

    QString contentID() const;
    void setContentID(const QString &s);

    QString contentLocation() const;
    void setContentLocation(const QString &s);

    QString contentDescription() const;
    void setContentDescription(const QString &s);

    QMailMessageContentDisposition contentDisposition() const;
    void setContentDisposition(const QMailMessageContentDisposition& disposition);

    QString contentLanguage() const;
    void setContentLanguage(const QString &s);

    int partNumber() const;
    void setPartNumber(int);

    QString displayName() const;
    QString identifier() const;

    QString attachmentPath() const;
    bool detachAttachment(const QString& path);

private:
    friend class QMailMessagePrivate;
    friend class QMailMessagePartContainerPrivate;

    void setAttachmentPath(const QString& path);

    void output(QDataStream& out, bool includeAttachments, bool stripInternal) const;
};

class QMailMessagePrivate;

class QTOPIAMAIL_EXPORT QMailMessage : public QMailMessagePartContainer
{
public:
    typedef QMailMessagePrivate ImplementationType;

    // Mail content needs to use CRLF explicitly
    static const char CarriageReturn;
    static const char LineFeed;
    static const char* CRLF;

    static QMailMessage fromRfc2822(const QByteArray &ba);
    static QMailMessage fromRfc2822File(const QString& fileName);

    enum EncodingFormat
    {
        HeaderOnlyFormat = 1,
        StorageFormat = 2,
        TransmissionFormat = 3,
        IdentityFormat = 4,
        SerializationFormat = 5
    }; 

    QByteArray toRfc2822(EncodingFormat format = TransmissionFormat) const;
    void toRfc2822(QDataStream& out, EncodingFormat format = TransmissionFormat) const;

    enum MailDataSelection
    {
        Header,
        HeaderAndBody	
    };

    QMailMessage();
    QMailMessage(const QMailId& id, MailDataSelection selection = HeaderAndBody);
    QMailMessage(const QString& uid, const QString& account, MailDataSelection selection = HeaderAndBody);

    enum MessageType
    {
        Mms     = 0x1,
        Sms     = 0x4,
        Email   = 0x8,
        System  = 0x10,
        None    = 0,
        AnyType = Mms | Sms | Email | System
    };

    enum MessageStatusFlag {
        Incoming       = 0x0001,
        Outgoing       = 0x0002,
        Sent           = 0x0004,
        Replied        = 0x0008,
        RepliedAll     = 0x0010,
        Forwarded      = 0x0020,
        Downloaded     = 0x0040,
        Read           = 0x0080,
        Removed        = 0x0100,
        ReadElsewhere  = 0x0200,
    };
    Q_DECLARE_FLAGS(Status, MessageStatusFlag)

    enum AttachmentsAction {
        LinkToAttachments = 0,
        CopyAttachments,
        CopyAndDeleteAttachments
    };

    QMailId id() const;
    void setId(QMailId id);

    QMailId parentFolderId() const;
    void setParentFolderId(QMailId val);

    MessageType messageType() const;
    void setMessageType(MessageType t);

    QMailAddress from() const;
    void setFrom(const QMailAddress &s);

    QString subject() const;
    void setSubject(const QString &s);

    QMailTimeStamp date() const;
    void setDate(const QMailTimeStamp &s);

    QList<QMailAddress> to() const;
    void setTo(const QList<QMailAddress>& s);
    void setTo(const QMailAddress& s);

    QList<QMailAddress> cc() const;
    void setCc(const QList<QMailAddress>& s);
    QList<QMailAddress> bcc() const;
    void setBcc(const QList<QMailAddress>& s);

    QList<QMailAddress> recipients() const;
    bool hasRecipients() const;

    QMailAddress replyTo() const;
    void setReplyTo(const QMailAddress &s);

    QString inReplyTo() const;
    void setInReplyTo(const QString &s);

    Status status() const;
    void setStatus(Status s);
    void setStatus(MessageStatusFlag flag, bool set);

    QString fromAccount() const;
    void setFromAccount(const QString &s);

    QString fromMailbox() const;
    void setFromMailbox(const QString &s);

    QString serverUid() const;
    void setServerUid(const QString &s);

    uint size() const;
    void setSize(uint i);

    uint indicativeSize() const;

    template <typename Stream> void serialize(Stream &stream) const;
    template <typename Stream> void deserialize(Stream &stream);

private:
    friend class QMailStore;
    friend class QMailStorePrivate;

    bool uncommittedChanges() const;
    bool uncommittedMetadataChanges() const;
    void changesCommitted();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QMailMessage::Status)

Q_DECLARE_USER_METATYPE_ENUM(QMailMessage::MessageType)
Q_DECLARE_USER_METATYPE_ENUM(QMailMessage::AttachmentsAction)

Q_DECLARE_USER_METATYPE(QMailMessage)

typedef QList<QMailMessage> QMailMessageList;

Q_DECLARE_METATYPE(QMailMessageList)
Q_DECLARE_USER_METATYPE_TYPEDEF(QMailMessageList, QMailMessageList)


// There is no good place to put this code; define it here, but we won't expose it to external clients
namespace QMail
{

    template<typename StringType>
    StringType unquoteString(const StringType& src)
    {
        // If a string has double-quote as the first and last characters, return the string
        // between those characters
        int length = src.length();
        if (length)
        {
            typename StringType::const_iterator const begin = src.constData();
            typename StringType::const_iterator const last = begin + length - 1;

            if ((last > begin) && (*begin == '"' && *last == '"'))
                return src.mid(1, length - 2);
        }

        return src;
    }

    template<typename StringType>
    StringType quoteString(const StringType& src)
    {
        StringType result("\"\"");

        // Return the input string surrounded by double-quotes, which are added if not present
        int length = src.length();
        if (length)
        {
            result.reserve(length + 2);

            typename StringType::const_iterator begin = src.constData();
            typename StringType::const_iterator last = begin + length - 1;

            if (*begin == '"')
                begin += 1;

            if ((last >= begin) && (*last == '"'))
                last -= 1;

            if (last >= begin)
                result.insert(1, StringType(begin, (last - begin + 1)));
        }

        return result;
    }

}

#endif
