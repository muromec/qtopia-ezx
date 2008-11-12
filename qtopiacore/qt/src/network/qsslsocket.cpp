/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtNetwork module of the Qt Toolkit.
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
** In addition, as a special exception, Trolltech gives permission to
** link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of it that use the same
** license as the "OpenSSL" library), and distribute the linked
** executables.  You must comply with the GNU General Public License
** version 2 or the GNU General Public License version 3 in all
** respects for all of the code used other than the "OpenSSL" code.
** If you modify this file, you may extend this exception to your
** version of the file, but you are not obligated to do so.  If you do
** not wish to do so, delete this exception statement from your
** version of this file.
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


//#define QSSLSOCKET_DEBUG

/*!
    \class QSslSocket
    \brief The QSslSocket class provides an SSL encrypted socket for both
    clients and servers.
    \since 4.3

    \reentrant
    \ingroup io
    \module network

    QSslSocket establishes a secure, encrypted TCP connection you can
    use for transmitting encrypted data. It can operate in both client
    and server mode, and it supports modern SSL protocols, including
    SSLv3 and TLSv1. By default, QSslSocket uses SSLv3, but you can
    change the SSL protocol by calling setProtocol() as long as you do
    it before the handshake has started.

    SSL encryption operates on top of the existing TCP stream after
    the socket enters the ConnectedState. There are two simple ways to
    establish a secure connection using QSslSocket: With an immediate
    SSL handshake, or with a delayed SSL handshake occurring after the
    connection has been established in unencrypted mode.

    The most common way to use QSslSocket is to construct an object
    and start a secure connection by calling connectToHostEncrypted().
    This method starts an immediate SSL handshake once the connection
    has been established.

    \code
        QSslSocket *socket = new QSslSocket(this);
        connect(socket, SIGNAL(encrypted()), this, SLOT(ready()));

        socket->connectToHostEncrypted("imap.example.com", 993);
    \endcode

    As with a plain QTcpSocket, QSslSocket enters the HostLookupState,
    ConnectingState, and finally the ConnectedState, if the connection
    is successful. The hand shake then starts automatically, and if it
    succeeds, the encrypted() signal is emitted to indicate the socket
    has entered the encrypted state and is ready for use.

    Note that data can be written to the socket immediately after the
    return from connectToHostEncrypted() (i.e., before the encrypted()
    signal is emitted). The data is queued in QSslSocket until after
    the encrypted() signal is emitted.

    An example of using the delayed SSL handshake to secure an
    existing connection is the case where an SSL server secures an
    incoming connection. Suppose you create an SSL server class as a
    subclass of QTcpServer. You would override
    QTcpServer::incomingConnection() with something like the example
    below, which first constructs an instance of QSslSocket and then
    calls setSocketDescriptor() to set the new socket's descriptor to
    the exiasting one passed in. It then initiates the SSL handshake
    by calling startServerEncryption().

    \code
        void SslServer::incomingConnection(int socketDescriptor)
        {
            QSslSocket *serverSocket = new QSslSocket;
            if (serverSocket->setSocketDescriptor(socketDescriptor)) {
                connect(serverSocket, SIGNAL(encrypted()), this, SLOT(ready()));
                serverSocket->startServerEncryption();
            } else {
                delete serverSocket;
            }
        }
    \endcode
    
    If an error occurs, QSslSocket emits signal sslErrors. In this
    case, if no action is taken to ignore the error(s), the connection
    is dropped. To continue, despite the occurrence of an error, you
    can call ignoreSslErrors(), either from within this slot after the
    error occurs, or anytime after construction of the QSslSocket and
    before the connection is attempted. This will allow QSslSocket to
    ignore the errors it encounters when establishing the identity of
    the peer. Ignoring errors during an SSL handshake should be used
    with caution, since a fundamental characteristic of secure
    connections is that they should be established with a successful
    handshake.
    
    Once encrypted, you use QSslSocket as a regular QTcpSocket. When
    readyRead() is emitted, you can call read(), canReadLine() and
    readLine(), or getChar() to read decrypted data from QSslSocket's
    internal buffer, and you can call write() or putChar() to write
    data back to the peer. QSslSocket will automatically encrypt the
    written data for you, and emit bytesWritten() once the data has
    been written to the peer.

    As a convenience, QSslSocket supports QTcpSocket's blocking
    functions waitForConnected(), waitForReadyRead(),
    waitForBytesWritten(), and waitForDisconnected(). It also provides
    waitForEncrypted(), which will block the calling thread until an
    encrypted connection has been established.

    \code
        QSslSocket socket;
        socket.connectToHostEncrypted("http.example.com", 443);
        if (!socket.waitForEncrypted()) {
            qDebug() << socket.errorString();
            return false;
        }

        socket.write("GET / HTTP/1.0\r\n\r\n");
        while (socket.waitForReadyRead())
            qDebug() << socket.readAll().data();
    \endcode

    QSslSocket provides an extensive, easy-to-use API for handling
    cryptographic ciphers, private keys, and local, peer, and
    Certification Authority (CA) certificates. It also provides an API
    for handling errors that occur during the handshake phase.

    Customize the socket's cryptographic cipher suite before the
    handshake phase with setCiphers() and setDefaultCiphers().
    Customize the socket's local certificate and private key before
    the handshake phase with setLocalCertificate() and setPrivateKey.
    Customize the CA certificate database with addCaCertificate(),
    addCaCertificates(), setCaCertificates(), addDefaultCaCertificate(),
    addDefaultCaCertificates(), and setDefaultCaCertificates(). For more
    information about ciphers and certificates, refer to QSslCipher and
    QSslCertificate.
    
    This product uses software developed by the OpenSSL Project for use in the
    OpenSSL Toolkit. You can download the necessary software from
    http://www.openssl.org/.

    \sa QSslCertificate, QSslCipher, QSslError
*/

