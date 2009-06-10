/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: clientregtree.h,v 1.5 2004/10/25 20:41:50 darrick Exp $
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

#ifndef _CLIENTREGTREE_H_
#define _CLIENTREGTREE_H_

#include "hxengin.h"  // IHXCallback def

class Client;
class Process;
_INTERFACE IHXRegistry;

class ClientRegTree
{
public:
    static const int MAX_TIME_STRING_LEN;

    enum Field
    {
        CLIENT,
        CONN_ID,
        ADDR,
        TIME,
        INTERFACE_ADDR,
        IN_PORT,
        PROTOCOL,
        VERSION,
        SESSION,
        SESSION_COUNT,
        CTRL_BYTES_SENT,
        IS_CLOAK,
        MAX_FIELDS
    };

    ClientRegTree();
    ~ClientRegTree();

    HX_RESULT           RegTreeInit(Client* pClient, IHXBuffer* pStartTime);

    UINT32              GetSeqNum() { return m_ulRegistryConnId; };
    UINT32              GetRegId(Field nField) { return m_ulRegId[nField]; };

    HX_RESULT           SetAddrs(IHXBuffer* pLocalAddr, IHXBuffer* pForeignAddr,
                                 UINT16 usForeignPort, BOOL is_cloak);

    HX_RESULT           SetProtocol(IHXBuffer* pValue);
    HX_RESULT           SetVersion(IHXBuffer* pValue);
    HX_RESULT           SetControlBytesSent(UINT32 ulControlBytesSent);

private:
    void                InitRegistry(IHXBuffer* pStartTime);
    HX_RESULT           DeleteRegEntry();

    LONG32              m_lRefCount;

    Process*            m_pProc;        // do not addref/release
    IUnknown*           m_pContext;
    IHXCommonClassFactory*      m_pClassFactory;
    IHXRegistry*        m_pRegistry;
    UINT32              m_ulRegistryConnId;
    UINT32              m_ulRegId[MAX_FIELDS];
};

#endif //_CLIENTREGTREE_H_
