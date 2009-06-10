/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: callback_container.h,v 1.12 2005/09/02 20:21:10 seansmith Exp $
 *
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 *
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _CALLBACK_CONTAINER_H_
#define _CALLBACK_CONTAINER_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "hxtypes.h"
#include "platform_config.h"
#include "servcallback.h"
#include "asyncio.h"
#include "hxengin.h"
#include "hxerror.h"

_INTERFACE IHXErrorMessages;

#ifdef DEV_POLL_SUPPORT
extern BOOL g_bUseDevPoll;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
extern BOOL g_bUseEPoll;
#endif // LINUX_EPOLL_SUPPORT

class CallbackContainer
{
public:
    CallbackContainer();
    ~CallbackContainer();

    /*
     * Exposed by API
     */

    void                add(int type, int fd, IHXCallback* cb, BOOL bThreadSafe = FALSE);
    void                remove(int type, int fd);
    void                enable(int type, int fd, BOOL bThreadSafe = FALSE);
    void                disable(int type, int fd);

    /*
     * Not exposed by API
     */

    void                init(int limit = 1000);
    void                add(int type, AsyncIO* i, IHXCallback* cb, BOOL bThreadSafe = FALSE);
    void                remove(int type, AsyncIO* i);
    void                enable(int type, AsyncIO* i, BOOL bThreadSafe = FALSE);
    void                disable(int type, AsyncIO* i);
    void                invoke_start();
    int                 invoke_n(int nNumReaders, int nNumWriters,
                                 UINT32* pNumReads, UINT32* pNumWrites);
    int                 invoke_n_ts(int nNumReaders, int nNumWriters);
    void                HandleBadFds(IHXErrorMessages* pErrorHandler);
    int                 Select(struct timeval* timeoutp);

private:
    Callbacks*          readers;
    Callbacks*          writers;
    int                 m_nReaderMembers;
    int                 m_nWriterMembers;
    int                 m_nTSReaderMembers;
    int                 m_nTSWriterMembers;

#ifdef DEV_POLL_SUPPORT
    Callbacks*          m_pReadersAndWriters;
    BOOL                m_bUseDevPoll;
    int                 m_nMembers;
    int                 m_nTSMembers;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    Callbacks*          m_pReadersAndWriters;
    BOOL                m_bUseEPoll;
    int                 m_nMembers;
    int                 m_nTSMembers;
#endif // LINUX_EPOLL_SUPPORT
};

inline
CallbackContainer::CallbackContainer() :
    readers(NULL),
    writers(NULL),
    m_nReaderMembers(0),
    m_nWriterMembers(0),
    m_nTSReaderMembers(0),
    m_nTSWriterMembers(0)
{
#ifdef DEV_POLL_SUPPORT
    m_pReadersAndWriters = NULL;
    m_bUseDevPoll = FALSE;
    m_nMembers = 0;
    m_nTSMembers = 0;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    m_pReadersAndWriters = NULL;
    m_bUseEPoll = FALSE;
    m_nMembers = 0;
    m_nTSMembers = 0;
#endif // LINUX_EPOLL_SUPPORT
}

inline
CallbackContainer::~CallbackContainer()
{
    HX_DELETE(readers);
    HX_DELETE(writers);

#ifdef DEV_POLL_SUPPORT
    HX_DELETE(m_pReadersAndWriters);
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    HX_DELETE(m_pReadersAndWriters);
#endif // LINUX_EPOLL_SUPPORT
}

