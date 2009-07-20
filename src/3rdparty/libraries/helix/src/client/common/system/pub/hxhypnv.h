/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxhypnv.h,v 1.2 2007/07/06 21:58:04 jfinnecy Exp $
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

#ifndef _HXHYPERNAVIGATE_
#define _HXHYPERNAVIGATE_

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

struct IHXHyperNavigate;

class HXHyperNavigate : public IHXHyperNavigate,
			 public IHXHyperNavigate2
{
public:

		HXHyperNavigate();
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

    /* Internal function */
    STDMETHOD(Init)	    (THIS_
			    IUnknown* pContext);
			    
    /************************************************************************
     *	Non-interface methods for use by Player on PLAYER_8_0_PREVIEW branch only
     */

    inline HXBOOL GetKeepTargetBehind(void) { return m_bKeepTargetBehind; }
    inline void SetKeepTargetBehind(HXBOOL bBehind) { m_bKeepTargetBehind = bBehind; }
	
	
protected:

    ~HXHyperNavigate();

    HXBOOL LaunchBrowserWithURL(const char* pURL, const char* pDefBrowser = NULL);
    
    LONG32		    m_lRefCount;
    IUnknown*               m_pContext;
    IHXPreferences*	    m_pPreferences;
    HXBOOL		    m_bInitialized;
    UINT32		    m_nLastLaunchTime;
    char*		    m_pLastURL;
    HXBOOL		    m_bKeepTargetBehind;
};


#endif /* _HXHYPERNAVIGATE_ */
