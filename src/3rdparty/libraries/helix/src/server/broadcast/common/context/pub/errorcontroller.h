/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: errorcontroller.h,v 1.2 2003/01/23 23:42:48 damonlan Exp $ 
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

#ifndef _ERRORCONTROLLER_H__
#include "hxtypes.h"
#include "hxcom.h"
#include "hxslist.h"
#include "hxerror.h"

class CErrorcontroller : public IHXErrorMessages, public IHXErrorSinkControl
{
public:
	CErrorcontroller()
	{
		m_lRefCount = 0;
	};

	~CErrorcontroller()
	{
		Close();
	};

	STDMETHOD(Close)();

    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32, AddRef)	(THIS);
    STDMETHOD_(ULONG32, Release)(THIS);

    /*
     *  IHXErrorSinkControl methods
     */
    STDMETHOD(AddErrorSink)(THIS_ IHXErrorSink*	pErrorSink,	
                             const UINT8     unLowSeverity,
                             const UINT8     unHighSeverity);


    STDMETHOD(RemoveErrorSink)(THIS_ IHXErrorSink* pErrorSink);


    /*
     *  IHXErrorMessages methods
     */


    STDMETHOD(Report)(THIS_ const UINT8	unSeverity,  
					  HX_RESULT	ulHXCode,
					  const ULONG32	ulUserCode,
					  const char*	pUserString,
					  const char*	pMoreInfoURL
					 );

    STDMETHOD_(IHXBuffer*, GetErrorText)	(THIS_
						HX_RESULT	ulHXCode);


private:
    LONG32			m_lRefCount;

	struct SErrorSinkInfo
	{
		IHXErrorSink *pErrorSink;
		UINT8	unLowSeverity;
		UINT8	unHighSeverity;
	};

	CHXSimpleList m_pErrorSinkListInfo;
};

#endif /*_ERRORCONTROLLER_H__*/

