/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: playhpnv.h,v 1.6 2007/07/06 21:58:16 jfinnecy Exp $
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

#ifndef _RPHYPERNAVIGATE_
#define _RPHYPERNAVIGATE_

_INTERFACE IHXGroupManager;
_INTERFACE IHXRequest;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXHyperNavigate
 * 
 *  Purpose:
 * 
 *	TBD
 * 
 *  IID_IHXHyperNavigate:
 * 
 *	{00000900-61DF-11d0-9CEE-080017035B43}
 * 
 */

struct	IHXHyperNavigate;
struct	IHXHTTPRedirectResponse;

class PlayerHyperNavigate : public IHXHyperNavigate,
			    public IHXHyperNavigate2,
			    public IHXHyperNavigateHint,
			    public IHXFileSystemManagerResponse,
			    public IHXHTTPRedirectResponse,
			    public IHXCallback
{
public:

    PlayerHyperNavigate();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

    /*
     *	IHXHyperNavigate methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigate::GoToURL
     *	Purpose:
     *	    Performs a simple Go To URL operation.
     */
    STDMETHOD(GoToURL)	    (THIS_
			    const char* pURL,
			    const char* pTarget);

    /************************************************************************
     *	Method:
     *	    IHXHyperNavigate2::Execute
     *	Purpose:
     *	    
     *	Parameters:
     *      pURL:	    URL (absolute or relative)
     *	    pTargetInstance:	
     *	    pTargetApplication: 
     *	    pTargetRegion:
     *	    pParams:
     */
    STDMETHOD(Execute)	    (THIS_
			    const char* pURL,
			    const char* pTargetInstance,
			    const char* pTargetApplication,
			    const char* pTargetRegion,
			    IHXValues* pParams);


    /************************************************************************
     *	Method:
     *	    IHXHyperNavigateHint::Hint
     *	Purpose:
     *	    
     *	Parameters:
     *      pURL:	    URL (absolute or relative)
     *	    pTarget:	    target for URL
     *	    pParams:        parameters on URL. Currently supported keys include:
     *                           "width":  desired width of target window (ULONG32)
     *                           "height": desired height of target window (ULONG32)
     *                           "begin":  time at which URL will be fired (ULONG32)
     *                                     (0xFFFFFFFF if not known)
     */
    STDMETHOD(Hint) (THIS_ const char* pURL,
                           const char* pTarget,
                           IHXValues*  pParams);

    HX_RESULT ExecuteWithContext    (const char* pURL,
				    const char* pTargetInstance,
				    const char* pTargetApplication,
				    const char* pTargetRegion,
				    IHXValues* pParams,
				    IUnknown*	pContext);

    /* Internal function */
    STDMETHOD(Init)	    (THIS_
			    IUnknown* pContext,
			    IHXHyperNavigate *pHyperNavigate, 
			    IHXHyperNavigateWithContext* pHyperNavigateWithContext);

    void     Close(void);

    /*
     *	IHXHTTPRedirectResponse methods
     */

    /************************************************************************
     *	Method:
     *	    IHXHTTPRedirectResponse::RedirectDone
     *	Purpose:
     *	    return the redirect URL
     */
    STDMETHOD(RedirectDone)		(THIS_
					 IHXBuffer* pURL);
    /*
     *	IHXFileSystemManagerResponse methods
     */

    /************************************************************************
     *	Method:
     *	IHXFileSystemManagerResponse::InitDone
     *	Purpose:
     */
    STDMETHOD(InitDone)	    (THIS_
			    HX_RESULT status);

    STDMETHOD(FileObjectReady)	(THIS_
				HX_RESULT status,
                                IUnknown* pObject);

    /*
     * The following method is deprecated and should return HXR_NOTIMPL
     */

    STDMETHOD(DirObjectReady)	(THIS_
				HX_RESULT status,
                                IUnknown* pDirObject);

    /*
     *	IHXCallback methods
     */
    STDMETHOD(Func)		(THIS);


protected:

    ~PlayerHyperNavigate();

    HXBOOL		m_bInitialized;
    LONG32		m_lRefCount;

    IUnknown*		m_pContext;
    IHXPlayer*		m_pPlayer;
    IHXGroupManager*	m_pGroupManager;
    IHXFileObject*	m_pFileObject;
    IHXHyperNavigate*	m_pHyperNavigate;
    IHXHyperNavigateWithContext* m_pHyperNavigateWithContext;
    IHXScheduler*	m_pScheduler;
    CallbackHandle	m_CallbackHandle;
    IHXRequest*	m_pPendingRequest;

    HX_RESULT	SendAdsCookies(char* pURL);
    HX_RESULT	HandleCommands(const char* pURL, const char* pTarget, IHXValues* pParams);
};


#endif /* _RPHYPERNAVIGATE_ */
