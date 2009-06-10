/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: acceptor.h,v 1.4 2004/07/11 23:46:13 tmarshall Exp $
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

#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_

#include "hxcom.h"
#include "hxerror.h"
#include "hxmon.h"
#include "sockio.h"
#include "tcpio.h"
#include "proc.h"
#include "engine.h"
#include "base_callback.h"

class ServerRegistry;

class Acceptor : public IHXActivePropUser
{
public:
                        Acceptor(Process* _proc);
                        ~Acceptor();
    int                 init(UINT32 local_addr, int port, int maxbacklog);
    int                 input();
    void                enable();
    void                disable();
    void                close();
    HX_RESULT           switchport(int newport);
    int                 error();
    void                reset_permissions(int aptapr, int ogid, int ouid,
                                int gid, int uid);
    void                set_permissions(int aptapr, int ogid, int ouid,
                                int gid, int uid);

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /************************************************************************
    * IHXActivePropUser::SetActiveInt
    *
    *    Async request to set int pName to ul.
    */
    STDMETHOD(SetActiveInt) (THIS_
                            const char* pName,
                            UINT32 ul,
                            IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::SetActiveStr
    *
    *    Async request to set string pName to string in pBuffer.
    */
    STDMETHOD(SetActiveStr) (THIS_
                            const char* pName,
                            IHXBuffer* pBuffer,
                            IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::SetActiveBuf
    *
    *    Async request to set buffer pName to buffer in pBuffer.
    */
    STDMETHOD(SetActiveBuf)     (THIS_
                                const char* pName,
                                IHXBuffer* pBuffer,
                                IHXActivePropUserResponse* pResponse);

    /************************************************************************
    * IHXActivePropUser::DeleteActiveProp
    *
    *   Async request to delete the active property.
    */
    STDMETHOD(DeleteActiveProp) (THIS_
                                const char* pName,
                                IHXActivePropUserResponse* pResponse);



protected:
    typedef enum
    {
        ENABLED,
        DISABLED
    } CB_STATE;

    CB_STATE state;

    class AcceptorInputCallback : public BaseCallback
    {
    public:
        STDMETHOD(Func) (THIS);
        Acceptor*   acceptor;
    };

    Process*                    proc;
    Engine*                     engine;
    IHXErrorMessages*           messages;
    AcceptorInputCallback*      input_callback;
    TCPIO*                      conn;
    ULONG32                     interface_ip;   // network byte order
    int                         port;
    int                         maxlisten;
    UINT32                      local_addr;
    ServerRegistry*             registry;
    IHXBuffer*                  m_res_buffer[1];

    virtual void                Accepted(TCPIO* tcp_io, sockaddr_in peer,
                                         int peerlen, int port) PURE;

};

inline int
Acceptor::error()
{
    if (conn)
        return conn->error();
    else
        return 0;
}

#endif /*_ACCEPTOR_H_*/
