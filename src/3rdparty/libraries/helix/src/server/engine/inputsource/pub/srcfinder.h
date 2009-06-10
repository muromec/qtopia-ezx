/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: srcfinder.h,v 1.12 2008/03/09 12:19:13 npatil Exp $ 
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

#ifndef _SRCFINDER_H_
#define _SRCFINDER_H_

#include "hxfiles.h"
#include "hxclientprofile.h"
#include "plgnhand.h"

class Process;
class URL;
class FSManager;


class CPacketHandler;

/*
 * The base class for input source finding.  This does not have any cache
 * functionality, do not use it unless you have a specific need to avoid
 * caching.
 */
class BasicSourceFinder : public IHXFileSystemManagerResponse,
                          public IHXFileMimeMapperResponse,
                          public IHXBroadcastMapperResponse
{
public:
    BasicSourceFinder(Process* pProc, ClientSession* pSession);
    virtual ~BasicSourceFinder(void);

    /* make this an interface at some point */    
    virtual HX_RESULT FindSource(IHXURL* pURL, ServerRequest* pRequest);
    virtual HX_RESULT FindNextSource(void);

    HX_RESULT GetClientProfileInfo(void);

    HX_RESULT Init(IHXSourceFinderFileResponse* pResp, IHXQoSProfileConfigurator* pQoSConfig);
    void      Close(void);

    HX_RESULT GetCurrentSourceTye( REF(CHXString) strType )
    {
	HX_RESULT result = HXR_FAIL;

	strType = m_CurrentSourceType;
	if ( strType.GetLength() > 0 )
	{
	    result = HXR_OK;
	}
	return result;
    };

    /* Iunknown */
    STDMETHOD_(ULONG32, AddRef)	    (THIS);
    STDMETHOD_(ULONG32, Release)    (THIS);
    STDMETHOD(QueryInterface)	    (THIS_ REFIID riid, void** ppvObj);

    /* IHXFileSystemManagerResponse */
    STDMETHOD(InitDone)		    (THIS_ 
                                     HX_RESULT status);
    STDMETHOD(FileObjectReady)	    (THIS_ 
                                     HX_RESULT status, 
                                     IUnknown* pFileObject);
    STDMETHOD(DirObjectReady)	    (THIS_ 
                                     HX_RESULT status,
                                     IUnknown* pDirObject);

    /* IHXFileMimeMapperResponse */
    STDMETHOD(MimeTypeFound)	    (THIS_
                                     HX_RESULT status,
                                     const char* pMimeType);

    /* IHXBroadcastMapperResponse */                                     
    STDMETHOD(BroadcastTypeFound)   (THIS_
                                     HX_RESULT status,
                                     const char* pBroadcastType);

protected:    
    HX_RESULT FindLive(IHXBroadcastMapper*);
    HX_RESULT FindStatic(const char* mime_type,
                                const char* extension);
    HX_RESULT FindSourceDone(HX_RESULT status, IUnknown* pSource);

    HX_RESULT FindFileFormat(const char* pMimeType, 
                             const char* pExt,
                             REF(IHXFileFormatObject*)pFF);

    HX_RESULT MakePacketHandler(IHXFileFormatObject* pFF,
                                REF(CPacketHandler*) pPushSource);

    HX_RESULT HandleFilters(IUnknown* pPushSource, REF(IUnknown*) pOut);			     
    HX_RESULT AttachASMFilter(IUnknown* pPushSource, REF(IUnknown*) pOut);

protected:
    LONG32                      m_ulRefCount;
    Process*                    m_pProc;
    IHXQoSProfileConfigurator*  m_pConfig;

    // get rid of this if it's ever possible...
    ClientSession*              m_pSession;

    IHXSourceFinderFileResponse* m_pResp;
    ServerRequest*              m_pRequest;
    IHXURL*                     m_pURL; // this needs to be a COM obj
    IHXClientProfileInfo*       m_pClientProfile;

    FSManager*                  m_pFSManager;
    IHXFileMimeMapper*          m_pMimeMapper;
    IHXBroadcastMapper*         m_pBroadcastMapper;

    IUnknown*                   m_pFileObject;
    ServerRequestWrapper*       m_pFileRequest;
    IHXFileFormatObject*        m_pFileFormatObject;
    ServerRequestWrapper*       m_pFileFormatRequest;
    IHXPSourceControl*          m_pSourceControl;

    PluginHandler::FileFormat::PluginInfo*  m_pCurrentPlugin;
    
    enum 
    {
        SF_CLOSED,
        SF_INITIALIZED,
        SF_FINDING
    }				m_findState;
    
    CHXString m_CurrentSourceType;
};

#endif /* _SRCFINDER_H_ */