/*!
    \enum QSslSocket::SslMode

    Describes the connection modes available for QSslSocket.

    \value UnencryptedMode The socket is unencrypted. Its
    behavior is identical to QTcpSocket.

    \value SslClientMode The socket is a client-side SSL socket.
    It is either alreayd encrypted, or it is in the SSL handshake
    phase (see QSslSocket::isEncrypted()).

    \value SslServerMode The socket is a client-side SSL socket.
    It is either already encrypted, or it is in the SSL handshake
    phase (see QSslSocket::isEncrypted()).
*/

/*!
    \fn QSslSocket::encrypted()

    This signal is emitted when QSslSocket enters encrypted mode. After this
    signal has been emitted, QSslSocket::isEncrypted() will return true, and
    all further transmissions on the socket will be encrypted.

    \sa QSslSocket::connectToHostEncrypted(), QSslSocket::isEncrypted()
*/

/*!
    \fn QSslSocket::modeChanged(QSslSocket::SslMode mode)

    This signal is emitted when QSslSocket changes from \l
    QSslSocket::UnencryptedMode to either \l QSslSocket::SslClientMode or \l
    QSslSocket::SslServerMode. \a mode is the new mode.

    \sa QSslSocket::mode()
*/

/*!
    \fn void QSslSocket::sslErrors(const QList<QSslError> &errors);
    
    QSslSocket emits this signal during the SSL handshake to indicate that an
    error has occurred while establishing the identity of the peer. The error
    is usually an indication that QSslSocket is unable to securely identify
    the peer. Unless any action is taken, the connection will be dropped after
    this signal has been emitted.

    If you want to continue connecting despite the errors that have occurred,
    you must call QSslSocket::ignoreErrors() from inside a slot connected to
    this signal. If you need to access the error list at a later point, you
    can call sslErrors() (without arguments).

    \a errors contains one or more errors that prevent QSslSocket from
    verifying the identity of the peer.
    
    Note: You cannot use Qt::QueuedConnection when connecting to this signal,
    or calling QSslSocket::ignoreErrors() will have no effect.
*/

#include "qsslcipher.h"
#include "qsslsocket.h"
#include "qsslsocket_openssl_p.h"

#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qmutex.h>
#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qhostinfo.h>

class QSslSocketGlobalData
{
public:
    QMutex mutex;
    QList<QSslCipher> ciphers;
    QList<QSslCipher> supportedCiphers;
    QList<QSslCertificate> caCertificates;
};
Q_GLOBAL_STATIC(QSslSocketGlobalData, globalData)

/*!
    Constructs a QSslSocket object. \a parent is passed to QObject's
    constructor. The new socket's \l {QSslCipher} {cipher} suite is
    set to the one returned by the static method defaultCiphers().
*/
QSslSocket::QSslSocket(QObject *parent)
    : QTcpSocket(*new QSslSocketBackendPrivate, parent)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::QSslSocket(" << parent << "), this =" << (void *)this;
#endif
    d->q_ptr = this;
    d->init();
    setCiphers(defaultCiphers());
}

/*!
    Destroys the QSslSocket.
*/
QSslSocket::~QSslSocket()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::~QSslSocket(), this =" << (void *)this;
#endif
    delete d->plainSocket;
    d->plainSocket = 0;
}

/*!
    Starts an encrypted connection to the device \a hostName on \a
    port, using \a mode as the \l OpenMode. This is equivalent to
    calling connectToHost() to establish the connection, followed by a
    call to startClientEncryption().

    QSslSocket first enters the HostLookupState. Then, after entering
    either the event loop or one of the waitFor...() functions, it
    enters the ConnectingState, emits connected(), and then initiates
    the SSL client handshake. At each state change, QSslSocket emits
    signal stateChanged().

    After initiating the SSL client handshake, if the identity of the
    peer can't be established, signal sslErrors() is emitted. If you
    want to ignore the errors and continue connecting, you must call
    ignoreSslErrors(), either from inside a slot function connected to
    the sslErrors() signal, or prior to entering encrypted mode. If
    ignoreSslErrors is not called, the connection is dropped, signal
    disconnected() is emitted, and QSslSocket returns to the
    UnconnectedState.

    If the SSL handshake is successful, QSslSocket emits encrypted().

    \code
        QSslSocket socket;
        connect(&socket, SIGNAL(encrypted()), receiver, SLOT(socketEncrypted()));

        socket.connectToHostEncrypted("imap", 993);
        socket->write("1 CAPABILITY\r\n");
    \endcode

    \bold{Note:} The example above shows that text can be written to
    the socket immediately after requesting the encrypted connection,
    before the encrypted() signal has been emitted. In such cases, the
    text is queued in the object and written to the socket \e after
    the connection is established and the encrypted() signal has been
    emitted.

    The default for \a mode is \l ReadWrite.

    If you want to create a QSslSocket on the server side of a connection, you
    should instead call startServerEncryption() upon receiving the incoming
    connection through QTcpServer.

    \sa connectToHost(), startClientEncryption(), waitForConnected(), waitForEncrypted()
*/
void QSslSocket::connectToHostEncrypted(const QString &hostName, quint16 port, OpenMode mode)
{
    Q_D(QSslSocket);
    if (d->state == ConnectedState || d->state == ConnectingState) {
        qWarning("QSslSocket::connectToHostEncrypted() called when already connecting/connected");
        return;
    }

    connectToHost(hostName, port, mode);
    d->autoStartHandshake = true;
}

/*!
    Initializes QSslSocket with the native socket descriptor \a
    socketDescriptor. Returns true if \a socketDescriptor is accepted
    as a valid socket descriptor; otherwise returns false.
    The socket is opened in the mode specified by \a openMode, and
    enters the socket state specified by \a state.

    \bold{Note:} It is not possible to initialize two sockets with the same
    native socket descriptor.

    \sa socketDescriptor()
*/
bool QSslSocket::setSocketDescriptor(int socketDescriptor, SocketState state, OpenMode openMode)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::setSocketDescriptor(" << socketDescriptor << ","
             << state << "," << openMode << ")";
#endif
    if (!d->plainSocket)
        d->createPlainSocket(openMode);
    bool retVal = d->plainSocket->setSocketDescriptor(socketDescriptor, state, openMode);
    d->cachedSocketDescriptor = d->plainSocket->socketDescriptor();
    setSocketError(d->plainSocket->error());
    setSocketState(state);
    setOpenMode(openMode);
    setLocalPort(d->plainSocket->localPort());
    setLocalAddress(d->plainSocket->localAddress());
    setPeerPort(d->plainSocket->peerPort());
    setPeerAddress(d->plainSocket->peerAddress());
    setPeerName(d->plainSocket->peerName());
    return retVal;
}

