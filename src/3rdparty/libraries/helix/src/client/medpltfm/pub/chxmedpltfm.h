/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxmedpltfm.h,v 1.17 2007/04/14 04:38:51 ping Exp $
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

#ifndef _CHXMEDPLTFM_H_
#define _CHXMEDPLTFM_H_

#include "unkimp.h"
#include "ihxmedpltfm.h"
#include "ihxobjectmanagerprivate.h"
#include "hxccf.h"
#include "hxslist.h"
#include "hxprefs.h"
#include "hxsched.h"
#include "hxclreg.h"
#include "hxnet.h"
#include "hxnetif.h"
#include "hxwintyp.h"
#include "hxwin.h"
#include "hxplugn.h"
#include "hxhyper.h"
#include "thrhypnv.h"
#include "chxmedpltfmkicker.h"
#include "hxthreadsync.h"

#if defined(_STATICALLY_LINKED) || !defined(HELIX_FEATURE_PLUGINHANDLER2)
#if defined(HELIX_CONFIG_CONSOLIDATED_CORE)
#include "basehand.h"
#else /* HELIX_CONFIG_CONSOLIDATED_CORE */
#include "hxpluginmanager.h"
#endif /* HELIX_CONFIG_CONSOLIDATED_CORE */
#else
#include "plghand2.h"
#endif /* _STATICALLY_LINKED */

struct PluginPathInfo
{
    char*	pszName;
    IHXBuffer*	pPath;

    PluginPathInfo()
    {
	pszName = NULL;
	pPath = NULL;
    };

    ~PluginPathInfo()
    {
	HX_VECTOR_DELETE(pszName);
	HX_RELEASE(pPath);
    };
};

class CHXMediaPlatform : public CUnknownIMP
		       , public IHXMediaPlatform
		       , public IHXCommonClassFactory
		       , public IHXObjectManagerPrivate
{
    // The IUnknown implementation declaration
    DECLARE_UNKNOWN( CHXMediaPlatform )

protected:

    typedef enum
    {
	HX_MP_COM_INSTRINSIC = 0,
	HX_MP_COM_REGULAR
    } HX_MP_COM_TYPE;

    HXBOOL		    m_bInitialized;
    HX_RESULT		    m_lastError;

    IUnknown*		    m_pExtContext;
    IUnknown*		    m_pPluginHandlerUnkown;
    IHXCommonClassFactory*  m_pExtCCF;

    // default services
    IHXPreferences*	    m_pPreferences;
    IHXScheduler*	    m_pScheduler;
    IHXScheduler2*	    m_pScheduler2;
    IHXOptimizedScheduler*  m_pOptimizedScheduler;
    IHXOptimizedScheduler2* m_pOptimizedScheduler2;
    IHXMutex*		    m_pMutex;
    IHXRegistry*	    m_pRegistry;
    IHXNetServices*	    m_pNetServices;
    IHXHyperNavigate*       m_pHyperNavigate;
    CHXMediaPlatformKicker* m_pKicker;
    HXNetInterface*         m_pNetInterfaces;
#if defined(HELIX_FEATURE_VIDEO)    
    IHXSiteEventHandler*    m_pSiteEventHandler;
#endif	// HELIX_FEATURE_VIDEO
#if defined(HELIX_FEATURE_REGISTRY)
    HXClientRegistry*	    m_pClientRegistry;
#endif /* HELIX_FEATURE_REGISTRY */
#if defined(HELIX_FEATURE_HYPER_NAVIGATE)
    HXThreadHyperNavigate*  m_pDefaultHyperNavigate;
#endif /* HELIX_FEATURE_HYPER_NAVIGATE */

    IHXScheduler*	    m_pExtScheduler;
    IHXMediaPlatformKicker* m_pExtKicker;

    CHXSimpleList*	    m_pPluginPaths;
    CHXMediaPlatform*	    m_pParent;
    CHXMediaPlatform*	    m_pRoot;
    CHXSimpleList*	    m_pChildren;
    HXLockKey		    m_PluginHandlerLockKey;
    CHXSimpleList*	    m_pSingleLoadPlugins;
    CHXSimpleList*          m_pLoadAtStartupPlugins;

    HX_RESULT		    InitBasicServices(void);
    HX_RESULT               LoadStartupPlugins(void);
    void                    UnloadStartupPlugins(void);
    HX_RESULT		    InitExtendableServices(void);
    HX_RESULT		    CheckAndQueryInterface(HX_MP_COM_TYPE type, REFIID riid, void** ppvObj);
    HX_RESULT		    CheckAndCreateInstance(HX_MP_COM_TYPE type, REFCLSID rclsid, void** ppUnknown);

    virtual HX_RESULT	    CreateIntrinsicType(REFCLSID rclsid, REF(IUnknown*) pUnknown, IUnknown* pOuter);
    virtual HX_RESULT	    CreateGeneralType(REFCLSID clsid, REF(IUnknown*) pObject, IUnknown* pUnkOuter, IUnknown* pContext);

    HX_RESULT		    CreateInstanceFromPluginHandler(REFCLSID rclsid, void** ppUnknown, IUnknown *pUnkOuter, IUnknown* pContext);

    HX_RESULT		    InitDefaultPreferences(void);

    HX_RESULT		    _InternalQI(REFIID riid, void** ppvObj);

public:
    static CHXMediaPlatform* CreateInstance(CHXMediaPlatform* pParent = NULL, 
					    CHXMediaPlatform* pRoot = NULL);

    CHXMediaPlatform(CHXMediaPlatform* pParent = NULL, CHXMediaPlatform* pRoot = NULL);
    ~CHXMediaPlatform(void);

    /*
     *  IHXMediaPlatform methods     
     *  see common/include/ihxmedpltfm.h for detail description
     */
    STDMETHOD(GetVersion)	(THIS_
			         UINT32* pVersion);

    STDMETHOD(AddPluginPath)	(THIS_
				 const char* pszName,
				 const char* pszPath);

    STDMETHOD(Init)		(THIS_
				 IUnknown* pContext);

    STDMETHOD(Close)		(THIS);

    STDMETHOD(Reset)		(THIS_
				 IUnknown* pContext,
				 HXBOOL	   bPlatformOnly = TRUE);

    STDMETHOD(Purge)		(THIS);

    STDMETHOD(CreateChildContext)   (THIS_
				     IHXMediaPlatform** ppChildContext);

    /*
     *	IHXCommonClassFactory methods
     */
    STDMETHOD(CreateInstance)		    (THIS_
					     REFCLSID	rclsid,
					     void**	ppUnknown);

    STDMETHOD(CreateInstanceAggregatable)   (THIS_
					     REFCLSID	    rclsid,
					     REF(IUnknown*) ppUnknown,
					     IUnknown*	    pUnkOuter);

    // IHXObjectManagerPrivate methods
    STDMETHOD(ObjectFromCLSIDPrivate) (THIS_ REFCLSID clsid,REF(IUnknown *)pObject, 
						IUnknown *pUnkOuter, IUnknown* pContext );

    STDMETHOD( UnloadPluginPrivate )(THIS_ REFCLSID clsid);
};

#endif /* _CHXMEDPLTFM_H_ */
