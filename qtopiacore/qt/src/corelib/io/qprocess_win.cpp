/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qprocess.h"
#include "qprocess_p.h"

#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <private/qwineventnotifier_p.h>
#include <private/qthread_p.h>
#include <qdebug.h>

#include "private/qfsfileengine_p.h" // for longFileName and win95FileName

#ifndef QT_NO_PROCESS
//#define QPROCESS_DEBUG

#define SLEEPMIN 10
#define SLEEPMAX 500
#define NOTIFYTIMEOUT 100

class QIncrementalSleepTimer
{
public:
    QIncrementalSleepTimer(int msecs)
        : totalTimeOut(msecs)
        , nextSleep(qMin(SLEEPMIN, totalTimeOut))
    {
        timer.start();
    }

    int nextSleepTime()
    {
        if (totalTimeOut == -1)
            return -1;

        int tmp = nextSleep;

        nextSleep = qMin(nextSleep * 2, qMin(SLEEPMAX, timeLeft()));

        return tmp;
    }

    int timeLeft()
    {
        return qMax(totalTimeOut - timer.elapsed(), 0);
    }

    bool hasTimedOut()
    {
        return timer.elapsed() >= totalTimeOut;
    }

    void resetIncrements()
    {
        nextSleep = qMin(SLEEPMIN, timeLeft());
    }

private:
    QTime timer;
    int totalTimeOut;
    int nextSleep;
};

class QWindowsPipeWriter : public QThread
{
    Q_OBJECT
public:
    QWindowsPipeWriter(HANDLE writePipe, QObject * parent = 0);
    ~QWindowsPipeWriter();

    bool waitForWrite(int msecs);
    qint64 write(const char *data, qint64 maxlen);
    qint64 bytesToWrite() const
    {
        QMutexLocker locker(&lock);
        return data.size();
    }
    bool hadWritten() const
    {
        return hasWritten;
    }

signals:
    void canWrite();

protected:
   void run();

private:
    mutable QMutex lock;
    QWaitCondition waitCondition;
    bool quitNow;
    HANDLE writePipe;
    QByteArray data;
    bool hasWritten;
};


QWindowsPipeWriter::QWindowsPipeWriter(HANDLE pipe, QObject * parent)
    : QThread(parent), quitNow(false), hasWritten(false)
{

    DuplicateHandle(GetCurrentProcess(), pipe, GetCurrentProcess(),
                         &writePipe, 0, FALSE, DUPLICATE_SAME_ACCESS);
}


QWindowsPipeWriter::~QWindowsPipeWriter()
{
    lock.lock();
    quitNow = true;
    waitCondition.wakeOne();
    lock.unlock();
    if (!wait(100))
        terminate();
    CloseHandle(writePipe);
}

bool QWindowsPipeWriter::waitForWrite(int msecs)
{
    QMutexLocker locker(&lock);
    bool hadWritten = hasWritten;
    hasWritten = false;
    if (hadWritten)
        return true;
    if (!waitCondition.wait(&lock, msecs))
        return false;
    hadWritten = hasWritten;
    hasWritten = false;
    return hadWritten;
}

qint64 QWindowsPipeWriter::write(const char *ptr, qint64 maxlen)
{
    if (!isRunning())
        return -1;

    QMutexLocker locker(&lock);
    data.append(QByteArray(ptr, maxlen));
    waitCondition.wakeOne();
    return maxlen;
}


void QWindowsPipeWriter::run()
{

    for (;;) {

        lock.lock();

        while(data.isEmpty() && (!quitNow)) {
            waitCondition.wakeOne();
            waitCondition.wait(&lock);
        }

        if (quitNow) {
            lock.unlock();
            break;
        }

        QByteArray copy = data;

        lock.unlock();

        const char *ptrData = copy.data();
        qint64 maxlen = copy.size();
        qint64 totalWritten = 0;
        while ((!quitNow) && totalWritten < maxlen) {
            DWORD written = 0;
            // Write 2k at a time to prevent flooding the pipe. If you
            // write too much (4k-8k), the pipe can close
            // unexpectedly.
            if (!WriteFile(writePipe, ptrData + totalWritten, qMin<int>(2048, maxlen - totalWritten), &written, 0)) {
                if (GetLastError() == 0xE8 /*NT_STATUS_INVALID_USER_BUFFER*/) {
                    // give the os a rest
                    msleep(100);
                    continue;
                }
                return;
            }
            totalWritten += written;
#if defined QPROCESS_DEBUG
            qDebug("QWindowsPipeWriter::run() wrote %d %d/%d bytes", written, int(totalWritten), int(maxlen));
#endif
            lock.lock();
            data.remove(0, written);
            hasWritten = true;
            lock.unlock();
        }
        emit canWrite();
    }
}

