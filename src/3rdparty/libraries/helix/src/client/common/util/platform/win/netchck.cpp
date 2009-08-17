/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: netchck.cpp,v 1.9 2006/02/07 19:27:25 ping Exp $
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

#include "hlxclib/windows.h"
#include "prefdefs.h"
#include "netchck.h"
#include "hxtick.h"
#include "hxassert.h"
#include "hxver.h"

#include <raserror.h>	    // for ras error codes

#define DEF_DNS_PORT 	    53 //tcp
#define DIALOG_CLASS	"#32770"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

// Comment out this definition for release versions that shouldn't have
// logging code compiled in.
//#define USE_NETDETECT_LOG 1

#ifdef USE_NETDETECT_LOG
    #include "hlxclib/fcntl.h"	    // for file constants
    #include "chxdataf.h"	    // CHXDataFile
    #include "hlxclib/time.h"
    #include "pref.h"	    // CPref
    #include "ihxpckts.h"   // IHXBuffer
#endif

namespace NetDetectLog
{
    #ifndef USE_NETDETECT_LOG
#ifndef _WINCE
	#define WRITE_LOG0(format) do {;} while(0)
	#define WRITE_LOG1(format, x1) do {;} while(0)
	#define WRITE_LOG_WINET(result, flags) do {;} while(0)
#else  //_WINCE
	#define WRITE_LOG0(format) HX_TRACE(format)
	#define WRITE_LOG1(format, x1) HX_TRACE(format, x1)
	#define WRITE_LOG_WINET(result, flags) HX_TRACE("InternetGetConnectedState: result=%d, flags=%d",(int)result, (int)flags)
#endif //_WINCE
	#define INITIALIZE_LOG() do {;} while(0)
	#define CLOSE_LOG() do {;} while(0)
    #else
	#define STR_EOL "\r\n"
	
	const char* const kszNetdetectLogFileName = "netdtlog.txt";
	const char* const kszNetdetectLogPrefKey = "netdetectlog";
	static CHXDataFile* g_pLogFile = NULL;

	HX_RESULT InitializeLog();
	HX_RESULT CloseLog();
	HX_RESULT WriteToLog(const CHXString& msg);
	HX_RESULT WriteToLogWinInetGetConnected(HXBOOL result, DWORD flags);

	#define INITIALIZE_LOG() do { InitializeLog(); } while(0)
	#define CLOSE_LOG() do { CloseLog(); } while(0)

	#define WRITE_LOG0(format)					\
	    WriteToLog(format);

