/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: clientguid.h,v 1.6 2005/02/25 21:30:41 darrick Exp $ 
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

#ifndef _CLIENTGUID_H_
#define _CLIENTGUID_H_

#include "hxengin.h"       // IHXCallback def
#include "clientregtree.h"  // Class def needed

// Forward decls

class ClientGUIDEntry;
class ClientGUIDTable;
class CHXMapStringToOb;
class Client;
struct IHXClientStats;

// Externs

extern ClientGUIDTable* g_pClientGUIDTable;

// Classes

class ClientGUIDTable         // One per server
{
public:
    ClientGUIDTable();

    friend class ClientGUIDEntry;

    ClientGUIDEntry*    GetCreateEntry(const char* playerGUID, Client* pClient);
    ClientGUIDEntry*    GetEntry(const char* playerGUID);

    BOOL                RemoveEntry(ClientGUIDEntry* pEntry);

private:
    ~ClientGUIDTable();		// Should never be destructed

    HX_MUTEX            m_Mutex;
    CHXMapStringToOb*   m_pTable;
};


class ClientGUIDEntry : public IHXCallback
{
public:

    static const int MAX_TIME_STRING_LEN;

    ClientGUIDEntry();
    ~ClientGUIDEntry();

    friend class ClientGUIDTable;

    void                        ClientGUIDEntryInit(Client* pClient);

    void                        SetPlayerGUID(const char* playerGUID);
    char*                       GetPlayerGUID()
                                    { return m_pPlayerGUID; };

    Client*			GetClient()
                                    { return m_pClient; };

    ClientRegTree*              GetRegTree()
                                    { return &m_RegTree; };

    HX_RESULT                   SetDestructDelay(unsigned int seconds);
    HX_RESULT                   Done();
    BOOL			RemoveFromTable();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj);

    STDMETHOD_(UINT32,AddRef)          (THIS);

    STDMETHOD_(UINT32,Release)         (THIS);

    /*
     * IHXCallback methods
     */
    STDMETHOD(Func) (THIS);


private:
    INT32               m_lRefCount;
    ClientRegTree       m_RegTree;
    IUnknown*           m_pContext;
    Client*             m_pClient;   // holds ref on this com object
    IHXClientStats*     m_pClientStats;
    char*               m_pPlayerGUID;
    HX_MUTEX            m_DefunctMutex;
    HX_MUTEX            m_RemoveMutex;
    BOOL		m_bDefunct;
    BOOL                m_bDone;
    BOOL                m_bInTable;
    unsigned int	m_nDestructDelay;
};


#endif //_CLIENTGUID_H_
