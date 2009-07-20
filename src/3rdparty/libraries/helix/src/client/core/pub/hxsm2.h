/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxsm2.h,v 1.7 2006/01/30 21:09:41 ping Exp $
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
#ifndef _HXSM2_H_
#define _HXSM2_H_

#include "hxtypes.h"
#include "hxcom.h"
#include "hxsmbw.h"
#include "hxslist.h"
#include "ihxpckts.h"

class HXSM2 : public IHXBandwidthManager
{
public:
    HXSM2();
    ~HXSM2();

    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /*
     *	IHXBandwidthManager methods
     */
    STDMETHOD(RegisterSource)(THIS_
			      HXSource* pSource,
			      IUnknown* pUnknown);

    STDMETHOD(RegisterSourcesDone)	(THIS);
    
    STDMETHOD_(HXBOOL, NotEnoughBandwidth)(THIS);

    STDMETHOD(UnRegisterSource)	(THIS_
				HXSource* pSource);

    /* If the source has enough data, it may tell the bandwidth
     * manager to cut down on accelerated buffering.
     */
    STDMETHOD(ChangeAccelerationStatus)	(THIS_
				HXSource* pSource,
				HXBOOL	   bMayBeAccelerated,
				HXBOOL	   bUseAccelerationFactor,
				UINT32	   ulAccelerationFactor);

    /* Called by HXPlayer at end of each presentation */
    STDMETHOD(PresentationDone)	(THIS);
    
    STDMETHOD(ChangeBW) (THIS_ 
		UINT32 newBW, HXSource* pSource);

    STDMETHOD(GetUpshiftBW) (THIS_ REF(UINT32) uUpshiftBW);

private:
    UINT32 GetStreamCount();

    void Recalc();

    UINT32 m_lRefCount;

    UINT32 m_ulNumSources;
    CHXSimpleList* m_pASMSourceInfo;

    UINT32 m_ulNumStreams;

    UINT32 m_ulOfferToRecalc;
    UINT32 m_ulSelectionBitRate;
    UINT32 m_ulMaxAccelBitRate;
    UINT32 m_ulSustainableBitRate;

    IHXValues* m_pSubscriptionVariables;
    HXBOOL m_bCheckOnDemandBw;
    IUnknown* m_pContext;
};

#endif /* _HXSM2_H_ */