static void qt_create_pipe(Q_PIPE *pipe, bool in)
{
    // Open the pipes.  Make non-inheritable copies of input write and output
    // read handles to avoid non-closable handles (this is done by the
    // DuplicateHandle() call).

    SECURITY_ATTRIBUTES secAtt = { sizeof( SECURITY_ATTRIBUTES ), NULL, TRUE };

    HANDLE tmpHandle;
    if (in) {                   // stdin
        if (!CreatePipe(&pipe[0], &tmpHandle, &secAtt, 1024 * 1024))
            return;
        if (!DuplicateHandle(GetCurrentProcess(), tmpHandle, GetCurrentProcess(),
                             &pipe[1], 0, FALSE, DUPLICATE_SAME_ACCESS))
            return;
    } else {                    // stdout or stderr
        if (!CreatePipe(&tmpHandle, &pipe[1], &secAtt, 1024 * 1024))
            return;
        if (!DuplicateHandle(GetCurrentProcess(), tmpHandle, GetCurrentProcess(),
                             &pipe[0], 0, FALSE, DUPLICATE_SAME_ACCESS))
            return;
    }

    CloseHandle(tmpHandle);
}

/*
    Create the pipes to a QProcessPrivate::Channel.

    This function must be called in order: stdin, stdout, stderr
*/
bool QProcessPrivate::createChannel(Channel &channel)
{
    Q_Q(QProcess);

    if (&channel == &stderrChannel && processChannelMode == QProcess::MergedChannels) {
        return DuplicateHandle(GetCurrentProcess(), stdoutChannel.pipe[1], GetCurrentProcess(),
                               &stderrChannel.pipe[1], 0, TRUE, DUPLICATE_SAME_ACCESS);
    }

    if (channel.type == Channel::Normal) {
        // we're piping this channel to our own process
        qt_create_pipe(channel.pipe, &channel == &stdinChannel);

        return true;
    } else if (channel.type == Channel::Redirect) {
        // we're redirecting the channel to/from a file
        SECURITY_ATTRIBUTES secAtt = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

        if (&channel == &stdinChannel) {
            // try to open in read-only mode
            channel.pipe[1] = INVALID_Q_PIPE;
            QT_WA({
                channel.pipe[0] =
                    CreateFileW((TCHAR*)QFSFileEnginePrivate::longFileName(channel.file).utf16(),
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &secAtt,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            }, {
                channel.pipe[0] =
                    CreateFileA(QFSFileEnginePrivate::win95Name(channel.file),
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &secAtt,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            });
            if (channel.pipe[0] != INVALID_Q_PIPE)
                return true;

            q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Could not open input redirection for reading")));
        } else {
            // open in write mode
            channel.pipe[0] = INVALID_Q_PIPE;
            DWORD creation;
            if (channel.append)
                creation = OPEN_ALWAYS;
            else
                creation = CREATE_ALWAYS;

            QT_WA({
                channel.pipe[1] =
                    CreateFileW((TCHAR*)QFSFileEnginePrivate::longFileName(channel.file).utf16(),
                                GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &secAtt,
                                creation,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            }, {
                channel.pipe[1] =
                    CreateFileA(QFSFileEnginePrivate::win95Name(channel.file),
                                GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &secAtt,
                                creation,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            });
            if (channel.pipe[1] != INVALID_Q_PIPE) {
                if (channel.append) {
                    SetFilePointer(channel.pipe[1], 0, NULL, FILE_END);
                }
                return true;
            }

            q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Could not open output redirection for writing")));
        }

        // could not open file
        processError = QProcess::FailedToStart;
        emit q->error(processError);
        cleanup();
        return false;
    } else {
        Q_ASSERT_X(channel.process, "QProcess::start", "Internal error");

        Channel *source;
        Channel *sink;

        if (channel.type == Channel::PipeSource) {
            // we are the source
            source = &channel;
            sink = &channel.process->stdinChannel;

            if (source->pipe[1] != INVALID_Q_PIPE) {
                // already constructed by the sink
                // make it inheritable
                HANDLE tmpHandle = source->pipe[1];
                if (!DuplicateHandle(GetCurrentProcess(), tmpHandle,
                                     GetCurrentProcess(), &source->pipe[1],
                                     0, TRUE, DUPLICATE_SAME_ACCESS))
                    return false;

                CloseHandle(tmpHandle);
                return true;
            }

            Q_ASSERT(source == &stdoutChannel);
            Q_ASSERT(sink->process == this && sink->type == Channel::PipeSink);

            qt_create_pipe(source->pipe, /* in = */ false); // source is stdout
            sink->pipe[0] = source->pipe[0];
            source->pipe[0] = INVALID_Q_PIPE;

            return true;
        } else {
            // we are the sink;
            source = &channel.process->stdoutChannel;
            sink = &channel;

            if (sink->pipe[0] != INVALID_Q_PIPE) {
                // already constructed by the source
                // make it inheritable
                HANDLE tmpHandle = sink->pipe[0];
                if (!DuplicateHandle(GetCurrentProcess(), tmpHandle,
                                     GetCurrentProcess(), &sink->pipe[0],
                                     0, TRUE, DUPLICATE_SAME_ACCESS))
                    return false;

                CloseHandle(tmpHandle);
                return true;
            }
            Q_ASSERT(sink == &stdinChannel);
            Q_ASSERT(source->process == this && source->type == Channel::PipeSource);

            qt_create_pipe(sink->pipe, /* in = */ true); // sink is stdin
            source->pipe[1] = sink->pipe[1];
            sink->pipe[1] = INVALID_Q_PIPE;

            return true;
        }
    }
}

void QProcessPrivate::destroyPipe(Q_PIPE pipe[2])
{
    if (pipe[0] == stdinChannel.pipe[0] && pipe[1] == stdinChannel.pipe[1] && pipeWriter) {
        delete pipeWriter;
        pipeWriter = 0;
    }

    if (pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(pipe[0]);
        pipe[0] = INVALID_Q_PIPE;
    }
    if (pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(pipe[1]);
        pipe[1] = INVALID_Q_PIPE;
    }
}


static QString qt_create_commandline(const QString &program, const QStringList &arguments)
{
    QString programName = program;
    if (!programName.startsWith(QLatin1Char('\"')) && !programName.endsWith(QLatin1Char('\"')) && programName.contains(QLatin1String(" ")))
        programName = QLatin1String("\"") + programName + QLatin1String("\"");
    programName.replace(QLatin1String("/"), QLatin1String("\\"));

    QString args;
    // add the prgram as the first arrg ... it works better
    args = programName + QLatin1String(" ");
    for (int i=0; i<arguments.size(); ++i) {
        QString tmp = arguments.at(i);
        // in the case of \" already being in the string the \ must also be escaped
        tmp.replace( QLatin1String("\\\""), QLatin1String("\\\\\"") );
        // escape a single " because the arguments will be parsed
        tmp.replace( QLatin1String("\""), QLatin1String("\\\"") );
        if (tmp.isEmpty() || tmp.contains(QLatin1Char(' ')) || tmp.contains(QLatin1Char('\t'))) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            QString endQuote(QLatin1String("\""));
            int i = tmp.length();
            while (i>0 && tmp.at(i-1) == QLatin1Char('\\')) {
                --i;
                endQuote += QLatin1String("\\");
            }
            args += QLatin1String(" \"") + tmp.left(i) + endQuote;
        } else {
            args += QLatin1Char(' ') + tmp;
        }
    }
    return args;
}

static QByteArray qt_create_environment(const QStringList &environment)
{
    QByteArray envlist;
    if (!environment.isEmpty()) {
        QStringList envStrings = environment;
	    int pos = 0;
	    // add PATH if necessary (for DLL loading)
        if (envStrings.filter(QRegExp(QLatin1String("^PATH="),Qt::CaseInsensitive)).isEmpty()) {
            QByteArray path = qgetenv("PATH");
            if (!path.isEmpty())
                envStrings.prepend(QString(QLatin1String("PATH=%1")).arg(QString::fromLocal8Bit(path)));
        }
        // add systemroot if needed
        if (envStrings.filter(QRegExp(QLatin1String("^SystemRoot="),Qt::CaseInsensitive)).isEmpty()) {
            QByteArray systemRoot = qgetenv("SystemRoot");
            if (!systemRoot.isEmpty())
                envStrings.prepend(QString(QLatin1String("SystemRoot=%1")).arg(QString::fromLocal8Bit(systemRoot)));
        }
#ifdef UNICODE
        if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)) {
            for (QStringList::ConstIterator it = envStrings.constBegin(); it != envStrings.constEnd(); ++it) {
                QString tmp = *it;
                uint tmpSize = sizeof(TCHAR) * (tmp.length()+1);
                envlist.resize(envlist.size() + tmpSize);
                memcpy(envlist.data()+pos, tmp.utf16(), tmpSize);
                pos += tmpSize;
	        }
	        // add the 2 terminating 0 (actually 4, just to be on the safe side)
	        envlist.resize( envlist.size()+4 );
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
	        envlist[pos++] = 0;
        } else
#endif // UNICODE
        {
            for (QStringList::ConstIterator it = envStrings.constBegin(); it != envStrings.constEnd(); it++) {
                QByteArray tmp = (*it).toLocal8Bit();
                uint tmpSize = tmp.length() + 1;
                envlist.resize(envlist.size() + tmpSize);
                memcpy(envlist.data()+pos, tmp.data(), tmpSize);
                pos += tmpSize;
            }
            // add the terminating 0 (actually 2, just to be on the safe side)
            envlist.resize(envlist.size()+2);
            envlist[pos++] = 0;
            envlist[pos++] = 0;
        }
    }
    return envlist;
}

void QProcessPrivate::startProcess()
{
    Q_Q(QProcess);

    bool success = false;

    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = 0;
    }
    pid = new PROCESS_INFORMATION;
    memset(pid, 0, sizeof(PROCESS_INFORMATION));

    processState = QProcess::Starting;
    emit q->stateChanged(processState);

    if (!createChannel(stdinChannel) ||
        !createChannel(stdoutChannel) ||
        !createChannel(stderrChannel))
        return;

    QString args = qt_create_commandline(program, arguments);
    QByteArray envlist = qt_create_environment(environment);

#if defined QPROCESS_DEBUG
    qDebug("Creating process");
    qDebug("   program : [%s]", program.latin1());
    qDebug("   args : %s", args.latin1());
    qDebug("   pass environment : %s", environment.isEmpty() ? "no" : "yes");
#endif

    DWORD dwCreationFlags = 0;
    if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based))
        dwCreationFlags |= CREATE_NO_WINDOW;