inline void
CallbackContainer::init(int limit)
{
#ifdef DEV_POLL_SUPPORT
    // if we're supposed to use /dev/poll, then try to
    if (g_bUseDevPoll)
    {
        m_pReadersAndWriters = new Callbacks();
        if (!(m_bUseDevPoll = m_pReadersAndWriters->init(limit, TRUE)))
        {
            HX_DELETE(m_pReadersAndWriters);
        }
    }
    else
    {
        m_bUseDevPoll = FALSE;
    }

    // if we failed to initialize using /dev/poll or we're not supposed
    // to use it, use select
    if (!m_bUseDevPoll)
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    // if we're supposed to use epoll, then try to
    if (g_bUseEPoll)
    {
        m_pReadersAndWriters = new Callbacks();
        if (!(m_bUseEPoll = m_pReadersAndWriters->init(limit, TRUE)))
        {
            HX_DELETE(m_pReadersAndWriters);
        }
    }
    else
    {
        m_bUseEPoll = FALSE;
    }

    // if we failed to initialize using /dev/poll or we're not supposed
    // to use it, use select
    if (!m_bUseEPoll)
#endif // LINUX_EPOLL_SUPPORT
    {
        readers = new Callbacks();
        readers->init(limit, FALSE);
        writers = new Callbacks();
        writers->init(limit, FALSE);
    }
}

inline void
CallbackContainer::add(int type, int fd, IHXCallback* cb, BOOL bThreadSafe)
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        m_pReadersAndWriters->add(fd, cb, type, bThreadSafe);
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        m_pReadersAndWriters->add(fd, cb, type, bThreadSafe);
    }
    else
#endif // DEV_POLL_SUPPORT
    {
        if (type == HX_READERS || type == HX_ACCEPTORS)
            readers->add(fd, cb, type, bThreadSafe);
        else
            writers->add(fd, cb, type, bThreadSafe);
    }
}

inline void
CallbackContainer::add(int type, AsyncIO* i, IHXCallback* cb, BOOL bThreadSafe)
{
    add(type, i->fd(), cb, bThreadSafe);
}

inline void
CallbackContainer::remove(int type, int fd)
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        m_pReadersAndWriters->remove(type, fd);
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        m_pReadersAndWriters->remove(type, fd);
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        if (type == HX_READERS || type == HX_ACCEPTORS)
            readers->remove(type, fd);
        else
            writers->remove(type, fd);
    }
}

inline void
CallbackContainer::remove(int type, AsyncIO* i)
{
    remove(type, i->fd());
}

inline void
CallbackContainer::enable(int type, int fd, BOOL bThreadSafe)
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        m_pReadersAndWriters->enable(fd, type, bThreadSafe);
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        m_pReadersAndWriters->enable(fd, type, bThreadSafe);
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        if (type == HX_READERS || type == HX_ACCEPTORS)
        {
            readers->enable(fd, type, bThreadSafe);
        }
        else
        {
            writers->enable(fd, type, bThreadSafe);
        }
    }
}

inline void
CallbackContainer::enable(int type, AsyncIO* i, BOOL bThreadSafe)
{
    enable(type, i->fd(), bThreadSafe);
}

inline void
CallbackContainer::disable(int type, int fd)
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        m_pReadersAndWriters->disable(fd, type);
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        m_pReadersAndWriters->disable(fd, type);
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        if (type == HX_READERS || type == HX_ACCEPTORS)
            readers->disable(fd, type);
        else
            writers->disable(fd, type);
    }
}

inline void
CallbackContainer::disable(int type, AsyncIO* i)
{
    disable(type, i->fd());
}

inline int
CallbackContainer::Select(struct timeval* timeoutp)
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        m_nMembers = 2;
        m_nTSMembers = 2;
        int milliseconds = timeoutp->tv_sec * 1000 + (timeoutp->tv_usec/1000);
        return m_pReadersAndWriters->Poll(milliseconds);
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        m_nMembers = 2;
        m_nTSMembers = 2;
        int milliseconds = timeoutp->tv_sec * 1000 + (timeoutp->tv_usec/1000);
        return m_pReadersAndWriters->EventPoll(milliseconds);
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        m_nReaderMembers = m_nTSReaderMembers = 2;
        m_nWriterMembers = m_nTSWriterMembers = 2;
        readers->prepare();
        writers->prepare();

        return ::select(max(readers->high(), writers->high()) + 1,
                        readers->set(), writers->set(), 0, timeoutp);
    }
}