	#define WRITE_LOG1(format, x1){				\
	    CHXString strFormat;						\
	    strFormat.Format(format, x1);					\
	    WriteToLog(strFormat);}

	#define WRITE_LOG_WINET(result, flags)	\
	    WriteToLogWinInetGetConnected(result, flags)

	HX_RESULT WriteToLogWinInetGetConnected(HXBOOL result, DWORD flags)
	{
	    // Flags for InternetGetConnectedState 
	    #define INTERNET_CONNECTION_MODEM           0x01
	    #define INTERNET_CONNECTION_LAN             0x02
	    #define INTERNET_CONNECTION_PROXY           0x04
	    #define INTERNET_CONNECTION_MODEM_BUSY      0x08  /* no longer used */
	    #define INTERNET_RAS_INSTALLED              0x10
	    #define INTERNET_CONNECTION_OFFLINE         0x20
	    #define INTERNET_CONNECTION_CONFIGURED      0x40

	    CHXString strFormat;						
	    if(result) 
	    {
		CHXString format("InternetGetConnectedState returned TRUE; flag = %s");
		if(flags & INTERNET_CONNECTION_MODEM)
		{
		    strFormat.Format(format, "INTERNET_CONNECTION_MODEM");				
		}
		else if(flags & INTERNET_CONNECTION_LAN)
		{
		    strFormat.Format(format, "INTERNET_CONNECTION_LAN");				
		}
		else if(flags & INTERNET_CONNECTION_PROXY)
		{
		    strFormat.Format(format, "INTERNET_CONNECTION_PROXY");				
		}
		else if(flags & INTERNET_CONNECTION_MODEM_BUSY)
		{
		    strFormat.Format(format, "INTERNET_CONNECTION_MODEM_BUSY");				
		}
		else if(flags & INTERNET_RAS_INSTALLED)
		{
		    strFormat.Format(format, "INTERNET_RAS_INSTALLED");				
		}
		else if(flags & INTERNET_CONNECTION_OFFLINE)
		{
		    strFormat.Format(format, "INTERNET_CONNECTION_OFFLINE");				
		}
		else if(flags & INTERNET_CONNECTION_CONFIGURED)
		{
		    strFormat.Format(format, "INTERNET_CONNECTION_CONFIGURED");				
		}
	    }
	    else
	    {
		strFormat = "InternetGetConnectedState returned FALSE";
	    }

	    return WriteToLog(strFormat);
	}

	HX_RESULT InitializeLog()
	{
	    HX_RESULT res = HXR_FAIL;
	
	    HXBOOL bShouldWriteLogFile = FALSE;

	    // Read from the preferences to see if we should write a log file.
	    CPref* pPreferences = CPref::open_shared_pref( HXVER_COMMUNITY );
	    if( pPreferences )
	    {
		IHXBuffer* pBuffer = NULL;
		if( SUCCEEDED( pPreferences->read_pref( kszNetdetectLogPrefKey, pBuffer ))) 
		{
		    bShouldWriteLogFile = 
			( atoi( reinterpret_cast<const char*>(pBuffer->GetBuffer())) == 1 );
		    HX_RELEASE(pBuffer);
		}

		HX_DELETE( pPreferences );
	    }

	    if( !bShouldWriteLogFile )
	    {
		return res;
	    }

	    // First try to open the file for appending.  If that doesn't work,
	    // then just create a new one.
	    g_pLogFile = CHXDataFile::Construct(m_pContext);
	    if (g_pLogFile)
	    {
		res = g_pLogFile->Open(kszNetdetectLogFileName, 
		    O_BINARY | O_WRONLY | O_APPEND, TRUE);
		if (FAILED(res))
		{
		    res = g_pLogFile->Open(kszNetdetectLogFileName, 
			O_BINARY | O_CREAT | O_TRUNC | O_WRONLY, TRUE);
		}

		if( SUCCEEDED( res ))
		{
		    CHXString strMsg;
		    strMsg.Format("%s################################################"
			"##############################%s", 
			STR_EOL, STR_EOL);
		    g_pLogFile->Write(strMsg, strMsg.GetLength());
		}
		else
		{
		    HX_DELETE(g_pLogFile);
		}
	    }
	    return res;
	}

	HX_RESULT CloseLog()
	{
	    if (g_pLogFile)
	    {
		g_pLogFile->Close();
		HX_DELETE(g_pLogFile);
	    }
	    return HXR_OK;
	}

	HX_RESULT WriteToLog(const CHXString& msg)
	{
	    HX_RESULT res = HXR_FAIL;

	    if (g_pLogFile)
	    {
		time_t now = time(NULL);
		struct tm* date = localtime(&now);
		
		char szTime[20] = { 0 }; /* Flawfinder: ignore */

#if !defined(WIN32_PLATFORM_PSPC)
		if(date)
		{
		    strftime(szTime, 19, "%X", date);
		}
#endif /* !defined(WIN32_PLATFORM_PSPC) */

		CHXString newMsg;
		newMsg.Format("%s: %s%s", szTime, (const char*)msg, STR_EOL);
		res = (g_pLogFile->Write(newMsg, newMsg.GetLength()) 
		    == newMsg.GetLength()) ? HXR_OK : HXR_FAIL;
	    }

	    return res;
	}

    #endif // USE_NETDETECT_LOG

    // "resource aquisition is init" - Init/ Close log file
    class NetDetectLogLib
    {
    public:
	NetDetectLogLib() { INITIALIZE_LOG(); }
	~NetDetectLogLib() { CLOSE_LOG(); }
    };

} //NetDetectLog namespace 

CHXNetCheck::CHXNetCheck(UINT32 timeout) : 
	XHXNetCheck(timeout)
	,m_pRmaNetServices(0)
	, m_pRmaTCPSocket(0)
	, m_pContext(0)
	, m_fConnected(FALSE)
	, m_fFailed(FALSE)
	, m_lRefCount(0)
#ifndef _WIN16
	, m_hRasApiModule(0)
	, m_pRasEnumConnections(0)