#ifdef UNICODE
    if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)) {
        dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
        STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,
	                                 (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                         (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                         0, 0, 0,
                                         STARTF_USESTDHANDLES,
                                         0, 0, 0,
                                         stdinChannel.pipe[0], stdoutChannel.pipe[1], stderrChannel.pipe[1]
        };
        success = CreateProcessW(0, (WCHAR*)args.utf16(),
                                 0, 0, TRUE, dwCreationFlags,
                                 environment.isEmpty() ? 0 : envlist.data(),
                                 workingDirectory.isEmpty() ? 0
                                    : (WCHAR*)QDir::toNativeSeparators(workingDirectory).utf16(),
                                 &startupInfo, pid);
    } else
#endif // UNICODE
    {
#ifndef Q_OS_TEMP
	    STARTUPINFOA startupInfo = { sizeof( STARTUPINFOA ), 0, 0, 0,
                                         (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                         (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                         0, 0, 0,
                                         STARTF_USESTDHANDLES,
                                         0, 0, 0,
                                         stdinChannel.pipe[0], stdoutChannel.pipe[1], stderrChannel.pipe[1]
	    };

	    success = CreateProcessA(0, args.toLocal8Bit().data(),
                                     0, 0, TRUE, dwCreationFlags, environment.isEmpty() ? 0 : envlist.data(),
                                     workingDirectory.isEmpty() ? 0
                                        : QDir::toNativeSeparators(workingDirectory).toLocal8Bit().data(),
                                     &startupInfo, pid);
#endif // Q_OS_TEMP
    }
#ifndef Q_OS_TEMP
    if (stdinChannel.pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(stdinChannel.pipe[0]);
        stdinChannel.pipe[0] = INVALID_Q_PIPE;
    }
    if (stdoutChannel.pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(stdoutChannel.pipe[1]);
        stdoutChannel.pipe[1] = INVALID_Q_PIPE;
    }
    if (stderrChannel.pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(stderrChannel.pipe[1]);
        stderrChannel.pipe[1] = INVALID_Q_PIPE;
    }
#endif

    if (!success) {
        cleanup();
        processError = QProcess::FailedToStart;
        q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Process failed to start")));
        emit q->error(processError);
        processState = QProcess::NotRunning;
        emit q->stateChanged(processState);
        return;
    }

    processState = QProcess::Running;

    if (threadData->eventDispatcher) {
        processFinishedNotifier = new QWinEventNotifier(pid->hProcess, q);
        QObject::connect(processFinishedNotifier, SIGNAL(activated(HANDLE)), q, SLOT(_q_processDied()));
        processFinishedNotifier->setEnabled(true);
        notifier = new QTimer(q);
        QObject::connect(notifier, SIGNAL(timeout()), q, SLOT(_q_notified()));
        notifier->start(NOTIFYTIMEOUT);
    }

    // give the process a chance to start ...
    Sleep(SLEEPMIN*2);
    _q_startupNotification();
}

bool QProcessPrivate::processStarted()
{
    return processState == QProcess::Running;
}

qint64 QProcessPrivate::bytesAvailableFromStdout() const
{
    if (stdoutChannel.pipe[0] == INVALID_Q_PIPE)
        return 0;

    DWORD bytesAvail = 0;
    PeekNamedPipe(stdoutChannel.pipe[0], 0, 0, 0, &bytesAvail, 0);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::bytesAvailableFromStdout() == %d", bytesAvail);
#endif
    if (processChannelMode == QProcess::ForwardedChannels && bytesAvail > 0) {
        QByteArray buf(bytesAvail, 0);
        DWORD bytesRead = 0;
        if (ReadFile(stdoutChannel.pipe[0], buf.data(), buf.size(), &bytesRead, 0) && bytesRead > 0) {
            HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hStdout) {
                DWORD bytesWritten = 0;
                WriteFile(hStdout, buf.data(), bytesRead, &bytesWritten, 0);
            }
        }
        bytesAvail = 0;
    }
    return bytesAvail;
}

qint64 QProcessPrivate::bytesAvailableFromStderr() const
{
    if (stderrChannel.pipe[0] == INVALID_Q_PIPE)
        return 0;

    DWORD bytesAvail = 0;
    PeekNamedPipe(stderrChannel.pipe[0], 0, 0, 0, &bytesAvail, 0);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::bytesAvailableFromStderr() == %d", bytesAvail);
#endif
    if (processChannelMode == QProcess::ForwardedChannels && bytesAvail > 0) {
        QByteArray buf(bytesAvail, 0);
        DWORD bytesRead = 0;
        if (ReadFile(stderrChannel.pipe[0], buf.data(), buf.size(), &bytesRead, 0) && bytesRead > 0) {
            HANDLE hStderr = GetStdHandle(STD_ERROR_HANDLE);
            if (hStderr) {
                DWORD bytesWritten = 0;
                WriteFile(hStderr, buf.data(), bytesRead, &bytesWritten, 0);
            }
        }
        bytesAvail = 0;
    }
    return bytesAvail;
}

qint64 QProcessPrivate::readFromStdout(char *data, qint64 maxlen)
{
    DWORD read = qMin(maxlen, bytesAvailableFromStdout());
    DWORD bytesRead = 0;

    if (read > 0 && !ReadFile(stdoutChannel.pipe[0], data, read, &bytesRead, 0))
        return -1;
    return bytesRead;
}

qint64 QProcessPrivate::readFromStderr(char *data, qint64 maxlen)
{
    DWORD read = qMin(maxlen, bytesAvailableFromStderr());
    DWORD bytesRead = 0;

    if (read > 0 && !ReadFile(stderrChannel.pipe[0], data, read, &bytesRead, 0))
        return -1;
    return bytesRead;
}


static BOOL CALLBACK qt_terminateApp(HWND hwnd, LPARAM procId)
{
    DWORD currentProcId = 0;
    GetWindowThreadProcessId(hwnd, &currentProcId);
    if (currentProcId == (DWORD)procId)
	    PostMessage(hwnd, WM_CLOSE, 0, 0);

    return TRUE;
}

void QProcessPrivate::terminateProcess()
{
    if (pid) {
        EnumWindows(qt_terminateApp, (LPARAM)pid->dwProcessId);
        PostThreadMessage(pid->dwThreadId, WM_CLOSE, 0, 0);
    }
}

void QProcessPrivate::killProcess()
{
    if (pid)
        TerminateProcess(pid->hProcess, 0xf291);
}

bool QProcessPrivate::waitForStarted(int)
{
    Q_Q(QProcess);

    if (processStarted())
        return true;

    if (processError == QProcess::FailedToStart)
        return false;

    processError = QProcess::Timedout;
    q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out")));
    return false;
}

bool QProcessPrivate::waitForReadyRead(int msecs)
{
    Q_Q(QProcess);

    QIncrementalSleepTimer timer(msecs);

    forever {
        if (!writeBuffer.isEmpty() && !_q_canWrite())
            return false;
        if (pipeWriter && pipeWriter->waitForWrite(0))
            timer.resetIncrements();

        bool readyReadEmitted = false;
        if (bytesAvailableFromStdout() != 0) {
            readyReadEmitted = _q_canReadStandardOutput() ? true : readyReadEmitted;
            timer.resetIncrements();
        }

        if (bytesAvailableFromStderr() != 0) {
            readyReadEmitted = _q_canReadStandardError() ? true : readyReadEmitted;
            timer.resetIncrements();
        }

        if (readyReadEmitted)
            return true;

        if (!pid)
            return false;
        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            // find the return value if there is noew data to read
            _q_processDied();
            return false;
        }

        Sleep(timer.nextSleepTime());
        if (timer.hasTimedOut())
            break;
    }

    processError = QProcess::Timedout;
    q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out")));
    return false;
}

