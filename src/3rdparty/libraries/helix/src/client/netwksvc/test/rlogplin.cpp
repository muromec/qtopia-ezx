/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rlogplin.cpp,v 1.7 2007/02/28 06:09:34 gahluwalia Exp $
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

#define INITGUID
#include "rlogplin.h"
#include "hxprefutil.h"
#include "hxprefs.h"
#include "chxpckts.h" // CHXHeader

class HXRLPErrorSink : public IHXErrorSink
{
public:
    HXRLPErrorSink(HXRemoteLoggerPlugin* pRLP);
    ~HXRLPErrorSink();

    /*
     *  IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     *  IHXErrorSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorSink::ErrorOccurred
     *	Purpose:
     *	    After you have registered your error sink with an
     *	    IHXErrorSinkControl (either in the server or player core) this
     *	    method will be called to report an error, event, or status message.
     *
     *	    The meaning of the arguments is exactly as described in
     *	    hxerror.h
     */
    STDMETHOD(ErrorOccurred)	(THIS_
				const UINT8	unSeverity,  
				const ULONG32	ulHXCode,
				const ULONG32	ulUserCode,
				const char*	pUserString,
				const char*	pMoreInfoURL
	);
private:
    ULONG32 m_lRefCount;
    HXRemoteLoggerPlugin* m_pRLP;
};

HXRLPErrorSink::HXRLPErrorSink(HXRemoteLoggerPlugin* pRLP) :
    m_pRLP(pRLP)
{}

HXRLPErrorSink::~HXRLPErrorSink()
{}

/*
 *  IUnknown methods
 */
STDMETHODIMP HXRLPErrorSink::QueryInterface(THIS_
					    REFIID riid,
					    void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)this },
	{ GET_IIDHANDLE(IID_IHXErrorSink), (IHXErrorSink*) this }
    };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXRLPErrorSink::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) HXRLPErrorSink::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

    /*
     *  IHXErrorSink methods
     */

    /************************************************************************
     *	Method:
     *	    IHXErrorSink::ErrorOccurred
     *	Purpose:
     *	    After you have registered your error sink with an
     *	    IHXErrorSinkControl (either in the server or player core) this
     *	    method will be called to report an error, event, or status message.
     *
     *	    The meaning of the arguments is exactly as described in
     *	    hxerror.h
     */
STDMETHODIMP HXRLPErrorSink::ErrorOccurred(THIS_
					   const UINT8	unSeverity,  
					   const ULONG32	ulHXCode,
					   const ULONG32	ulUserCode,
					   const char*	pUserString,
					   const char*	pMoreInfoURL)
{
    HX_RESULT res = HXR_OK ;
    if (m_pRLP)
    {
	res = m_pRLP->ErrorOccurred(unSeverity, ulHXCode, ulUserCode,
				    pUserString, pMoreInfoURL);
    }

    return res;
}

HXRemoteLoggerPlugin::HXRemoteLoggerPlugin() :
    m_lRefCount(0),
    m_pErrorSink(NULL),
    m_pRemoteLogger(NULL)    
{}

HXRemoteLoggerPlugin::~HXRemoteLoggerPlugin()
{
    HX_RELEASE(m_pErrorSink);
    
    if (m_pRemoteLogger)
    {
	m_pRemoteLogger->Close();
    }
    HX_RELEASE(m_pRemoteLogger);
}

/*
 *	IUnknown methods
 */
STDMETHODIMP HXRemoteLoggerPlugin::QueryInterface(THIS_
						  REFIID riid,
						  void** ppvObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXPlugin*)this },
	{ GET_IIDHANDLE(IID_IHXPlugin), (IHXPlugin*) this },
	{ GET_IIDHANDLE(IID_IHXGenericPlugin), (IHXGenericPlugin*) this },
	{ GET_IIDHANDLE(IID_IHXPlayerCreationSink), (IHXPlayerCreationSink*) this },
	{ GET_IIDHANDLE(IID_IHXComponentPlugin), (IHXComponentPlugin*) this }
    };
    return QIFind(qiList, QILISTSIZE(qiList), riid, ppvObj);
}

STDMETHODIMP_(ULONG32) HXRemoteLoggerPlugin::AddRef(THIS)
{
    return InterlockedIncrement(&m_lRefCount);
}


STDMETHODIMP_(ULONG32) HXRemoteLoggerPlugin::Release(THIS)
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }
    
    delete this;
    return 0;
}

/*
 *	IHXPlugin methods
 */

