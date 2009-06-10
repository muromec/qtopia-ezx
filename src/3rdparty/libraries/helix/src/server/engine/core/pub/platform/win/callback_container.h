/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: callback_container.h,v 1.7 2007/08/18 00:21:11 dcollins Exp $
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

#include "servcallback.h"
#include "hxengin.h"
#include "asyncio.h"
#include "hxerror.h"

enum SocketType
{
    HX_READERS,
    HX_WRITERS,
    HX_ACCEPTORS,
    HX_CONNECTORS
};


class CallbackContainer
{
public:
    CallbackContainer();
    ~CallbackContainer();


    void                add(int type, SOCKET socket, IHXCallback* cb, BOOL bThreadSafe = TRUE);
    void                add(int type, AsyncIO* io, IHXCallback *cb, BOOL bThreadSafe = TRUE);
    void                remove(int type, SOCKET socket);
    void                remove(int type, AsyncIO* io);
    void                enable(int type, SOCKET socket, BOOL bThreadSafe = TRUE);
    void                enable(int type, AsyncIO* io, BOOL bThreadSafe = TRUE);
    void                disable(int type, SOCKET socket);
    void                disable(int type, AsyncIO* io);
    fd_set*             get_set(int type);
    void                invoke_start();
    BOOL                invoke_n(int nNumReaders, int nNumWriters,
                                 UINT32* pNumReads, UINT32* pNumWrites);
    BOOL                invoke_n_ts(int nNumReaders, int nNumWriters);
    int                 Select(struct timeval* timeoutp);
    void                HandleBadFds(IHXErrorMessages* pErrorHandler);

private:
    Callbacks*          m_pCallbacks[4];
    CHXMapLongToObj     m_map;
    int                 m_nReaderMembers;
    int                 m_nWriterMembers;
    int                 m_nTSReaderMembers;
    int                 m_nTSWriterMembers;
};

inline
CallbackContainer::CallbackContainer()
{
    m_pCallbacks[HX_READERS] = new Callbacks();
    m_pCallbacks[HX_WRITERS] = new Callbacks();
    m_pCallbacks[HX_ACCEPTORS] = m_pCallbacks[HX_READERS];
    m_pCallbacks[HX_CONNECTORS] = new Callbacks();
    m_nReaderMembers = 0;
    m_nWriterMembers = 0;
    m_nTSReaderMembers = 0;
    m_nTSWriterMembers = 0;
}

inline
CallbackContainer::~CallbackContainer()
{
    delete m_pCallbacks[HX_READERS];
    delete m_pCallbacks[HX_WRITERS];
    m_pCallbacks[HX_ACCEPTORS] = 0;
    delete m_pCallbacks[HX_CONNECTORS];
}

inline void
CallbackContainer::add(int type, SOCKET socket, IHXCallback* cb, BOOL bThreadSafe)
{
    m_pCallbacks[type]->Add(socket, cb, bThreadSafe);
    enable(type, socket, bThreadSafe);
}

inline void
CallbackContainer::add(int type, AsyncIO* io, IHXCallback* cb, BOOL bThreadSafe)
{
    add(type, io->socket(), cb, bThreadSafe);
}

inline void
CallbackContainer::remove(int type, SOCKET socket)
{
    m_pCallbacks[type]->Remove(socket);
    disable(type, socket);
}

inline void
CallbackContainer::remove(int type, AsyncIO* io)
{
    remove(type, io->socket());
}

inline void
CallbackContainer::enable(int type, SOCKET s, BOOL bThreadSafe)
{
    m_pCallbacks[type]->Enable(s, bThreadSafe);
}

inline void
CallbackContainer::enable(int type, AsyncIO* io, BOOL bThreadSafe)
{
    enable(type, io->socket(), bThreadSafe);
}

inline void
CallbackContainer::disable(int type, SOCKET s)
{
    m_pCallbacks[type]->Disable(s);
}

inline void
CallbackContainer::disable(int type, AsyncIO* io)
{
    disable(type, io->socket());
}

inline fd_set*
CallbackContainer::get_set(int type)
{
    return m_pCallbacks[type]->GetSet();
}

inline void
CallbackContainer::invoke_start()
{
    m_nReaderMembers = m_nTSReaderMembers = 2;
    m_nWriterMembers = m_nTSWriterMembers = 2;
    m_pCallbacks[HX_READERS]->InvokeStart();
    m_pCallbacks[HX_WRITERS]->InvokeStart();
}

