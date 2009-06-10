/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: error_sink_handler.h,v 1.3 2003/09/04 22:39:08 dcollins Exp $ 
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

#ifndef _ERROR_SINK_HANDLER_H_
#define _ERROR_SINK_HANDLER_H_

#include "debug.h"
#include "proc.h"
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxslist.h"

class ErrorSinkHandler {
public:
    	    	    	    ErrorSinkHandler();

    HX_RESULT AddErrorSink	(THIS_  IHXErrorSink*	pErrorSink,
					const UINT8	unLowSeverity,
					const UINT8	unHighSeverity,
					int		nProcnum);

    HX_RESULT RemoveErrorSink	(THIS_ IHXErrorSink*	pErrorSink);  

private:
    class Sink
    {
    public:
	IHXErrorSink*	    m_pErrorSink;
	UINT8		    m_unLowSeverity;
	UINT8		    m_unHighSeverity;
	int		    m_nProcnum;
    private:
	ErrorSinkHandler*   m_pErrorSinkHandler;

	Sink(IHXErrorSink*	    pErrorSink,
	     ErrorSinkHandler*	    pErrorSinkHandler,
	     int		    nProcnum);
	~Sink();

	friend class ErrorSinkHandler;
    };

    Process*		    m_pProc;
    CHXSimpleList*	    m_pSinkList;

    friend class ErrorHandler;
};

inline
ErrorSinkHandler::Sink::Sink(
				IHXErrorSink*	    pErrorSink,
	     			ErrorSinkHandler*   pErrorSinkHandler,
	     			int		    nProcnum
	    		    )
	: m_pErrorSink	    	(pErrorSink)
	, m_pErrorSinkHandler	(pErrorSinkHandler)
	, m_nProcnum	    	(nProcnum)
	, m_unLowSeverity	(HXLOG_EMERG)
	, m_unHighSeverity    	(HXLOG_INFO)
{
    m_pErrorSink->AddRef();
}

inline
ErrorSinkHandler::Sink::~Sink()
{
    m_pErrorSink->Release();
}


inline
ErrorSinkHandler::ErrorSinkHandler()
{
    m_pSinkList = new CHXSimpleList();
}

inline HX_RESULT
ErrorSinkHandler::AddErrorSink(	IHXErrorSink* pErrorSink,
				const UINT8     unLowSeverity,
				const UINT8     unHighSeverity,
				int		nProcnum)
{
    if (pErrorSink)
    {
	Sink* pSink = new Sink(pErrorSink, this, nProcnum);
    	pSink->m_unLowSeverity = unLowSeverity;
    	pSink->m_unHighSeverity = unHighSeverity;

    	m_pSinkList->AddTail(pSink);

	return HXR_OK;
    }
    else
	return HXR_FAIL;
}

inline HX_RESULT
ErrorSinkHandler::RemoveErrorSink(IHXErrorSink* pErrorSink)
{
    LISTPOSITION pos = m_pSinkList->GetHeadPosition();
    while (pos)
    {
        Sink* pSink = (Sink*)m_pSinkList->GetAt(pos);
        if (pSink->m_pErrorSink == pErrorSink)
        {
            (void)m_pSinkList->RemoveAt(pos);
            delete pSink;
	    return HXR_OK;
        }
        (void)m_pSinkList->GetNext(pos);
    }

    return HXR_FAIL;
}

#endif // _ERROR_SINK_HANDLER_H_