/*!
    Returns the current mode for the socket; either UnencryptedMode, where
    QSslSocket behaves identially to QTcpSocket, or one of SslClientMode or
    SslServerMode, where the client is either negotiating or in encrypted
    mode.

    When the mode changes, QSslSocket emits modeChanged()

    \sa SslMode
*/
QSslSocket::SslMode QSslSocket::mode() const
{
    Q_D(const QSslSocket);
    return d->mode;
}

/*!
    Returns true if the socket is encrypted; otherwise, false is returned.

    An encrypted socket encrypts all data that is written by calling write()
    or putChar() before the data is written to the network, and descrypts all
    incoming data as the data is received from the network, before you call
    read(), readLine() or getChar().

    QSslSocket emits encrypted() when it enters encrypted mode.

    You can call sessionCipher() to find which cryptographic cipher is used to
    encrypt and decrypt your data.

    \sa mode()
*/
bool QSslSocket::isEncrypted() const
{
    Q_D(const QSslSocket);
    return d->connectionEncrypted;
}

/*!
    Returns the socket's SSL protocol. By default, \l QSsl::SslV3 is used.

    \value setProtocol()
*/
QSsl::SslProtocol QSslSocket::protocol() const
{
    Q_D(const QSslSocket);
    return d->protocol;
}

/*!
    Sets the socket's SSL protocol to \a protocol. This will affect the next
    initiated handshake; calling this function on an already-encrypted socket
    will not affect the socket's protocol.
*/
void QSslSocket::setProtocol(QSsl::SslProtocol protocol)
{
    Q_D(QSslSocket);
    d->protocol = protocol;
}

/*!
    \reimp

    Returns the number of decrypted bytes that are immediately available for
    reading.
*/
qint64 QSslSocket::bytesAvailable() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QIODevice::bytesAvailable() + (d->plainSocket ? d->plainSocket->bytesAvailable() : 0);
    return QIODevice::bytesAvailable() + d->readBuffer.size();
}

/*!
    \reimp

    Returns the number of uneccrypted bytes that are waiting to be encrypted
    and written to the network.
*/
qint64 QSslSocket::bytesToWrite() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return d->plainSocket ? d->plainSocket->bytesToWrite() : 0;
    return d->writeBuffer.size();
}

/*!
    \reimp

    Returns true if you can read one while line (terminated by a single ASCII
    '\n' character) of decrypted characters; otherwise, false is returned.
*/
bool QSslSocket::canReadLine() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QIODevice::canReadLine() || (d->plainSocket && d->plainSocket->canReadLine());
    return QIODevice::canReadLine() || (!d->readBuffer.isEmpty() && d->readBuffer.canReadLine());
}

/*!
    \reimp
*/
void QSslSocket::close()
{
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::close()";
#endif
    QTcpSocket::close();
}

/*!
    \reimp
*/
bool QSslSocket::atEnd() const
{
    Q_D(const QSslSocket);
    if (d->mode == UnencryptedMode)
        return QIODevice::atEnd() && (!d->plainSocket || d->plainSocket->atEnd());
    return QIODevice::atEnd() && d->readBuffer.isEmpty();
}

/*!
    This function writes as much as possible from the internal write buffer to
    the underlying network socket, without blocking. If any data was written,
    this function returns true; otherwise false is returned.

    Call this function if you need QSslSocket to start sending buffered data
    immediately. The number of bytes successfully written depends on the
    operating system. In most cases, you do not need to call this function,
    because QAbstractSocket will start sending data automatically once control
    goes back to the event loop. In the absence of an event loop, call
    waitForBytesWritten() instead.

    \sa write(), waitForBytesWritten()
*/
// Note! docs copied from QAbstractSocket::flush()
bool QSslSocket::flush()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::flush()";
#endif
    return d->plainSocket ? d->plainSocket->flush() : false;
}

/*!
    Aborts the current connection and resets the socket. Unlike
    disconnectFromHost(), this function immediately closes the socket,
    clearing any pending data in the write buffer.

    \sa disconnectFromHost(), close()
*/
void QSslSocket::abort()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::abort()";
#endif
    if (d->plainSocket)
        d->plainSocket->abort();
}

/*!
    Sets the socket's local certificate to \a certificate. The local
    certificate is necessary if you need to confirm your identity to the
    peer. It is used together with the private key; if you set the local
    certificate, you must also set the private key.

    The local certificate and private key are always necessary for server
    sockets, but are also rarely used by client sockets if the server requires
    the client to authenticate.

    \sa localCertificate(), setPrivateKey()
*/
void QSslSocket::setLocalCertificate(const QSslCertificate &certificate)
{
    Q_D(QSslSocket);
    d->localCertificate = certificate;
}

/*!
    \overload

    Sets the socket's local \l {QSslCertificate} {certificate} to the
    first one found in file \a path, which is parsed according to the 
    specified \a format.
*/
void QSslSocket::setLocalCertificate(const QString &path,
                                     QSsl::EncodingFormat format)
{
    Q_D(QSslSocket);
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        d->localCertificate = QSslCertificate(file.readAll(), format);
}

/*!
    Returns the socket's local \l {QSslCertificate} {certificate}, or
    an empty certificate if no local certificate has been assigned.

    \sa setLocalCertificate(), privateKey()
*/
QSslCertificate QSslSocket::localCertificate() const
{
    Q_D(const QSslSocket);
    return d->localCertificate;
}