#endif
	
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
// Purpose:	The function checks if we can attempt a connection without 
//		    being prompted to connect.  
//		Returns true if we can, false otherwise.
//
// Notes:	There is no single function for determining if a machine is 
//		connected to the internet, and it is impossible to reliably 
//		determine what is happening without side effects - such as 
//		automatic network connections taking place.
//		
//		MSDN article - 
//		"HOWTO: Detecting If you have a Connection to the Internet"
//
//		"Usually the best way to determine if you have a connection 
//		to a particular computer is to attempt the connection. If the 
//		autodial feature of Windows is enabled then attempting the 
//		connection may cause the default Internet dialup connectoid 
//		to be opened, and you will be prompted with your credentials to connect.
//
//		To avoid having the default Internet connectoid dialed, 
//		the InternetGetConnectedState function can be used to determine 
//		if there is a default Internet dialup connectoid configured and 
//		whether it is currently active or not. If there is a default 
//		Internet dialup connectoid configured and it is not currently 
//		active then InternetGetConnectedState will return FALSE. 
//		If InternetGetConnectedState returns TRUE then you can attempt 
//		to connect to the Internet resource without fear of being prompted 
//		to connect to another Internet Service Provider.
//
//		You cannot rely solely on the fact that InternetGetConnectedState 
//		returning TRUE means that you have a valid active Internet 
//		connection. It is impossible for InternetGetConnectedState 
//		to determine if the entire connection to the Internet is 
//		functioning without sending a request to a server. This is why 
//		you need to send a request to determine if you are really connected 
//		or not. You can be assured however that if InternetGetConnectedState 
//		returns TRUE, that attempting your connection will NOT cause you 
//		to be prompted to connect to the default Internet Service Provider.
//
//		Be aware that InternetGetConnectedState only reports the status 
//		of the default Internet connectoid on Internet Explorer 4.x. 
//		If a nondefault connectoid is connected, InternetGetConnectedState 
//		always returns FALSE (unless a LAN connection is used). 
//		With Internet Explorer 4.x configured to use a LAN connection, 
//		InternetGetConectedState always returns TRUE.
//
//		Internet Explorer 5 behaves differently. If you are currently dialed 
//		into a different dial-up in Internet Explorer 5, InternetGetConnectedState 
//		reports dial-up connection status as long as any connectoid is dialed 
//		or an active LAN connection exists." 
//
//******************************************************************************


