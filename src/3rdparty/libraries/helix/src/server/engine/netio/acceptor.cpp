/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: acceptor.cpp,v 1.5 2004/07/11 23:46:13 tmarshall Exp $
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

#ifdef _UNIX
#include <sys/param.h>
#include <errno.h>
#endif
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "tcpio.h"
#include "engine.h"
#include "servreg.h"
#include "acceptor.h"
#include "base_errmsg.h"
#include "servbuffer.h"
#include "debug.h"

#ifdef _UNIX
#if defined _AIX
#define SETEUID(uid) setreuid(-1, (uid))
#define SETEGID(gid) setregid(-1, (gid))
#elif defined _HPUX
#define SETEUID(uid) setresuid(-1, (uid), -1)
#define SETEGID(gid) setresgid(-1, (gid), -1)
#else
#define SETEUID(uid) seteuid(uid)
#define SETEGID(gid) setegid(gid)
#endif
#endif // _UNIX

Acceptor::Acceptor(Process* _proc)
        : state(DISABLED)
        , proc(_proc)
        , engine(_proc->pc->engine)
        , messages(_proc->pc->error_handler)
        , conn(0)
        , interface_ip(0)
        , port(0)
        , maxlisten(0)
        , local_addr(0)
        , registry(_proc->pc->registry)
{
    if (messages)
        messages->AddRef();
    input_callback = new AcceptorInputCallback;
    input_callback->AddRef();
    input_callback->acceptor = this;
}

Acceptor::~Acceptor()
{
    close();
    if (messages)
    {
        messages->Release();
        messages = 0;
    }

    input_callback->Release();
    input_callback = NULL;
}

void
Acceptor::close()
{
    if (conn)
    {

#ifdef _UNIX
        /*
         * if we failed to initialize the listen socket then even though
         * the TCPIO object has been created, it will not have a valid
         * file descriptor. so if we try to destory such a socket, this
         * method gets called and the callbacks.remove() call ASSERTs
         * because the file descriptor is invalid.
         *
         * so the condition prevents the assert from happening, in case
         * the socket initialization failed and we tried to destroy the
         * socket.
         */
        if (conn->fd() > 0)
            engine->callbacks.remove(HX_ACCEPTORS, conn);
#else
        engine->callbacks.remove(HX_ACCEPTORS, conn);
#endif
        engine->UnRegisterSock();
        delete conn;
        conn = 0;
    }
}

int
Acceptor::init(UINT32 l_a, int p, int ml)
{
    DPRINTF(D_INFO, ("Acceptor::init(local_addr(%lu), port(%d))\n",
            htonl(l_a), p));

    local_addr = l_a;
    maxlisten = ml;
    conn = new TCPIO();
    engine->RegisterSock();
    interface_ip = htonl(local_addr);
    port = p;
    if (conn->error())
        return -1;

#ifdef WIN32
    /*
     * We do this to make sure that REUSEADDR is off for NT because
     * REUSEADDR for NT means REUSEPORT as well.
     * On unix, we don't want to wait for the SO_LINGER timeout so we'll
     * leave that with REUSEADDR even though we don't actually need it.
     */
    if (conn->init(local_addr, port, FALSE, FALSE) < 0)
        return -1;
    if (conn->listen(maxlisten) < 0)
        return -1;
#else /* WIN32 */
    /*
     * in order to get access to the restricted ports on UNIX we need to
     * have super-user privlieges. sometimes the server is required to run
     * with non-superuser privileges inspite of root having run the server.
     * this little piece of code below just resets the gid and uid of the
     * process to its original (possibly root) privileges and then gets
     * the restricted port, after which it switches back to the non-superuser
     * privileges.
     */
    INT32 aptapr = -1;
    int gid = -1;
    int uid = -1;
    int ogid = -1;
    int ouid = -1;

    if (registry)
        registry->GetInt("Config.AllowPluginsToAccessRestrictedPorts",
            &aptapr, proc);
    if (aptapr > 0)
    {
        gid = getegid();
        uid = geteuid();
        ogid = getgid();
        ouid = getuid();
    }
    reset_permissions(aptapr, ogid, ouid, gid, uid);
    if (conn->init(local_addr, (INT16)port, FALSE) < 0)
    {
        set_permissions(aptapr, ogid, ouid, gid, uid);
        return -1;
    }

    if (conn->listen(maxlisten) < 0)
    {
        set_permissions(aptapr, ogid, ouid, gid, uid);
        return -1;
    }
    set_permissions(aptapr, ogid, ouid, gid, uid);

#endif /* WIN32 */

    engine->callbacks.add(HX_ACCEPTORS, conn, input_callback, TRUE);
    engine->callbacks.disable(HX_ACCEPTORS, conn);
    state = DISABLED;
    return 0;
}