/************************************************************************
 *	Method:
 *	    IHXPlugin::GetPluginInfo
 *	Purpose:
 *	    Returns the basic information about this plugin. Including:
 *
 *	    bMultipleLoad	Whether or not this plugin can be instantiated
 *				multiple times. All File Formats must set
 *				this value to TRUE.  The only other type of
 *				plugin that can specify bMultipleLoad=TRUE is
 *				a filesystem plugin.  Any plugin that sets
 *				this flag to TRUE must not use global variables
 *				of any type.
 *
 *				Setting this flag to TRUE implies that you
 *				accept that your plugin may be instantiated
 *				multiple times (possibly in different
 *				address spaces).  Plugins are instantiated
 *				multiple times only in the server (for
 *				performance reasons).
 *
 *				An example of a plugin, that must set this
 *				flag to FALSE is a filesystem plugin that 
 *				uses a single TCP connection to communicate
 *				with a database.
 *				
 *	    pDescription	which is used in about UIs (can be NULL)
 *	    pCopyright		which is used in about UIs (can be NULL)
 *	    pMoreInfoURL	which is used in about UIs (can be NULL)
 *	    ulVersionNumber	The version of this plugin.
 */
static const char z_pDescription[] = "Helix Remote Logger Plugin";
static const char z_pCopyright[] = "RealNetworks 2003";
static const char z_pMoreInfoURL[] = "";

