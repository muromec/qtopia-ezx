/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: source_finder.h,v 1.4 2004/05/03 19:02:49 tmarshall Exp $ 
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

#ifndef _SOURCE_FINDER_H_
#define _SOURCE_FINDER_H_

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxfiles.h"
#include "hxsrc.h"
#include "source.h"
#include "fsmanager.h"

class CServerSourceFinder : public IHXSourceFinderObject,
    public IHXFileSystemManagerResponse,
    public IHXBroadcastMapperResponse,
    public IHXFileMimeMapperResponse
{
public:
    CServerSourceFinder(Process* proc);

	/* IHXUnknown methods */

    STDMETHOD(QueryInterface)		(THIS_
					REFIID riid,
					void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)		(THIS);

    STDMETHOD_(ULONG32,Release)		(THIS);

	/* IHXSourceFinderObject methods */

    STDMETHOD(Init)			(THIS_ 
					IUnknown*	pUnknown);

    STDMETHOD(Find)			(THIS_
					IHXRequest* 	pRequest);

    STDMETHOD(Done)			(THIS);

	/* IHXFileSystemManagerResponse methods */


    STDMETHOD(InitDone)			(THIS_
					HX_RESULT	status);

    STDMETHOD(FileObjectReady)		(THIS_
					HX_RESULT	status,
					IUnknown*	pFileObject);

    STDMETHOD(DirObjectReady)		(THIS_
					HX_RESULT	status,
					IUnknown*	pDirObject);

	/* IHXBroadcastMapperResponse methods */

    STDMETHOD(BroadcastTypeFound)	(THIS_
					HX_RESULT	status,
					const char*	pType);

    /* IHXFileMapperResponse method */
    STDMETHOD(MimeTypeFound) (THIS_
			      HX_RESULT status,
			      const char* pMimeType);

private:
    LONG32			m_lRefCount;
    IHXSourceFinderResponse*	m_pResponse;
    IUnknown*                   m_pFileObject;
    IHXFileMimeMapper*         m_pMimeMapper;
    FSManager*			m_pFSManager;
    IHXBroadcastMapper*	m_pBroadcastMapper;
    Process*			m_pProc;
    URL*			m_pURL;
    IHXRequest*                m_pRequest;
    BOOL			m_bFindPending;

    HX_RESULT FindStatic(IUnknown*, const char* mime_type,
			 const char* extension);
   
    ~CServerSourceFinder();

};

#endif /* _SOURCE_FINDER_H_ */