/*!
    Returns the peer's digital certificate (i.e., the immediate
    certificate of the host you are connected to), or a null
    certificate, if the peer has not assigned a certificate.
    
    The peer certificate is checked automatically during the
    handshake phase, so this function is normally used to fetch
    the certificate for display or for connection diagnostic
    purposes. It contains information about the peer, including
    its host name, the certificate issuer, and the peer's public
    key.

    Because the peer certificate is set during the handshake phase, it
    is safe to access the peer certificate from a slot connected to
    the sslErrors() signal or the encrypted() signal.

    If a null certificate is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to check the peer's complete chain of certificates,
    use peerCertificateChain() to get them all at once.

    \sa peerCertificateChain()
*/
QSslCertificate QSslSocket::peerCertificate() const
{
    Q_D(const QSslSocket);
    return d->peerCertificate;
}

/*!
    Returns the peer's chain of digital certificates, or an empty list
    of certificates.

    Peer certificates are checked automatically during the handshake
    phase. This function is normally used to fetch certificates for
    display, or for performing connection diagnostics. Certificates
    contain information about the peer and the certificate issuers,
    including host name, issuer names, and issuer public keys.

    The peer certificates are set in QSslSocket during the handshake
    phase, so it is safe to call this function from a slot connected
    to the sslErrors() signal or the encrypted() signal.

    If an empty list is returned, it can mean the SSL handshake
    failed, or it can mean the host you are connected to doesn't have
    a certificate, or it can mean there is no connection.

    If you want to get only the peer's immediate certificate, use
    peerCertificate().

    \sa peerCertificate()
*/
QList<QSslCertificate> QSslSocket::peerCertificateChain() const
{
    Q_D(const QSslSocket);
    return d->peerCertificateChain;
}

/*!
    Returns the socket's cryptographic \l {QSslCipher} {cipher}, or a
    null cipher if the connection isn't encrypted. The socket's cipher
    for the session is set during the handshake phase. The cipher is
    used to encrypt and decrypt data transmitted through the socket.

    QSslSocket also provides functions for setting the ordered list of
    ciphers from which the handshake phase will eventually select the
    session cipher. This ordered list must be in place before the
    handshake phase begins.

    \sa ciphers(), setCiphers(), setDefaultCiphers(), defaultCiphers(),
    supportedCiphers()
*/
QSslCipher QSslSocket::sessionCipher() const
{
    Q_D(const QSslSocket);
    return d->sessionCipher();
}

/*!
    Sets the socket's private \l {QSslKey} {key} to \a key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to
    SSL peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.

    \sa privateKey(), setLocalCertificate()
*/
void QSslSocket::setPrivateKey(const QSslKey &key)
{
    Q_D(QSslSocket);
    d->privateKey = key;
}

/*!
    \overload

    Reads the string in file \a fileName and decodes it using
    a specified \a algorithm and encoding \a format to construct
    an \l {QSslKey} {SSL key}. If the encoded key is encrypted,
    \a passPhrase is used to decrypt it.

    The socket's private key is set to the constructed key. The
    private key and the local \l {QSslCertificate} {certificate} are
    used by clients and servers that must prove their identity to SSL
    peers.

    Both the key and the local certificate are required if you are
    creating an SSL server socket. If you are creating an SSL client
    socket, the key and local certificate are required if your client
    must identify itself to an SSL server.
    
    \sa privateKey(), setLocalCertificate()
*/
void QSslSocket::setPrivateKey(const QString &fileName, QSsl::KeyAlgorithm algorithm,
                               QSsl::EncodingFormat format, const QByteArray &passPhrase)
{
    Q_D(QSslSocket);
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        d->privateKey = QSslKey(file.readAll(), algorithm,
				format, QSsl::PrivateKey, passPhrase);
    }
}

/*!
    Returns this socket's private key.

    \sa setPrivateKey(), localCertificate()
*/
QSslKey QSslSocket::privateKey() const
{
    Q_D(const QSslSocket);
    return d->privateKey;
}

/*!
    Returns this socket's current cryptographic cipher suite. This
    list is used during the socket's handshake phase for choosing a
    session cipher. The returned list of ciphers is ordered by
    descending preference. (i.e., the first cipher in the list is the
    most preferred cipher). The session cipher will be the first one
    in the list that is also supported by the peer.

    By default, the handshake phase can choose any of the ciphers
    supported by this system's SSL libraries, which may vary from
    system to system. The list of ciphers supported by this system's
    SSL libraries is returned by supportedCiphers(). You can restrict
    the list of ciphers used for choosing the session cipher for this
    socket by calling setCiphers() with a subset of the supported
    ciphers. You can revert to using the entire set by calling
    setCiphers() with the list returned by supportedCiphers().

    You can restrict the list of ciphers used for choosing the session
    cipher for \e all sockets by calling setDefaultCiphers() with a
    subset of the supported ciphers. You can revert to using the
    entire set by calling setCiphers() with the list returned by
    supportedCiphers().

    \sa setCiphers(), defaultCiphers(), setDefaultCiphers(), supportedCiphers()
*/
QList<QSslCipher> QSslSocket::ciphers() const
{
    Q_D(const QSslSocket);
    return d->ciphers;
}

/*!
    Sets the cryptographic cipher suite for this socket to \a ciphers,
    which must contain a subset of the ciphers in the list returned by
    supportedCiphers().

    Restricting the cipher suite must be done before the handshake
    phase, where the session cipher is chosen.

    \sa ciphers(), setDefaultCiphers(), supportedCiphers()
*/
void QSslSocket::setCiphers(const QList<QSslCipher> &ciphers)
{
    Q_D(QSslSocket);
    d->ciphers = ciphers;
}

/*!
    Sets the cryptographic cipher suite for this socket to \a ciphers, which
    is a colon-separated list of cipher suite names. The ciphers are listed in
    order of preference, starting with the most preferred cipher. For example:

    \code
        QSslSocket socket;
        socket.setCiphers("DHE-RSA-AES256-SHA:DHE-DSS-AES256-SHA:AES256-SHA");
    \endcode

    Each cipher name in \a ciphers must be the name of a cipher in the
    list returned by supportedCiphers().  Restricting the cipher suite
    must be done before the handshake phase, where the session cipher
    is chosen.

    \sa ciphers(), setDefaultCiphers(), supportedCiphers()
*/
void QSslSocket::setCiphers(const QString &ciphers)
{
    Q_D(QSslSocket);
    d->ciphers.clear();
    foreach (QString cipherName, ciphers.split(QLatin1String(":"),QString::SkipEmptyParts)) {
        for (int i = 0; i < 3; ++i) {
            // ### Crude
            QSslCipher cipher(cipherName, QSsl::SslProtocol(i));
            if (!cipher.isNull())
                d->ciphers << cipher;
        }
    }
}

