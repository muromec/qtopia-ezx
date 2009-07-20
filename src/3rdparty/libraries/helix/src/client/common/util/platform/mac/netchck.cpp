/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: netchck.cpp,v 1.7 2007/07/06 21:58:08 jfinnecy Exp $
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

#ifndef _MAC_MACHO
#include "OpenTransport.h"
#include "OpenTptInternet.h"
#endif
#include "prefdefs.h"
#include "netchck.h"
#include "hxtick.h"


#define DEF_HTTP_PORT 	    80
#define DEF_DNS_PORT 	    53 //tcp


enum {
	kOTTCPDialUnknown = 0,
	kOTTCPDialTCPDisabled,
	kOTTCPDialYes,
	kOTTCPDialNo
};

// From SCNetwork.h in SystemConfiguration.framework

typedef Boolean (*SCNetworkCheckReachabilityByNameProcPtr) ( const char *nodename,
                                                             int *flags );



CHXNetCheck::CHXNetCheck(UINT32 timeout) :
	XHXNetCheck(timeout)
	, m_pRmaNetServices(0)
	, m_pRmaTCPSocket(0)
	, m_pContext(0)
	, m_fConnected(FALSE)
	, m_fFailed(FALSE)
	, m_lRefCount(0)
	
{
}

CHXNetCheck::~CHXNetCheck()
{
	if (m_pContext)
		m_pContext->Release();
	m_pContext = NULL;

	if (m_pRmaNetServices)
		m_pRmaNetServices->Release();
	m_pRmaNetServices = NULL;

	if (m_pRmaTCPSocket)
		m_pRmaTCPSocket->Release();
	m_pRmaTCPSocket = NULL;
	
}




/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::QueryInterface
//  Purpose:
//	Implement this to export the interfaces supported by your 
//	object.
//
STDMETHODIMP CHXNetCheck::QueryInterface(REFIID riid, void** ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
	    AddRef();
	    *ppvObj = this;
	    return HXR_OK;
    }
    if (IsEqualIID(riid, IID_IHXTCPResponse))
    {
	    AddRef();
	    *ppvObj = (IHXTCPResponse*)this;
	    return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//  Method:
//	IUnknown::AddRef
//  Purpose:
//	Everyone usually implements this the same... feel free to use
//	this implementation.
//
STDMETHODIMP_(UINT32) CHXNetCheck::AddRef()
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
STDMETHODIMP_(UINT32) CHXNetCheck::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}





/*
 *	IHXTCPResponse methods
 */

/************************************************************************
 *	Method:
 *	    IHXTCPResponse::ConnectDone
 *	Purpose:
 *	    A Connect operation has been completed or an error has occurred.
 */
