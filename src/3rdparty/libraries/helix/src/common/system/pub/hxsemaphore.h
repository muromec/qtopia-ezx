/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef HX_SEMAPHORE_H__
#define HX_SEMAPHORE_H__

class HXEvent;
class HXMutex;

// This provides a default semaphore implementation based on HXEvent and HXMutex. It
// is indended for cases where a platform-specific implementation is not available.
// Consider writing and using a platform-specific implementation for sake of better
// performance.
//

class HXSemaphore 
{
private:
   
public:
    static HX_RESULT MakeSemaphore(HXSemaphore*& pOut, IUnknown* pContext, UINT32 count = 0, const char* pName = NULL);
    ~HXSemaphore();

	void Signal(UINT32 count = 1);
	HX_RESULT Wait(UINT32 msWait = ALLFS); 
	UINT32 GetCount() const;

private:
    HXSemaphore();	
	
    HX_RESULT Init(IUnknown* pContext, UINT32 count, const char* pName);

	HXSemaphore& operator=( HXSemaphore & );

	IHXEvent*	m_pEvent;
	UINT32		m_count;
	IHXMutex*	m_pMutex;
};



#endif // HX_SEMAPHORE_H__
