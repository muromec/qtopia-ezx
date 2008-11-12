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


#include "qsslsocket_openssl_p.h"
#include "qsslsocket_openssl_symbols_p.h"
#include "qsslsocket.h"
#include "qsslcertificate_p.h"
#include "qsslcipher_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/qvarlengtharray.h>

// Useful defines
#define SSL_ERRORSTR() QString::fromLocal8Bit(q_ERR_error_string(q_ERR_get_error(), NULL))

/* \internal

    From OpenSSL's thread(3) manual page:

    OpenSSL can safely be used in multi-threaded applications provided that at
    least two callback functions are set.

    locking_function(int mode, int n, const char *file, int line) is needed to
    perform locking on shared data structures.  (Note that OpenSSL uses a
    number of global data structures that will be implicitly shared
    when-whenever ever multiple threads use OpenSSL.)  Multi-threaded
    applications will crash at random if it is not set.  ...
    ...
    id_function(void) is a function that returns a thread ID. It is not
    needed on Windows nor on platforms where getpid() returns a different
    ID for each thread (most notably Linux)
*/
class OpenSslLocks
{
public:
    inline OpenSslLocks()
        : initLocker(QMutex::Recursive),
          locksLocker(QMutex::Recursive)
    {
        QMutexLocker locker(&locksLocker);
        int numLocks = q_CRYPTO_num_locks();
        locks = new QMutex *[numLocks];
        memset(locks, 0, numLocks * sizeof(QMutex *));
    }
    inline ~OpenSslLocks()
    {
        QMutexLocker locker(&locksLocker);
        for (int i = 0; i < q_CRYPTO_num_locks(); ++i)
            delete locks[i];
        delete [] locks;
    }
    inline QMutex *lock(int num)
    {
        QMutexLocker locker(&locksLocker);
        QMutex *tmp = locks[num];
        if (!tmp)
            tmp = locks[num] = new QMutex(QMutex::Recursive);
        return tmp;
    }

    QMutex *globalLock()
    {
        return &locksLocker;
    }

    QMutex *initLock()
    {
        return &initLocker;
    }

private:
    QMutex initLocker;
    QMutex locksLocker;
    QMutex **locks;
};
Q_GLOBAL_STATIC(OpenSslLocks, openssl_locks)

extern "C" {
static void locking_function(int mode, int lockNumber, const char *, int)
{
    QMutex *mutex = openssl_locks()->lock(lockNumber);

    // Lock or unlock it
    if (mode & CRYPTO_LOCK)
        mutex->lock();
    else
        mutex->unlock();
}
static unsigned long id_function()
{
    return (unsigned long)QThread::currentThreadId();
}
} // extern "C"

QSslSocketBackendPrivate::QSslSocketBackendPrivate()
    : ssl(0),
      ctx(0),
      readBio(0),
      writeBio(0),
      session(0)
{
    // Calls SSL_library_init().
    ensureInitialized();
}

QSslSocketBackendPrivate::~QSslSocketBackendPrivate()
{
}

QSslCipher QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(SSL_CIPHER *cipher)
{
    QSslCipher ciph;

    char buf [256];
    QString descriptionOneLine = QString::fromLatin1(q_SSL_CIPHER_description(cipher, buf, sizeof(buf)));

    QStringList descriptionList = descriptionOneLine.split(QLatin1String(" "), QString::SkipEmptyParts);
    if (descriptionList.size() > 5) {
        // ### crude code.
        ciph.d->isNull = false;
        ciph.d->name = descriptionList.at(0);

        QString protoString = descriptionList.at(1);
        ciph.d->protocolString = protoString;
        ciph.d->protocol = QSsl::UnknownProtocol;
        if (protoString == QLatin1String("SSLv3"))
            ciph.d->protocol = QSsl::SslV3;
        else if (protoString == QLatin1String("SSLv2"))
            ciph.d->protocol = QSsl::SslV2;
        else if (protoString == QLatin1String("TLSv1"))
            ciph.d->protocol = QSsl::TlsV1;
        
        if (descriptionList.at(2).startsWith(QLatin1String("Kx=")))
            ciph.d->keyExchangeMethod = descriptionList.at(2).mid(3);
        if (descriptionList.at(3).startsWith(QLatin1String("Au=")))
            ciph.d->authenticationMethod = descriptionList.at(3).mid(3);
        if (descriptionList.at(4).startsWith(QLatin1String("Enc=")))
            ciph.d->encryptionMethod = descriptionList.at(4).mid(4);
        ciph.d->exportable = (descriptionList.size() > 6 && descriptionList.at(6) == QLatin1String("export"));

        ciph.d->bits = cipher->strength_bits;
        ciph.d->supportedBits = cipher->alg_bits;

    }
    return ciph;
}