/*!
    Sets the default cryptographic cipher suite for all sockets in
    this application to \a ciphers, which must contain a subset of the
    ciphers in the list returned by supportedCiphers().

    Restricting the default cipher suite only affects SSL sockets
    that perform their handshake phase after the default cipher
    suite has been changed.

    \sa setCiphers(), defaultCiphers(), supportedCiphers()
*/
void QSslSocket::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QSslSocketPrivate::setDefaultCiphers(ciphers);
}

/*!
    Returns the default cryptographic cipher suite for all sockets in
    this application. This list is used during the socket's handshake
    phase when negotiating with the peer to choose a session cipher.
    The list is ordered by preference (i.e., the first cipher in the
    list is the most preferred cipher).

    By default, the handshake phase can choose any of the ciphers
    supported by this system's SSL libraries, which may vary from
    system to system. The list of ciphers supported by this system's
    SSL libraries is returned by supportedCiphers().

    \sa supportedCiphers()
*/
QList<QSslCipher> QSslSocket::defaultCiphers()
{
    return QSslSocketPrivate::defaultCiphers();
}

/*!
    Returns the list of cryptographic ciphers supported by this
    system. This list is set by the system's SSL libraries and may
    vary from system to system.

    \sa defaultCiphers(), ciphers(), setCiphers()
*/
QList<QSslCipher> QSslSocket::supportedCiphers()
{
    return QSslSocketPrivate::supportedCiphers();
}

/*!
  Searches all files in the \a path for certificates encoded in the
  specified \a format and adds them to this socket's CA certificate
  database. \a path can be explicit, or it can contain wildcards in
  the format specified by \a syntax. Returns true if one or more
  certificates are added to the socket's CA certificate database;
  otherwise returns false.

  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  For more precise control, use addCaCertificate().

  \sa addCaCertificate(), QSslCertificate::fromPath()
*/
bool QSslSocket::addCaCertificates(const QString &path, QSsl::EncodingFormat format,
                                   QRegExp::PatternSyntax syntax)
{
    Q_D(QSslSocket);
    QList<QSslCertificate> certs = QSslCertificate::fromPath(path, format, syntax);
    if (certs.isEmpty())
        return false;

    d->localCaCertificates += certs;
    return true;
}

/*!
  Adds the \a certificate to this socket's CA certificate database.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  To add multiple certificates, use addCaCertificates().

  \sa caCertificates(), setCaCertificates()
*/
void QSslSocket::addCaCertificate(const QSslCertificate &certificate)
{
    Q_D(QSslSocket);
    d->localCaCertificates += certificate;
}

/*!
  Adds the \a certificates to this socket's CA certificate database.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  For more precise control, use addCaCertificate().

  \sa caCertificates(), addDefaultCaCertificate()
*/
void QSslSocket::addCaCertificates(const QList<QSslCertificate> &certificates)
{
    Q_D(QSslSocket);
    d->localCaCertificates += certificates;
}

/*!
  Sets this socket's CA certificate database to be \a certificates.
  The certificate database must be set prior to the SSL handshake.
  The CA certificate database is used by the socket during the
  handshake phase to validate the peer's certificate.

  The CA certificate database can be reset to the current default CA
  certificate database by calling this function with the list of CA
  certificates returned by defaultCaCertificates().

  \sa defaultCaCertificates()
*/
void QSslSocket::setCaCertificates(const QList<QSslCertificate> &certificates)
{
    Q_D(QSslSocket);
    d->useLocalCaCertificatesOnly = true;
    d->localCaCertificates = certificates;
}

/*!
  Returns this socket's CA certificate database. The CA certificate
  database is used by the socket during the handshake phase to
  validate the peer's certificate. It can be moodified prior to the
  handshake with addCaCertificate(), addCACertificates(), and
  setCaCertificates().

  \sa addCaCertificate(), addCaCertificates(), setCaCertificates()
*/
QList<QSslCertificate> QSslSocket::caCertificates() const
{
    Q_D(const QSslSocket);
    if (d->useLocalCaCertificatesOnly)
        return d->localCaCertificates;
    return d->defaultCaCertificates() + d->localCaCertificates;
}

/*!
    Searches all files in the \a path for certificates with the
    specified \a encoding and adds them to the default CA certificate
    database. \a path can be an explicit file, or it can contain
    wildcards in the format specified by \a syntax. Returns true if
    any CA certificates are added to the default database.

    Each SSL socket's CA certificate database is initialized to the
    default CA certificate database.

    \sa defaultCaCertificates(), addCaCertificates(), addDefaultCaCertificate()
*/
bool QSslSocket::addDefaultCaCertificates(const QString &path, QSsl::EncodingFormat encoding,
                                          QRegExp::PatternSyntax syntax)
{
    return QSslSocketPrivate::addDefaultCaCertificates(path, encoding, syntax);
}

/*!
    Adds \a certificate to the default CA certificate database.  Each
    SSL socket's CA certificate database is initialized to the default
    CA certificate database.

    \sa defaultCaCertificates(), addCaCertificates()
*/
void QSslSocket::addDefaultCaCertificate(const QSslCertificate &certificate)
{
    QSslSocketPrivate::addDefaultCaCertificate(certificate);
}

/*!
    Adds \a certificates to the default CA certificate database.  Each
    SSL socket's CA certificate database is initialized to the default
    CA certificate database.

    \sa defaultCaCertificates(), addCaCertificates()
*/
void QSslSocket::addDefaultCaCertificates(const QList<QSslCertificate> &certificates)
{
    QSslSocketPrivate::addDefaultCaCertificates(certificates);
}