HX_RESULT
Acceptor::switchport(int newport)
{
    TCPIO* newconn;
    newconn = new TCPIO();
    if (newconn->error())
    {
        delete newconn;
        newconn = 0;
        return HXR_FAIL;
    }

#ifdef WIN32
    if (newconn->init(local_addr, newport, FALSE, FALSE) < 0)
    {
        delete newconn;
        newconn = 0;
        return HXR_FAIL;
    }
    if (newconn->listen(maxlisten) < 0)
    {
        HX_ASSERT(0);
        delete newconn;
        newconn = 0;
        return HXR_FAIL;
    }
#else /* WIN32 */
    /* see Acceptor::init() for explanation of code below */
    INT32 aptapr = -1;
    int gid = -1;
    int uid = -1;
    int ogid = -1;
    int ouid = -1;

    registry->GetInt("Config.AllowPluginsToAccessRestrictedPorts",
        &aptapr, proc);
    if (aptapr > 0)
    {
        gid = getegid();
        uid = geteuid();
        ogid = getgid();
        ouid = getuid();
    }

    reset_permissions(aptapr, ogid, ouid, gid, uid);
    if (newconn->init(local_addr, (INT16)newport, FALSE) < 0)
    {
        delete newconn;
        newconn = 0;
        set_permissions(aptapr, ogid, ouid, gid, uid);
        return HXR_FAIL;
    }

    if (newconn->listen(maxlisten) < 0)
    {
        HX_ASSERT(0);
        delete newconn;
        newconn = 0;
        set_permissions(aptapr, ogid, ouid, gid, uid);
        return HXR_FAIL;
    }
    set_permissions(aptapr, ogid, ouid, gid, uid);
#endif /* WIN32 */

    /*
     * If we get here, then all is cool and we have bound to the new port.
     * Time to kill off the old one.
     */
    close();
    conn = newconn;
    newconn = 0;
    port = newport;
    engine->callbacks.add(HX_ACCEPTORS, conn, input_callback, TRUE);
    if(state == DISABLED)
    {
        engine->callbacks.disable(HX_ACCEPTORS, conn);
    }
    return HXR_OK;
}

void
Acceptor::enable()
{
    engine->callbacks.enable(HX_ACCEPTORS, conn, TRUE);
    state = ENABLED;
}

void
Acceptor::disable()
{
    engine->callbacks.disable(HX_ACCEPTORS, conn);
    state = DISABLED;
}

inline int
Acceptor::input()
{
    /*
     * Accept all pending connections.
     * All new connections are considered from unknown clients.
     * We discover their identity through initial communication.
     */
    for (;;)
    {
        sockaddr_in peer;
        INT32 peerlen = sizeof peer;
        TCPIO* tcp_io = conn->accept(&peer, &peerlen);

        if (!tcp_io)
        {
            if (conn->error() == EWOULDBLOCK)
                break;

            ERRMSG(messages,
                   "Acceptor::input: error: %s\n", strerror(conn->error()));

            break;
        }

        HXAtomicIncUINT32(g_pSocketAcceptCount);

        HX_ASSERT(FALSE);
        engine->RegisterSock();
        Accepted(tcp_io, peer, peerlen, port);
    }
    return 0;
}

STDMETHODIMP
Acceptor::AcceptorInputCallback::Func()
{
    return acceptor->input();
}


/*
 * Basic com stuff.
 */
