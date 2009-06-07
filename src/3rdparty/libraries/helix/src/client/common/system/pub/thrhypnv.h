/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: thrhypnv.h,v 1.1 2007/04/14 04:37:59 ping Exp $
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

#ifndef _HXTHREADHYPERNAVIGATE_
#define _HXTHREADHYPERNAVIGATE_

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
struct IUnknown;
class  HXThread;
class  HXMutex;
class  HXHyperNavigate;
class  CHXSimpleList;

struct HyperCommand
{
    HyperCommand()
    {
	m_Type	    = 0;
	m_pURL	    = NULL;
	m_pTarget   = NULL;
    };
    
    ~HyperCommand()
    {
	HX_VECTOR_DELETE(m_pURL);
	HX_VECTOR_DELETE(m_pTarget);
    };

    UINT16  m_Type;
    char*   m_pURL;
    char*   m_pTarget;
};

class HXThreadHyperNavigate : public IHXHyperNavigate,
			       public IHXHyperNavigate2	
{
public:

		HXThreadHyperNavigate();
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

    void		    Stop(void);

    void		    UseThread(HXBOOL bUseThread = TRUE);

protected:

    ~HXThreadHyperNavigate();

    void		    StartHyperThread();
    void		    StopHyperThread();

    LONG32		    m_lRefCount;
    IUnknown*		    m_pContext;
    HX_BITFIELD		    m_bInitialized : 1;
    HX_BITFIELD		    m_bUseThread : 1;

    /* Used to schedule events in a separate thread */
    friend void* HyperThreadRoutine (void * pArg);

    IHXThread*		m_pThread;
    IHXEvent*		m_pQuitEvent;
    HXHyperNavigate*	m_pHyperNavigate;

};


#endif /* _HXTHREADHYPERNAVIGATE_ */