HXBOOL 
CHXNetCheck::FInternetAvailable(HXBOOL fPing, HXBOOL fProxy)
{
	using namespace NetDetectLog; 
	NetDetectLogLib logLib;

	HXBOOL fRet = FALSE;
	UINT16 nPort = DEF_HTTP_PORT;
	
#ifndef _WIN16

	WRITE_LOG0("FInternetAvailable fc entered...");

	HKEY hKey = 0 ;
	DWORD fAutoDial = 0 ;
	DWORD regType = 0 ;
	DWORD cbBuf;

#if !defined(WIN32_PLATFORM_PSPC)
	// set the error mode since we'll do some DLL loading and we want the OS to be
	// quiet whhen we do this
	UINT oldErrorMode = ::SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
#endif /* !defined(WIN32_PLATFORM_PSPC) */

	// XXXSO - agreed to use InternetGetConnectedState function only.
	// The rest of the code should be called when this function is not available.

	HXBOOL fConnected = FALSE;
	if( SUCCEEDED( WinInetGetConnected( fConnected )))
	{
	    fRet = fPing ? SmartPing() : fConnected;
	    goto Ret;
	}

// XXXKM - agreed to remove proxy impact on net detect; instead let the following code determine
// if we are connected or not - doing so will alleviate issues with proxy and dialup
#if 0
	if (fProxy)
	    return TRUE;
#endif

	WRITE_LOG0("Old algorithm entered ... ");
	WRITE_LOG0("Checking autodial flag ...");

	// XXXKM - check this early; if autodial is set, then return FALSE temporarily
	// check the autodial bit in the registry.
	cbBuf = sizeof(fAutoDial);
	if (RegOpenKey(HKEY_CURRENT_USER, OS_STRING("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"),
		&hKey) == ERROR_SUCCESS)
	{
	    OSVERSIONINFO osv;

	    memset(&osv, 0, sizeof(osv));
	    osv.dwOSVersionInfoSize = sizeof(osv);
	    GetVersionEx(&osv);
	    if (osv.dwPlatformId != VER_PLATFORM_WIN32_NT)
	    {
		WRITE_LOG0("Checking Win95/98 EnableAutodial flag in HKCU/Software/Microsoft/Windows"
		    "/Current Version/Internet Settings...");

		// check for the Win95/98 flag
		RegQueryValueEx(hKey, OS_STRING("EnableAutodial"), NULL, &regType, (BYTE *) &fAutoDial, &cbBuf);
	    }
	    else
	    {
		WRITE_LOG0("Checking for the RAS key on NT.");

		// XXXKM - check for the RAS key on NT; only way I could find around the assert/crash in rasapi32.dll
		// in situations where the dll existed, but ras was disabled/removed on the machine
		HKEY hRasKey = 0;
		if (::RegOpenKey(HKEY_LOCAL_MACHINE, OS_STRING("SOFTWARE\\Microsoft\\RAS"), &hRasKey) == ERROR_SUCCESS)
		{
		    // close the ras key
		    ::RegCloseKey(hRasKey);

		    // initialize to FALSE so this works the way it always has if something doesn't go right
		    fAutoDial = FALSE;
		    if (!m_hRasApiModule)
			m_hRasApiModule = ::LoadLibrary(OS_STRING("rasapi32.dll"));

		    if (m_hRasApiModule)
		    {
			WRITE_LOG0("rasapi32.dll loaded...");
			
			// try and get the API to enumerate RAS devices
			FPRASENUMDEVICES fpRasEnumDevices = (FPRASENUMDEVICES)GetProcAddress(m_hRasApiModule,
			    OS_STRING("RasEnumDevicesA"));
			if (!fpRasEnumDevices)
			{
			    fpRasEnumDevices  = (FPRASENUMDEVICES)GetProcAddress(m_hRasApiModule,
			    OS_STRING("RasEnumDevicesW"));
			}

			// we use this to see if there is a RAS enabled device on this system and also to 
			// check for initialization of the RAS subsystem since it will return an error if 
			// RAS is disabled or fails to initialize for whatever reason
			// XXXKM - this was added to prevent crashing on Win NT systems in the EnumEntries call
			// on machines where RAS has been disabled due to hardware profiles
			if (fpRasEnumDevices)
			{
			    WRITE_LOG0("Checking the RAS enabled devices...");
			    
			    RASDEVINFO rasDevInfo[1];
			    rasDevInfo[0].dwSize = sizeof(rasDevInfo);
			    DWORD bufferSize = sizeof(rasDevInfo);
			    DWORD numEntries = 0;
			    DWORD retVal = fpRasEnumDevices(rasDevInfo, &bufferSize, &numEntries);

			    // check if there was actually a RAS enabled device; this API will return an error
			    // if there are no RAS devices enabled, or if the RAS subsystem couldn't be initialized
			    if (retVal == ERROR_BUFFER_TOO_SMALL || retVal == ERROR_SUCCESS && numEntries == 1)
			    {
				// find the right entrypoint in the rasapi32.dll
				FPRASENUMENTRIES fpRasEnumEntries = (FPRASENUMENTRIES)GetProcAddress(m_hRasApiModule,
				    OS_STRING("RasEnumEntriesA"));
				if (!fpRasEnumEntries)
				{
				    fpRasEnumEntries = (FPRASENUMENTRIES)GetProcAddress(m_hRasApiModule,
				    OS_STRING("RasEnumEntriesW"));
				}

				// if we've got the RAS entries function, then call it
				if (fpRasEnumEntries)
				{
				    WRITE_LOG0("Calling RasEnumEntries function...");
				    
				    // setup the RAS entry structure for a single dialup item
				    RASENTRYNAME rasEntryName[1];
				    rasEntryName[0].dwSize = sizeof(rasEntryName);
				    bufferSize = sizeof(rasEntryName);
				    numEntries = 0;
				    // call the function to see how many dialup entries there are
				    retVal = fpRasEnumEntries(NULL, NULL, rasEntryName, &bufferSize, &numEntries);
				    // if our buffer is too small (i.e. there are more than one phone book entries)
				    // or we get a single item, then we have dialup, so set autodial to TRUE so 
				    // we'll tread lightly
				    if (retVal == ERROR_BUFFER_TOO_SMALL || (numEntries == 1 && retVal == ERROR_SUCCESS))
					fAutoDial = TRUE;
				}
			    }
			}
		    }
		}
	    }
	    // close the key when we're done with it
	    ::RegCloseKey(hKey);
	}

	WRITE_LOG1("Checked AUTODIAL flag = %d.", fAutoDial);

	// XXXKM - only check netcard active status if autodial is not on; if it's on, then
	// assume we are doing stuff through RAS connection only and ignore netcard for now;
	// only temporary
	if (!fAutoDial && FNetCardActive())
	{
	    WRITE_LOG0("FNetCardActive fc returned TRUE. goto exit ...");

	    fRet = TRUE;
	    goto Ret;
	}


	if (!fAutoDial) // 	if it's not set, go ahead and try the network
	{
	    if (fPing)
		fRet = SmartPing();
	    else 
		fRet = TRUE;

	    goto Ret;
	}
	else // See if we have an active RAS connection
	{		
	    WRITE_LOG0("Any active RAS connection?");

	    DWORD cRasConns = 0;
	    RASCONN rgRasConn[5];
	    DWORD cb = sizeof(rgRasConn);
	    
	    if (!m_hRasApiModule)
		    m_hRasApiModule= LoadLibrary(OS_STRING("rasapi32.dll"));
	    // add this code if this ever gets into win16; win16 will return nonnull on error from load library  #if _WIN16 if (m_handle < HINSTANCE_ERROR) m_handle = NULL; #endif
	    if (!m_hRasApiModule) // Dialup networking is not installed.
	    {
		    WRITE_LOG0("Dialup networking not installed ...");

	    	    if (fPing)
		    	fRet = SmartPing();
	    	    else 
		    	fRet = TRUE;
		    goto Ret;
	    }

	    if (!m_pRasEnumConnections)
		    m_pRasEnumConnections =
			(FPRASENUMCONNECTIONS)GetProcAddress(m_hRasApiModule,OS_STRING("RasEnumConnectionsA"));

	    if (!m_pRasEnumConnections) // Dialup networking is not installed.
	    {
		    WRITE_LOG0("Cannot load RasEnumConnections fc...");

		    if (fPing)
		    	fRet = SmartPing();
	    	    else 
		    	fRet = TRUE;
		    goto Ret;
	    }

	    rgRasConn[0].dwSize = sizeof(RASCONN);
	    m_pRasEnumConnections(rgRasConn, &cb, &cRasConns);
	    
	    WRITE_LOG1("RasEnumConnections found %d connections...", cRasConns);

	    if (cRasConns)
	    {

	    	    if (fPing)
		    	fRet = SmartPing();
	    	    else 
		    	fRet = TRUE;
		    goto Ret;
	    }
	    else
	    {
		    fRet = FALSE;
		    goto Ret;
	    }
	}
Ret:

	// free up the library if loaded
	if (m_hRasApiModule)
	{
	    ::FreeLibrary(m_hRasApiModule);
	    m_hRasApiModule = NULL;
	}

#if !defined(WIN32_PLATFORM_PSPC)
	// reset the error mode to the old value
	::SetErrorMode(oldErrorMode);
#endif /* !defined(WIN32_PLATFORM_PSPC) */

	WRITE_LOG1("FInternetAvailable terminated with %s value...", 
	    fRet ? "TRUE" : "FALSE");

	return(fRet);

#else
	// Win16 section
	// =-=w16.0  for win16 always returns true, since there is no popup dialog in the way
	return (TRUE);
#endif

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
	ULONG32 ulStartTime, ulCurrentTime, ulElapsedTime;
	HXBOOL fRet = FALSE;

	// If we don't have the network services interface yet than try and get it here
	if (m_pContext && !m_pRmaNetServices)
	    m_pContext->QueryInterface(IID_IHXNetworkServices, (void**)&m_pRmaNetServices);

	if (!m_pRmaTCPSocket && m_pRmaNetServices)
		m_pRmaNetServices->CreateTCPSocket(&m_pRmaTCPSocket);

	if (!m_pRmaTCPSocket)
		return FALSE;

	m_fFailed = m_fConnected = FALSE;
	m_pRmaTCPSocket->Init(this);
	
	m_pRmaTCPSocket->Connect(szHostName, nPort);

	ulElapsedTime = 0;
	
	// Get start time
	ulStartTime = HX_GET_TICKCOUNT();
	while (!m_fFailed && !m_fConnected && (ulElapsedTime < m_Timeout))
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
	ULONG32 ulStartTime, ulCurrentTime;
	MSG			    msg;
	char	szClassName[16] = ""; /* Flawfinder: ignore */
	HWND	hActiveWindow = NULL;

	
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
		    // XXXJDL Since we are pumping messages through here while we block elsewhere in the calling code
		    // we need to get the free dialog navigation support from windows for modeless dialog boxes.  Modal dialog
		    // boxes are fine since they will not go through this message loop.  I have been using this method
		    // in the main player application and it seems to work ok.  Let me know if this causes pain for anyone.
		    // Basically if the active window is a dialog box class then we are assuming it is a modeless dialog box
		    // and calling IsDialogMessage with this handle.  If the active window is neither a dialog class or
		    // doesn't process the dialog message then we do the normal message dispatching
		    hActiveWindow = ::GetActiveWindow();
		    szClassName[0] = '\0';
		    ::GetClassName(hActiveWindow,
				   OS_STRING2(szClassName,sizeof(szClassName)),
				   sizeof(szClassName));
		    if (strcmp(szClassName,DIALOG_CLASS) || !IsDialogMessage(hActiveWindow, &msg))
		    {
			TranslateMessage(&msg);
       			DispatchMessage(&msg);
		    }
		}
	    }

	    // If we have waited ulInterval time then drop out
	    ulCurrentTime = HX_GET_TICKCOUNT();
	} while (CALCULATE_ELAPSED_TICKS(ulStartTime, ulCurrentTime) < ulInterval);

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
#ifndef _WIN16
    strDNS.Empty();
    HKEY hKey = 0;

    // Win95/98 likes it here
    RegOpenKey(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Services\\VxD\\MSTCP\\Parameters",
	    &hKey);

    if (hKey != NULL)
    {
    	GetNameServerKey(hKey, "NameServer", strDNS);
		if (strDNS.IsEmpty())
    		GetNameServerKey(hKey, "DhcpNameServer", strDNS);
		RegCloseKey(hKey);
    }

    // Some Win 95/98 machines put it here..
    if (strDNS.IsEmpty())
    {
    	RegOpenKey(HKEY_LOCAL_MACHINE, OS_STRING("System\\CurrentControlSet\\Services\\VxD\\MSTCP"),
	    	&hKey);
    
    	if (hKey != NULL)
    	{
    	    GetNameServerKey(hKey, "NameServer", strDNS);
			if (strDNS.IsEmpty())
    			GetNameServerKey(hKey, "DhcpNameServer", strDNS);
			RegCloseKey(hKey);
    	}
    }


    // Try WinNT registry location.
    if (strDNS.IsEmpty())
    {
	RegOpenKey(HKEY_LOCAL_MACHINE, OS_STRING("System\\CurrentControlSet\\Services\\Tcpip\\Parameters"),
		&hKey);
    
    	if (hKey != NULL)
    	{
    	    GetNameServerKey(hKey, "NameServer", strDNS);
			if (strDNS.IsEmpty())
    			GetNameServerKey(hKey, "DhcpNameServer", strDNS);
			RegCloseKey(hKey);
    	}
    }

    // Another  WinNT registry location.
    if (strDNS.IsEmpty())
    {
	RegOpenKey(HKEY_LOCAL_MACHINE, OS_STRING("System\\CurrentControlSet\\Services\\Tcpip"),
		&hKey);
    	if (hKey != NULL)
    	{
    	    GetNameServerKey(hKey, "NameServer", strDNS);
			if (strDNS.IsEmpty())
    			GetNameServerKey(hKey, "DhcpNameServer", strDNS);
			RegCloseKey(hKey);
    	}
    }


    // I want to make sure this technique of getting DNS address works. Email bpitzel
    // if it fails.
    HX_ASSERT( !strDNS.IsEmpty() );

