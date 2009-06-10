/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: minifileobj.h,v 1.11 2008/03/14 18:27:13 gajia Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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
#ifndef _MINIFILEOBJ_H_
#define _MINIFILEOBJ_H_

#ifdef HELIX_FEATURE_MMF_DATASOURCE
#include "ihxmmfdatasource.h"
#endif

/****************************************************************************
 * Includes
 */
#include "hxfiles.h"  /* IHXFileObject, IHXRequestHandler, etc. */

#if defined (_WINDOWS ) && defined (_WIN32)
#include <winbase.h>
#endif
/****************************************************************************
 *
 *  CHXMiniFileObject Class
 *
 *  This class inherits the interfaces required to create a File Object,
 *  which is used by the File System plug-in to handle file I/O. This class
 *  implements the IHXFileObject interface which handles the actual low
 *  level file access. The IHXRequestHandler interface is used to obtain
 *  the requested URL; while the IHXFileExists interface determines if the
 *  requested file actually exists. Since we are using COM, this class also
 *  inherits COM's IUnknown interface to handle reference counting and
 *  interface query.
 */
class CHXMiniFileObject :  public IHXFileObject,
			   public IHXDirHandler,
			   public IHXRequestHandler,
			   public IHXFileExists,
			   public IHXFileStat,
			   public IHXGetFileFromSamePool
#if defined(HELIX_FEATURE_MMF_DATASOURCE)			   
			  ,public IHXMMFDataSourceObserver
#endif			   
{
public:

    CHXMiniFileObject(IHXCommonClassFactory* pClassFactory,
                      const char* pBasePath, IUnknown* pContext);
    virtual ~CHXMiniFileObject(void);


    /************************************************************************
     *  IHXFileObject Interface Methods
     */
    STDMETHOD(Init       ) (THIS_ UINT32 access,IHXFileResponse* pFileResp);
    STDMETHOD(GetFilename) (THIS_ REF(const char*) pFileName);
    STDMETHOD(Read       ) (THIS_ UINT32 byteCount);
    STDMETHOD(Write      ) (THIS_ IHXBuffer* pDataToWrite);
    STDMETHOD(Seek       ) (THIS_ UINT32 offset, HXBOOL bRelative);
    STDMETHOD(Advise     ) (THIS_ UINT32 useage);
    STDMETHOD(Close      ) (THIS);


    /************************************************************************
     *  IHXRequestHandler Interface Methods
     */
    STDMETHOD(SetRequest) (THIS_     IHXRequest*  pRequest);
    STDMETHOD(GetRequest) (THIS_ REF(IHXRequest*) pRequest);


    /************************************************************************
     *  IHXFileExists Interface Methods
     */
    STDMETHOD(DoesExist)(THIS_ const char* pFilePath,
			 IHXFileExistsResponse* pFileResponse);


    /************************************************************************
     *  IUnknown COM Interface Methods
     */
    STDMETHOD (QueryInterface ) (THIS_ REFIID ID, void** ppInterfaceObj);
    STDMETHOD_(UINT32, AddRef ) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

    /************************************************************************
     *  IHXFileStat Interface Methods
     */
    STDMETHOD(Stat)(THIS_ IHXFileStatResponse* pFileStatResponse);

    /************************************************************************
     *  IHXDirHandler Interface Methods
     */
    STDMETHOD(InitDirHandler)(THIS_ IHXDirHandlerResponse* pDirResponse);

    /************************************************************************
     *  IHXDirHandler Interface Methods
     */
    STDMETHOD(CloseDirHandler)	(THIS);

    /************************************************************************
     *  IHXDirHandler Interface Methods
     */
    STDMETHOD(MakeDir)	(THIS);

    /************************************************************************
     *  IHXDirHandler Interface Methods
     */
    STDMETHOD(ReadDir)	(THIS);

    /************************************************************************
     *  IHXGetFileFromSamePool Interface Methods
     */
    STDMETHOD(GetFileObjectFromPool) (THIS_ IHXGetFileFromSamePoolResponse* response);
    
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    /************************************************************************
     *  IHXMMFDataSourceObserver Interface Methods
     */
     void ReadDone(THIS_ IHXBuffer * pBuffer, ULONG32 ulCount);
#endif

protected:

    /****** Protected Class Variables ****************************************/
    INT32                   m_RefCount;       // Object's reference count
    IHXCommonClassFactory*  m_pClassFactory;  // Creates common classes
    IHXFileResponse*        m_pFileResponse;  // Provides completion notif.
    FILE*                   m_pFile;          // Actual file pointer
    char*                   m_pFilename;      // Object's copy of file name
    IHXRequest*             m_pRequest;       // Used to get requested URL
    char*                   m_pBasePath;      // Platform's root path
    UINT32                  m_FileAccessMode; // Current file access mode
    IHXDirHandlerResponse*  m_pDirResponse;   // Target for dir functions
#if defined (_WINDOWS ) && defined (_WIN32)
    HANDLE                  m_hFileHandle; // Used for WIN32 ReadDir()
    WIN32_FIND_DATA         m_FileInfo;            // Used for WIN32 ReadDir()
#endif
    HXBOOL                  m_bInReadDone;
    UINT32                  m_lPendingByteCount;
    IHXBuffer*              m_pPendingReadBuf;
    IUnknown*               m_pContext;
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    IHXMMFDataSource*       m_pDataSource;
    HXBOOL                  m_bAsyncReadSupported;
#endif
    char*                   m_pProtocolString;

#if defined(HELIX_FEATURE_MMF_DATASOURCE)    
    IHXBuffer* 	       	    m_pPendingReadBufAsync;
#endif
    /****** Protected Class Methods ******************************************/
    STDMETHOD(OpenFile             ) (THIS_ UINT32 fileMode);
    STDMETHOD(GetModeStr           ) (THIS_ UINT32 fileAccessMode, char* modeStr);
    STDMETHOD(ConvertToPlatformPath) (THIS_ REF(char*)  pFilePathPlatform, const char* pFilePath);
    STDMETHOD(AllocateReadBuffer   ) (THIS_ UINT32 byteCount, IHXBuffer** pBuffer);
    STDMETHOD(DoReadLoop           ) (THIS);
    STDMETHOD(DoReadLoopEx         ) (THIS_ UINT32 byteCount, IHXBuffer* pBuffer);
    STDMETHOD_(UINT32, DoRead      ) (THIS_ IHXBuffer* pBuffer);
    STDMETHOD(DoReadDone           ) (THIS_ HX_RESULT readResult, IHXBuffer* pBuffer);
    STDMETHOD_(void, ReadDoneError ) (THIS_ HX_RESULT theError);

    HXBOOL IsReadError();
};

#endif  /* _MINIFILEOBJ_H_ */