bool QProcessPrivate::waitForBytesWritten(int msecs)
{

    Q_Q(QProcess);

    QIncrementalSleepTimer timer(msecs);

    forever {
        // Check if we have any data pending: the pipe writer has
        // bytes waiting to written, or it has written data since the
        // last time we called pipeWriter->waitForWrite().
        bool pendingDataInPipe = pipeWriter && (pipeWriter->bytesToWrite() || pipeWriter->hadWritten());

        // If we don't have pending data, and our write buffer is
        // empty, we fail.
        if (!pendingDataInPipe && writeBuffer.isEmpty())
            return false;

        // If we don't have pending data and we do have data in our
        // write buffer, try to flush that data over to the pipe
        // writer.  Fail on error.
        if (!pendingDataInPipe) {
            if (!_q_canWrite())
                return false;
        }

        // Wait for the pipe writer to acknowledge that it has
        // written. This will succeed if either the pipe writer has
        // already written the data, or if it manages to write data
        // within the given timeout. If the write buffer was non-empty
        // and the pipeWriter is now dead, that means _q_canWrite()
        // destroyed the writer after it successfully wrote the last
        // batch.
        if (!pipeWriter || pipeWriter->waitForWrite(0))
            return true;

        // If we wouldn't write anything, check if we can read stdout.
        if (bytesAvailableFromStdout() != 0) {
            _q_canReadStandardOutput();
            timer.resetIncrements();
        }

        // Check if we can read stderr.
        if (bytesAvailableFromStderr() != 0) {
            _q_canReadStandardError();
            timer.resetIncrements();
        }

        // Check if the process died while reading.
        if (!pid)
            return false;

        // Wait for the process to signal any change in its state,
        // such as incoming data, or if the process died.
        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            _q_processDied();
            return false;
        }

        // Only wait for as long as we've been asked.
        if (timer.hasTimedOut())
            break;
    }

    processError = QProcess::Timedout;
    q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out")));
    return false;
}