// ### This list is shared between all threads, and protected by a
// mutex. Investigate using thread local storage instead.
struct QSslErrorList
{
    QMutex mutex;
    QList<int> errors;
};
Q_GLOBAL_STATIC(QSslErrorList, _q_sslErrorList)
static int q_X509Callback(int ok, X509_STORE_CTX *ctx)
{
    if (!ok)
        _q_sslErrorList()->errors << ctx->error;
    return ctx->error;
}

bool QSslSocketBackendPrivate::initSslContext()
{
    Q_Q(QSslSocket);

    // Create and initialize SSL context. Accept SSLv2, SSLv3 and TLSv1.
    bool client = (mode == QSslSocket::SslClientMode);
    switch (protocol) {
    case QSsl::SslV2:
        ctx = q_SSL_CTX_new(client ? q_SSLv2_client_method() : q_SSLv2_server_method());
        break;
    case QSsl::SslV3:
        ctx = q_SSL_CTX_new(client ? q_SSLv3_client_method() : q_SSLv3_server_method());
        break;
    case QSsl::AnyProtocol:
    default:
        ctx = q_SSL_CTX_new(client ? q_SSLv23_client_method() : q_SSLv23_server_method());
        break;
    case QSsl::TlsV1:
        ctx = q_SSL_CTX_new(client ? q_TLSv1_client_method() : q_TLSv1_server_method());
        break;
    }
    if (!ctx) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL context (%1)").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Enable all bug workarounds.
    q_SSL_CTX_set_options(ctx, SSL_OP_ALL);

    // Initialize ciphers
    QByteArray cipherString;
    int first = true;
    foreach (const QSslCipher &cipher, ciphers.isEmpty() ? defaultCiphers() : ciphers) {
        if (first)
            first = false;
        else
            cipherString.append(":");
        cipherString.append(cipher.name().toLatin1());
    }

    if (!q_SSL_CTX_set_cipher_list(ctx, cipherString.data())) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Invalid or empty cipher list (%1)").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Add all our CAs to this store.
    foreach (const QSslCertificate &caCertificate, q->caCertificates())
        q_X509_STORE_add_cert(ctx->cert_store, (X509 *)caCertificate.handle());

    // Register a custom callback to get all verification errors.
    X509_STORE_set_verify_cb_func(ctx->cert_store, q_X509Callback);

    if (!localCertificate.isNull()) {
        // Require a private key as well.
        if (privateKey.isNull()) {
            q->setErrorString(QSslSocket::tr("Cannot provide a certificate with no key, %1").arg(SSL_ERRORSTR()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }

        // Load certificate
        if (!q_SSL_CTX_use_certificate(ctx, (X509 *)localCertificate.handle())) {
            q->setErrorString(QSslSocket::tr("Error loading local certificate, %1").arg(SSL_ERRORSTR()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }

        // Load private key
        EVP_PKEY *pkey = q_EVP_PKEY_new();
        if (privateKey.algorithm() == QSsl::Rsa)
            q_EVP_PKEY_assign_RSA(pkey, (RSA *)privateKey.handle());
        else
            q_EVP_PKEY_assign_DSA(pkey, (DSA *)privateKey.handle());
        if (!q_SSL_CTX_use_PrivateKey(ctx, pkey)) {
            q->setErrorString(QSslSocket::tr("Error loading private key, %1").arg(SSL_ERRORSTR()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }

        // Check if the certificate matches the private key.
        if (!q_SSL_CTX_check_private_key(ctx)) {
            q->setErrorString(QSslSocket::tr("Private key does not certificate public key, %1").arg(SSL_ERRORSTR()));
            emit q->error(QAbstractSocket::UnknownSocketError);
            return false;
        }
    }

    // Create and initialize SSL session
    if (!(ssl = q_SSL_new(ctx))) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session, %1").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Clear the session.
    q_SSL_clear(ssl);
    errorList.clear();

    // Initialize memory BIOs for encryption and decryption.
    readBio = q_BIO_new(q_BIO_s_mem());
    writeBio = q_BIO_new(q_BIO_s_mem());
    if (!readBio || !writeBio) {
        // ### Bad error code
        q->setErrorString(QSslSocket::tr("Error creating SSL session: %1").arg(SSL_ERRORSTR()));
        q->setSocketError(QAbstractSocket::UnknownSocketError);
        emit q->error(QAbstractSocket::UnknownSocketError);
        return false;
    }

    // Assign the bios.
    q_SSL_set_bio(ssl, readBio, writeBio);

    if (mode == QSslSocket::SslClientMode)
        q_SSL_set_connect_state(ssl);
    else
        q_SSL_set_accept_state(ssl);

    return true;
}

/*!
    \internal

    Declared static in QSslSocketPrivate, makes sure the SSL libraries have
    been initialized.
*/
bool QSslSocketPrivate::ensureInitialized()
{
    if (!q_resolveOpenSslSymbols())
        return false;

    // Check if the library itself needs to be initialized.
    QMutexLocker locker(openssl_locks()->initLock());
    static int q_initialized = false;
    if (!q_initialized) {
        q_initialized = true;

        // Initialize OpenSSL.
        q_CRYPTO_set_id_callback(id_function);
        q_CRYPTO_set_locking_callback(locking_function);
        if (q_SSL_library_init() != 1)
            return false;
        q_SSL_load_error_strings();

        // Initialize OpenSSL's random seed.
        if (!q_RAND_status()) {
            struct {
                int msec;
                int sec;
                void *stack;
            } randomish;

            // This is probably not secure enough.
            randomish.stack = (void *)&randomish;
            randomish.msec = QTime::currentTime().msec();
            randomish.sec = QTime::currentTime().second();
            q_RAND_seed((const char *)&randomish, sizeof(randomish));
        }

        resetDefaultCiphers();
        setDefaultCaCertificates(systemCaCertificates());
    }
    return true;
}

/*!
    \internal

    Declared static in QSslSocketPrivate, backend-dependent loading of
    application-wide global ciphers.
*/
void QSslSocketPrivate::resetDefaultCiphers()
{
    SSL_CTX *myCtx = q_SSL_CTX_new(q_SSLv23_client_method());
    SSL *mySsl = q_SSL_new(myCtx);

    QList<QSslCipher> ciphers;

    STACK_OF(SSL_CIPHER) *supportedCiphers = q_SSL_get_ciphers(mySsl);
    for (int i = 0; i < q_sk_SSL_CIPHER_num(supportedCiphers); ++i) {
        if (SSL_CIPHER *cipher = q_sk_SSL_CIPHER_value(supportedCiphers, i)) {
            if (cipher->valid) {
                QSslCipher ciph = QSslSocketBackendPrivate::QSslCipher_from_SSL_CIPHER(cipher);
                if (!ciph.isNull()) {
                    if (!ciph.name().toLower().startsWith(QLatin1String("adh")))
                        ciphers << ciph;
                }
            }
        }
    }

    q_SSL_CTX_free(myCtx);
    q_SSL_free(mySsl);

    setDefaultSupportedCiphers(ciphers);
    setDefaultCiphers(ciphers);
}

QList<QSslCertificate> QSslSocketPrivate::systemCaCertificates()
{
#ifdef QQ_OS_UNIX
    // Check known locations for the system's default bundle.  ### On Windows,
    // we should use CAPI to find the bundle, and not rely on default unix
    // locations.
    const char *standardLocations[] = {"/etc/ssl/certs/",
#if 0
                                       // KDE uses KConfig for its SSL store,
                                       // but it also stores the bundle at
                                       // this location
                                       "$HOME/.kde/share/apps/kssl/ca-bundle.crt",
#endif
                                       0};
    const char **it = standardLocations;
    QStringList nameFilter;
    nameFilter << QLatin1String("*.pem") << QLatin1String("*.crt");
    while (*it) {
        if (QDirIterator(QLatin1String(*it), nameFilter).hasNext())
            return certificatesFromPath(QLatin1String(*it));
        ++it;
    }
#endif

    // Qt provides a default bundle when we cannot detect the system's default
    // bundle.
    QFile caBundle(QLatin1String(":/trolltech/network/ssl/qt-ca-bundle.crt"));
    if (caBundle.open(QIODevice::ReadOnly | QIODevice::Text))
        return QSslCertificate::fromDevice(&caBundle);

    // Unreachable; return no bundle.
    return QList<QSslCertificate>();
}

void QSslSocketBackendPrivate::startClientEncryption()
{
    if (!initSslContext()) {
        // ### report error: internal OpenSSL failure
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    testConnection();
    transmit();
}

void QSslSocketBackendPrivate::startServerEncryption()
{
    if (!initSslContext()) {
        // ### report error: internal OpenSSL failure
        return;
    }

    // Start connecting. This will place outgoing data in the BIO, so we
    // follow up with calling transmit().
    testConnection();
    transmit();
}

/*!
    \internal

    Transmits encrypted data between the BIOs and the socket.
*/
void QSslSocketBackendPrivate::transmit()
{
    Q_Q(QSslSocket);

    // If we don't have any SSL context, don't bother transmitting.
    if (!ssl)
        return;

    bool transmitting;
    do {
        transmitting = false;
        
        // If the connection is secure, we can transfer data from the write
        // buffer (in plain text) to the write BIO through SSL_write.
        if (connectionEncrypted && !writeBuffer.isEmpty()) {
            int nextDataBlockSize;
            while ((nextDataBlockSize = writeBuffer.nextDataBlockSize()) > 0) {
                int writtenBytes = q_SSL_write(ssl, writeBuffer.readPointer(), nextDataBlockSize);
                if (writtenBytes <= 0) {
                    // ### Better error handling.
                    q->setErrorString(QSslSocket::tr("Unable to write data: %1").arg(SSL_ERRORSTR()));
                    q->setSocketError(QAbstractSocket::UnknownSocketError);
                    emit q->error(QAbstractSocket::UnknownSocketError);
                    return;
                }
                writeBuffer.free(writtenBytes);
            }
        }

        // Check if we've got any data to be written to the socket.
        QVarLengthArray<char, 4096> data;
        int pendingBytes;
        while (plainSocket->isValid() && (pendingBytes = q_BIO_pending(writeBio)) > 0) {
            // Read encrypted data from the write BIO into a buffer.
            data.resize(pendingBytes);
            int encryptedBytesRead = q_BIO_read(writeBio, data.data(), pendingBytes);

            // Write encrypted data from the buffer to the socket.
            plainSocket->write(data.constData(), encryptedBytesRead);
            transmitting = true;
        }

        // Check if we've got any data to be read from the socket.
        while ((pendingBytes = plainSocket->bytesAvailable()) > 0) {
            // Read encrypted data from the socket into a buffer.
            data.resize(pendingBytes);
            int decryptedBytesRead = plainSocket->read(data.data(), pendingBytes);

            // Write encrypted data from the buffer into the read BIO.
            q_BIO_write(readBio, data.constData(), decryptedBytesRead);
            transmitting = true;
        }

        // If the connection isn't secured yet, this is the time to retry the
        // connect / accept.
        if (!connectionEncrypted) {
            if (testConnection()) {
                connectionEncrypted = true;
                transmitting = true;
            } else if (plainSocket->state() != QAbstractSocket::ConnectedState) {
                break;
            }
        }

        // If the request is small and the remote host closes the transmission
        // after sending, there's a chance that testConnection() will already
        // have triggered a shutdown.
        if (!ssl)
            continue;

        int readBytes = 0;
        data.resize(4096);
        ::memset(data.data(), 0, data.size());
        do {
            // Don't use SSL_pending(). It's very unreliable.
            if ((readBytes = q_SSL_read(ssl, data.data(), data.size())) > 0) {
                char *ptr = readBuffer.reserve(readBytes);
                ::memcpy(ptr, data.data(), readBytes);
                emit q->readyRead();
                transmitting = true;
                continue;
            }

            // Error.
            switch (q_SSL_get_error(ssl, readBytes)) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                // Out of data.
                break;
            case SSL_ERROR_ZERO_RETURN:
                // The remote host closed the connection.
                plainSocket->disconnectFromHost();
                break;
            default:
                // ### Handle errors better.
                q->setErrorString(QSslSocket::tr("Error while reading: %1").arg(SSL_ERRORSTR()));
                q->setSocketError(QAbstractSocket::UnknownSocketError);
                emit q->error(QAbstractSocket::UnknownSocketError);
                break;
            }
        } while (ssl && readBytes > 0);
    } while (ssl && ctx && transmitting);
}

bool QSslSocketBackendPrivate::testConnection()
{
    Q_Q(QSslSocket);

    // Check if the connection has been established. Get all errors from the
    // verification stage.
    _q_sslErrorList()->mutex.lock();
    _q_sslErrorList()->errors.clear();
    int result = (mode == QSslSocket::SslClientMode) ? q_SSL_connect(ssl) : q_SSL_accept(ssl);
    errorList << _q_sslErrorList()->errors;
    _q_sslErrorList()->mutex.unlock();

    // Check if we're encrypted or not.
    if (result <= 0) {
        switch (q_SSL_get_error(ssl, result)) {
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
            // The handshake is not yet complete.
            break;
        default:
            // ### Handle errors better
            q->setErrorString(QSslSocket::tr("Error during SSL handshake: %1").arg(SSL_ERRORSTR()));
            q->setSocketError(QAbstractSocket::UnknownSocketError);
            emit q->error(QAbstractSocket::UnknownSocketError);
            q->abort();
        }
        return false;
    }

    // Store the peer certificate and chain. For clients, the peer certificate
    // chain includes the peer certificate; for servers, it doesn't. Both the
    // peer certificate and the chain may be empty if the peer didn't present
    // any certificate.
    peerCertificateChain = STACKOFX509_to_QSslCertificates(q_SSL_get_peer_cert_chain(ssl));
    X509 *x509 = q_SSL_get_peer_certificate(ssl);
    peerCertificate = QSslCertificatePrivate::QSslCertificate_from_X509(x509);
    q_X509_free(x509);

    // This is now.
    QDateTime now = QDateTime::currentDateTime();
    QList<QSslError> errors;

    // Check all certificates in the certificate chain.
    foreach (QSslCertificate cert, peerCertificateChain) {
        // Check for certificate validity
        if (cert.effectiveDate() >= now) {
            errors << QSslError(QSslError::CertificateNotYetValid);
        } else if (cert.expiryDate() <= now) {
            errors << QSslError(QSslError::CertificateExpired);
        }
    }

    // Check the peer certificate itself. First try the subject's common name
    // (CN) as a wildcard, then try all alternate subject name DNS entries the
    // same way.
    if (!peerCertificate.isNull()) {
        QString peerName = q->peerName();
        QString commonName = peerCertificate.subjectInfo(QSslCertificate::CommonName);

        QRegExp regexp(commonName, Qt::CaseInsensitive, QRegExp::Wildcard);
        if (!regexp.exactMatch(peerName)) {
            bool matched = false;
            foreach (QString altName, peerCertificate.alternateSubjectNames().values(QSsl::DnsEntry)) {
                regexp.setPattern(altName);
                if (regexp.exactMatch(peerName)) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                // No matches in common names or alternate names.
                errors << QSslError(QSslError::HostNameMismatch);
            }
        }
    } else {
        errors << QSslError(QSslError::NoPeerCertificate);
    }

    // Verify the authenticity of the certificate. This code should really go
    // into QSslCertificate.  ### Crude and inefficient.

    // Check if the certificate is OK.
    for (int i = 0; i < errorList.size(); ++i) {
        int err = errorList.at(i);
        switch (err) {
        case X509_V_OK:
            // X509_V_OK is also reported if the peer had no certificate.
            break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
            errors << QSslError(QSslError::UnableToGetIssuerCertificate); break;
        case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
            errors << QSslError(QSslError::UnableToDecryptCertificateSignature); break;
        case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
            errors << QSslError(QSslError::UnableToDecodeIssuerPublicKey); break;
        case X509_V_ERR_CERT_SIGNATURE_FAILURE:
            errors << QSslError(QSslError::CertificateSignatureFailed); break;
        case X509_V_ERR_CERT_NOT_YET_VALID:
            errors << QSslError(QSslError::CertificateNotYetValid); break;
        case X509_V_ERR_CERT_HAS_EXPIRED:
            errors << QSslError(QSslError::CertificateExpired); break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
            errors << QSslError(QSslError::InvalidNotBeforeField); break;
        case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
            errors << QSslError(QSslError::InvalidNotAfterField); break;
        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
            errors << QSslError(QSslError::SelfSignedCertificate); break;
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            errors << QSslError(QSslError::SelfSignedCertificateInChain); break;
        case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
            errors << QSslError(QSslError::UnableToGetLocalIssuerCertificate); break;
        case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
            errors << QSslError(QSslError::UnableToVerifyFirstCertificate); break;
        case X509_V_ERR_CERT_REVOKED:
            errors << QSslError(QSslError::CertificateRevoked); break;
        case X509_V_ERR_INVALID_CA:
            errors << QSslError(QSslError::InvalidCaCertificate); break;
        case X509_V_ERR_PATH_LENGTH_EXCEEDED:
            errors << QSslError(QSslError::PathLengthExceeded); break;
        case X509_V_ERR_INVALID_PURPOSE:
            errors << QSslError(QSslError::InvalidPurpose); break;
        case X509_V_ERR_CERT_UNTRUSTED:
            errors << QSslError(QSslError::CertificateUntrusted); break;
        case X509_V_ERR_CERT_REJECTED:
            errors << QSslError(QSslError::CertificateRejected); break;
        default:
            errors << QSslError(QSslError::UnspecifiedError); break;
        }
    }

    if (!errors.isEmpty()) {
        sslErrors = errors;
        emit q->sslErrors(errors);
        if (!ignoreSslErrors) {
            q->setErrorString(sslErrors.first().errorString());
            plainSocket->disconnectFromHost();
            return false;
        }
    } else {
        sslErrors.clear();
    }
    
    connectionEncrypted = true;
    emit q->encrypted();
    return true;
}

void QSslSocketBackendPrivate::disconnectFromHost()
{
    if (ssl) {
        q_SSL_shutdown(ssl);
        transmit();
    }
    plainSocket->disconnectFromHost();
}

void QSslSocketBackendPrivate::disconnected()
{
    if (ssl) {
        q_SSL_free(ssl);
        ssl = 0;
    }
    if (ctx) {
        q_SSL_CTX_free(ctx);
        ctx = 0;
    }
}

QSslCipher QSslSocketBackendPrivate::sessionCipher() const
{
    if (!ssl || !ctx)
        return QSslCipher();
    SSL_CIPHER *sessionCipher = q_SSL_get_current_cipher(ssl);
    return sessionCipher ? QSslCipher_from_SSL_CIPHER(sessionCipher) : QSslCipher();
}

QList<QSslCertificate> QSslSocketBackendPrivate::STACKOFX509_to_QSslCertificates(STACK_OF(X509) *x509)
{
    ensureInitialized();
    QList<QSslCertificate> certificates;
    for (int i = 0; i < q_sk_X509_num(x509); ++i) {
        if (X509 *entry = q_sk_X509_value(x509, i))
            certificates << QSslCertificatePrivate::QSslCertificate_from_X509(entry);
    }
    return certificates;
}
