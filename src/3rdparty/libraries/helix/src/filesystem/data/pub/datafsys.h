/* ***** BEGIN LICENSE BLOCK *****
 *
 * Source last modified: $Id:
 *
 * Copyright Notices:
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
 *
 * Patent Notices: This file may contain technology protected by one or
 * more of the patents listed at www.helixcommunity.org
 *
 * 1.   The contents of this file, and the files included with this file,
 * are protected by copyright controlled by RealNetworks and its
 * licensors, and made available by RealNetworks subject to the current
 * version of the RealNetworks Public Source License (the "RPSL")
 * available at  * http://www.helixcommunity.org/content/rpsl unless
 * you have licensed the file under the current version of the
 * RealNetworks Community Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply.  You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * 2.  Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above.  Please note that RealNetworks and its
 * licensors disclaim any implied patent license under the GPL.
 * If you wish to allow use of your version of this file only under
 * the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting Paragraph 1 above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete Paragraph 1 above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 *
 * This file is part of the Helix DNA Technology.  RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.   Copying, including reproducing, storing,
 * adapting or translating, any or all of this material other than
 * pursuant to the license terms referred to above requires the prior
 * written consent of RealNetworks and its licensors
 *
 * This file, and the files included with this file, is distributed
 * and made available by RealNetworks on an 'AS IS' basis, WITHOUT
 * WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS
 * AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING
 * WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _DATAFSYS_H_
#define _DATAFSYS_H_

class DataFileSystem : public IHXPlugin,
                         public IHXFileSystemObject
{
public:
    static HX_RESULT STDAPICALLTYPE HXCreateInstance(IUnknown** ppIUnknown);
    static HX_RESULT CanUnload();
    static HX_RESULT STDAPICALLTYPE HXShutdown(void);

    DataFileSystem();

    ~DataFileSystem();

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    // *** IHXPlugin methods ***

    /************************************************************************
     *  Method:
     *      IHXPlugin::GetPluginInfo
     *  Purpose:
     *      Returns the basic information about this plugin. Including:
     *
     *      bLoadMultiple       whether or not this plugin DLL can be loaded
     *                          multiple times. All File Formats must set
     *                          this value to TRUE.
     *      pDescription        which is used in about UIs (can be NULL)
     *      pCopyright          which is used in about UIs (can be NULL)
     *      pMoreInfoURL        which is used in about UIs (can be NULL)
     */
    STDMETHOD(GetPluginInfo)    (THIS_
                                REF(HXBOOL)      /*OUT*/ bLoadMultiple,
                                REF(const char*) /*OUT*/ pDescription,
                                REF(const char*) /*OUT*/ pCopyright,
                                REF(const char*) /*OUT*/ pMoreInfoURL,
                                REF(ULONG32)     /*OUT*/ ulVersionNumber
                                );

    /************************************************************************
     *  Method:
     *      IHXPlugin::InitPlugin
     *  Purpose:
     *      Initializes the plugin for use. This interface must always be
     *      called before any other method is called. This is primarily needed
     *      so that the plugin can have access to the context for creation of
     *      IHXBuffers and IMalloc.
     */
    STDMETHOD(InitPlugin)   (THIS_
                            IUnknown*   /*IN*/  pContext);

    // *** IHXFileSystemObject methods ***
    STDMETHOD(GetFileSystemInfo)    (THIS_
                                    REF(const char*) /*OUT*/ pShortName,
                                    REF(const char*) /*OUT*/ pProtocol);

    STDMETHOD(InitFileSystem) (THIS_ IHXValues* options);

    STDMETHOD(CreateFile)       (THIS_
                                IUnknown**    /*OUT*/   ppFileObject);

    STDMETHOD(CreateDir)        (THIS_
                                IUnknown**     /*OUT*/     ppDirObject);

private:
    LONG32                      m_lRefCount;
    static const IID            zm_myIID;
    static const char*          zm_pDescription;
    static const char*          zm_pCopyright;
    static const char*          zm_pMoreInfoURL;
    static const char*          zm_pShortName;
    static const char*          zm_pProtocol;
    IUnknown*                   m_pContext;
};

/////////////////////////////////////////////////////////////////////////////
//
//  Class:
//
//      DataFileObject
//
//  Purpose:
//
//      File object to handle Data functions
//