STDMETHODIMP HXRemoteLoggerPlugin::GetPluginInfo(THIS_
				REF(HXBOOL)	 /*OUT*/ bMultipleLoad,
				REF(const char*) /*OUT*/ pDescription,
				REF(const char*) /*OUT*/ pCopyright,
				REF(const char*) /*OUT*/ pMoreInfoURL,
				REF(ULONG32)	 /*OUT*/ ulVersionNumber)
{
    bMultipleLoad = FALSE;
    pDescription = z_pDescription;
    pCopyright = z_pCopyright;
    pMoreInfoURL = z_pMoreInfoURL;
    ulVersionNumber = 1;
    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPlugin::InitPlugin
 *	Purpose:
 *	    Initializes the plugin for use. This interface must always be
 *	    called before any other method is called. This is primarily needed 
 *	    so that the plugin can have access to the context for creation of
 *	    IHXBuffers and IMalloc.
 */
STDMETHODIMP HXRemoteLoggerPlugin::InitPlugin(THIS_
					      IUnknown*   /*IN*/  pContext)
{
    HX_RESULT res = HXR_FAILED;

    if (pContext)
    {
	IHXPlayerSinkControl* pPSinkCtl = NULL;
	
	if (HXR_OK == pContext->QueryInterface(IID_IHXPlayerSinkControl,
					       (void**)&pPSinkCtl))
	{
	    pPSinkCtl->AddSink(this);
	}

	HX_RELEASE(pPSinkCtl);


	m_pErrorSink = new HXRLPErrorSink(this);
	HX_ADDREF(m_pErrorSink);
	
	IHXPreferences* pPrefs = NULL;
	if (HXR_OK == pContext->QueryInterface(IID_IHXPreferences,
					       (void**)&pPrefs))
	{
	    IHXBuffer* pRemoteHost = NULL;
	    IHXBuffer* pRemoteFile = NULL;
	    UINT32 ulPort = 0;

	    if ((HXR_OK == pPrefs->ReadPref("RemoteLoggerHost", pRemoteHost))&&
		(HXR_OK == ReadPrefUINT32(pPrefs, "RemoteLoggerPort", ulPort))&&
		(HXR_OK == pPrefs->ReadPref("RemoteLoggerFile", pRemoteFile)))
	    {
		m_pRemoteLogger = new HXRemoteLogger();

		if (m_pRemoteLogger)
		{
		    m_pRemoteLogger->AddRef();
		    res = m_pRemoteLogger->Init(pContext,
						(char*)pRemoteHost->GetBuffer(),
						(UINT16)ulPort,
						(char*)pRemoteFile->GetBuffer());
		    if (HXR_OK == res)
		    {
			m_pRemoteLogger->Log("logger started\n");
		    }
		}
	    }

	    HX_RELEASE(pRemoteHost);
	    HX_RELEASE(pRemoteFile);
	    
	}
	HX_RELEASE(pPrefs);
    }

    return res;
}

/*
 *	IHXGenericPlugin methods
 */

STDMETHODIMP HXRemoteLoggerPlugin::IsGeneric(THIS_
					     REF(HXBOOL)	 /*OUT*/ bIsGeneric)
{
    bIsGeneric = TRUE;

    return HXR_OK;
}

/*
 * IHXPlayerCreationSink Methods
 */

/************************************************************************
 *	Method:
 *	    IHXPlayerCreationSink::PlayerCreated
 *	Purpose:
 *	    Notification when a new player is created
 *
 */
STDMETHODIMP HXRemoteLoggerPlugin::PlayerCreated(THIS_
						 IHXPlayer* pPlayer)
{
    IHXErrorSinkControl* pSinkControl = NULL;
    if (m_pErrorSink &&
	(HXR_OK == pPlayer->QueryInterface(IID_IHXErrorSinkControl,
					   (void**)&pSinkControl)))
    {
	pSinkControl->AddErrorSink(m_pErrorSink, 
				   HXLOG_EMERG, HXLOG_DEBUG);
    }
    HX_RELEASE(pSinkControl);

    return HXR_OK;
}

/************************************************************************
 *	Method:
 *	    IHXPlayerCreationSink::PlayerClosed
 *	Purpose:
 *	    Notification when an exisitng player is closed
 *
 */
STDMETHODIMP HXRemoteLoggerPlugin::PlayerClosed(THIS_
						IHXPlayer* pPlayer)
{
    IHXErrorSinkControl* pSinkControl = NULL;
    if (m_pErrorSink &&
	(HXR_OK == pPlayer->QueryInterface(IID_IHXErrorSinkControl,
					   (void**)&pSinkControl)))
    {
	pSinkControl->RemoveErrorSink(m_pErrorSink);
    }
    HX_RELEASE(pSinkControl);

    return HXR_OK;
}

/*
 *	IHXComponentPlugin methods
 */

/************************************************************************
 *	Method:
 *	    IHXComponentPlugin::GetNumberComponents
 *	Purpose:
 */
STDMETHODIMP_(UINT32) HXRemoteLoggerPlugin::GetNumComponents(THIS)
{
    return 1;
}

/************************************************************************
 *	Method:
 *	    IHXComponentPlugin::GetPackageName
 *	Purpose:
 */
STDMETHODIMP_(char const*) HXRemoteLoggerPlugin::GetPackageName(THIS) CONSTMETHOD
{
    return "RemoteLogger";
}

/************************************************************************
 *  Method:
 *	    IHXComponentPlugin::GetComponentInfoAtIndex
 *  Purpose:
 */
STDMETHODIMP HXRemoteLoggerPlugin::GetComponentInfoAtIndex(THIS_
  				               UINT32 /*IN*/  nIndex,
					       REF(IHXValues*)  /*OUT*/ pInfo)
{
    pInfo = NULL;

    if (nIndex  == 0)
    {
	pInfo = new CHXHeader();

	if (pInfo)
	{
	    pInfo->AddRef();
	}
    }

    return (pInfo) ? HXR_OK : HXR_FAILED;
}

/************************************************************************
 *  Method:
 *	    IHXComponentPlugin::CreateComponentInstance
 *  Purpose:
 */
STDMETHODIMP HXRemoteLoggerPlugin::CreateComponentInstance(THIS_
				   REFCLSID	    /*IN*/  rclsid,
				   REF(IUnknown*)  /*OUT*/ ppUnknown,
				   IUnknown*	    /*IN*/  pUnkOuter)
{
    ppUnknown = NULL;
    return HXR_NOINTERFACE;
}

HX_RESULT HXRemoteLoggerPlugin::ErrorOccurred(const UINT8	unSeverity,  
					      const ULONG32	ulHXCode,
					      const ULONG32	ulUserCode,
					      const char*	pUserString,
					      const char*	pMoreInfoURL)
{
    if (m_pRemoteLogger)
    {
	if (pUserString)
	{
	    int length = strlen(pUserString);
	    char* pBuf = new char[length + 2];

	    if (pBuf)
	    {
		strcpy(pBuf, pUserString);
		pBuf[length] = '\n';
		pBuf[length + 1] = '\0';

		m_pRemoteLogger->Log(pBuf);
	    }
	    delete [] pBuf;
	}
    }
    return HXR_OK;
}

/****************************************************************************
 * 
 *  Function:
 * 
 *        HXCreateInstance()
 * 
 *  Purpose:
 * 
 *        Function implemented by all plugin DLL's to create an instance of 
 *        any of the objects supported by the DLL. This method is similar to 
 *        Window's CoCreateInstance() in its purpose, except that it only 
 *        creates objects from this plugin DLL.
 *
 *        NOTE: Aggregation is never used. Therefore and outer unknown is
 *        not passed to this function, and you do not need to code for this
 *        situation.
 * 
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(HXCREATEINSTANCE)
(
    IUnknown**  /*OUT*/        ppIUnknown
)
{
    *ppIUnknown = (IUnknown*)(IHXPlugin*)new HXRemoteLoggerPlugin;
    if (*ppIUnknown)
    {
        (*ppIUnknown)->AddRef();
        return HXR_OK;
    }
    return HXR_OUTOFMEMORY;
}


/****************************************************************************
 * 
 *  Function:
 * 
 *        CanUnload()
 * 
 *  Purpose:
 * 
 *        Function implemented by all plugin DLL's if it returns HXR_OK 
 *        then the pluginhandler can unload the DLL
 *
 */
STDAPI ENTRYPOINTCALLTYPE ENTRYPOINT(CanUnload2)(void)
{
    return HXR_FAIL;
}