#endif
}

#ifndef _WIN16
void CHXNetCheck::GetNameServerKey(HKEY hKey, const char* szKeyName, CHXString& strDNS)
{
    char szDNS[MAX_PATH]; /* Flawfinder: ignore */
    DWORD regType = 0 ;
    DWORD cbuf;
    cbuf = sizeof(szDNS);

    if (ERROR_SUCCESS == RegQueryValueEx(hKey, OS_STRING(szKeyName), NULL, &regType, (BYTE *) szDNS, &cbuf))
    {
	strDNS = szDNS;
	
	// Win95/98 separate multiple DNS addresses with ","
	// WinNT separate multiple DNS addresses with ","
	// We will grab the first IP address in the list
	strDNS = strDNS.SpanExcluding(" ,");
	strDNS.TrimLeft();
	strDNS.TrimRight();
    }
}
#endif /* _WIN16 */


HXBOOL 
CHXNetCheck::SmartPing()
{
    using namespace NetDetectLog; 
    WRITE_LOG0("SmartPing fc entered ...");

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


HXBOOL 
CHXNetCheck::FNetCardActive()
{
#if defined(_WIN16) || defined(WIN32_PLATFORM_PSPC)
    return TRUE;
#else
	char szEnum_Name[ENUM_MAX][20] = {REGSTR_KEY_ROOTENUM, REGSTR_KEY_BIOSENUM, /* Flawfinder: ignore */
	REGSTR_KEY_PCIENUM, REGSTR_KEY_ISAENUM, REGSTR_KEY_EISAENUM,
	REGSTR_KEY_PCMCIAENUM};

    using namespace NetDetectLog;
    WRITE_LOG0("FNetCardActive entered ...");

    char szClass[64]; /* Flawfinder: ignore */
    ULONG ulType;
    ULONG32 cbData;
    CHXString sCurrentKey;
    CHXString sCurrentFiles;
    CHXString sDeviceID;
    char szCurrentDeviceNode[STRINGSIZE]; /* Flawfinder: ignore */
    char szCurrentDevice[STRINGSIZE]; /* Flawfinder: ignore */
    char szSoftwareKey[STRINGSIZE]; /* Flawfinder: ignore */
    HXBOOL bFoundKey = FALSE;
    int i;
    OSVERSIONINFO osv;

    memset(&osv, 0, sizeof(osv));
    osv.dwOSVersionInfoSize = sizeof(osv);
    GetVersionEx(&osv);
    if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
    	return FALSE;

    using namespace NetDetectLog; 
    WRITE_LOG0("Win95/98 : Looking for devices of the Net class...");

    // we want to look through the ENUM\BIOS, ENUM\ISAPNP, and ENUM\PCI
    // trees for devices of the Net class.  For each display device
    // we find we will ask the Device manager if it is active.
    for (i = 0; i < ENUM_MAX && !bFoundKey; i++)
    {
	HKEY hBusKey = 0;
	sCurrentKey = (CHXString)REGSTR_KEY_ENUM + BACKSLASH + szEnum_Name[i];

	// Start reading the devices
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, OS_STRING(sCurrentKey), 0, KEY_READ, &hBusKey))
	{
	    cbData = sizeof(szCurrentDeviceNode);
	    ULONG32 dwDevNodeEnumIndex = 0;
	    HKEY hDeviceNodeKey;

	    // Enumerate all of the devices on the bus
	    while (!bFoundKey &&
		!RegEnumKeyEx(hBusKey, dwDevNodeEnumIndex, 
			      OS_STRING2(szCurrentDeviceNode, cbData), 
			      &cbData, NULL, NULL, NULL, NULL))
	    {
		// get the registry key for the current device node
		hDeviceNodeKey = 0;
		sCurrentKey = (CHXString)REGSTR_KEY_ENUM + BACKSLASH + szEnum_Name[i] + BACKSLASH + (CHXString)szCurrentDeviceNode;

		if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, OS_STRING(sCurrentKey), 0, KEY_READ, &hDeviceNodeKey))
		{
		    cbData = sizeof(szCurrentDevice);
		    ULONG32 dwHWDeviceEnumIndex = 0;
		    HKEY hHWDeviceKey;

		    // enumerate all of the hardware devices in this Device Node
		    while (!bFoundKey &&
			!RegEnumKeyEx(hDeviceNodeKey, dwHWDeviceEnumIndex, 
				      OS_STRING2(szCurrentDevice, cbData),
				      &cbData, NULL, NULL, NULL, NULL))
		    {
			// get the registry key for the current hardware device
			int nBaseKeyLength = sCurrentKey.GetLength();

			hHWDeviceKey = 0;
			sCurrentKey += BACKSLASH + (CHXString) szCurrentDevice;

			if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, OS_STRING(sCurrentKey), 0, KEY_READ, &hHWDeviceKey))
			{
			    // Ask if this device is a "Net" device by checking it's class
			    cbData = sizeof(szClass);
			    szClass[0] = 0;

			    if (!RegQueryValueEx(hHWDeviceKey, OS_STRING("Class"), 0, &ulType, (LPBYTE) szClass, &cbData)
				&& !stricmp(szClass, "Net"))
			    {
				// if it is a display device then we want to find out if it is
				// the active device for this class.
				cbData = sizeof(szSoftwareKey);
				sDeviceID = (CHXString) szEnum_Name[i] + BACKSLASH + (CHXString)szCurrentDeviceNode
				    + BACKSLASH + (CHXString) szCurrentDevice;

				// get the registry key name for the software key for the device, if it is not
				// null and this is the active device then we are done.
				if (!RegQueryValueEx(hHWDeviceKey, OS_STRING("Driver"), 0, &ulType, (LPBYTE) szSoftwareKey, &cbData)
				    && szSoftwareKey[0] != '0'
				    && DevNodeIsActive((sDeviceID)))	// Call Config Manager in 16-bit code

				{
				    bFoundKey = TRUE;
				    WRITE_LOG1("Found active node %s...", (char const*) sDeviceID);
				}
			    }
			}

			// reset the key to the base for the next time through the loop
			sCurrentKey = sCurrentKey.Left(nBaseKeyLength);

			if (hHWDeviceKey)
			{
			    RegCloseKey(hHWDeviceKey);
			    hHWDeviceKey = 0;
			}

			// move to next device key
			dwHWDeviceEnumIndex++;

			// reset size to szCurrentDevice size
			cbData = sizeof(szCurrentDevice);
		    }
		}

		if (hDeviceNodeKey)
		{
		    RegCloseKey(hDeviceNodeKey);
		    hDeviceNodeKey = 0;
		}

		// move to the next device node
		dwDevNodeEnumIndex++;

		// reset the size to szCurrentDeviceNode size
		cbData = sizeof(szCurrentDeviceNode);
	    }
	}

	if (hBusKey)
	{
	    RegCloseKey(hBusKey);
	    hBusKey = 0;
	}
    }

    return bFoundKey;