/*!
    Sets the default CA certificate database to \a certificates. The
    default CA certificate database is originally set to your system's
    default CA certificate database. If no system default database is
    found, Qt will provide its own default database. You can override
    the default CA certificate database with your own CA certificate
    database using this function.

    Each SSL socket's CA certificate database is initialized to the
    default CA certificate database.

    \sa addDefaultCaCertificate()
*/
void QSslSocket::setDefaultCaCertificates(const QList<QSslCertificate> &certificates)
{
    QSslSocketPrivate::setDefaultCaCertificates(certificates);
}

/*!
    Returns the current default CA certificate database. This database
    is originally set to your system's default CA certificate database.
    If no system default database is found, Qt will provide its own
    default database. You can override the default CA certificate database
    with your own CA certificate database using setDefaultCaCertificates().

    Each SSL socket's CA certificate database is initialized to the
    default CA certificate database.

    \sa caCertificates()
*/
QList<QSslCertificate> QSslSocket::defaultCaCertificates()
{
    return QSslSocketPrivate::defaultCaCertificates();
}

/*!
    Returns the system default CA certificate database for your
    system. This database is normally found in a standard place for
    your system. If it is not found there, Qt will provide its own
    default CA certificate database. The CA certificate database
    returned by this function is used to initialize the database
    returned by defaultCaCertificates(). You can replace that database
    with your own with setDefaultCaCertificates().

    \sa caCertificates(), defaultCaCertificates(), setDefaultCaCertificates()
*/
QList<QSslCertificate> QSslSocket::systemCaCertificates()
{
    QSslSocketPrivate::ensureInitialized();
    return QSslSocketPrivate::systemCaCertificates();
}

/*!
    Waits until the socket is connected, or \a msecs milliseconds,
    whichever happens first. If the connection has been established,
    this function returns true; otherwise it returns false.

    \sa QAbstractSocket::waitForConnected()
*/
bool QSslSocket::waitForConnected(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    bool retVal = d->plainSocket->waitForConnected(msecs);
    if (!retVal) {
        setSocketState(d->plainSocket->state());
        setSocketError(d->plainSocket->error());
        setErrorString(d->plainSocket->errorString());
    }
    return retVal;
}

/*!
    Waits until the socket has completed the SSL handshake and has
    emitted encrypted(), or \a msecs milliseconds, whichever comes
    first. If encrypted() has been emitted, this function returns
    true; otherwise (e.g., the socket is disconnected, or the SSL
    handshake fails), false is returned.

    The following example waits up to one second for the socket to be
    encrypted:

    \code
        socket->connectToHostEncrypted("imap", 993);
        if (socket->waitForEncrypted(1000))
            qDebug("Encrypted!");
    \endcode

    If msecs is -1, this function will not time out.

    \sa startClientEncryption(), startServerEncryption(), encrypted(), isEncrypted()
*/
bool QSslSocket::waitForEncrypted(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket || d->connectionEncrypted)
        return false;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return false;

    QTime stopWatch;
    stopWatch.start();

    if (d->plainSocket->state() != QAbstractSocket::ConnectedState) {
        // Wait until we've entered connected state.
        if (!d->plainSocket->waitForConnected(msecs))
            return false;
    }

    while (!d->connectionEncrypted) {
        // Start the handshake, if this hasn't been started yet.
        if (d->mode == UnencryptedMode)
            startClientEncryption();
        // Loop, waiting until the connection has been encrypted or an error
        // occurs.
        if (!d->plainSocket->waitForReadyRead(qBound(0, msecs - stopWatch.elapsed(), msecs)))
            return false;
    }
    return d->connectionEncrypted;
}

/*!
    \reimp
*/
bool QSslSocket::waitForReadyRead(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->waitForReadyRead(msecs);

    int oldReadBufferSize = d->readBuffer.size();
    
    QTime stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs))
            return false;
    }

    while (d->plainSocket->waitForReadyRead(qBound(0, msecs - stopWatch.elapsed(), msecs))) {
        if (d->readBuffer.size() != oldReadBufferSize) {
            // If the read buffer has grown, readyRead() must have been emitted.
            return true;
        }
    }
    return false;
}

/*!
    \reimp
*/
bool QSslSocket::waitForBytesWritten(int msecs)
{
    Q_D(QSslSocket);
    if (!d->plainSocket)
        return false;
    if (d->mode == UnencryptedMode)
        return d->plainSocket->waitForBytesWritten(msecs);

    QTime stopWatch;
    stopWatch.start();

    if (!d->connectionEncrypted) {
        // Wait until we've entered encrypted mode, or until a failure occurs.
        if (!waitForEncrypted(msecs))
            return false;
    }

    return d->plainSocket->waitForBytesWritten(qBound(0, msecs - stopWatch.elapsed(), msecs));
}

/*!
    Waits until the socket has disconnected or \a msecs milliseconds,
    whichever comes first. If the connection has been disconnected,
    this function returns true; otherwise it returns false.

    \sa QAbstractSocket::waitForDisconnected()
*/
bool QSslSocket::waitForDisconnected(int msecs)
{
    Q_D(QSslSocket);

    // require calling connectToHost() before waitForDisconnected()
    if (state() == UnconnectedState) {
        qWarning("QSslSocket::waitForDisconnected() is not allowed in UnconnectedState");
        return false;
    }

    if (!d->plainSocket)
        return false;
    bool retVal = d->plainSocket->waitForDisconnected(msecs);
    if (!retVal) {
        setSocketState(d->plainSocket->state());
        setSocketError(d->plainSocket->error());
        setErrorString(d->plainSocket->errorString());
    }
    return retVal;
}

/*!
    Returns a list of the last SSL errors that occurred. This is the
    same list as QSslSocket passes via the sslErrors() signal. If the
    connection has been encrypted with no errors, this function will
    return an empty list.

    \sa connectToHostEncrypted()
*/
QList<QSslError> QSslSocket::sslErrors() const
{
    Q_D(const QSslSocket);
    return d->sslErrors;
}

