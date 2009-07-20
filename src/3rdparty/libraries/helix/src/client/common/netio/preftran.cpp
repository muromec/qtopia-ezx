/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: preftran.cpp,v 1.39 2007/04/27 20:16:13 ping Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#include "hxcom.h"
#include "hxtypes.h"
#include "hlxclib/string.h"
#include "hlxclib/stdio.h"
#ifdef _MACINTOSH
//#include <stat.mac.h> // Not needed on Mac and doesn't exist in CW Pro 7.
#else
#include "hlxclib/sys/types.h"
#include "hlxclib/sys/stat.h"
#endif
#include "hlxclib/time.h"
#if defined(_AIX)
#include <ctype.h>
#endif

#include "hlxclib/sys/socket.h"
#if defined(_WINDOWS)  
#include "hlxclib/windows.h"
#endif /* _WINDOWS */

#include "hxresult.h"
#include "hxslist.h"
#include "ihxpckts.h"
#include "pckunpck.h"
#include "hxstrutl.h"
#include "dbcs.h"
#include "dllpath.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxthread.h"
#include "netbyte.h"
#include "hxpxymgr.h"
#include "portaddr.h"
#include "prefdefs.h"
#include "hxengin.h"
#include "hxnetif.h"
#include "pacutil.h"
#include "preftran.h"


#ifdef _UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#if defined _SOLARIS || defined _FREEBSD || defined _OPENBSD || defined _NETBSD
#include <sys/sockio.h>
#endif
#include <net/if.h>
#include <sys/ioctl.h>
#endif

#include "hlxclib/stdlib.h" // for i64toa

#if defined (_UNIX) && !defined(_SUN) && !defined(_SCO_UW) && !defined(_HPUX) && !defined(_IRIX) && !defined(_OSF1)
#include <sys/file.h>
#endif /* UNIX */

#define RM_PREFTRAN_CAPTION	"# RealMedia Preferred Transport File\n# This is a generated file!  Do not edit.\n\n"
#define RM_PREFTRAN_DIR_NAME	"preftran/"
#define RM_PREFTRAN_FILE_LOCK	"PrefTranFileLock"

#include "hxdir.h" /* for OS_SEPARATOR_CHAR and OS_SEPARATOR_STRING */


HXPreferredTransport::HXPreferredTransport(HXPreferredTransportManager* pOwner)
		     : m_lRefCount(0)
		     , m_pHost(NULL)
		     , m_bHTTPNG(FALSE)
		     , m_ulHost(0)
		     , m_ulParentPlaybacks(0)
		     , m_uPlaybacks(0)
		     , m_uCloakPort(0)
                     , m_uTransportAttempted(0)
		     , m_lastUsedTime(0)
		     , m_state(PTS_UNKNOWN)
		     , m_prefTransportClass(PTC_UNKNOWN)
		     , m_prefTransportProtocol(PTP_UNKNOWN)
		     , m_prefTransportType(UnknownMode)
                     , m_ulABD(0)
		     , m_pOwner(pOwner)
		     , m_pPrefTransportSinkList(NULL)
{
    HX_ADDREF(m_pOwner);
}

HXPreferredTransport::~HXPreferredTransport()
{
    Close();
}