inline void
CallbackContainer::HandleBadFds(IHXErrorMessages* pErrorHandler)
{
    int ret, fd, i;
    struct sockaddr name;
    socklen_t namelen;

#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
        return;
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
        return;
#endif

    readers->prepare();
    int nfds = readers->members();
    int hi = readers->high();
    for (i = 0; i < nfds; i++)
    {
        fd = readers->fd(i);
        if (fd < 0)
            continue;
        ret = getsockname(fd, &name, &namelen);
        if (ret < 0 && errno == EBADF)
        {
            char buf[64];
            sprintf(buf, "%d: select(r fd=%d) = EBADF\n", getpid(), fd);
            pErrorHandler->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, buf, 0);
            readers->remove(HX_READERS, fd);
        }
    }

    writers->prepare();
    nfds = writers->members();
    hi = writers->high();
    for (i = 0; i < nfds; i++)
    {
        fd = writers->fd(i);
        if (fd < 0)
            continue;
        ret = getsockname(fd, &name, &namelen);
        if (ret < 0 && errno == EBADF)
        {
            char buf[64];
            sprintf(buf, "%d: select(w fd=%d) = EBADF\n", getpid(), fd);
            pErrorHandler->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, buf, 0);
            writers->remove(HX_WRITERS, fd);
        }
    }
}

inline void
CallbackContainer::invoke_start()
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        m_pReadersAndWriters->invoke_start();
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        m_pReadersAndWriters->invoke_start();
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        readers->invoke_start();
        writers->invoke_start();
    }
}

inline int
CallbackContainer::invoke_n(int nReaders,
                            int nWriters,
                            UINT32* pNumReads,
                            UINT32* pNumWrites)
{

#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        int i;
        for (i = 0; i < nReaders && m_nMembers > 0; i++)
        {
            m_nMembers = m_pReadersAndWriters->invoke_one(pNumReads, pNumWrites);
        }
        return m_nMembers > 0;
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        int i;
        for (i = 0; i < nReaders && m_nMembers > 0; i++)
        {
            m_nMembers = m_pReadersAndWriters->invoke_one(pNumReads, pNumWrites);
        }
        return m_nMembers > 0;
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        int i;
        for (i = 0; i < nReaders && m_nReaderMembers > 1; i++)
        {
            m_nReaderMembers = readers->invoke_one();
            if (m_nReaderMembers >= 1)
                (*pNumReads)++;
        }

        for (i = 0; i < nWriters && m_nWriterMembers > 1; i++)
        {
            m_nWriterMembers = writers->invoke_one();
            if (m_nWriterMembers >= 1)
                (*pNumWrites)++;
        }

        return m_nReaderMembers + m_nWriterMembers > 2;
    }
}

inline int
CallbackContainer::invoke_n_ts(int  nReaders,
                               int  nWriters)
{
#ifdef DEV_POLL_SUPPORT
    if (m_bUseDevPoll)
    {
        int i;
        for (i = 0; i < nReaders && m_nTSMembers > 0; i++)
        {
            m_nTSMembers = m_pReadersAndWriters->invoke_one_ts();
        }
        return m_nTSMembers > 0;
    }
    else
#endif // DEV_POLL_SUPPORT
#ifdef LINUX_EPOLL_SUPPORT
    if (m_bUseEPoll)
    {
        int i;
        for (i = 0; i < nReaders && m_nTSMembers > 0; i++)
        {
            m_nTSMembers = m_pReadersAndWriters->invoke_one_ts();
        }
        return m_nTSMembers > 0;
    }
    else
#endif // LINUX_EPOLL_SUPPORT
    {
        int i;
        for (i = 0; i < nReaders && m_nTSReaderMembers > 1; i++)
        {
            m_nTSReaderMembers = readers->invoke_one_ts();
        }

        for (i = 0; i < nWriters && m_nTSWriterMembers > 1; i++)
        {
            m_nTSWriterMembers = writers->invoke_one_ts();
        }

        return m_nTSReaderMembers + m_nTSWriterMembers > 2;
    }
}

#endif /*_CALLBACK_CONTAINER_H_*/