/*!
    Returns true if this platform supports SSL; otherwise, returns
    false. If the platform doesn't support SSL, the socket will fail
    in the connection phase.
*/
bool QSslSocket::supportsSsl()
{
    return QSslSocketPrivate::ensureInitialized();
}

/*!
    Starts a delayed SSL handshake for a client connection. This
    function can be called when the socket is in the \l ConnectedState
    but still in the \l UnencryptedMode. If it is not yet connected,
    or if it is already encrypted, this function has no effect.

    Clients that implement STARTTLS functionality often make use of
    delayed SSL handshakes. Most other clients can avoid calling this
    function directly by using connectToHostEncrypted() instead, which
    automatically performs the handshake.

    \sa connectToHostEncrypted(), startServerEncryption()
*/
void QSslSocket::startClientEncryption()
{
    Q_D(QSslSocket);
    if (d->mode != UnencryptedMode) {
        qWarning("QSslSocket::startClientEncryption: cannot start handshake on non-plain connection");
        return;
    }
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::startClientEncryption()";
#endif
    d->mode = SslClientMode;
    emit modeChanged(d->mode);
    d->startClientEncryption();
}

/*!
    Starts a delayed SSL handshake for a server connection. This
    function can be called when the socket is in the \l ConnectedState
    but still in \l UnencryptedMode. If it is not connected or it is
    already encrypted, the function has no effect.

    For server sockets, calling this function is the only way to
    initiate the SSL handshake. Most servers will call this function
    immediately upon receiving a connection, or as a result of having
    received a protocol-specific command to enter SSL mode (e.g, the
    server may respond to receiving the string "STARTTLS\r\n" by
    calling this function).

    The most common way to implement an SSL server is to create a
    subclass of QTcpServer and reimplement
    QTcpServer::incomingConnection(). The returned socket descriptor
    is then passed to QSslSocket::setSocketDescriptor().
    
    \sa connectToHostEncrypted(), startClientEncryption()
*/
void QSslSocket::startServerEncryption()
{
    Q_D(QSslSocket);
    if (d->mode != UnencryptedMode) {
        qWarning("QSslSocket::startClientEncryption: cannot start handshake on non-plain connection");
        return;
    }
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::startServerEncryption()";
#endif
    d->mode = SslServerMode;
    emit modeChanged(d->mode);
    d->startServerEncryption();
}

/*!
    This slot tells QSslSocket to ignore errors during QSslSocket's
    handshake phase and continue connecting. If you want to continue
    with the connection even if errors occur during the handshake
    phase, then you must call this slot, either from a slot connected
    to sslErrors(), or before the handshake phase. If you don't call
    this slot, either in response to errors or before the handshake,
    the connection will be dropped after the sslErrors() signal has
    been emitted.

    If there are no errors during the SSL handshake phase (i.e., the
    identity of the peer is established with no problems), QSslSocket
    will not emit the sslErrors() signal, and it is unnecessary to
    call this function.

    Ignoring errors that occur during an SSL handshake should be done
    with caution. A fundamental characteristic of secure connections
    is that they should be established with an error free handshake.

    \sa sslErrors()
*/
void QSslSocket::ignoreSslErrors()
{
    Q_D(QSslSocket);
    d->ignoreSslErrors = true;
}

/*!
    \internal
*/
void QSslSocket::connectToHostImplementation(const QString &hostName, quint16 port,
                                             OpenMode openMode)
{
    Q_D(QSslSocket);
    d->init();
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::connectToHostImplementation("
             << hostName << "," << port << "," << openMode << ")";
#endif
    if (!d->plainSocket) {
#ifdef QSSLSOCKET_DEBUG
        qDebug() << "\tcreating internal plain socket";
#endif
        d->createPlainSocket(openMode);
#ifndef QT_NO_NETWORKPROXY
        d->plainSocket->setProxy(proxy());
#endif
    }
    setOpenMode(openMode);
    d->plainSocket->connectToHost(hostName, port, openMode);
    d->cachedSocketDescriptor = d->plainSocket->socketDescriptor();
}

/*!
    \internal
*/
void QSslSocket::disconnectFromHostImplementation()
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::disconnectFromHostImplementation()";
#endif
    if (!d->plainSocket)
        return;
    if (d->mode == UnencryptedMode) {
        d->plainSocket->disconnectFromHost();
    } else {
        d->disconnectFromHost();
    }
}

/*!
    \reimp
*/
qint64 QSslSocket::readData(char *data, qint64 maxlen)
{
    Q_D(QSslSocket);
    qint64 readBytes = 0;

    if (d->mode == UnencryptedMode && !d->autoStartHandshake) {
        readBytes = d->plainSocket->read(data, maxlen);
    } else {
        do {
            const char *readPtr = d->readBuffer.readPointer();
            int bytesToRead = qMin<int>(maxlen - readBytes, d->readBuffer.nextDataBlockSize());
            ::memcpy(data + readBytes, readPtr, bytesToRead);
            readBytes += bytesToRead;
            d->readBuffer.free(bytesToRead);
        } while (!d->readBuffer.isEmpty() && readBytes < maxlen);
    }
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::readData(" << (void *)data << "," << maxlen << ") ==" << readBytes;
#endif
    return readBytes;
}