bool QProcessPrivate::waitForFinished(int msecs)
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::waitForFinished(%d)", msecs);
#endif

    QIncrementalSleepTimer timer(msecs);

    forever {
        if (!writeBuffer.isEmpty() && !_q_canWrite())
            return false;
        if (pipeWriter && pipeWriter->waitForWrite(0))
            timer.resetIncrements();

        if (bytesAvailableFromStdout() != 0) {
            _q_canReadStandardOutput();
            timer.resetIncrements();
        }

        if (bytesAvailableFromStderr() != 0) {
            _q_canReadStandardError();
            timer.resetIncrements();
        }

        if (!pid)
            return true;

        if (WaitForSingleObject(pid->hProcess, timer.nextSleepTime()) == WAIT_OBJECT_0) {
            _q_processDied();
            return true;
        }

        if (timer.hasTimedOut())
            break;
    }
    processError = QProcess::Timedout;
    q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out")));
    return false;
}


void QProcessPrivate::findExitCode()
{
    DWORD theExitCode;
    if (GetExitCodeProcess(pid->hProcess, &theExitCode)) {
        exitCode = theExitCode;
        //### for now we assume a crash if exit code is less than -1 or the magic number
        crashed = (exitCode == 0xf291 || (int)exitCode < 0);
    }
}

