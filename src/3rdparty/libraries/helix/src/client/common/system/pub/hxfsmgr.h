/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxfsmgr.h,v 1.6 2007/07/06 21:58:04 jfinnecy Exp $
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

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXFileSystemManager
 * 
 *  Purpose:
 * 
 *      Gives out File Objects based on URLs
 * 
 *  IID_IHXFileSystemManager:
 *  
 *	{00000207-61DF-11d0-9CEE-080017035B43}
 * 
 */

#ifndef _RMAFSMANAGER
#define _RMAFSMANAGER

struct IHXFileSystemManager;
struct IHXFileSystemManagerResponse;
struct IHXGetFileFromSamePoolResponse;
struct IHXScheduler;

typedef enum
{
    e_None = 0,
    e_GetFileObjectPending = 1,
    e_GetRelativeFileObjectPending = 2
} PendingState;


class HXFileSystemManager : public IHXFileSystemManager,
			     public IHXGetFileFromSamePoolResponse
{
private:
    LONG32			    m_lRefCount;
    IHXFileSystemManagerResponse*  m_pFSManagerResponse;
    IHXGetFileFromSamePool*        m_pSamePool;
    IUnknown*			    m_pContext;

    IHXRequest*		    m_pRequest;

    ~HXFileSystemManager();

public:

    HXFileSystemManager(IUnknown* pContext);
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *	IHXFileSystemManager methods
     */

    /************************************************************************
     *	Method:
     *	    IHXFileSystemManager::Init
     *	Purpose:
     */
    STDMETHOD(Init) (THIS_
		    IHXFileSystemManagerResponse* /*IN*/  pFileManagerResponse
		    );

    STDMETHOD(GetFileObject)     (THIS_
				  IHXRequest* pRequest,
				  IHXAuthenticator* pAuthenticator);
    STDMETHOD(GetNewFileObject)  (THIS_
				  IHXRequest* pRequest,
				  IHXAuthenticator* pAuthenticator);

    STDMETHOD(GetRelativeFileObject) (THIS_
				      IUnknown* pOriginalObject,
				      const char* pRelativePath);


    STDMETHOD(GetDirObjectFromURL)	(THIS_
                                        const char* pURL);

    STDMETHOD(FileObjectReady)          (THIS_
					 HX_RESULT status,
					 IUnknown* pUnknown);

    // functions to mainatin the list file options of the file system. 

    static void		InitMountPoints(IUnknown* pContext);
    static HX_RESULT	AddMountPoint(	    const char* pszShortName,
					    const char* pszMountPoint,
					    IHXValues* pOptions,
					    IUnknown*	pContext);
    static IHXValues*	GetOptionsGivenURL(const char* pURL);

#if !defined(HELIX_CONFIG_NOSTATICS)
    static HXBOOL			zm_IsInited;
    static CHXMapStringToOb	zm_ShortNameMap;
    static CHXMapStringToOb	zm_ProtocolMap;
    static CHXSimpleList	zm_CacheList;
#else
    static const HXBOOL		         zm_IsInited;
    static const CHXMapStringToOb* const zm_ShortNameMap;
    static const CHXMapStringToOb* const zm_ProtocolMap;
    static const CHXSimpleList* const    zm_CacheList;
#endif

    class RMAFSManagerCallback : public IHXCallback
    {
    public:
	HXFileSystemManager*	m_pFSManager;
	CallbackHandle		m_Handle;
	HXBOOL			m_bIsCallbackPending;


			    RMAFSManagerCallback(HXFileSystemManager*	pFSManager);
	/*
	 *	IUnknown methods
	 */
	STDMETHOD(QueryInterface)	(THIS_
					REFIID riid,
					void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)	(THIS);

	STDMETHOD_(ULONG32,Release)	(THIS);

	/*
	 *	IHXCallback methods
	 */
	STDMETHOD(Func)		(THIS);

    protected:
			    ~RMAFSManagerCallback();

	LONG32		m_lRefCount;
    };
    friend class RMAFSManagerCallback;

    RMAFSManagerCallback*   m_pCallback;
    IHXScheduler*	    m_pScheduler;
    PendingState	    m_State;
    IUnknown*		    m_pOriginalObject;
    char*		    m_pRelativePath;

    HX_RESULT ProcessGetRelativeFileObjectPending();
    HX_RESULT ProcessGetFileObjectPending();
    void      ProcessPendingRequest();
};


/*  
    this class is used to cache the IHXValues which were obatined from 
    the server config file. 
*/

class CCacheInstance
{
public:
    IUnknown*	pInstance;
    CHXString	m_mount_point;
    CHXString	m_szProtocol;
    CHXString	m_szShortName;
    IHXValues*	m_pOptions;
};

#endif /*_RMAFSMANAGER*/