#endif


}



HXBOOL
CHXNetCheck::DevNodeIsActive(const char *szDeviceID)
{
#ifdef _WIN16
    HX_ASSERT (FALSE); // should never get called in win16
    return TRUE;
#else
    HINSTANCE hDevNodeInst;
    DWORD dwStatus;
    DWORD dwProblemNumber;
    DWORD cr;

    DWORD (WINAPI * pGetDevNodeStatus32Call) (const char *, LPDWORD, LPDWORD);

    // 10 is a magic number for the configuration manager api
    // that eventually gets called in the 16 bit dll.
    dwStatus = dwProblemNumber = cr = 10;
    pGetDevNodeStatus32Call = 0;

    hDevNodeInst = LoadLibrary(OS_STRING(_32BIT_DLLNAME));
    // add this code if this ever gets into win16; win16 will return nonnull on error from load library  #if _WIN16 if (m_handle < HINSTANCE_ERROR) m_handle = NULL; #endif

    if (hDevNodeInst)
    {
	(FARPROC &) pGetDevNodeStatus32Call = GetProcAddress(hDevNodeInst, OS_STRING("GetDevNodeStatus32Call"));

	if (pGetDevNodeStatus32Call)
	{
	    cr = pGetDevNodeStatus32Call(szDeviceID, &dwStatus, &dwProblemNumber);
	}

	FreeLibrary(hDevNodeInst);
    }

    if (cr == 0
	&& dwProblemNumber == 0)
	return TRUE;
    else
	return FALSE;
#endif
}