STDMETHODIMP
Acceptor::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXActivePropUser))
    {
        AddRef();
        *ppvObj = (IHXActivePropUser*)this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(ULONG32)
Acceptor::AddRef()
{
    return 0xffffffff;
}

STDMETHODIMP_(ULONG32)
Acceptor::Release()
{
    return 0xffffffff;
}

/************************************************************************
* IHXActivePropUser::SetActiveInt
*
*    Async request to set int pName to ul.
*/
STDMETHODIMP
Acceptor::SetActiveInt(const char* pName,
                        UINT32 ul,
                        IHXActivePropUserResponse* pResponse)
{
    /*
     * Since we are only active for ports we know that someone is
     * trying to change our port.  We don't event have to know which
     * port we are (ex. rtsp, pna ...)
     */

    /*
     * If they are just trying to set it to what it was, tell them
     * that they succeeded.
     */
    if(ul == (UINT32)port)
    {
        /*
         * Set this so we can verify the commit.
         */
        pResponse->SetActiveIntDone(HXR_OK, pName, ul, 0, 0);
        return HXR_OK;
    }
    HX_RESULT res;
    res = switchport(ul);
    IHXBuffer* pBuf = 0;
    UINT32 ulNumRes = 0;
    if (res != HXR_OK)
    {
        pBuf = new ServerBuffer(TRUE);
        char errstr[256];
        in_addr addr;
        addr.s_addr = interface_ip;
        sprintf(errstr, "Local address %s: port in use.", inet_ntoa(addr));
        pBuf->Set((const unsigned char*)errstr, strlen(errstr) + 1);
        m_res_buffer[0] = pBuf;
        ulNumRes = 1;
    }

    pResponse->SetActiveIntDone(res, pName, ul, m_res_buffer, ulNumRes);
    if (pBuf)
    {
        pBuf->Release();
    }
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveStr
*
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
Acceptor::SetActiveStr(const char* pName,
                        IHXBuffer* pBuffer,
                        IHXActivePropUserResponse* pResponse)
{
    pResponse->SetActiveStrDone(HXR_FAIL, pName, pBuffer, 0, 0);
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::SetActiveBuf
*
*    Async request to set buffer pName to buffer in pBuffer.
*/
STDMETHODIMP
Acceptor::SetActiveBuf(const char* pName,
                            IHXBuffer* pBuffer,
                            IHXActivePropUserResponse* pResponse)
{
    pResponse->SetActiveBufDone(HXR_FAIL, pName, pBuffer, 0, 0);
    return HXR_OK;
}

/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*       Async request to delete the active property.
*/
STDMETHODIMP
Acceptor::DeleteActiveProp(const char* pName,
                            IHXActivePropUserResponse* pResponse)
{
    IHXBuffer* pBuf = new ServerBuffer(TRUE);
    char errstr[256];
    in_addr addr;
    addr.s_addr = interface_ip;
    sprintf(errstr, "Server using port on addr %s", inet_ntoa(addr));
    pBuf->Set((const unsigned char*)errstr, strlen(errstr) + 1);
    m_res_buffer[0] = pBuf;
    pResponse->DeleteActivePropDone(HXR_FAIL, pName, m_res_buffer, 1);
    return HXR_OK;
}

void
Acceptor::reset_permissions(int aptapr, int ogid, int ouid, int gid, int uid)
{
#ifdef _UNIX
    // reset orig uid/orig gid
    if (aptapr > 0)
    {
        if (ogid != gid)
            if (SETEGID(ogid) < 0)
                perror("could not reset permissions");

        if (ouid != uid)
            if (SETEUID(ouid) < 0)
                perror("could not reset permissions");
    }
#endif /* _UNIX */
}

void
Acceptor::set_permissions(int aptapr, int ogid, int ouid, int gid, int uid)
{
#ifdef _UNIX
    if (aptapr > 0)
    {
        if (ogid != gid)
            if (SETEGID(gid) < 0)
                perror("could not set permissions");
        if (ouid != uid)
            if (SETEUID(uid) < 0)
                perror("could not set permissions");
    }
#endif /* _UNIX */
}

