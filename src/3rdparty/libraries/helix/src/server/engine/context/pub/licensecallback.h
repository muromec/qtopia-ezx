/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: licensecallback.h,v 1.3 2003/03/05 15:38:13 dcollins Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
 
#ifndef _LICENSECALLBACK_H_
#define _LICENSECALLBACK_H_

class LicenseCallbackManager : public IHXLicenseCallback,
                               public IHXPropWatchResponse
{
public:
    // Constructors
    LicenseCallbackManager(Process* pProc);

    // Destructors
    ~LicenseCallbackManager();

public:
    //Operations
    //IUnknown Methods
    STDMETHOD_(UINT32, AddRef) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);
    STDMETHOD (QueryInterface) (THIS_ REFIID riid, void** ppvObj);


    //IHXLicenseCallback Methods
    STDMETHOD(Register) (THIS_ 
        IHXCallback* pCallback);

    STDMETHOD(DeRegister) (THIS_
        IHXCallback* pCallback);

    //IHXPropWatchResponse Methods
    STDMETHOD(AddedProp)	(THIS_
		const UINT32		ulId,
		const HXPropType   	propType,
		const UINT32		ulParentID);

    STDMETHOD(ModifiedProp)	(THIS_
		const UINT32		ulId,
		const HXPropType   	propType,
		const UINT32		ulParentID);

    STDMETHOD(DeletedProp)	(THIS_
		const UINT32		ulId,
		const UINT32		ulParentID);

private:
    UINT32 m_lRefCount;
    CHXSimpleList* m_pCBList;
    IHXPropWatch* m_pWatch;
    IHXRegistry* m_pRegistry;
};

#endif
