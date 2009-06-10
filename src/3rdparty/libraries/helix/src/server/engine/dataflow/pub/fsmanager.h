/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fsmanager.h,v 1.11 2008/10/06 21:34:41 ckarusala Exp $
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

#ifndef _FSMANAGER_H_
#define _FSMANAGER_H_

#include "hxcom.h"
#include "hxfiles.h"
#include "hxcache.h"
#include "proc.h"
#include "safe_object_delete.h"
#include "plgnhand.h"
#include "ihxurlparser.h"

class URL;
class PluginHandler;
class FileObjectWrapper;
class FileSystemWrapper;
class CDistAdviseWrapper;

class FSManager;

DECLARE_SAFE_DELETION(FSManager)

class FSManager : public IHXFileSystemManager,
                  public IHXFileExistsResponse,
                  public IHXGetFileFromSamePoolResponse,
                  public IHXHTTPRedirect,
                  public IHXHTTPRedirectResponse
                , public IHXContentDistributionAdviseResponse
{
    USE_SAFE_DELETION(FSManager)

public:
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32, AddRef) (THIS);
    STDMETHOD_(ULONG32, Release)(THIS);

    STDMETHOD(Init) (THIS_
                    IHXFileSystemManagerResponse* /*IN*/  pFileManagerResponse
                    );

    STDMETHOD(GetFileObject)            (THIS_
                                         IHXRequest* pRequest,
                                         IHXAuthenticator* pAuth);

    STDMETHOD(GetNewFileObject)         (THIS_
                                         IHXRequest* pRequest,
                                         IHXAuthenticator* pAuth);

    STDMETHOD(GetRelativeFileObject)    (THIS_
                                         IUnknown*,
                                         const char*);

    STDMETHOD(GetDirObjectFromURL)      (THIS_
                                         const char* pURL);

    STDMETHOD(DoesExistDone)            (THIS_
                                         BOOL bExists);

    STDMETHOD(FileObjectReady)          (THIS_
                                         HX_RESULT status,
                                         IUnknown* pUnknown);


    STDMETHOD(Init)             (THIS_
                                IHXHTTPRedirectResponse* pRedirectResponse);
    STDMETHOD(SetResponseObject)(THIS_
                                IHXHTTPRedirectResponse* pRedirectResponse);

    STDMETHOD(RedirectDone)     (THIS_
                                IHXBuffer* pURL);

    /*
     * IHXContentDistributionAdviseResponse methods
     */

    STDMETHOD(OnLocalResultDone)                (THIS_
                                                 HX_RESULT status);

    STDMETHOD(OnCacheRequestDone)               (THIS_
                                                 HX_RESULT status);

    STDMETHOD(OnCacheResultDone)                (THIS_
                                                 HX_RESULT status);

    STDMETHOD(OnSiteCacheResultDone)            (THIS_
                                                 HX_RESULT status);

    STDMETHOD(OnPurgeCacheURLDone)              (THIS_
                                                 HX_RESULT status);

                        FSManager(Process* p);


    HX_RESULT           AsyncCreateFileDone(HX_RESULT, FileObjectWrapper*);
    void                GetLastPlugin(PluginHandler::FileSystem::PluginInfo* & pPlugin);

    UINT32              m_mount_point_len;
    BOOL                m_bIsDone;



private:
    ~FSManager();

    void                AddRTSPImportHeader();
    HX_RESULT           FileReadyHook(HX_RESULT result, IUnknown* pFileObject);

    ULONG32                         m_ulRefCount;
    IHXFileSystemManagerResponse*   m_pResponse;
    Process*                        proc;
    IHXScheduler*                   m_scheduler;
    IHXURL*                         m_url;
    IUnknown*                       m_file_object;
    IHXFileExists*                  m_file_exists;
    IHXGetFileFromSamePool*         m_pool;
    FileSystemWrapper*              m_file_system_wrapper;
    IHXHTTPRedirectResponse*        m_pRedirectResponse;
    BOOL                            m_bHandleRedirect;
    BOOL                            m_bCollectReadStats;

    PluginHandler::FileSystem::PluginInfo* m_last_plugin;
    IHXValues*                      m_last_options;
    IHXValues*                      m_first_options;

    CHXMapLongToObj*                m_pCachedFSMap;

    CHXString m_plusPathLeft;
    CHXString m_plusPathRight;
    UINT32  m_ulHandlingPlusUrl;

    /* These members added to hold data between asynchronous cdist calls */

    IHXRequest*                     m_pRequest;
    CDistAdviseWrapper*             m_pAdvise;
    IUnknown*                       m_pFileObject;
    HX_RESULT                       m_iResult;
    BOOL                            m_bCheckingCache;
    BOOL                            m_bIsCDistEligible;
    BOOL                            m_bFound;
    BOOL                            m_bEnableAMPDebug;


    HX_RESULT CheckNextPlugin(PluginHandler::FileSystem::PluginInfo*);
};

#endif