inline BOOL
CallbackContainer::invoke_n(int nReaders,
                            int nWriters,
                            UINT32* pNumReads,
                            UINT32* pNumWrites)
{
    int i;

    Callbacks* pReaders = m_pCallbacks[HX_READERS];
    Callbacks* pWriters = m_pCallbacks[HX_WRITERS];

    for (i = 0; i < nReaders && m_nReaderMembers > 1; i++)
    {
        m_nReaderMembers = pReaders->InvokeOne();
        if (m_nReaderMembers >= 1)
            (*pNumReads)++;
    }

    for (i = 0; i < nWriters && m_nWriterMembers > 1; i++)
    {
        m_nWriterMembers = pWriters->InvokeOne();
        if (m_nWriterMembers >= 1)
            (*pNumWrites)++;
    }

    m_pCallbacks[HX_CONNECTORS]->Invoke();

    return m_nReaderMembers + m_nWriterMembers > 2;
}

inline BOOL
CallbackContainer::invoke_n_ts(int  nReaders,
                               int  nWriters)
{
    int i;
    Callbacks* pReaders = m_pCallbacks[HX_READERS];
    Callbacks* pWriters = m_pCallbacks[HX_WRITERS];

    for (i = 0; i < nReaders && m_nTSReaderMembers > 1; i++)
    {
        m_nTSReaderMembers = pReaders->InvokeOneTS();
    }

    for (i = 0; i < nWriters && m_nTSWriterMembers > 1; i++)
    {
        m_nTSWriterMembers = pWriters->InvokeOneTS();
    }

    return m_nTSReaderMembers + m_nTSWriterMembers > 2;
}

inline int
CallbackContainer::Select(struct timeval* timeoutp)
{
    m_pCallbacks[HX_READERS]->Prepare();
    m_pCallbacks[HX_WRITERS]->Prepare();
    m_pCallbacks[HX_CONNECTORS]->Prepare();


    return select(0, m_pCallbacks[HX_READERS]->GetSet(),
                  m_pCallbacks[HX_WRITERS]->GetSet(),m_pCallbacks[HX_CONNECTORS]->GetSet(), timeoutp);

}

inline void
CallbackContainer::HandleBadFds(IHXErrorMessages* pErrorHandler)
{
    int ret, fd, i;
    struct sockaddr name;
    socklen_t namelen;

    Callbacks* pReaders = m_pCallbacks[HX_READERS];
    pReaders->Prepare();
    fd_set* read_set = pReaders->GetSet();
    int nRfds = read_set->fd_count;

    for (i = 0; i < nRfds; i++)
    {
        fd = read_set->fd_array[i];
        ret = getsockname(fd, &name, &namelen);
        if (ret < 0 && WSAGetLastError() == WSAENOTSOCK)
        {
            char buf[64];
            sprintf(buf, "select error (r fd=%d) = WSAENOTSOCK\n", fd);
            pErrorHandler->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, buf, 0);
            pReaders->Remove(fd);
        }
    }

    Callbacks* pWriters = m_pCallbacks[HX_WRITERS];
    pWriters->Prepare();
    fd_set* write_set = pWriters->GetSet();
    int nWfds = write_set->fd_count;

    for (i = 0; i < nWfds; i++)
    {
        fd = write_set->fd_array[i];
        ret = getsockname(fd, &name, &namelen);
        if (ret < 0 && WSAGetLastError() == WSAENOTSOCK)
        {
            char buf[64];
            sprintf(buf, "select error (w fd=%d) = WSAENOTSOCK\n", fd);
            pErrorHandler->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, buf, 0);
            pWriters->Remove(fd);
        }
    }

    Callbacks* pConnectors = m_pCallbacks[HX_CONNECTORS];
    pConnectors->Prepare();
    fd_set* connector_set = pConnectors->GetSet();
    int nCfds = connector_set->fd_count;

    for (i = 0; i < nCfds; i++)
    {
        fd = connector_set->fd_array[i];
        ret = getsockname(fd, &name, &namelen);
        if (ret < 0 && WSAGetLastError() == WSAENOTSOCK)
        {
            char buf[64];
            sprintf(buf, "select error (c fd=%d) = WSAENOTSOCK\n", fd);
            pErrorHandler->Report(HXLOG_ERR, HXR_FAIL, (ULONG32)HXR_FAIL, buf, 0);
            pConnectors->Remove(fd);
        }
    }
}

#endif /*_CALLBACK_CONTAINER_H_*/
