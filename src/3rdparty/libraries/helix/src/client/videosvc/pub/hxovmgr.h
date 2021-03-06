/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxovmgr.h,v 1.5 2006/02/16 23:07:09 ping Exp $
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

#ifndef _HXOVMGR_H_
#define _HXOVMGR_H_

struct CStatPoint
{
    UINT32          ulTime;
    UINT32          ulPixels;    
};

struct CSiteStats
{
    UINT32                  ulFirstTime; /* I do not even know why I need this */
    UINT32                  ulNumPixelsPerSecond;
    CHXSimpleList           samples;
    IHXOverlayResponse*    pResp;
};

class HXOverlayManager : public IHXOverlayManager,
                         public IHXCallback
{
protected:
    LONG32		    m_lRefCount;
    IUnknown*		    m_pContext;
    CHXSimpleList           m_ListOfSiteStats;
    IHXOverlayResponse*    m_pCurrentOverlayOwner;
    CallbackHandle          m_CallbackHandle;
    IHXScheduler*          m_pScheduler;
    IHXOverlayResponse*    m_pOldOverlaySite;
    IHXOverlayResponse*    m_pNewOverlaySite;
    IHXMutex*              m_pMutex;
    float                   m_fThemoStatFactor;
    HXBOOL                    m_bChangingOwner;

    void ScheduleCallback(IHXOverlayResponse* pOld, IHXOverlayResponse* pNew);
    void ValidateCurrentOwner();
    void AddStatPoint(CSiteStats* pStats, UINT32 ulNumPixels, UINT32 ulTime);

public:

    HXOverlayManager(IUnknown* pContext);
    virtual ~HXOverlayManager(void);

    void Close();
    void Initialize(); 

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

   
    /*
     *	IHXCommonClassFactory methods
     */

    STDMETHOD(HasOverlay)      (THIS_
                    IHXOverlayResponse* pResp
                    ) ;
    
    STDMETHOD(AddStats)      (THIS_
                    IHXOverlayResponse* pResp,
                    UINT32    ulNumPixels
                    ) ;

    
    STDMETHOD(RemoveOverlayRequest)(THIS_ IHXOverlayResponse* pResp ) ;

   /*
    *  IHXCallback methods
    */
   STDMETHOD(Func) (THIS);


};

#endif // _HXOVMGR_H_
