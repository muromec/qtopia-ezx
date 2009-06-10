/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: preftran.h,v 1.13 2007/07/06 21:58:00 jfinnecy Exp $
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

#ifndef _PREFTRAN_H_
#define _PREFTRAN_H_

#include "hxpreftr.h"

class HXNetInterface;
class HXPreferredTransportManager;
class HXPreferredTransport;
class HXSubnetManager;
class HXEvent;

class HXPreferredTransport : public IHXPreferredTransport
{
private:
    LONG32			    m_lRefCount;
    
    CHXString*			    m_pHost;
    HXBOOL			    m_bHTTPNG;
    UINT32			    m_ulHost;
    UINT32			    m_ulParentPlaybacks;
    UINT16			    m_uPlaybacks;
    UINT16			    m_uCloakPort;
    UINT8                           m_uTransportAttempted;
    time_t			    m_lastUsedTime;
    PreferredTransportState	    m_state;
    PreferredTransportClass	    m_prefTransportClass;
    PreferredTransportProtocol	    m_prefTransportProtocol;
    TransportMode		    m_prefTransportType;
    UINT32                          m_ulABD;
    HXPreferredTransportManager*    m_pOwner;
    CHXSimpleList*		    m_pPrefTransportSinkList;

#if 0
    void    myTestSuite();
    void    myTest(UINT32 ulTestCase, HX_RESULT theError, HXPreferredTransport* pTransport);
#endif

    friend class HXPreferredTransportManager;

    ~HXPreferredTransport();


protected:  

public:
    HXPreferredTransport(HXPreferredTransportManager* pOwner);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPreferredTransport methods
     */
    STDMETHOD_(PreferredTransportState, GetState) (THIS);

    STDMETHOD_(PreferredTransportClass, GetClass) (THIS);

    STDMETHOD(GetTransport)		(REF(TransportMode) /* OUT */   prefTransportType,
					REF(UINT16)	    /* OUT */   uCloakPort);
    
    STDMETHOD(SetTransport)		(TransportMode	    /* IN  */   prefTransportType,
					UINT16		    /* IN  */   uCloakPort);

    STDMETHOD(SwitchTransport)		(HX_RESULT	    /* IN  */   error,
					REF(TransportMode)  /* INOUT */ prefTransportType);

    STDMETHOD(RemoveTransport)		(THIS);

    STDMETHOD(AbortTransport)       (THIS);

    STDMETHOD_(HXBOOL, ValidateTransport) (TransportMode	    /* IN */	prefTransportType);

    STDMETHOD(AddTransportSink)		(IHXPreferredTransportSink*	/* IN  */   pPrefTransportSink);

    STDMETHOD(RemoveTransportSink)	(IHXPreferredTransportSink*	/* IN  */   pPrefTransportSink);

    STDMETHOD_(HXBOOL, GetHTTPNG)		(THIS);

    STDMETHOD(SetHTTPNG)		(HXBOOL bHTTPNG);

    STDMETHOD(SetAutoBWDetectionValue)  (THIS_
                                         UINT32         ulBW);

    STDMETHOD(GetAutoBWDetectionValue)  (THIS_
                                         REF(UINT32)    ulBW);    

    void	    Initialize(void);
    void	    Close(void);
};