void QProcessPrivate::flushPipeWriter()
{
    if (pipeWriter && pipeWriter->bytesToWrite() > 0) {
        pipeWriter->waitForWrite(ULONG_MAX);
    }
}

qint64 QProcessPrivate::pipeWriterBytesToWrite() const
{
    return pipeWriter ? pipeWriter->bytesToWrite() : qint64(0);
}

qint64 QProcessPrivate::writeToStdin(const char *data, qint64 maxlen)
{
    Q_Q(QProcess);

    if (!pipeWriter) {
        pipeWriter = new QWindowsPipeWriter(stdinChannel.pipe[1], q);
        pipeWriter->start();
    }

    return pipeWriter->write(data, maxlen);
}

bool QProcessPrivate::waitForWrite(int msecs)
{
    Q_Q(QProcess);

    if (!pipeWriter || pipeWriter->waitForWrite(msecs))
        return true;

    processError = QProcess::Timedout;
    q->setErrorString(QLatin1String(QT_TRANSLATE_NOOP(QProcess, "Process opeation timed out")));
    return false;
}

void QProcessPrivate::_q_notified()
{
    notifier->stop();

    if (!writeBuffer.isEmpty() && (!pipeWriter || pipeWriter->waitForWrite(0)))
        _q_canWrite();

    if (bytesAvailableFromStdout())
        _q_canReadStandardOutput();

    if (bytesAvailableFromStderr())
        _q_canReadStandardError();

    if (processState != QProcess::NotRunning)
        notifier->start(NOTIFYTIMEOUT);
}