/*!
    \reimp
*/
qint64 QSslSocket::writeData(const char *data, qint64 len)
{
    Q_D(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::writeData(" << (void *)data << "," << len << ")";
#endif
    if (d->mode == UnencryptedMode && !d->autoStartHandshake)
        return d->plainSocket->write(data, len);

    char *writePtr = d->writeBuffer.reserve(len);
    ::memcpy(writePtr, data, len);

    if (d->connectionEncrypted)
        d->transmit();

    return len;
}

/*!
    \internal
*/
QSslSocketPrivate::QSslSocketPrivate()
    : protocol(QSsl::SslV3), plainSocket(0)
{
}

/*!
    \internal
*/
QSslSocketPrivate::~QSslSocketPrivate()
{
}

/*!
    \internal
*/
void QSslSocketPrivate::init()
{
    mode = QSslSocket::UnencryptedMode;
    autoStartHandshake = false;
    connectionEncrypted = false;
    ignoreSslErrors = false;
    useLocalCaCertificatesOnly = false;

    readBuffer.clear();
    writeBuffer.clear();
    peerCertificate.clear();
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::defaultCiphers()
{
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->ciphers;
}

/*!
    \internal
*/
QList<QSslCipher> QSslSocketPrivate::supportedCiphers()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->supportedCiphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->ciphers = ciphers;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultSupportedCiphers(const QList<QSslCipher> &ciphers)
{
    QMutexLocker locker(&globalData()->mutex);
    globalData()->supportedCiphers = ciphers;
}

/*!
    \internal
*/
QList<QSslCertificate> QSslSocketPrivate::defaultCaCertificates()
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    return globalData()->caCertificates;
}

/*!
    \internal
*/
void QSslSocketPrivate::setDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->caCertificates = certs;
}

/*!
    \internal
*/
bool QSslSocketPrivate::addDefaultCaCertificates(const QString &path, QSsl::EncodingFormat format,
                                                 QRegExp::PatternSyntax syntax)
{
    QSslSocketPrivate::ensureInitialized();
    QList<QSslCertificate> certs = QSslCertificate::fromPath(path, format, syntax);
    if (certs.isEmpty())
        return false;

    QMutexLocker locker(&globalData()->mutex);
    globalData()->caCertificates += certs;
    return true;
}

/*!
    \internal
*/
void QSslSocketPrivate::addDefaultCaCertificate(const QSslCertificate &cert)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->caCertificates += cert;
}

/*!
    \internal
*/
void QSslSocketPrivate::addDefaultCaCertificates(const QList<QSslCertificate> &certs)
{
    QSslSocketPrivate::ensureInitialized();
    QMutexLocker locker(&globalData()->mutex);
    globalData()->caCertificates += certs;
}

/*!
    \internal
*/
void QSslSocketPrivate::createPlainSocket(QIODevice::OpenMode openMode)
{
    Q_Q(QSslSocket);
    q->setOpenMode(openMode); // <- from QIODevice
    q->setSocketState(QAbstractSocket::UnconnectedState);
    q->setSocketError(QAbstractSocket::UnknownSocketError);
    q->setLocalPort(0);
    q->setLocalAddress(QHostAddress());
    q->setPeerPort(0);
    q->setPeerAddress(QHostAddress());
    q->setPeerName(QString());

    plainSocket = new QTcpSocket(q);
    q->connect(plainSocket, SIGNAL(connected()),
               q, SLOT(_q_connectedSlot()));
    q->connect(plainSocket, SIGNAL(hostFound()),
               q, SLOT(_q_hostFoundSlot()));
    q->connect(plainSocket, SIGNAL(disconnected()),
               q, SLOT(_q_disconnectedSlot()));
    q->connect(plainSocket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
               q, SLOT(_q_stateChangedSlot(QAbstractSocket::SocketState)));
    q->connect(plainSocket, SIGNAL(error(QAbstractSocket::SocketError)),
               q, SLOT(_q_errorSlot(QAbstractSocket::SocketError)));
    q->connect(plainSocket, SIGNAL(readyRead()),
               q, SLOT(_q_readyReadSlot()));
    q->connect(plainSocket, SIGNAL(bytesWritten(qint64)),
               q, SLOT(_q_bytesWrittenSlot(qint64)));
    q->connect(plainSocket, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)),
               q, SIGNAL(proxyAuthenticationRequired(QNetworkProxy,QAuthenticator*)));

    readBuffer.clear();
    writeBuffer.clear();
    connectionEncrypted = false;
    peerCertificate.clear();
    peerCertificateChain.clear();
    mode = QSslSocket::UnencryptedMode;
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_connectedSlot()
{
    Q_Q(QSslSocket);
    q->setLocalPort(plainSocket->localPort());
    q->setLocalAddress(plainSocket->localAddress());
    q->setPeerPort(plainSocket->peerPort());
    q->setPeerAddress(plainSocket->peerAddress());
    q->setPeerName(plainSocket->peerName());
    cachedSocketDescriptor = plainSocket->socketDescriptor();

#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_connectedSlot()";
    qDebug() << "\tstate =" << q->state();
    qDebug() << "\tpeer =" << q->peerName() << q->peerAddress() << q->peerPort();
    qDebug() << "\tlocal =" << QHostInfo::fromName(q->localAddress().toString()).hostName()
             << q->localAddress() << q->localPort();
#endif
    emit q->connected();

    if (autoStartHandshake)
        q->startClientEncryption();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_hostFoundSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_hostFoundSlot()";
    qDebug() << "\tstate =" << q->state();
#endif
    emit q->hostFound();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_disconnectedSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_disconnectedSlot()";
    qDebug() << "\tstate =" << q->state();
#endif
    disconnected();
    emit q->disconnected();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_stateChangedSlot(QAbstractSocket::SocketState state)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_stateChangedSlot(" << state << ")";
#endif
    q->setSocketState(state);
    emit q->stateChanged(state);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_errorSlot(QAbstractSocket::SocketError error)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_errorSlot(" << error << ")";
    qDebug() << "\tstate =" << q->state();
    qDebug() << "\terrorString =" << q->errorString();
#endif
    q->setSocketError(plainSocket->error());
    q->setErrorString(plainSocket->errorString());
    emit q->error(error);
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_readyReadSlot()
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_readyReadSlot() -" << plainSocket->bytesAvailable() << "bytes available";
#endif
    if (mode == QSslSocket::UnencryptedMode) {
        emit q->readyRead();
        return;
    }

    transmit();
}

/*!
    \internal
*/
void QSslSocketPrivate::_q_bytesWrittenSlot(qint64 written)
{
    Q_Q(QSslSocket);
#ifdef QSSLSOCKET_DEBUG
    qDebug() << "QSslSocket::_q_bytesWrittenSlot(" << written << ")";
#endif
    emit q->bytesWritten(written);
}

// For private slots
#define d d_ptr
#include "moc_qsslsocket.cpp"