STDMETHODIMP CHXNetCheck::ConnectDone	(HX_RESULT		status)
{
	HX_ASSERT(m_fConnected == FALSE); // We shouldn't be getting called if 
									// we aren't expecting it.
	if (status == HXR_OK)
		m_fConnected = TRUE;
	else
		m_fFailed = TRUE;

	return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXTCPResponse::ReadDone
 *	Purpose:
 *	    A Read operation has been completed or an error has occurred.
 *	    The data is returned in the IHXBuffer.
 */
STDMETHODIMP CHXNetCheck::ReadDone		(HX_RESULT		status,
				IHXBuffer*		pBuffer)
{
HX_ASSERT(FALSE);
return HXR_OK;
}


/************************************************************************
 *	Method:
 *	    IHXTCPResponse::WriteReady
 *	Purpose:
 *	    This is the response method for WantWrite.
 *	    If HX_RESULT is ok, then the TCP channel is ok to Write to.
 */
STDMETHODIMP CHXNetCheck::WriteReady	(HX_RESULT		status)
{
HX_ASSERT(FALSE);
return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXTCPResponse::Closed
 *	Purpose:
 *	    This method is called to inform you that the TCP channel has
 *	    been closed by the peer or closed due to error.
 */
STDMETHODIMP CHXNetCheck::Closed(HX_RESULT		status)
{
m_fConnected = FALSE;
return HXR_OK;
}




//******************************************************************************
//
// Method:	CHXNetCheck::Init
//
// Purpose:	Sets up the CHXNetCheck object with a context.
//		
//
// Notes:	n/a
//
//******************************************************************************


HX_RESULT 
CHXNetCheck::Init(IUnknown *pContext)
{
	HX_RESULT result = HXR_OK;
	if (!pContext)
		return HXR_FAILED;

	m_pContext = pContext;
	m_pContext->AddRef();
	return result;
}



//******************************************************************************
//
// Method:	CHXNetCheck::FInternetAvailable
//
// Purpose:	Checks to see if the internet is available.  Returns true if it is, 
//			false otherwise.
//		
//
// Notes:	n/a
//
//******************************************************************************

HXBOOL 
CHXNetCheck::FInternetAvailable(HXBOOL fPing,HXBOOL fProxy)
{
    HXBOOL fRet = FALSE;
    HXBOOL fAutoDial = FALSE;
    UINT16	nPort = DEF_HTTP_PORT;

    ULONG32  willTCPDial;
    OSErr TCPError = HXMacNetWillDial(NULL, &willTCPDial);
    HX_ASSERT(noErr == TCPError);
    
    if (noErr == TCPError)
    {
	switch ( willTCPDial ) 
	{
	    case kOTTCPDialTCPDisabled:
	    // TCP/IP is disabled
	    break;
	    case kOTTCPDialUnknown:
	    // We don't know whether opening a TCP endpoint will dial the modem 
	    break;
	    case kOTTCPDialYes:
	    // Opening a TCP endpoint will dial the modem.
	    fAutoDial = TRUE;
	    break;
	    case kOTTCPDialNo:
	    // Opening a TCP endpoint will not dial the modem.
	    break;
    	}
    }
    else
    {
    	return FALSE; // no network conection or no PPP setup
    }
	    
    if (!fAutoDial)
    {
    	if (fPing)
	{
	    // try to get DNS address to ping.
	    CHXString strPingAddr;
	    GetDNSAddress(strPingAddr);
	    nPort = DEF_DNS_PORT;

	    // No DNS address? Default to a known web server.
	    // XXXBJP pinging video.real.com, used in Beta 1, should be
	    // changed for Beta 2!!
	    if (strPingAddr.IsEmpty())
	    {
		HX_ASSERT(FALSE);
		strPingAddr = "209.66.98.23";
		nPort = DEF_HTTP_PORT;
	    }

	    fRet = Ping(strPingAddr, nPort, FALSE);
	
	}
	else 
	    fRet = TRUE;
    }
	
    return (fRet);
}

//******************************************************************************
//
// Method:	CHXNetCheck::Ping
//
// Purpose:	Tests to see if we can open a TCP connection to the given hostname. 
//			if fAynchronous is true, we call back a response object provided by
//			the caller (through Init).  Otherwise we block.
//		
//
// Notes:	n/a
//
//******************************************************************************

HXBOOL CHXNetCheck::Ping(const char *szHostName, UINT16 nPort, HXBOOL fAsynchronous)
{
	ULONG32 ulStartTime, ulCurrentTime, ulInterval, ulElapsedTime;
	HXBOOL fRet = FALSE;
	
	// If we don't have the network services interface yet than try and get it here
	if (m_pContext && !m_pRmaNetServices)
	    m_pContext->QueryInterface(IID_IHXNetworkServices, (void**)&m_pRmaNetServices);

	if (!m_pRmaTCPSocket)
		m_pRmaNetServices->CreateTCPSocket(&m_pRmaTCPSocket);

	if (!m_pRmaTCPSocket)
		return FALSE;

	m_fFailed = m_fConnected = FALSE;
	m_pRmaTCPSocket->Init(this);
	
	m_pRmaTCPSocket->Connect(szHostName, nPort);

	ulElapsedTime = 0;

	ulInterval = 30000;
	
	// Get start time
	ulStartTime = HX_GET_TICKCOUNT();
	while (!m_fFailed && !m_fConnected && (ulElapsedTime < ulInterval))
	{
		SleepWell(1000);
	    ulCurrentTime = HX_GET_TICKCOUNT();
		ulElapsedTime = CALCULATE_ELAPSED_TICKS(ulStartTime, ulCurrentTime);
	}
	fRet = m_fConnected;

	m_pRmaTCPSocket->Release();
	m_pRmaTCPSocket = NULL;

	return (fRet);
}


//******************************************************************************
//
// Method:	CHXNetCheck::GetDNSAddress
//
// Purpose:	Determines the IP address of the user's primary DNS server.
//
// Notes:	Returns IP address in numeric form, in a CHXString. An empty
//		string is returned when the IP address cannot be determined.
//
//******************************************************************************

void CHXNetCheck::GetDNSAddress(CHXString& strDNS)
{
	// Get information about the interface ( 0 is default )
	
	InetInterfaceInfo theInfo;	
	OSStatus result = OTInetGetInterfaceInfo (&theInfo, 0);
	
	// Retrieve DNS from structure
	
	InetHost theDNS = theInfo.fDNSAddr;
	
	// Convert InetHost to char*
	
	char theDNSString[255]; /* Flawfinder: ignore */
	OTInetHostToString (theDNS,theDNSString);
	
	// Copy that to the CHXString
	
	strDNS = theDNSString;
	
	// We are done ! 
}



//******************************************************************************
//
// Method:	CHXNetCheck::SleepWell
//
// Purpose:	This method sleeps but continues to pump messages.  This allows us to 
//			block properly, even under such platforms as Win16.
//		
//
// Notes:	n/a
//
//******************************************************************************

void CHXNetCheck::SleepWell(ULONG32 ulInterval)
{
#if 0
	ULONG32 ulStartTime, ulCurrentTime;
	MSG			    msg;

	
	// Get start time
	ulStartTime = HX_GET_TICKCOUNT();
	do
	{
	    // Keep pumping messages
   	    if(PeekMessage(&msg, NULL,0,0,PM_REMOVE))
	    {
		if(msg.message == WM_QUIT) 
		{
		    PostQuitMessage(0);
		    break;
		}
		else
		{
		    TranslateMessage(&msg);
       		    DispatchMessage(&msg);
		}
	    }

	    // If we have waited ulInterval time then drop out
	    ulCurrentTime = HX_GET_TICKCOUNT();
	} while (CALCULATE_ELAPSED_TICKS(ulStartTime, ulCurrentTime) < ulInterval);
#endif
}

HXBOOL 
CHXNetCheck::SmartPing()
{
   UINT16 nPort = DEF_HTTP_PORT;

    // try to get DNS address to ping.
    CHXString strPingAddr;
    GetDNSAddress(strPingAddr);
    nPort = DEF_DNS_PORT;
    
    // No DNS address? Default to a known web server.
    // XXXBJP pinging video.real.com, used in Beta 1, should be
    // changed for Beta 2!!
    if (strPingAddr.IsEmpty())
    {
	    strPingAddr = "209.66.98.23"; // video.real.com .. UGHHHH!!
	    nPort = DEF_HTTP_PORT;
    }

    return (Ping(strPingAddr, nPort, FALSE));
}




OSStatus HXMacNetWillDial( char* remoteHost, ULONG32* willDial )
{
	OSStatus outResult = paramErr;
	
	Boolean success = FALSE;
	
	if( !willDial )
	{
		return outResult;
	}
	
	*willDial = kOTTCPDialUnknown;
	
	char* nodename = "real.com";
	
	if( remoteHost )
	{
		nodename = remoteHost;
	}
	
	SCNetworkCheckReachabilityByNameProcPtr proc;
	int flags;
	
	CFBundleRef sysConfigBundle = CFBundleGetBundleWithIdentifier( CFSTR("com.apple.SystemConfiguration") );

	if( sysConfigBundle )
	{
		proc = (SCNetworkCheckReachabilityByNameProcPtr) CFBundleGetFunctionPointerForName( sysConfigBundle, 
																							CFSTR("SCNetworkCheckReachabilityByName") );
		if( proc )
		{
			success = (*proc) ( nodename, &flags );
		}
		
		CFRelease( sysConfigBundle );
	}
	
	if( success )
	{
		// see SCNetwork.h in SystemConfiguration.framework for
		// the meanings of flags
		if( flags & 1<<2 )
		{
			*willDial = kOTTCPDialYes;
		}
		else
		{
			*willDial = kOTTCPDialNo;
		}
		
		outResult = noErr;
		}
	
	return outResult;
}

