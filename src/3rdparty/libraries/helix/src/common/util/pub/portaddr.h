/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: portaddr.h,v 1.7 2007/07/06 20:39:23 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _PORTADDR_H_
#define _PORTADDR_H_

//XXXLCM ipv6 here

#include "hxpxymgr.h"

typedef struct _udp_ports
{
    UINT16  uFrom;
    UINT16  uTo;
} UDP_PORTS;

HX_RESULT   ReadUDPPorts(IHXBuffer* pValue, REF(CHXSimpleList*) pUDPPortList);
HX_RESULT   ReadListEntries(IHXBuffer* pValue, REF(CHXSimpleList*) pEntryList);
HXBOOL	    IsValidWildcardEntry(const char* pszValue);
HXBOOL	    IsValidSubnetEntry(const char* pszValue);

class CommonEntry
{
protected:
    UINT8   m_nChunks;
    char*   m_pszValue;
    char**  m_pChunks;

public:

    CommonEntry(const char* pszValue);
    ~CommonEntry(void);

    virtual HXBOOL IsEqual(const char* pszValue) = 0;
};

class NonWideCardEntry : public CommonEntry
{
public:
    
    NonWideCardEntry(const char* pszValue);

    HXBOOL IsEqual(const char* pszValue);
};

class WideCardEntry : public CommonEntry
{
public:

    WideCardEntry(const char* pszValue);

    HXBOOL IsEqual(const char* pszValue);
};

class SubnetEntry : public CommonEntry
{
private:
    UINT32  m_ulSubnet;
    UINT32  m_ulSubnetMask;

public:

    SubnetEntry(const char* pszValue);

    HXBOOL IsEqual(const char* pszValue);
};
    
class HXSubnetManager
{
private:
    IUnknown*		m_pContext;
    CHXSimpleList*	m_pEntryList;
    IHXPreferences*	m_pPreferences;
    IHXBuffer*		m_pPrevSubnet;

    void		ResetEntryList(void);

public:
    
    HXSubnetManager(IUnknown* pContext);
    ~HXSubnetManager();

    void    Initialize(void);
    HXBOOL    IsSubnet(const char* pszDomain);
    void    Close(void);
};

class HXProxyManager : public IHXProxyManager
{
private:
    LONG32		m_lRefCount;

    IHXPreferences*	m_pPreferences;
    IHXBuffer*		m_pPrevNoProxyFor;
    IUnknown*		m_pContext;
    CHXSimpleList*	m_pEntryList;

    void		ResetEntryList(void);

    ~HXProxyManager();

public:
    
    HXProxyManager();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXProxyManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXProxyManager::Initialize
     *	Purpose:
     *	    Initialize the proxy manager
     */
    STDMETHOD(Initialize)	(THIS_
				 IUnknown* pContext);

    /************************************************************************
     *	Method:
     *	    IHXProxyManager::IsExemptionHost
     *	Purpose:
     *	    check whether the host should be by-passed the proxy
     */
    STDMETHOD_(HXBOOL, IsExemptionHost)	(THIS_
					 char* pszDomain);

    void	Close(void);
};

#endif /* _PORTADDR_H_ */
