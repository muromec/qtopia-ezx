/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fastfile_factory.h,v 1.5 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _FASTFILE_FACTORY_H_
#define _FASTFILE_FACTORY_H_

#include "hxcom.h"
#include "hxtypes.h"
#include "hxfiles.h"

class Dict;
class CDistMIIStatistics;

//////////////////////////////////////////////////////////////////////
// FastFileFactory
//////////////////////////////////////////////////////////////////////

class FastFileFactory : public IHXFastFileFactory
                      , public IHXFastFileFactory2
{
public:
    FastFileFactory(IUnknown* pContext);

    /*
     *	IUnknown methods
     */
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID		  /*IN*/  riid,
    	    	    	    	void**		  /*OUT*/ ppvObj);

    /*
     * IHXFastFileFactory methods
     */

    STDMETHOD(Wrap)		(THIS_
				 REF(IUnknown*) /*OUT*/ pWrapper,
				 IUnknown*      /*IN*/  pFileObj,
				 UINT32         /*IN*/  ulBlockSize,
                                 BOOL           /*IN*/  bAlignReads,
                                 BOOL           /*IN*/  bCacheStats);

    STDMETHOD(Wrap)		(THIS_
				 REF(IUnknown*) /*OUT*/ pWrapper,
				 IUnknown*      /*IN*/  pFileObj,
				 UINT32         /*IN*/  ulBlockSize,
                                 BOOL           /*IN*/  bAlignReads,
                                 BOOL           /*IN*/  bCacheStats,
                                 UINT32         /*IN*/  ulMaxBlockSize);

    friend class FastFile;

private:
    ~FastFileFactory();

    ULONG32		m_ulRefCount;
    IUnknown* 		m_pContext;
    Dict*		m_pFastFileDict;
    CDistMIIStatistics* m_pMIIStats;
    BOOL                m_bUseWrap2;
    UINT32              m_ulMaxMemUse;
};


#endif /* _FASTFILE_FACTORY_H_ */