class DataFileObject : public IHXFileObject,
                         public IHXFileStat,
                         public IHXFileExists,
                         public IHXRequestHandler,
                         public IHXFileMimeMapper
{
public:
    DataFileObject(IUnknown* pContext);
    ~DataFileObject();

    /*
     *  IUnknown methods
     */

    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     *  IHXFileObject methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileObject::Init
     *  Purpose:
     *      Associates a file object with the file response object it should
     *      notify of operation completness. This method should also check
     *      for validity of the object (for example by opening it if it is
     *      a local file).
     */
    STDMETHOD(Init)             (THIS_
                                ULONG32             /*IN*/      ulFlags,
                                IHXFileResponse*   /*IN*/       pFileResponse);

    /************************************************************************
     *  Method:
     *      IHXFileObject::GetFilename
     *  Purpose:
     *      Returns the filename (without any path information) associated
     *      with a file object.
     */
    STDMETHOD(GetFilename)      (THIS_
                                REF(const char*)    /*OUT*/  pFilename);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Close
     *  Purpose:
     *      Closes the file resource and releases all resources associated
     *      with the object.
     */
    STDMETHOD(Close)            (THIS);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Read
     *  Purpose:
     *      Reads a buffer of data of the specified length from the file
     *      and asynchronously returns it to the caller via the
     *      IHXFileResponse interface passed in to Init.
     */
    STDMETHOD(Read)             (THIS_
                                ULONG32             ulCount);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Write
     *  Purpose:
     *      Writes a buffer of data to the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Write)            (THIS_
                                IHXBuffer*          pBuffer);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Seek
     *  Purpose:
     *      Seeks to an offset in the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Seek)             (THIS_
                                 ULONG32 ulOffset,
                                 HXBOOL  bRelative);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Stat
     *  Purpose:
     *      Collects information about the file that is returned to the
     *      caller in an IHXStat object
     */
    STDMETHOD(Stat)             (THIS_
                                IHXFileStatResponse* pFileStatResponse);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Advise
     *  Purpose:
     *      To pass information to the File Object
     */
    STDMETHOD(Advise)   (THIS_
                        ULONG32 ulInfo);

    // IHXFileExists interface
    /************************************************************************
     *  Method:
     *      IHXFileExists::DoesExist
     *  Purpose:
     */
    STDMETHOD(DoesExist) (THIS_
                        const char*             /*IN*/  pPath,
                        IHXFileExistsResponse* /*IN*/  pFileResponse);

    //IHXRequestHandler methods
    /************************************************************************
     *  Method:
     *      IHXRequestHandler::SetRequest
     *  Purpose:
     *      Associates an IHXRequest with an object
     */
    STDMETHOD(SetRequest)       (THIS_
                                IHXRequest*        /*IN*/  pRequest);

    /************************************************************************
     *  Method:
     *      IHXRequestHandler::GetRequest
     *  Purpose:
     *      Gets the IHXRequest object associated with an object
     */
    STDMETHOD(GetRequest)       (THIS_
                                REF(IHXRequest*)  /*OUT*/  pRequest);

    /*
     *  IHXFileMimeMapper methods
     */

    /************************************************************************
     *  Method:
     *      IHXFileMimeMapper::FindMimeType
     *  Purpose:
     */
    STDMETHOD(FindMimeType) (THIS_
                            const char*             /*IN*/  pURL,
                            IHXFileMimeMapperResponse* /*IN*/  pMimeMapperResponse
                            );


private:
    LONG32                      m_lRefCount;
    IUnknown*                   m_pContext;
    IHXCommonClassFactory*      m_pClassFactory;
    IHXFileResponse*            m_pFileResponse;
    IHXRequest*         m_pRequest;
    IHXBuffer*                  m_pDataURL;
    CHXString                   m_MediaType;
    UINT32                      m_ulFilePointer;

    STDMETHOD(ParseURL)(THIS_ const char* pURL, CHXString& mimeString,
        IHXBuffer* pData, IHXRequest* pRequest);

#ifdef HELIX_FEATURE_TONE_GENERATOR
    STDMETHOD(SetSequence)(UINT32 ulNote, UINT32 ulToneDuration,
                 UINT32 ulToneVolume, IHXBuffer* pData);
#endif

};

#endif // ndef _DATAFSYS_H_