class HXPreferredTransportManager : public IHXPreferredTransportManager,
			            public IHXNetInterfacesAdviseSink
{
private:
    LONG32		    m_lRefCount;
    IUnknown*		    m_pContext;

    HX_BITFIELD		    m_bInitialized : 1;
    HX_BITFIELD		    m_bSave : 1;
    HX_BITFIELD		    m_bDisableUDP : 1;
    char*		    m_pszFile;
    UINT32		    m_ulRTSPTransportMask;
    UINT32		    m_ulPNMTransportMask;
    UINT32		    m_ulLocalHost;
    UINT32		    m_ulSubnetMask;
    UINT32		    m_ulSubnet;
    UINT32		    m_ulPlaybacks;
    UINT32                  m_ulMasterABD;
    time_t		    m_lastRTSPPreferencesModifiedTime;
    time_t		    m_lastPNMPreferencesModifiedTime;

    TransportMode	    m_internalTransportType;
    TransportMode	    m_externalTransportType;
    TransportMode	    m_rtspTransportTypeStartWith;
    TransportMode	    m_pnmTransportTypeStartWith;
    HXSubnetManager*	    m_pSubnetManager;
    CHXSimpleList*	    m_pPrefHostTransportList;
    CHXSimpleList*	    m_pPrevPrefHostTransportList;
#if defined(HELIX_FEATURE_PROXYMGR)
    IHXProxyManager*	    m_pProxyManager;
#else
    void*		    m_pProxyManager;
#endif /* HELIX_FEATURE_PROXYMGR */
    IHXPreferences*	    m_pPreferences;
    HXNetInterface*	    m_pHXNetInterface;

#ifdef _WINDOWS
    IHXEvent*		    m_pLock;
#elif _UNIX
    int			    m_fileID;
#endif /* _WINDOWS */

    ~HXPreferredTransportManager();


    friend class HXPreferredTransport;

protected:  

    HX_RESULT	    _Initialize(void);
    HX_RESULT	    ReadPreferences(HXBOOL  bRTSPProtocol, UINT32& ulTransportMask);
    void	    TransportSet(HXPreferredTransport* pPreferredTransport, HXBOOL bSave);

    HX_RESULT	    CollectNetworkInfo(void);
    HX_RESULT	    PrepPrefTransport(void);
    HX_RESULT	    OpenPrefTransport(void);
    HX_RESULT	    SavePrefTransport(void);
    void	    ResetPrefTransport(CHXSimpleList* pPrefHostTransportList);

    HX_RESULT	    FileReadLine(FILE* fp, char* pLine, UINT32 ulLineBuf, UINT32* pBytesRead);    
    HX_RESULT	    FileWriteLine(FILE* fp, HXPreferredTransport* pPrefTransport);
    HX_RESULT	    FileWriteClass(FILE* fp, 
 				   PreferredTransportClass prefTransportClass, 
				   TransportMode transportType,
				   PreferredTransportProtocol protocol,
				   UINT32 ulTransportMask,
				   time_t lastModifiedTime,
                                   UINT32 ulABD);
    
    HX_RESULT	    SwitchTransport(HX_RESULT error, 
 				    HXPreferredTransport* pPrefTransport,
 				    REF(TransportMode) prefTransportType);

    HX_RESULT	    UpShiftTransport(HXPreferredTransport* pPrefTransport,
				     REF(TransportMode) prefTransportType);

    HXBOOL	    ValidateTransport(HXPreferredTransport* pPrefTransport,
				      TransportMode prefTransportType);

    CHXString*	    GetMasterDomain(const char* pszHostName);
    TransportMode   GetTransportPreferred(HXPreferredTransport* pPrefTransport);
    TransportMode   GetHigherTransport(TransportMode mode1, TransportMode mode2);
    TransportMode   GetLowerTransport(TransportMode mode1, TransportMode mode2);
    PreferredTransportClass GetTransportClass(const char* pszHostName, UINT32 ulHostAddress);
    void	    InitTransportTypeStartWith(UINT32 ulTransportMask, TransportMode& transportStartWith); 

public:
    HXPreferredTransportManager(IUnknown* pContext);

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXPreferredTransportManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::Initialize
     *	Purpose:
     *	    Initialize the transport manager such as re-reading the preferences
     */
    STDMETHOD(Initialize)		(THIS);

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::GetPrefTransport
     *	Purpose:
     *	    Get preferred host transport
     */
    STDMETHOD(GetPrefTransport)		(const char*			/* IN  */ pszHostName,
					PreferredTransportProtocol	/* IN  */ prefTransportProtocol,
					REF(IHXPreferredTransport*)	/* OUT */ pPrefTransport);

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::RemovePrefTransport
     *	Purpose:
     *	    Remove preferred host transport
     */
    STDMETHOD(RemovePrefTransport)	(IHXPreferredTransport*	/* IN  */ pPrefTransort);

    /************************************************************************
     *	Method:
     *	    IHXPreferredTransportManager::GetTransportPreference
     *	Purpose:
     *	    Get transport preference set by the user
     */
    STDMETHOD(GetTransportPreference)	(PreferredTransportProtocol	/* IN  */ prefTransportProtocol,
					 REF(UINT32)			/* OUT */ ulPreferenceMask);

    STDMETHOD(SetAutoBWDetectionValue)  (THIS_
                                         UINT32         ulBW);

    STDMETHOD(GetAutoBWDetectionValue)  (THIS_
                                         REF(UINT32)    ulBW);    

    /*
     *	IHXNetInterfacesAdviseSink methods
     */
    STDMETHOD(NetInterfacesUpdated)	(THIS);

    void	Close(void);
};

#endif /* _PREFTRAN_H_ */