bool QProcessPrivate::startDetached(const QString &program, const QStringList &arguments, const QString &workingDir, qint64 *pid)
{
    QString args = qt_create_commandline(program, arguments);

    bool success = false;

    PROCESS_INFORMATION pinfo;

#ifdef UNICODE
    if (!(QSysInfo::WindowsVersion & QSysInfo::WV_DOS_based)) {
        STARTUPINFOW startupInfo = { sizeof( STARTUPINFO ), 0, 0, 0,
                                     (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                     (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                   };
        success = CreateProcessW(0, (WCHAR*)args.utf16(),
                                 0, 0, FALSE, CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE, 0, 
                                 workingDir.isEmpty() ? (const WCHAR *)0 : (const WCHAR *)workingDir.utf16(),
                                 &startupInfo, &pinfo);
    } else
#endif // UNICODE
    {
#ifndef Q_OS_TEMP
       STARTUPINFOA startupInfo = { sizeof( STARTUPINFOA ), 0, 0, 0,
                                     (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                     (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
                                     0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                                  };
       success = CreateProcessA(0, args.toLocal8Bit().data(),
                                0, 0, FALSE, CREATE_NEW_CONSOLE, 0,
                                workingDir.isEmpty() ? (LPCSTR)0 : workingDir.toLocal8Bit().constData(),
                                &startupInfo, &pinfo);
#endif // Q_OS_TEMP
    }

    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
        if (pid)
            *pid = pinfo.dwProcessId;
    }

    return success;
}


#include "qprocess_win.moc"

#endif // QT_NO_PROCESS