HX_RESULT CHXNetCheck::WinInetGetConnected(HXBOOL& bConnected)
{
    using namespace NetDetectLog; 
    WRITE_LOG0("WinInetGetConnected fc entered...");

    HINSTANCE hWinInet;
    HXBOOL (WINAPI * pInternetGetConnectedState) (LPDWORD lpdwFlags, DWORD dwReserved);

    hWinInet= LoadLibrary(OS_STRING(WININET_DLL));

    if (!hWinInet)
    {
	WRITE_LOG0("WinInet not present ...");
    	return HXR_FAIL;
    }

    (FARPROC &) pInternetGetConnectedState = GetProcAddress(hWinInet, OS_STRING("InternetGetConnectedState"));
    if (!pInternetGetConnectedState )
    {
	WRITE_LOG0("InternetGetConnectedState fc cannot be loaded ...");
    	
	FreeLibrary(hWinInet);
    	return HXR_FAIL;
    }

    DWORD dwFlags;
    bConnected = pInternetGetConnectedState(&dwFlags, 0);
    FreeLibrary(hWinInet);
    
    WRITE_LOG_WINET(bConnected, dwFlags);

#ifdef _INTERNETGETCONNECTEDSTATE_MAY_FAIL_UNEXPECTEDELY_
    if (!bConnected) return HXR_FAIL;
#else  //_INTERNETGETCONNECTEDSTATE_MAY_FAIL_UNEXPECTEDELY_
    return HXR_OK;
#endif //_INTERNETGETCONNECTEDSTATE_MAY_FAIL_UNEXPECTEDELY_
}
