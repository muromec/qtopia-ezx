/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: errhand.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _ERRHAND_H_
#define _ERRHAND_H_

#include "hxerror.h"
#include "ihxpckts.h"
#include "simple_callback.h"

class ErrorHandler: public IHXErrorMessages
{
public:
    ErrorHandler(Process* pProc);
    ~ErrorHandler();

    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);
    STDMETHOD(Report)           (THIS_
                                const UINT8     unSeverity,
                                HX_RESULT       ulHXCode,
                                const ULONG32   ulUserCode,
                                const char*     pUserString,
                                const char*     pMoreInfoURL
                                );
    STDMETHOD_(IHXBuffer*, GetErrorText) (THIS_ HX_RESULT ulHXCode);

private:
    class ErrorCallback : public SimpleCallback
    {
    public:
	void                func(Process* proc);
	IHXErrorSink*	    m_pErrSink;
	UINT8		    m_unSeverity;
	ULONG32		    m_ulHXCode;
	ULONG32		    m_ulUserCode;
	IHXBuffer*	    m_pUserString;
	IHXBuffer*	    m_pMoreInfoURL;

    private:
			    ~ErrorCallback();

    friend class ErrorHandler;

    };

    LONG32		m_lRefCount;
    Process*		m_pProc;
};

#endif /* _ERRHAND_H_ */