STDMETHODIMP
HXPreferredTransport::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPreferredTransport), (IHXPreferredTransport*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPreferredTransport*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXPreferredTransport::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXPreferredTransport::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 * IHXPreferredTransport methods
 */
STDMETHODIMP
HXPreferredTransport::GetTransport (REF(TransportMode)	/* OUT */   prefTransportType,
				    REF(UINT16)		/* OUT */   ulCloakPort)
{
    HX_RESULT	rc = HXR_OK;

    prefTransportType = UnknownMode;
    ulCloakPort = 0;

    switch (m_state)
    {
    case PTS_CREATE:
	m_state = PTS_PENDING;
	prefTransportType = m_prefTransportType;
	ulCloakPort = m_uCloakPort;
	break;
    case PTS_PENDING:
	rc = HXR_WOULD_BLOCK;
	break;
    case PTS_READY:
	prefTransportType = m_prefTransportType;
	ulCloakPort = m_uCloakPort;
	break;
    default:
	break;
    }

    // try to avoid UDP when Cisco's VPN is in use
    // there is fragmentation issue with UDP in Cisco's VPN software
    // Case F034774 filed with Cisco
    if (HXR_WOULD_BLOCK != rc && m_pOwner && m_pOwner->m_bDisableUDP)
    {
        while (prefTransportType > UnknownMode &&
               prefTransportType < TCPMode)
        {
            m_pOwner->SwitchTransport(HXR_OK, this, prefTransportType);
        }

        if (UnknownMode == prefTransportType)
        {
            prefTransportType = m_prefTransportType;
        }
    }

    return rc;
}
    
STDMETHODIMP
HXPreferredTransport::SetTransport(TransportMode   /* IN  */   prefTransportType,
				   UINT16	   /* IN  */   ulCloakPort)
{
    HX_RESULT	rc = HXR_OK;
    HXBOOL	bSave = FALSE;
    UINT32	ulTransportMask = 0;

    HX_ASSERT(UnknownMode != prefTransportType);

    if (m_pOwner)
    {
	if (PTP_RTSP == m_prefTransportProtocol)
	{
	    ulTransportMask = m_pOwner->m_ulRTSPTransportMask;
	}
	else
	{
	    ulTransportMask = m_pOwner->m_ulPNMTransportMask;
    	}
    }

    // Don't set transportType if it falls out of the range(ulTransportMask)
    //
    // This could happen when the transport is explicitly specified in protocol
    // scheme(i.e. rtspu for UDP) even though UDP is not selected. Such transport
    // selection only applies to that particular URL, so we need to exclude it from
    // preferred transport database which applies to all the URLs from a particular
    // server
    if (!(ulTransportMask & prefTransportType))
    {
        goto exit;
    }

    // we will mask UDP to Multicast if Multicast is selected given
    // the fact the server will always try UDP along with Multicast request
    if (UDPMode == prefTransportType && m_pOwner)
    {
	if (PTP_RTSP == m_prefTransportProtocol)
	{
	    ulTransportMask = m_pOwner->m_ulRTSPTransportMask;
	}
	else
	{
	    ulTransportMask = m_pOwner->m_ulPNMTransportMask;
    	}

	if (ulTransportMask & ATTEMPT_MULTICAST)
	{
	    prefTransportType = MulticastMode;
	}
    }

    m_state = PTS_READY;
    m_uCloakPort = ulCloakPort;
    m_lastUsedTime = time(NULL);

    if (m_prefTransportType != prefTransportType)
    {
	bSave = TRUE;
	m_uPlaybacks = 0;
	m_prefTransportType = prefTransportType;
    }

    if (m_pPrefTransportSinkList)
    {
	CHXSimpleList::Iterator i = m_pPrefTransportSinkList->Begin();
	for (; i != m_pPrefTransportSinkList->End(); ++i)
	{
	    IHXPreferredTransportSink* pSink = (IHXPreferredTransportSink*)*i;
	    pSink->TransportSucceeded(m_prefTransportType, m_uCloakPort);
	}
    }

    if (m_pOwner)
    {
        m_pOwner->TransportSet(this, bSave);
    }

exit:

    return rc;
}

STDMETHODIMP
HXPreferredTransport::SwitchTransport(HX_RESULT	    /* IN  */   error,
				      REF(TransportMode)    /* INOUT */	prefTransportType)
{
    if (m_pOwner)
    {
        return m_pOwner->SwitchTransport(error, this, prefTransportType);
    }

    return HXR_FAILED;
}

STDMETHODIMP
HXPreferredTransport::RemoveTransport()
{
    m_state = PTS_UNKNOWN;
    m_uPlaybacks = 0;
    m_uCloakPort = 0;
    m_prefTransportType = UnknownMode;
    m_lastUsedTime = 0;

    if (m_pPrefTransportSinkList)
    {
	CHXSimpleList::Iterator i = m_pPrefTransportSinkList->Begin();
	for (; i != m_pPrefTransportSinkList->End(); ++i)
	{
	    IHXPreferredTransportSink* pSink = (IHXPreferredTransportSink*)*i;
	    pSink->TransportFailed();
	}
    }

    return HXR_OK;
}

STDMETHODIMP
HXPreferredTransport::AbortTransport()
{
    m_state = PTS_CREATE;
    if (m_pPrefTransportSinkList)
    {
        CHXSimpleList::Iterator i = m_pPrefTransportSinkList->Begin();
        for (; i != m_pPrefTransportSinkList->End(); ++i)
        {
            IHXPreferredTransportSink* pSink = (IHXPreferredTransportSink*)*i;
            pSink->TransportAborted();
        }
    }

    return HXR_OK;
}
    
STDMETHODIMP_(HXBOOL)
HXPreferredTransport::ValidateTransport(TransportMode    /* IN */	prefTransportType)
{
    if (m_pOwner)
    {
        return m_pOwner->ValidateTransport(this, prefTransportType);
    }

    return FALSE;
}

STDMETHODIMP_(PreferredTransportState)
HXPreferredTransport::GetState()
{
    return m_state;
}

STDMETHODIMP_(PreferredTransportClass)
HXPreferredTransport::GetClass()
{
    return m_prefTransportClass;
}

STDMETHODIMP
HXPreferredTransport::AddTransportSink(IHXPreferredTransportSink*	/* IN  */   pPrefTransportSink)
{
    LISTPOSITION lPosition = 0;

    if (!pPrefTransportSink)
    {
	return HXR_FAILED;
    }

    if (!m_pPrefTransportSinkList)
    {
	m_pPrefTransportSinkList = new CHXSimpleList;
    }
    else
    {
	lPosition = m_pPrefTransportSinkList->Find(pPrefTransportSink);
    }

    if (!lPosition)
    {
	pPrefTransportSink->AddRef();
	m_pPrefTransportSinkList->AddTail(pPrefTransportSink);
    }

    return HXR_OK;
}

STDMETHODIMP
HXPreferredTransport::RemoveTransportSink(IHXPreferredTransportSink*	/* IN  */   pPrefTransportSink)
{
    if (!m_pPrefTransportSinkList)
    {
	return HXR_UNEXPECTED;
    }

    LISTPOSITION lPosition = m_pPrefTransportSinkList->Find(pPrefTransportSink);
    if (!lPosition)
    {
	return HXR_UNEXPECTED;
    }

    m_pPrefTransportSinkList->RemoveAt(lPosition);
    HX_RELEASE(pPrefTransportSink);
    
    return HXR_OK;
}

STDMETHODIMP_(HXBOOL)
HXPreferredTransport::GetHTTPNG()
{
    return m_bHTTPNG;
}

STDMETHODIMP
HXPreferredTransport::SetHTTPNG(HXBOOL bHTTPNG)
{
    m_bHTTPNG = bHTTPNG;
    return HXR_OK;
}

STDMETHODIMP
HXPreferredTransport::SetAutoBWDetectionValue(UINT32 ulBW)
{
    m_ulABD = ulBW;
    return HXR_OK;
}

STDMETHODIMP
HXPreferredTransport::GetAutoBWDetectionValue(REF(UINT32) ulBW)
{
    ulBW = m_ulABD;
    return (ulBW)?HXR_OK:HXR_FAILED;
}

void
HXPreferredTransport::Initialize()
{
    TransportMode upShiftToTransport = UnknownMode;

    if (!m_pOwner)
    {
        return;
    }

    if (m_lastUsedTime && PTS_READY == m_state)
    {
	// detect whether the preferences has been changed
	// since its last use
	if (PTP_RTSP == m_prefTransportProtocol &&
	    m_lastUsedTime < m_pOwner->m_lastRTSPPreferencesModifiedTime)
	{
	    m_state = PTS_CREATE;
	    m_uPlaybacks = 0;
	    m_prefTransportType = m_pOwner->m_rtspTransportTypeStartWith;
	}
	else if (PTP_PNM == m_prefTransportProtocol &&
	         m_lastUsedTime < m_pOwner->m_lastPNMPreferencesModifiedTime)
	{
	    m_state = PTS_CREATE;
	    m_uPlaybacks = 0;
	    m_prefTransportType = m_pOwner->m_pnmTransportTypeStartWith;
	}
	else if (m_pOwner->m_ulPlaybacks != m_ulParentPlaybacks)
	{
	    m_ulParentPlaybacks = m_pOwner->m_ulPlaybacks;   
	    m_uPlaybacks++;
	    // save the playback counter for UpShift
	    if (PTP_RTSP == m_prefTransportProtocol &&
		m_prefTransportType > m_pOwner->m_rtspTransportTypeStartWith)
	    {
		m_pOwner->m_bSave = TRUE;
	    }
	    else if (PTP_PNM == m_prefTransportProtocol &&
		     m_prefTransportType > m_pOwner->m_pnmTransportTypeStartWith)
	    {
		m_pOwner->m_bSave = TRUE;
	    }		

	    // attempt transport upshift every 3rd play
	    m_uPlaybacks = m_uPlaybacks % 3;
	    if (0 == m_uPlaybacks)
	    {
		upShiftToTransport = m_prefTransportType;
		m_pOwner->UpShiftTransport(this, upShiftToTransport);

		if (upShiftToTransport != m_prefTransportType)
		{
		    m_state = PTS_CREATE;
		    m_prefTransportType = upShiftToTransport;
		}
	    }
	}
    }
    // this is the first use
    else
    {
	if (PTS_CREATE == m_state)
	{
	    m_uPlaybacks = 0;
	    m_prefTransportType = m_pOwner->GetTransportPreferred(this);
	}
	else
	{
	    HX_ASSERT(PTS_PENDING == m_state);
	}
    }
    
    // sanity check
    if (UnknownMode != m_prefTransportType)
    {
	if (PTP_RTSP == m_prefTransportProtocol)
	{
	    HX_ASSERT(m_prefTransportType >= m_pOwner->m_rtspTransportTypeStartWith);
	}
	else if (PTP_PNM == m_prefTransportProtocol)
	{
	    HX_ASSERT(m_prefTransportType >= m_pOwner->m_pnmTransportTypeStartWith);
	}

        m_uTransportAttempted = (1 << (INT8)m_prefTransportType);
    }

    return;
}

void
HXPreferredTransport::Close()
{
    HX_DELETE(m_pHost);

    if (m_pPrefTransportSinkList)
    {
	CHXSimpleList::Iterator i = m_pPrefTransportSinkList->Begin();
	for (; i != m_pPrefTransportSinkList->End(); ++i)
	{
	    IHXPreferredTransportSink* pSink = (IHXPreferredTransportSink*)*i;
	    HX_RELEASE(pSink);
	}
	HX_DELETE(m_pPrefTransportSinkList);
    }

    HX_RELEASE(m_pOwner);
}

HXPreferredTransportManager::HXPreferredTransportManager(IUnknown* pContext)
			    : m_lRefCount(0)	
			    , m_bInitialized(FALSE)
			    , m_bSave(FALSE)
                            , m_bDisableUDP(FALSE)
			    , m_pszFile(NULL)
			    , m_ulRTSPTransportMask(ATTEMPT_AUTOTRANSPORT)
			    , m_ulPNMTransportMask(ATTEMPT_AUTOTRANSPORT)
			    , m_ulLocalHost(0)
			    , m_ulSubnetMask(0xFFFFFFFF)
			    , m_ulSubnet(0)
			    , m_ulPlaybacks(0)
                            , m_ulMasterABD(0)
			    , m_lastRTSPPreferencesModifiedTime(0)
			    , m_lastPNMPreferencesModifiedTime(0)
			    , m_internalTransportType(MulticastMode)
			    , m_externalTransportType(MulticastMode)
			    , m_rtspTransportTypeStartWith(MulticastMode)
			    , m_pnmTransportTypeStartWith(MulticastMode)
			    , m_pSubnetManager(NULL)
			    , m_pPrefHostTransportList(NULL)
			    , m_pPrevPrefHostTransportList(NULL)
			    , m_pProxyManager(NULL)
			    , m_pPreferences(NULL)
			    , m_pHXNetInterface(NULL)
#ifdef _WINDOWS
			    , m_pLock(NULL)
#elif _UNIX
			    , m_fileID(0)
#endif /* _WINDOWS */
{    
    if (pContext)
    {
	m_pContext = pContext;
	m_pContext->AddRef();
    }
}

HXPreferredTransportManager::~HXPreferredTransportManager()
{
    Close();
}

STDMETHODIMP
HXPreferredTransportManager::QueryInterface(REFIID riid, void**ppvObj)
{
    QInterfaceList qiList[] =
        {
            { GET_IIDHANDLE(IID_IHXPreferredTransportManager), (IHXPreferredTransportManager*)this },
            { GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPreferredTransportManager*)this },
        };
    
    return ::QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXPreferredTransportManager::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::Release
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(ULONG32) 
HXPreferredTransportManager::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT    
HXPreferredTransportManager::_Initialize(void)
{
    HX_RESULT	hr = HXR_OK;
    
    if (!m_pContext)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (!m_pSubnetManager)
    {
	m_pSubnetManager = new HXSubnetManager((IUnknown*)m_pContext);
    }

#if defined(HELIX_FEATURE_PROXYMGR)
    if (!m_pProxyManager)
    {
	m_pContext->QueryInterface(IID_IHXProxyManager, (void**)&m_pProxyManager);
	HX_ASSERT(m_pProxyManager);
    }
#endif /* #if defined(HELIX_FEATURE_PROXYMGR) */

    CollectNetworkInfo();

    PrepPrefTransport();

    OpenPrefTransport();

    m_bInitialized = TRUE;

cleanup:

    return hr;
}

HX_RESULT
HXPreferredTransportManager::ReadPreferences(HXBOOL  bRTSPProtocol, UINT32& ulTransportMask)
{
    HXBOOL	bAutoTransport = TRUE;
    HXBOOL	bMulticast = FALSE;
    HXBOOL	bUDP = FALSE;
    HXBOOL	bTCP = FALSE;
    HXBOOL	bHTTPCloak = FALSE; 

    ulTransportMask = 0;

    ReadPrefBOOL(m_pPreferences, "AutoTransport", bAutoTransport);

    if (bAutoTransport)
    {
	ulTransportMask = ATTEMPT_AUTOTRANSPORT;

        WritePrefUINT32(m_pPreferences, bRTSPProtocol?"AttemptRTSPvMulticast":"AttemptPNAvMulticast", (HXBOOL)(ulTransportMask & ATTEMPT_MULTICAST));
        WritePrefUINT32(m_pPreferences, bRTSPProtocol?"AttemptRTSPvUDP":"AttemptPNAvUDP", (HXBOOL)(ulTransportMask & ATTEMPT_UDP));
        WritePrefUINT32(m_pPreferences, bRTSPProtocol?"AttemptRTSPvTCP":"AttemptPNAvTCP", (HXBOOL)(ulTransportMask & ATTEMPT_TCP));
        WritePrefUINT32(m_pPreferences, bRTSPProtocol?"AttemptRTSPvHTTP":"AttemptPNAvHTTP", (HXBOOL)(ulTransportMask & ATTEMPT_HTTPCLOAK));
    }
    else
    {
        ReadPrefBOOL(m_pPreferences, bRTSPProtocol?"AttemptRTSPvMulticast":"AttemptPNAvMulticast", bMulticast);
        ReadPrefBOOL(m_pPreferences, bRTSPProtocol?"AttemptRTSPvUDP":"AttemptPNAvUDP", bUDP);
        ReadPrefBOOL(m_pPreferences, bRTSPProtocol?"AttemptRTSPvTCP":"AttemptPNAvTCP", bTCP);
        ReadPrefBOOL(m_pPreferences, bRTSPProtocol?"AttemptRTSPvHTTP":"AttemptPNAvHTTP", bHTTPCloak);

	if (bMulticast)
	{
	    ulTransportMask |= ATTEMPT_MULTICAST;
	}

	if (bUDP)
	{
	    ulTransportMask |= ATTEMPT_UDP;
	}

	if (bTCP)
	{
	    ulTransportMask |= ATTEMPT_TCP;
	}

	if (bHTTPCloak)
	{
	    ulTransportMask |= ATTEMPT_HTTPCLOAK;
	}
    }

    return HXR_OK;
}

void	
HXPreferredTransportManager::Close(void)
{
    if (m_bSave)
    {
	SavePrefTransport();
    }

    ResetPrefTransport(m_pPrevPrefHostTransportList);
    HX_DELETE(m_pPrevPrefHostTransportList);

    ResetPrefTransport(m_pPrefHostTransportList);
    HX_DELETE(m_pPrefHostTransportList);

    HX_DELETE(m_pSubnetManager);

    if (m_pHXNetInterface)
    {
	m_pHXNetInterface->RemoveAdviseSink((IHXNetInterfacesAdviseSink*)this);
	HX_RELEASE(m_pHXNetInterface);
    }

#if defined(HELIX_FEATURE_PROXYMGR)
    HX_RELEASE(m_pProxyManager);
#endif /* HELIX_FEATURE_PROXYMGR */

    HX_RELEASE(m_pPreferences);
    HX_RELEASE(m_pContext);

    HX_VECTOR_DELETE(m_pszFile);

    m_bInitialized = FALSE;

#ifdef _WINDOWS
    HX_RELEASE(m_pLock);
#endif /* _WINDOWS */
}

void
HXPreferredTransportManager::TransportSet(HXPreferredTransport* pPreferredTransport,
					   HXBOOL bSave)
{
    if (PTC_INTERNAL == pPreferredTransport->m_prefTransportClass)
    {
	m_internalTransportType = pPreferredTransport->m_prefTransportType;
    }
    else if (PTC_EXTERNAL == pPreferredTransport->m_prefTransportClass)
    {
	m_externalTransportType = pPreferredTransport->m_prefTransportType;
    }
    else
    {
	HX_ASSERT(FALSE);
    }

    if (bSave)
    {
	m_bSave = TRUE;
    }
}
					      
/************************************************************************
 *	Method:
 *	    IHXPreferredTransportManager::Initialize
 *	Purpose:
 *	    Initialize the transport manager such as re-reading the preferences
 */
STDMETHODIMP
HXPreferredTransportManager::Initialize()
{
    HX_RESULT	rc = HXR_OK;
    HXBOOL	bPreferenceModified = FALSE;
    UINT32	ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
    UINT32	ulPNMTransportMask = ATTEMPT_AUTOTRANSPORT;

    //myTestSuite();

    if (!m_bInitialized)
    {
	rc = _Initialize();
    }

    HX_ASSERT(HXR_OK == rc);

    ResetPrefTransport(m_pPrevPrefHostTransportList);

    ReadPreferences(TRUE, ulRTSPTransportMask);
    ReadPreferences(FALSE, ulPNMTransportMask);

    if (ulRTSPTransportMask != m_ulRTSPTransportMask)
    {
	bPreferenceModified = TRUE;
	m_ulRTSPTransportMask = ulRTSPTransportMask;
	m_lastRTSPPreferencesModifiedTime = time(NULL);
    }

    if (ulPNMTransportMask != m_ulPNMTransportMask)
    {
	bPreferenceModified = TRUE;
	m_ulPNMTransportMask = ulPNMTransportMask;
	m_lastPNMPreferencesModifiedTime = time(NULL);
    }

    InitTransportTypeStartWith(m_ulRTSPTransportMask, m_rtspTransportTypeStartWith);
    InitTransportTypeStartWith(m_ulPNMTransportMask, m_pnmTransportTypeStartWith);

    // re-initialize internal/external transport type to its lowest start transport
    // type when we detect the transport preference has been changed
    if (bPreferenceModified)
    {
	m_externalTransportType = m_internalTransportType = GetLowerTransport(m_rtspTransportTypeStartWith,
									      m_pnmTransportTypeStartWith);
    }

    m_pSubnetManager->Initialize();

#if defined(HELIX_FEATURE_PROXYMGR)
    m_pProxyManager->Initialize(m_pContext);
#endif /* HELIX_FEATURE_PROXYMGR */

    m_ulPlaybacks++;

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXPreferredTransportManager::GetPrefTransport
 *	Purpose:
 *	    Get preferred host transport
 */
STDMETHODIMP
HXPreferredTransportManager::GetPrefTransport(const char*		    /* IN  */ pszHostName,
					      PreferredTransportProtocol    /* IN  */ prefTransportProtocol,
					      REF(IHXPreferredTransport*)   /* OUT */ pPrefTransport)
{
    HX_RESULT		    rc = HXR_OK;
    HXBOOL		    bFound = FALSE;
    UINT32		    ulIAddress = 0;
    UINT32		    ulHostAddress = 0;
    HXPreferredTransport*   pTransport = NULL;
    PreferredTransportClass prefTransportClass = PTC_EXTERNAL;
    CHXString*		    pDomainHost = NULL;
    CHXSimpleList::Iterator i;

    pPrefTransport = NULL;

    if (!m_bInitialized)
    {
	rc = _Initialize();
	if (HXR_OK != rc)
	{
	    goto cleanup;
	}
    }

    if (pszHostName)
    {
        if(IsNumericAddr(pszHostName, strlen(pszHostName)))
        {
	    ulIAddress = HXinet_addr(pszHostName);
	    ulHostAddress = DwToHost(ulIAddress);
	    pDomainHost = new CHXString(pszHostName);
        }
        else
        {
	    pDomainHost = GetMasterDomain(pszHostName);
        }

        prefTransportClass = GetTransportClass(pszHostName, ulHostAddress);

        if (m_pPrefHostTransportList)
        {
	    i = m_pPrefHostTransportList->Begin();
	    for (; i != m_pPrefHostTransportList->End(); ++i)
	    {
	        pTransport = (HXPreferredTransport*)*i;

	        // based on host name for now
	        if (pTransport->m_pHost &&
                    !pTransport->m_pHost->CompareNoCase((const char*)*pDomainHost)   &&
		    pTransport->m_prefTransportProtocol == prefTransportProtocol    &&
		    pTransport->m_prefTransportClass == prefTransportClass)
	        {
		    bFound = TRUE;
		    break;
	        }
	    }
        }
    }

    if (bFound)
    {
	// previous transport attempt is failed
	if (pTransport->m_state == PTS_UNKNOWN)
	{
	    pTransport->m_state = PTS_CREATE;
	}
	 
	pTransport->Initialize();
        pPrefTransport = pTransport;
	pPrefTransport->AddRef();
	HX_DELETE(pDomainHost);
    }
    else
    {
	pTransport = new HXPreferredTransport(this);
	pTransport->AddRef();

	pTransport->m_pHost = pDomainHost;
	pTransport->m_ulHost = ulHostAddress;
	pTransport->m_prefTransportClass = prefTransportClass;
	pTransport->m_prefTransportProtocol = prefTransportProtocol; 	
	pTransport->m_state = PTS_CREATE;

	pTransport->Initialize();

        if (!m_pPrefHostTransportList)
        {
	    m_pPrefHostTransportList = new CHXSimpleList();
        }

	m_pPrefHostTransportList->AddTail(pTransport);
	rc = pTransport->QueryInterface(IID_IHXPreferredTransport, (void**)&pPrefTransport);
    }

    if (pTransport->m_state != PTS_READY)
    {
	m_bSave = TRUE;
    }

cleanup:

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXPreferredTransportManager::RemovePrefTransport
 *	Purpose:
 *	    Remove preferred host transport
 */
STDMETHODIMP
HXPreferredTransportManager::RemovePrefTransport(IHXPreferredTransport*   /* IN */ pPrefTransport)
{
    HX_RESULT		    rc = HXR_OK;
    HXPreferredTransport*  pTransport = NULL;
    LISTPOSITION	    lPos = NULL;

    if (!m_pPrefHostTransportList)
    {
	goto cleanup;
    }

    lPos = m_pPrefHostTransportList->GetHeadPosition();
    while (lPos && m_pPrefHostTransportList->GetCount())
    {	    
	pTransport = (HXPreferredTransport*)m_pPrefHostTransportList->GetAt(lPos);
	
	if (pTransport == pPrefTransport)
	{
	    HX_RELEASE(pTransport);
	    lPos = m_pPrefHostTransportList->RemoveAt(lPos);
	}
	else
	{
	    m_pPrefHostTransportList->GetNext(lPos);
	}
    }

cleanup:

    return rc;
}

/************************************************************************
 *	Method:
 *	    IHXPreferredTransportManager::GetTransportPreference
 *	Purpose:
 *	    Get transport preference set by the user
 */
STDMETHODIMP
HXPreferredTransportManager::GetTransportPreference(PreferredTransportProtocol	/* IN  */ prefTransportProtocol,
						     REF(UINT32)		/* OUT */ ulPreferenceMask)
{
    HX_RESULT	rc = HXR_OK;

    if (PTP_RTSP == prefTransportProtocol)
    {
	ulPreferenceMask = m_ulRTSPTransportMask;
    }
    else if (PTP_PNM == prefTransportProtocol)
    {
	ulPreferenceMask = m_ulPNMTransportMask;
    }
    else
    {
	rc = HXR_FAILED;
    }

    return rc;
}

STDMETHODIMP
HXPreferredTransportManager::SetAutoBWDetectionValue(UINT32 ulBW)
{
    m_ulMasterABD = ulBW;
    return HXR_OK;
}

STDMETHODIMP
HXPreferredTransportManager::GetAutoBWDetectionValue(REF(UINT32) ulBW)
{
    ulBW = m_ulMasterABD;
    return (ulBW)?HXR_OK:HXR_FAILED;
}

/*
 *	IHXNetInterfacesAdviseSink methods
 */
STDMETHODIMP
HXPreferredTransportManager::NetInterfacesUpdated()
{

    HX_RESULT	rc = HXR_OK;

    if (m_bSave)
    {
	SavePrefTransport();
    }

    ResetPrefTransport(m_pPrevPrefHostTransportList);
    m_pPrevPrefHostTransportList = m_pPrefHostTransportList;
    m_pPrefHostTransportList = NULL;

    HX_VECTOR_DELETE(m_pszFile);

    m_bInitialized = FALSE;

    return rc;
}

HX_RESULT	    
HXPreferredTransportManager::CollectNetworkInfo(void)
{
    HX_RESULT	rc = HXR_FAILED;
    UINT16	i = 0;
    UINT32	uNI = 0;
    NIInfo*	pTempNIInfo = NULL;
    NIInfo*	pNIInfo = NULL;

    m_bDisableUDP = FALSE;

    if (!m_pHXNetInterface)
    {
	if (HXR_OK != m_pContext->QueryInterface(IID_IHXNetInterfaces, (void**)&m_pHXNetInterface))
	{
	    goto cleanup;
	}
	m_pHXNetInterface->AddAdviseSink((IHXNetInterfacesAdviseSink*)this);
    }

    if (m_pHXNetInterface)
    {
	uNI = m_pHXNetInterface->GetNumOfNetInterfaces();

	for (i = 0; i < uNI; i++)
	{
	    if (HXR_OK == m_pHXNetInterface->GetNetInterfaces(i, pTempNIInfo))
	    {
		if (pTempNIInfo && 
                    (NI_OPER_STATUS_OPERATIONAL == pTempNIInfo->status) &&                    
                    (NI_LOOPBACK != pTempNIInfo->type))
		{		    
                    if (pTempNIInfo->pAddressInfo)
                    {
                        // PPP precedes the rest of networks
		        if (NI_PPP == pTempNIInfo->type)
		        {
                            pNIInfo = pTempNIInfo;
		        }
                        // choose the 1st non-PPP net interface
                        else if (!pNIInfo)
                        {
                            pNIInfo = pTempNIInfo;
                        }
                    }

                    if (pTempNIInfo->pDescription)
                    {
                        // disable UDP when Cisco's VPN is in use
                        // there is fragmentation issue with UDP in Cisco's VPN software
                        // Case F034774 filed with Cisco
                        if (strstr((const char*)pTempNIInfo->pDescription->GetBuffer(), "Cisco Systems VPN"))
                        {
                            m_bDisableUDP = TRUE;
                        }
                    }
		}
	    }
	}

	if (pNIInfo && pNIInfo->pAddressInfo)
	{
            m_ulLocalHost = DwToHost((pNIInfo->pAddressInfo->pAddress) ? HXinet_addr((const char*)pNIInfo->pAddressInfo->pAddress->GetBuffer()) : 0);
            m_ulSubnet = DwToHost((pNIInfo->pAddressInfo->pSubnet) ? HXinet_addr((const char*)pNIInfo->pAddressInfo->pSubnet->GetBuffer()) : 0);
                        
            if (pNIInfo->pAddressInfo->ulSubnetPrefix)
            {
                m_ulSubnetMask = (m_ulSubnetMask >> (32 - pNIInfo->pAddressInfo->ulSubnetPrefix));
                m_ulSubnetMask = (m_ulSubnetMask << (32 - pNIInfo->pAddressInfo->ulSubnetPrefix));
            }

	    rc = HXR_OK;
	}
    }

#ifndef _WINCE
    HX_ASSERT(m_ulSubnetMask);
#endif

cleanup:

    return rc;
}

HX_RESULT	    
HXPreferredTransportManager::PrepPrefTransport(void)
{
    HX_RESULT   hr = HXR_OK;
#ifndef _VXWORKS
    char        buffer[MAX_DISPLAY_NAME] = {0}; /* Flawfinder: ignore */
    const char* pszValue = NULL;
    IHXBuffer*  pBuffer = NULL;
    IHXBuffer*  pPrefBuffer = NULL;
    CHXString   strUserDataPath;

    if (!m_pPreferences &&  
        HXR_OK != m_pContext->QueryInterface(IID_IHXPreferences, (void**)&m_pPreferences))
    {
        hr = HXR_FAILED;
        goto cleanup;
    }

    if (HXR_OK == m_pPreferences->ReadPref("UserSDKDataPath", pBuffer) && pBuffer)
    {
        pszValue = (char*) pBuffer->GetBuffer();
    }

    if (!pszValue)
    {
        // construct base path
#ifdef _UNIX
        strUserDataPath = (const char*) getenv("HOME");
        strUserDataPath += OS_SEPARATOR_CHAR;
        strUserDataPath += ".helix";
        strUserDataPath += OS_SEPARATOR_CHAR;
        pszValue = (const char*) strUserDataPath;
#else
        pszValue = GetDLLAccessPath()->GetPath(DLLTYPE_COMMON);
#endif        
    }

    // determine whether preferred transport setting of this network configuration
    // has been saved
#ifdef _MAC_MACHO
	sprintf(buffer, "networkconfig_%lX.txt", m_ulSubnet);
#else
    i64toa(m_ulSubnet, buffer, 16);
    strcat(buffer, ".txt"); /* Flawfinder: ignore */
#endif
		   
    m_pszFile = new char[strlen(pszValue) + strlen(buffer) + 10];
    ::strcpy(m_pszFile, pszValue); /* Flawfinder: ignore */
    if (m_pszFile[::strlen(m_pszFile)-1] != OS_SEPARATOR_CHAR)
    {
        strcat(m_pszFile, OS_SEPARATOR_STRING); /* Flawfinder: ignore */
    }

    strcat(m_pszFile, buffer); /* Flawfinder: ignore */

#if !defined (__TCS__)
    // we maintain max # of preferred transport config files with
    // the least used file deleted
    m_pPreferences->ReadPref("PreferredTransportFiles", pPrefBuffer);

    ::AddFileToFileListWithCap(buffer, 0, pszValue, pPrefBuffer, m_pContext);
    m_pPreferences->WritePref("PreferredTransportFiles", pPrefBuffer);
#endif /* __TCS__ */

    HX_RELEASE(pPrefBuffer);
    HX_RELEASE(pBuffer);

#endif /* _VXWORKS */

  cleanup:

    return hr;
}

void
HXPreferredTransportManager::ResetPrefTransport(CHXSimpleList* pPrefHostTransportList)
{
    HXPreferredTransport*   pTransport = NULL;

    while (pPrefHostTransportList && pPrefHostTransportList->GetCount() > 0)
    {
	pTransport = (HXPreferredTransport*)pPrefHostTransportList->RemoveHead();
	pTransport->Close();
	HX_RELEASE(pTransport);
    }
}

HX_RESULT	
HXPreferredTransportManager::FileReadLine(FILE* fp, char* pLine, UINT32 ulLineBuf, UINT32* pBytesRead)
{
    HX_RESULT	hr = HXR_OK;
    UINT32	i = 0;
    UINT32	ulBytesRead = 0;
    char*	pTmpBuf = NULL;

    if (!fp)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

    if (ulLineBuf < 1) 
    {
        *pBytesRead = 0;
        goto cleanup;
    }

    ulBytesRead = fread(pLine, sizeof(char), ulLineBuf, fp);
    pTmpBuf = pLine;

    if (ulBytesRead)
    {
	while (i < ulBytesRead) 
	{
#ifdef _MACINTOSH
	    if (pTmpBuf[i] == 10 || pTmpBuf[i] == 13)
#else
	    if (pTmpBuf[i] == 10)
#endif
	    {   // LF
		if (pTmpBuf[i+1])
		{
		    pTmpBuf[i+1] = '\0';
		}

		// Back the file pointer up.
		fseek(fp, (long)((i + 1) - ulBytesRead), SEEK_CUR);
		*pBytesRead = i + 1;
		break;
	    }
	    i++;
	}
    }
    else
    {
	hr = HXR_FAILED;
    }
    
cleanup:

    return hr;
}

HX_RESULT
HXPreferredTransportManager::FileWriteLine(FILE* fp, HXPreferredTransport* pPrefTransport)
{
    HX_RESULT	rc = HXR_OK;
    char        buffer[36] = {0}; /* Flawfinder: ignore */

    // Theline format is the following fields, in this order, seperated by tabs
    // (\t):
    //
    //  hostname     - string(null by default)
    //  class        - transport class(PTC_Internal by default)
    //  protocol     - protocol type(RTSP vs PNM)
    //  transport    - preferred transport(UDPMode by default)
    //  cloakport    - cloak port
    //  expires      - Experation time.
    //  HTTPNG       - HTTP Next Generation(???)
    //  playbacks    - # of playbacks
    //  ABD          - AutoBWDetection value for this host

    if (pPrefTransport->m_pHost)
    {	    
	fwrite( (const char*)*(pPrefTransport->m_pHost),
                sizeof(char),
                pPrefTransport->m_pHost->GetLength(),
                fp);
    }
    else
    {
        HX_ASSERT(FALSE);
    }
    fwrite("\t", sizeof(char), 1, fp);

    itoa(pPrefTransport->m_prefTransportClass, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);	
    fwrite("\t", sizeof(char), 1, fp);

    itoa(pPrefTransport->m_prefTransportProtocol, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);	
    fwrite("\t", sizeof(char), 1, fp);

    itoa(pPrefTransport->m_prefTransportType, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    itoa(pPrefTransport->m_uCloakPort, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    snprintf(buffer, 36, "%lu", pPrefTransport->m_lastUsedTime); /* Flawfinder: ignore */
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    itoa(pPrefTransport->m_bHTTPNG, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    itoa(pPrefTransport->m_uPlaybacks, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    snprintf(buffer, 10, "%lu", pPrefTransport->m_ulABD); /* Flawfinder: ignore */
    fwrite(buffer, sizeof(char), strlen(buffer), fp);

    fwrite(LINEBREAK, sizeof(char), LINEBREAK_LEN, fp);

    return rc;
}

HX_RESULT
HXPreferredTransportManager::FileWriteClass(FILE* fp, 
					     PreferredTransportClass prefTransportClass, 
					     TransportMode transportType,
					     PreferredTransportProtocol	protocol,
					     UINT32 ulTransportMask,
					     time_t lastModifiedTime,
                                             UINT32 ulABD)
{
    HX_RESULT	rc = HXR_OK;
    char        buffer[36] = {0}; /* Flawfinder: ignore */

    // Theline format is the following fields, in this order, seperated by tabs
    // (\t):
    //
    //  hostname     - string(null by default)
    //  class        - transport class(PTC_Internal by default)
    //  protocol     - protocol type(RTSP vs PNM)
    //  transport    - preferred transport(UDPMode by default)
    //  cloakport    - cloak port
    //  expires      - Experation time.
    //  HTTPNG       - HTTP Next Generation(???)
    //  playbacks    - # of playbacks
    //  ABD          - AutoBWDetection value for this host

    fwrite("localhost", sizeof(char), 9, fp);
    fwrite("\t", sizeof(char), 1, fp);

    itoa(prefTransportClass, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);	
    fwrite("\t", sizeof(char), 1, fp);

    itoa(transportType, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    itoa(protocol, buffer, 10);
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    snprintf(buffer,36, "%lu", ulTransportMask); /* Flawfinder: ignore */
    fwrite(buffer, sizeof(char), strlen(buffer), fp);
    fwrite("\t", sizeof(char), 1, fp);

    snprintf(buffer,36, "%lu", lastModifiedTime); /* Flawfinder: ignore */
    fwrite(buffer, sizeof(char), strlen(buffer), fp);

    snprintf(buffer,36, "%lu", ulABD); /* Flawfinder: ignore */
    fwrite(buffer, sizeof(char), strlen(buffer), fp);

    fwrite(LINEBREAK, sizeof(char), LINEBREAK_LEN, fp);

    return rc;
}

HX_RESULT
HXPreferredTransportManager::SwitchTransport(HX_RESULT error, 
					     HXPreferredTransport* pPrefTransport,
					     REF(TransportMode) prefTransportType)
{
    HX_RESULT   rc = HXR_OK;
    UINT8       pos = 0;
    UINT8&      uTransportAttempted = pPrefTransport->m_uTransportAttempted;
    INT8        tempTransport = (INT8)prefTransportType;
    INT8        outTransport = (INT8)UnknownMode;
    UINT32	ulTransportMask = ATTEMPT_AUTOTRANSPORT;

    // we have attempted all the transports available
    if (0xF == uTransportAttempted)
    {
        goto cleanup;
    }

    if (PTP_RTSP == pPrefTransport->m_prefTransportProtocol)
    {
	ulTransportMask = m_ulRTSPTransportMask;
    }
    else if (PTP_PNM == pPrefTransport->m_prefTransportProtocol)
    {
	ulTransportMask = m_ulPNMTransportMask;
    }
    else
    {
	HX_ASSERT(FALSE);
    }

    // special case
    // switch to HTTPCloaking(or vise versa) directly if the following list of network
    // errors have been occured
    if (error == HXR_NET_CONNECT	    ||
	error == HXR_DNR		    ||
	error == HXR_SERVER_DISCONNECTED    ||
	error == HXR_DOC_MISSING	    ||
	error == HXR_BAD_SERVER		    ||
	error == HXR_PROXY_NET_CONNECT)
    {
        if (prefTransportType != HTTPCloakMode)
        {   
            if (ulTransportMask & ATTEMPT_HTTPCLOAK)
            {
	        outTransport = HTTPCloakMode;
            }
        }
        else
        {
            if (ulTransportMask & ATTEMPT_MULTICAST)
            {
                outTransport = MulticastMode;
            }
            else if (ulTransportMask & ATTEMPT_UDP)
            {
                outTransport = UDPMode;
            }
            else if (ulTransportMask & ATTEMPT_TCP)
            {
                outTransport = TCPMode;
            }
        }
        uTransportAttempted = 0xF;
    }
    else
    {
        // find the next avaialble transport which hasn't been
        // attempted yet
        while (0xF != uTransportAttempted)
        {
            pos = (++tempTransport) % MAX_TRANSPORT_MODE;

            uTransportAttempted |= (1 << pos);

            if ((ulTransportMask >> pos) & 1)
            {
                // multicast and UDP are always attempted
                // at the same time if both are enabled
                // so we'll only attempt either of them
                if ((ulTransportMask & 1) && ((ulTransportMask >> 1) & 1))
                {
                    if ((MulticastMode == (TransportMode)pos &&
                         ((uTransportAttempted >> 1) & 1)) ||
                        (UDPMode == (TransportMode)pos &&
                         (uTransportAttempted & 1)))
                    {
                        continue;
                    }
                }
                 
                outTransport = pos;
                break;
            }
        }
    }

cleanup:

    HX_ASSERT((INT8)prefTransportType != outTransport);

    prefTransportType = (TransportMode)outTransport;

    return rc;
}

HX_RESULT
HXPreferredTransportManager::UpShiftTransport(HXPreferredTransport* pPrefTransport,
					       REF(TransportMode) prefTransportType)
{
    HX_RESULT	rc = HXR_OK;
    UINT32	ulTransportMask = ATTEMPT_AUTOTRANSPORT;

    HX_ASSERT(UnknownMode != prefTransportType);

#if defined(HELIX_FEATURE_TCP_OVER_UDP)
    // TCPMode is the toppest on the transport switching list
    if (TCPMode == prefTransportType)
    {
        goto cleanup;
    }
#else
    // MulticastMode/UDPMode is the toppest on the transport switching list
    if (MulticastMode == prefTransportType ||
	UDPMode == prefTransportType)
    {
	goto cleanup;
    }
#endif /* HELIX_FEATURE_TCP_OVER_UDP */

    if (PTP_RTSP == pPrefTransport->m_prefTransportProtocol)
    {
	ulTransportMask = m_ulRTSPTransportMask;
    }
    else if (PTP_PNM == pPrefTransport->m_prefTransportProtocol)
    {
	ulTransportMask = m_ulPNMTransportMask;
    }
    else
    {
	HX_ASSERT(FALSE);
    }

    if (prefTransportType == HTTPCloakMode)
    {
        if (ulTransportMask & ATTEMPT_TCP)
        {
	    prefTransportType = TCPMode;
	    goto cleanup;
	}
	// this is intentional...since we always try UDP
	// when we try Multicast.
	else if (ulTransportMask & ATTEMPT_MULTICAST)
	{
	    prefTransportType = MulticastMode;
	}
	else if (ulTransportMask & ATTEMPT_UDP)
	{
	    prefTransportType = UDPMode;
	}
    }
    else if (prefTransportType == TCPMode)
    {
	// this is intentional...since we always try UDP
	// when we try Multicast.
	if (ulTransportMask & ATTEMPT_MULTICAST)
	{
	    prefTransportType = MulticastMode;
	}
	else if (ulTransportMask & ATTEMPT_UDP)
	{
	    prefTransportType = UDPMode;
	}
    }

cleanup:

    return rc;
}

HXBOOL
HXPreferredTransportManager::ValidateTransport(HXPreferredTransport* pPrefTransport,
						TransportMode prefTransportType)
{
    HXBOOL    bResult = TRUE;
    UINT32  ulTransportMask = ATTEMPT_AUTOTRANSPORT;

    if (PTP_RTSP == pPrefTransport->m_prefTransportProtocol)
    {
	ulTransportMask = m_ulRTSPTransportMask;
    }
    else if (PTP_PNM == pPrefTransport->m_prefTransportProtocol)
    {
	ulTransportMask = m_ulPNMTransportMask;
    }
    else
    {
	HX_ASSERT(FALSE);
    }

    switch (prefTransportType)
    {
    case MulticastMode:
	bResult = (ulTransportMask & ATTEMPT_MULTICAST);
	break;
    case UDPMode:
	bResult = (ulTransportMask & ATTEMPT_UDP);
	break;
    case TCPMode:
	bResult = (ulTransportMask & ATTEMPT_TCP);
	break;
    case HTTPCloakMode:
	bResult = (ulTransportMask & ATTEMPT_HTTPCLOAK);
	break;
    default:
	bResult = FALSE;
	break;
    }

    return bResult;
}

CHXString*
HXPreferredTransportManager::GetMasterDomain(const char* pszHostName)
{
    int		nFields = 0;
    CHXString*	pOutString = NULL;
    CHXString	inString;
    CHXString	outString;
    CHXString	domainExt;

    inString = pszHostName;
    
    nFields = inString.CountFields('.');

    // a valid master domain contains either one or two fields
    // such as "dingdong", "realguide.com" and "rbn.com"
    if (nFields <= 2)
    {
	pOutString = new CHXString(inString);
    }
    else
    {
	domainExt = inString.NthField('.', nFields);

	// distinguish between domestic domains and international domains
	if (!domainExt.CompareNoCase("com")	||
	    !domainExt.CompareNoCase("net")	||
	    !domainExt.CompareNoCase("org")	||
	    !domainExt.CompareNoCase("edu")	||
	    !domainExt.CompareNoCase("gov")	||
	    !domainExt.CompareNoCase("mil"))
	{
	    // save the last 2 nodes for domestic domains
	    outString = inString.NthField('.', nFields - 1);
	    outString += ".";
	    outString += inString.NthField('.', nFields);
	}
	else
	{
	    // save the last 3 nods for international domains
	    outString = inString.NthField('.', nFields - 2);
	    outString += ".";
	    outString += inString.NthField('.', nFields - 1);
	    outString += ".";
	    outString += inString.NthField('.', nFields);
	}
	
	pOutString = new CHXString(outString);
    }

    return pOutString;
}

TransportMode
HXPreferredTransportManager::GetTransportPreferred(HXPreferredTransport* pPrefTransport)
{
    TransportMode   transportMode = UnknownMode;
 
    if (PTC_INTERNAL == pPrefTransport->m_prefTransportClass)
    {
	if (PTP_RTSP == pPrefTransport->m_prefTransportProtocol)
	{
	    transportMode = GetHigherTransport(m_rtspTransportTypeStartWith,
					       m_internalTransportType);
	}
	else if (PTP_PNM == pPrefTransport->m_prefTransportProtocol)
	{
	    transportMode = GetHigherTransport(m_pnmTransportTypeStartWith,
					       m_internalTransportType);
	}
	else
	{
	    HX_ASSERT(FALSE);
	}
    }
    else if (PTC_EXTERNAL == pPrefTransport->m_prefTransportClass)
    {
	if (PTP_RTSP == pPrefTransport->m_prefTransportProtocol)
	{
	    transportMode = m_rtspTransportTypeStartWith; 
//			    GetHigherTransport(m_rtspTransportTypeStartWith,
//					       m_externalTransportType);
	}
	else if (PTP_PNM == pPrefTransport->m_prefTransportProtocol)
	{
	    transportMode = m_pnmTransportTypeStartWith;
//			    GetHigherTransport(m_pnmTransportTypeStartWith,
//					       m_externalTransportType);
	}
	else
	{
	    HX_ASSERT(FALSE);
	}
    }
 
    return transportMode;
}

TransportMode
HXPreferredTransportManager::GetHigherTransport(TransportMode mode1, TransportMode mode2)
{
    if (mode1 < mode2)
    {
	return mode2;
    }
    else
    {
	return mode1;
    }    
}
    
TransportMode
HXPreferredTransportManager::GetLowerTransport(TransportMode mode1, TransportMode mode2)
{
    if (mode1 > mode2)
    {
	return mode2;
    }
    else
    {
	return mode1;
    }    
}

PreferredTransportClass
HXPreferredTransportManager::GetTransportClass(const char* pszHostName, UINT32 ulHostAddress)
{
    PreferredTransportClass	prefTransportClass = PTC_EXTERNAL;

    // Internal server is defined as:
    // a. a media server on the same network subnet as the client, or
    // b. a media server on a network subnet that meets the "Peer Subnet" criteria 
    //    (either as an explicit entry or by meeting wildcard criteria), or
    // c. a media server that meets the "Proxy Exclude" criteria 
    //    (either as an explicit entry or by meeting wildcard criteria)
    if ((m_ulSubnetMask && m_ulSubnet && ((ulHostAddress & m_ulSubnetMask) == m_ulSubnet))  ||
	m_pSubnetManager->IsSubnet(pszHostName)						    
#if defined(HELIX_FEATURE_PROXYMGR)
	|| m_pProxyManager->IsExemptionHost((char*)pszHostName)
#endif /* HELIX_FEATURE_PROXYMGR */
	)
    {
	prefTransportClass = PTC_INTERNAL;
    }

    return prefTransportClass;
}

void
HXPreferredTransportManager::InitTransportTypeStartWith(UINT32		ulTransportMask, 
							 TransportMode&	transportStartWith)
{
    transportStartWith = UnknownMode;

    if (ulTransportMask & ATTEMPT_MULTICAST)
    {
	transportStartWith = MulticastMode;
    }
    else if (ulTransportMask & ATTEMPT_UDP)
    {
	transportStartWith = UDPMode;
    }
    else if (ulTransportMask & ATTEMPT_TCP)
    {
	transportStartWith = TCPMode;
    }
    else if (ulTransportMask & ATTEMPT_HTTPCLOAK)
    {
	transportStartWith = HTTPCloakMode;
    }
    else
    {
	HX_ASSERT(FALSE);
    }

    return;
}

HX_RESULT
HXPreferredTransportManager::OpenPrefTransport(void)
{
    HX_RESULT	    hr = HXR_OK;
    char*	    pszHostName = NULL;
    char*	    pszClass = NULL;
    char*	    pszProtocol = NULL;
    char*	    pszTransport = NULL; 
    char*	    pszTransportMask = NULL;
    char*	    pszCloakPort = NULL;
    char*	    pszExpires = NULL;
    char*	    pszHTTPNG = NULL;
    char*	    pszPlaybacks = NULL;
    char*           pszABD = NULL;
    char*	    pszUnknownField = NULL;
    char	    buffer[LINE_BUFFER_SIZE] = {0}; /* Flawfinder: ignore */
    UINT32	    ulBytesRead = 0;
    FILE*	    fp = NULL;
    HXPreferredTransport* pTransport = NULL;
    PreferredTransportClass transportClass = PTC_UNKNOWN;
 
    if (!m_pszFile)
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

#ifdef _WINDOWS
    if (!m_pLock)
    {
	CreateEventCCF((void**)&m_pLock, m_pContext, RM_PREFTRAN_FILE_LOCK, FALSE);
    }
    else
    {
	m_pLock->Wait(ALLFS);
    }
#endif /* _WINDOWS */

    if (!(fp = fopen(m_pszFile, "r+b")))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

#if defined (_UNIX) && !defined(_SUN) && !defined(_SCO_UW) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    m_fileID = fileno(fp);    
    flock(m_fileID, LOCK_EX);
#endif /* _UNIX */

     /* line format is:
     *
     * hostname \t class \t protocol \t transport \t cloakport \t time
     *
     * hostname	    - string(null by default)
     * class	    - transport class(PTC_Internal by default)
     * protocol	    - protocol type(RTSP vs PNM)
     * transport    - preferred transport(UDPMode by default)
     * cloakport    - cloak port
     * time	    - time of last used or modified
     * ABD          - AutoBWDetection
     */
    while (HXR_OK == FileReadLine(fp, &buffer[0], LINE_BUFFER_SIZE, &ulBytesRead))
    {
	if (*buffer == '#' || *buffer == CR || *buffer == LF || *buffer == 0)
	{
	    continue;
	}

	pszHostName = buffer;
	    
	if(!(pszClass = strchr(pszHostName, '\t')))
	{
	    continue;
	}
	*pszClass++ = '\0';
	if(*pszClass == CR || *pszClass == LF || *pszClass == 0)
	{
	    continue;
	}

	if (0 == strcasecmp(pszHostName, "localhost"))
	{
	    transportClass = (PreferredTransportClass)atoi(pszClass);

	    if(!(pszTransport = strchr(pszClass, '\t')))
	    {
		continue;
	    }
	    *pszTransport++ = '\0';
	    if(*pszTransport == CR || *pszTransport == LF || *pszTransport == 0)
	    {
		continue;
	    }

	    if(!(pszProtocol = strchr(pszTransport, '\t')))
	    {
		continue;
	    }
	    *pszProtocol++ = '\0';
	    if(*pszProtocol == CR || *pszProtocol == LF || *pszProtocol == 0)
	    {
		continue;
	    }

	    if(!(pszTransportMask = strchr(pszProtocol, '\t')))
	    {
		continue;
	    }
	    *pszTransportMask++ = '\0';
	    if(*pszTransportMask == CR || *pszTransportMask == LF || *pszTransportMask == 0)
	    {
		continue;
	    }

	    if(!(pszExpires = strchr(pszTransportMask, '\t')))
	    {
		continue;
	    }
	    *pszExpires++ = '\0';
	    if(*pszExpires == CR || *pszExpires == LF || *pszExpires == 0)
	    {
		continue;
	    }

	    if(!(pszABD = strchr(pszExpires, '\t')))
	    {
		continue;
	    }
	    *pszABD++ = '\0';
	    if(*pszABD == CR || *pszABD == LF || *pszABD == 0)
	    {
		continue;
	    }

	    if (!(pszUnknownField = strchr(pszABD, '\t')))
	    {
		// remove the '\n' from the end of the entry
		pszABD = ::StripLine(pszABD);
	    }
	    else
	    {
		*pszUnknownField++ = '\0';
	    }

	    if (PTC_INTERNAL == transportClass)
	    {
		m_internalTransportType = (TransportMode)atoi(pszTransport);
#ifdef _MACINTOSH
		m_ulRTSPTransportMask = atoi64(pszTransportMask);
		m_lastRTSPPreferencesModifiedTime = (time_t)atoi64(pszExpires);
#else
		m_ulRTSPTransportMask = atol(pszTransportMask);
		m_lastRTSPPreferencesModifiedTime = atol(pszExpires);
#endif
                m_ulMasterABD = (pszABD)?atol(pszABD):0;
	    }
	    else if (PTC_EXTERNAL == transportClass)
	    {
		m_externalTransportType = (TransportMode)atoi(pszTransport);
#ifdef _MACINTOSH
		m_ulPNMTransportMask = atoi64(pszTransportMask);
		m_lastPNMPreferencesModifiedTime = (time_t)atoi64(pszExpires);
#else
		m_ulPNMTransportMask = atol(pszTransportMask);
		m_lastPNMPreferencesModifiedTime = atol(pszExpires);
#endif
                m_ulMasterABD = (pszABD)?atol(pszABD):0;
	    }
	    else
	    {
		HX_ASSERT(FALSE);
	    }
	}
	else
	{
	    if(!(pszProtocol = strchr(pszClass, '\t')))
	    {
		continue;
	    }
	    *pszProtocol++ = '\0';
	    if(*pszProtocol == CR || *pszProtocol == LF || *pszProtocol == 0)
	    {
		continue;
	    }

	    if(!(pszTransport = strchr(pszProtocol, '\t')))
	    {
		continue;
	    }
	    *pszTransport++ = '\0';
	    if(*pszTransport == CR || *pszTransport == LF || *pszTransport == 0)
	    {
		continue;
	    }

	    if(!(pszCloakPort = strchr(pszTransport, '\t')))
	    {
		continue;
	    }
	    *pszCloakPort++ = '\0';
	    if(*pszCloakPort == CR || *pszCloakPort == LF || *pszCloakPort == 0)
	    {
		continue;
	    }

	    if(!(pszExpires = strchr(pszCloakPort, '\t')))
	    {
		continue;
	    }
	    *pszExpires++ = '\0';
	    if(*pszExpires == CR || *pszExpires == LF || *pszExpires == 0)
	    {
		continue;
	    }

	    if(!(pszHTTPNG = strchr(pszExpires, '\t')))
	    {
		continue;
	    }
	    *pszHTTPNG++ = '\0';
	    if(*pszHTTPNG == CR || *pszHTTPNG == LF || *pszHTTPNG == 0)
	    {
		continue;
	    }

	    if (!(pszPlaybacks = strchr(pszHTTPNG, '\t')))
	    {
		continue;
	    }
	    *pszPlaybacks++ = '\0';
	    if(*pszPlaybacks == CR || *pszPlaybacks == LF || *pszPlaybacks == 0)
	    {
		continue;
	    }

	    if (!(pszABD = strchr(pszPlaybacks, '\t')))
	    {
		continue;
	    }
	    *pszABD++ = '\0';
	    if(*pszABD == CR || *pszABD == LF || *pszABD == 0)
	    {
		continue;
	    }
            
	    if (!(pszUnknownField = strchr(pszABD, '\t')))
	    {
		// remove the '\n' from the end of the entry
		pszABD = ::StripLine(pszABD);
	    }
	    else
	    {
		*pszUnknownField++ = '\0';
	    }

	    // construct a new preferred transport struct
	    pTransport = new HXPreferredTransport(this);
	    pTransport->AddRef();

	    pTransport->m_pHost = new CHXString(pszHostName);
	    pTransport->m_prefTransportClass = (PreferredTransportClass)atoi(pszClass);
	    pTransport->m_prefTransportProtocol = (PreferredTransportProtocol)atoi(pszProtocol);
	    pTransport->m_prefTransportType = (TransportMode)atoi(pszTransport);
	    pTransport->m_uCloakPort = atoi(pszCloakPort);
#ifdef _MACINTOSH
	    pTransport->m_lastUsedTime = (time_t)atoi64(pszExpires);
#else
	    pTransport->m_lastUsedTime = atol(pszExpires);
#endif
            pTransport->m_ulABD = atol(pszABD);
	    pTransport->m_bHTTPNG = atoi(pszHTTPNG);
	    pTransport->m_uPlaybacks = atoi(pszPlaybacks);
	    pTransport->m_state = PTS_READY;

	    if (!m_pPrefHostTransportList)
	    {
		m_pPrefHostTransportList = new CHXSimpleList();
	    }

	    m_pPrefHostTransportList->AddTail(pTransport);
	}
    }

cleanup:

#if defined (_UNIX) && !defined(_SUN) && !defined(_SCO_UW) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    flock(m_fileID, LOCK_UN);
#endif /* _UNIX */

    if (fp)
    {
	fclose(fp);
    }

#ifdef _WINDOWS
    if (m_pLock)
    {
	m_pLock->SignalEvent();
    }
#endif /* _WINDOWS */

    return hr;
}

HX_RESULT	
HXPreferredTransportManager::SavePrefTransport(void)
{
    HX_RESULT		    hr = HXR_OK;
    FILE*		    fp = NULL;
    HXPreferredTransport*  pTransport = NULL;
    CHXSimpleList::Iterator  i;

    if (!m_pszFile)
    {
	goto cleanup;
    }

#ifdef _WINDOWS
    if (!m_pLock)
    {
	CreateEventCCF((void**)&m_pLock, m_pContext, RM_PREFTRAN_FILE_LOCK, FALSE);
    }
    else
    {
	m_pLock->Wait(ALLFS);
    }
#endif /* _WINDOWS */

    if (!(fp = fopen(m_pszFile, "w")))
    {
	hr = HXR_FAILED;
	goto cleanup;
    }

#ifdef _UNIX
    //Make the permisions on the cookies file User read/write only.
    if( chmod( m_pszFile, S_IRUSR | S_IWUSR ) != 0 )
    {
        HX_ASSERT( "Can't change permision on cookies file." == NULL );
    }
    
#endif    

#if defined (_UNIX) && !defined(_SUN) && !defined(_SCO_UW) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    m_fileID = fileno(fp);    
    flock(m_fileID, LOCK_EX);
#endif /* _UNIX */

    fwrite(RM_PREFTRAN_CAPTION, sizeof(char), strlen(RM_PREFTRAN_CAPTION), fp);

    // save preferred class information first
    FileWriteClass(fp, PTC_INTERNAL, m_internalTransportType, PTP_RTSP, m_ulRTSPTransportMask, m_lastRTSPPreferencesModifiedTime, m_ulMasterABD);
    FileWriteClass(fp, PTC_EXTERNAL, m_externalTransportType, PTP_PNM, m_ulPNMTransportMask, m_lastPNMPreferencesModifiedTime, m_ulMasterABD);

    // save preferred host information
    if (m_pPrefHostTransportList)
    {
	for (i = m_pPrefHostTransportList->Begin(); i != m_pPrefHostTransportList->End(); ++i)
	{
	    pTransport = (HXPreferredTransport*)(*i);
	    if (pTransport->m_pHost && PTS_READY == pTransport->m_state)
	    {
            FileWriteLine(fp, pTransport);
	    }
	}
    }

    m_bSave = FALSE;
    
cleanup:

#if defined (_UNIX) && !defined(_SUN) && !defined(_SCO_UW) && !defined(_HPUX) && !defined(_IRIX) && !defined(_AIX) && !defined(_OSF1)
    flock(m_fileID, LOCK_UN);
#endif /* _UNIX */

    if (fp)
    {
	fclose(fp);
    }

#ifdef _WINDOWS
    if (m_pLock)
    {
	m_pLock->SignalEvent();
    }
#endif /* _WINDOWS */

    return(hr);
}

#if 0
void
HXPreferredTransportManager::myTestSuite()
{
    HX_RESULT   theError = HXR_OK;
    UINT32      ulTestCases = 14;
    UINT32      i = 0;  
    
    HXPreferredTransport* pTransport = new HXPreferredTransport(this);
    pTransport->AddRef();

    pTransport->m_pHost = NULL;
    pTransport->m_ulHost = 0;
    pTransport->m_prefTransportClass = PTC_INTERNAL;
    pTransport->m_prefTransportProtocol = PTP_RTSP; 	
    pTransport->m_state = PTS_CREATE;

    {FILE* f1 = ::fopen("c:\\preftran.txt", "a+"); ::fprintf(f1, "\n-1=Unknown, 0=Multicast, 1=UDP, 2=TCP, 3=HTTPCloak\n");::fclose(f1);}    
    {FILE* f1 = ::fopen("c:\\preftran.txt", "a+"); ::fprintf(f1, "\n1=AttemptMulticast, 2=AttemptUDP, 4=AttemptTCP, 8=AttemptHTTPCloak, 31=AutoTransport\n");::fclose(f1);}    

    for (i = 0; i < ulTestCases; i++)
    {
        theError = HXR_OK;

        switch (i)
        {
        case 0:
            m_ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
            m_rtspTransportTypeStartWith = MulticastMode;
            break;
        case 1:
            m_ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
            m_rtspTransportTypeStartWith = TCPMode;
            break;
        case 2:
            m_ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
            m_rtspTransportTypeStartWith = UDPMode;
            break;
        case 3:
            m_ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
            m_rtspTransportTypeStartWith = HTTPCloakMode;
            break;
        case 4:
            m_ulRTSPTransportMask = ATTEMPT_MULTICAST | ATTEMPT_UDP | ATTEMPT_TCP | ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = TCPMode;
            break;
        case 5:
            m_ulRTSPTransportMask = ATTEMPT_TCP | ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = TCPMode;
            break;
        case 6:
            m_ulRTSPTransportMask = ATTEMPT_MULTICAST | ATTEMPT_UDP | ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = UDPMode;
            break;
        case 7:
            m_ulRTSPTransportMask = ATTEMPT_MULTICAST | ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = MulticastMode;
            break;
        case 8:
            m_ulRTSPTransportMask = ATTEMPT_UDP | ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = HTTPCloakMode;
            break;
        case 9:
            m_ulRTSPTransportMask = ATTEMPT_UDP | ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = UnknownMode;
            break;
        case 10:
            m_ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
            m_rtspTransportTypeStartWith = UDPMode;
            theError = HXR_NET_CONNECT;
            break;
        case 11:
            m_ulRTSPTransportMask = ATTEMPT_AUTOTRANSPORT;
            m_rtspTransportTypeStartWith = HTTPCloakMode;
            theError = HXR_BAD_SERVER;
            break;
        case 12:
            m_ulRTSPTransportMask = ATTEMPT_UDP;
            m_rtspTransportTypeStartWith = UDPMode;
            theError = HXR_BAD_SERVER;
            break;
        case 13:
            m_ulRTSPTransportMask = ATTEMPT_HTTPCLOAK;
            m_rtspTransportTypeStartWith = HTTPCloakMode;
            theError = HXR_BAD_SERVER;
            break;
        default:
            break;
        }

        myTest(i, theError, pTransport);
    }

    HX_RELEASE(pTransport);

    return;
}

void
HXPreferredTransportManager::myTest(UINT32 ulTestCase, 
                                    HX_RESULT theError,
                                    HXPreferredTransport* pTransport)
{
    pTransport->m_prefTransportType = m_rtspTransportTypeStartWith;
    pTransport->m_uTransportAttempted = (1 << (INT8)m_rtspTransportTypeStartWith);

    {FILE* f1 = ::fopen("c:\\preftran.txt", "a+"); ::fprintf(f1, "\nTest %i: TransportMask=%lu\t TransportStartWith=%i Error=%lx\n", ulTestCase, m_ulRTSPTransportMask, m_rtspTransportTypeStartWith, theError);::fclose(f1);}    
    while (UnknownMode != pTransport->m_prefTransportType)
    {
        SwitchTransport(theError, pTransport, pTransport->m_prefTransportType);
        {FILE* f1 = ::fopen("c:\\preftran.txt", "a+"); ::fprintf(f1, "SwitchTransport returns %i\n", pTransport->m_prefTransportType);::fclose(f1);}
    }
}
#endif
