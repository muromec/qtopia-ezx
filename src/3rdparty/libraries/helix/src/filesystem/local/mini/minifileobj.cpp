/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: minifileobj.cpp,v 1.39 2008/07/08 19:49:40 anuj_dhamija Exp $
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

/****************************************************************************
 * Includes
 */
#include "hxtypes.h"
#include "hlxclib/stdio.h"    /* FILE */
#include "hlxclib/string.h"   /* strcpy, etc. */
#include "hlxclib/sys/stat.h" /* stat() */

#include "hxcom.h"     /* IUnknown */
#include "hxcomm.h"    /* IHXCommonClassFactory */
#include "ihxpckts.h"  /* IHXValues, IHXBuffers */
#include "rtsputil.h"

#include "minifilesys.h"  /* FILE_SYS_PROTOCOL */
#include "minifileobj.h"  /* CHXMiniFileObject */

#include "debug.h" /* DPRINTF */
#include "hxdir.h" /* OS_SEPERATOR_STRING */

#include "hxerror.h"  /* IHXErrorMessages */

#if defined (_WINDOWS ) && defined (_WIN32)
#ifndef _WINCE
#include <direct.h>    /* mkdir, etc. */
#endif
#elif defined (_MACINTOSH)
#include <unix.h>      /* fileno */
#endif

#if defined(HELIX_FEATURE_MMF_DATASOURCE)
#include "ihxmmfdatasource.h"
#endif

#define D_MINI_FO 0x1000000

// CHXMiniFileObject Class Methods

/****************************************************************************
 *  CHXMiniFileObject::CHXMiniFileObject
 *
 *  Constructor
 */
CHXMiniFileObject::CHXMiniFileObject(IHXCommonClassFactory* pClassFactory, 
                                     const char* pBasePath, 
                                     IUnknown* pContext): 
      m_RefCount             (0)
      ,m_pClassFactory       (pClassFactory)
      ,m_pFileResponse       (NULL)
      ,m_pFile               (NULL)
      ,m_pFilename           (NULL)
      ,m_pRequest            (NULL)
      ,m_pBasePath           (NULL)
#if defined (_WINDOWS ) && defined (_WIN32)
      ,m_hFileHandle         (0)
#endif
      ,m_FileAccessMode      (0)
      ,m_pDirResponse        (NULL)
      ,m_bInReadDone         (FALSE)
      ,m_lPendingByteCount   (0)
      ,m_pPendingReadBuf     (NULL)
      ,m_pContext            (pContext)
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
      ,m_pDataSource         (NULL)
      ,m_bAsyncReadSupported (FALSE)
      ,m_pPendingReadBufAsync (NULL)
#endif
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::CHXMiniFileObject()\n"));
    m_pContext->AddRef();

    m_pProtocolString = FILE_SYS_PROTOCOL;

    // Signify that we need to keep a reference to this object
    if (m_pClassFactory != NULL)
    {
        m_pClassFactory->AddRef();
    }

    if (pBasePath)
    {
        m_pBasePath = new char[strlen(pBasePath) + 1];

        if (m_pBasePath)
        {
            strcpy(m_pBasePath, pBasePath);
        }
    }
}


/****************************************************************************
 *  CHXMiniFileObject::~CHXMiniFileObject
 *
 *  Destructor. It is essential to call the Close() routine before destroying
 *  this object.
 */
CHXMiniFileObject::~CHXMiniFileObject(void)
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::~CHXMiniFileObject()\n"));

    Close();
	HX_RELEASE(m_pContext);
    delete [] m_pBasePath;
    m_pBasePath = 0;

    if (m_pFilename != NULL)
    {
        delete[] m_pFilename;
        m_pFilename = 0;
    }

    if (m_pClassFactory != NULL)
    {
        m_pClassFactory->Release();
        m_pClassFactory = NULL;
    }
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    HX_RELEASE(m_pDataSource);
    HX_RELEASE(m_pPendingReadBuf);
    if(m_bAsyncReadSupported)
    {
        HX_RELEASE(m_pPendingReadBufAsync);
    }
#endif
}


/****************************************************************************
 *  CHXMiniFileObject::OpenFile
 *
 *  This routine opens a file according to the access mode given. It is
 *  called while initializing the File Object.
 */
STDMETHODIMP CHXMiniFileObject::OpenFile(UINT32 fileAccessMode)
{
    HX_RESULT result = HXR_OK;

    // Construct the proper access mode string
    char modeStr[4];
    result = GetModeStr(fileAccessMode, modeStr);

    if (result == HXR_OK)
    {
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
        IHXMMFDataSource *pDataSource = NULL;
        if(m_pRequest)
        {
            IHXRequestContext *pRequestContext = NULL;
            m_pRequest->QueryInterface(IID_IHXRequestContext, 
                                       (void**)&pRequestContext);
            if(pRequestContext)
            {
                IUnknown *pRequester = NULL;
                pRequestContext->GetRequester(pRequester);
                if(pRequester)
                {
                    pRequester->QueryInterface(IID_IHXMMFDataSource, 
                                               (void**)&pDataSource);
                    HX_RELEASE(pRequester);
                }
                HX_RELEASE(pRequestContext);
            }
        }
        if(pDataSource)
        {
           // This Request supports data source API. Set this to m_pDataSource.
           // m_pDataSource will be used for all file operations.
           HX_RELEASE(m_pDataSource);
           m_pDataSource = pDataSource;
           pDataSource = NULL;
           m_bAsyncReadSupported = m_pDataSource->AsyncReadSupported();
           result = m_pDataSource->Open2(m_pRequest, modeStr, this, this);
        }
        else
#endif
        {
            // Open the file with the proper access mode
            m_pFile = fopen(m_pFilename, modeStr);
            result =  m_pFile ? HXR_OK : HXR_DOC_MISSING;
        }
    }

    return result;
}


/****************************************************************************
 *  CHXMiniFileObject::GetModeStr
 *
 *  This routine converts the given access mode to a mode string.
 */
STDMETHODIMP CHXMiniFileObject::GetModeStr(UINT32 fileAccessMode,
                                           char* modeStr)
{
    HX_RESULT result = HXR_OK;

    if (fileAccessMode & HX_FILE_READ)
    {
        strcpy(modeStr, "r");
        if (fileAccessMode & HX_FILE_WRITE)
        {
            strcat(modeStr, "+");
        }
        if (fileAccessMode & HX_FILE_BINARY)
        {
            strcat(modeStr, "b");
        }
    }
    else if (fileAccessMode & HX_FILE_WRITE)
    {
        strcpy(modeStr, "w");
        if (fileAccessMode & HX_FILE_BINARY)
        {
            strcat(modeStr, "b");
        }
    }
    else if (fileAccessMode == 0)
    {
        fileAccessMode = HX_FILE_READ | HX_FILE_BINARY;
        strcpy(modeStr, "rb");
    }
    else
    {
        result = HXR_INVALID_PARAMETER;
    }

    return result;
}


/****************************************************************************
 *  CHXMiniFileObject::ConvertToPlatformPath
 *
 *  This routine converts the given file path to a platform specific file
 *  path based upon the naming conventions of that platform. The platform
 *  specific path name is required to properly open the file.
 */
STDMETHODIMP CHXMiniFileObject::ConvertToPlatformPath(REF(char*)  pFilePathPlatform, const char* pFilePath)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    pFilePathPlatform =  0;

    if (m_pBasePath && pFilePath)
    {
        // Create new string
        pFilePathPlatform =
            new char[ strlen(m_pBasePath) + strlen(pFilePath) + 2 ];
    }

    UINT32 length = strlen(m_pProtocolString) + 1; // Add 1 for the colon
    char* pProtocolString = new char[length + 1];

    if (pFilePathPlatform && pProtocolString && m_pBasePath)
    {
        // Prepend base path, if any
        if (strlen(m_pBasePath) > 0)
        {
            strcpy(pFilePathPlatform, m_pBasePath);
            strcat(pFilePathPlatform, OS_SEPARATOR_STRING);
            strcat(pFilePathPlatform, pFilePath);
        }
        else
        {
            strcpy(pFilePathPlatform, pFilePath);
        }

        // Strip protocol string, if any
        strcpy(pProtocolString, m_pProtocolString);
        strcat(pProtocolString, ":");
        if (strnicmp(pFilePathPlatform, pProtocolString, length) == 0)
        {
            //copy the rest of the string back onto itself.
            memmove( (void*) pFilePathPlatform,
                     (void*) &pFilePathPlatform[length],
                     (strlen( &pFilePathPlatform[length] )+1)*sizeof(char)
                     );

            if ((pFilePathPlatform[0] == '/') &&
                (pFilePathPlatform[1] == '/'))
            {
                // "file://" forms

                // Find next '/'
                const char* pNext = strchr(pFilePathPlatform + 2, '/');

                if (pNext)
                {
                    // "file://host/path" or "file:///path" form.
                    // Copy everything after the third '/' on non-UNIX and
 		    // everything after the second '/' on UNIX (except MacOS X)
                    memmove( (void*) pFilePathPlatform,
#if defined(_UNIX) && !defined(_MAC_UNIX)
                             (void*) pNext,
                             (strlen(pNext)+1)*sizeof(char)
#else
                             (void*) (pNext+1),
                             (strlen(pNext+1)+1)*sizeof(char)
#endif
                             );
                    pNext = 0;
                    res = HXR_OK;
                }
                else
                {
                    // Forms: file://c:\file.ra
                    //        file://file.ra
                    memmove( (void*) pFilePathPlatform,
                             (void*) (pFilePathPlatform+2),
                             (strlen(pFilePathPlatform+2)+1)*sizeof(char)
                             );
                    res = HXR_OK;
                }
            }
            else
            {
                res = HXR_OK;
            }

            if (HXR_OK == res)
            {
                // Replace path slashes with platform specific path separators
                // and watch for the parameter delimiter
                char* pCur = pFilePathPlatform;
                for (; *pCur && (*pCur != '?'); pCur++)
                {
                    if (*pCur == '/')
                    {
                        *pCur = OS_SEPARATOR_CHAR;
                    }
                }

                /*
                 * Strip off the parameters
                 */
                if (*pCur == '?')
                {
                    *pCur = '\0';
                }
            }
        }
        else
        {
			if (NULL == strstr(pFilePathPlatform,"//"))
				res = HXR_OK; // allow path/file w/o file://
			else
	            res = HXR_INVALID_PROTOCOL;
        }
    }

    delete [] pProtocolString;
    pProtocolString = 0;

    if (res != HXR_OK)
    {
        delete [] pFilePathPlatform;
        pFilePathPlatform = 0;
    }

    return res;
}


// IHXFileObject Interface Methods

/****************************************************************************
 *  IHXFileObject::Init
 *
 *  This routine associates this File Object with a File Response object
 *  which is notified when file operations (read, write, seek, etc.) are
 *  complete. This method also checks the validity of the file by actually
 *  opening it.
 */
STDMETHODIMP CHXMiniFileObject::Init( UINT32 fileAccessMode, IHXFileResponse* pFileResponse )
{
    /*
     * Associate this File Object with a File Response object for completion
     * notification.
     */
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Init()\n"));

    if (pFileResponse != NULL)
    {
        // Release any previous File Response objects
        if (m_pFileResponse != NULL)
        {
            m_pFileResponse->Release();
        }

        m_pFileResponse = pFileResponse;
        m_pFileResponse->AddRef();
    }
    else
    {
        return HXR_INVALID_PARAMETER;
    }

#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    if(m_pRequest && m_pDataSource)
    {
        // Has requested access mode changed?
        if ((fileAccessMode == m_FileAccessMode) ||
            (fileAccessMode == 0))
        {
            // reset to start of file
            m_pDataSource->Seek2(0, SEEK_SET, this);
            // notify that file is ready
            m_pFileResponse->InitDone(HXR_OK);
            return HXR_OK;
        }
        else // Access mode has changed
        {
            m_pDataSource->Close2( this );
            HX_RELEASE(m_pDataSource);
        }
    }
    else 
#endif
    if (m_pFile != NULL) // File is already open
    {
    /*
     * Open the file and notify File Response when complete
     */
        // Has requested access mode changed?
        if ((fileAccessMode == m_FileAccessMode) ||
            (fileAccessMode == 0))
        {
            // reset to start of file
            fseek(m_pFile, 0, SEEK_SET);

            // notify that file is ready
            m_pFileResponse->InitDone(HXR_OK);
            return HXR_OK;
        }
        else // Access mode has changed
        {
            fclose(m_pFile);
            m_pFile = NULL;
        }
    }
    m_FileAccessMode = fileAccessMode;

    HX_RESULT fileOpenResult = OpenFile(fileAccessMode);
    if (fileOpenResult == HXR_OK )
    {
    	fileOpenResult = m_pFileResponse->InitDone(fileOpenResult);
	}

    return fileOpenResult;
}


/****************************************************************************
 *  IHXFileObject::GetFilename
 *
 *  This routine returns the name of the requested file (without any path
 *  information). This method may be called by the File Format plug-in if the
 *  short name of the file is required.
 */
STDMETHODIMP CHXMiniFileObject::GetFilename( REF(const char*) pFileName )
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::GetFilename()\n"));

    pFileName = NULL;
    HX_RESULT   result = HXR_OK;

    // Find the separator character before the file name
    pFileName = ::strrchr(m_pFilename, OS_SEPARATOR_CHAR);

    if (pFileName != NULL) // Found
    {
        // File name starts after the separator charactor
        pFileName++;
    }
    else // Not found
    {
        pFileName = m_pFilename;
    }

    return result;
}


/****************************************************************************
 *  IHXFileObject::Read
 *
 *  This routine reads a block of data of the specified length from the file.
 *  When reading has completed, the caller is asynchronously notified via the
 *  File Response object associated with this File Object. This method is
 *  called by the File Format plug-in when it needs to read from the file.
 */
STDMETHODIMP CHXMiniFileObject::Read( UINT32 byteCount )
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Read(%u)\n", byteCount));

    HX_RESULT result = HXR_UNEXPECTED;

    if (m_pPendingReadBuf == NULL)
    {
        if (byteCount > 0x000FFFFF)
        {
            m_bInReadDone = FALSE;
            m_pFileResponse->ReadDone(HXR_FAILED, NULL);
            return HXR_INVALID_PARAMETER;
        }

        m_lPendingByteCount = byteCount;
        result = AllocateReadBuffer(byteCount, &m_pPendingReadBuf);

        if (result == HXR_OK && m_pPendingReadBuf != NULL)
        {
            if (!m_bInReadDone)
            {
                result = DoReadLoop();
            }
        }
    }

    return result;
}


/****************************************************************************
 *  IHXFileObject::AllocateReadBuffer
 *
 *  This routine is a helper method to allocate a buffer of the given 
 *  byteCount. 
 */
STDMETHODIMP CHXMiniFileObject::AllocateReadBuffer(UINT32 byteCount,
                                                   IHXBuffer** pBuffer)
{
    HX_RESULT result = HXR_OK;

    if(m_pClassFactory != NULL)
    {
        m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)pBuffer);
    }
	
    if(pBuffer != NULL)
    {
        result = (*pBuffer)->SetSize(byteCount);
		
        if(HXR_OK != result)
        {
            HX_RELEASE(*pBuffer);
        }
    }
    else
    {
        result = HXR_OUTOFMEMORY;
    }
	
    return result;
}


/****************************************************************************
 *  IHXFileObject::DoReadLoop
 *
 *  This routine is a helper method to perform the actual read function
 *  in a loop. The reason for the loop is to continually allow Read() to
 *  occur from within the ReadDone callback.
 */
STDMETHODIMP CHXMiniFileObject::DoReadLoop()
{
    HX_RESULT result = HXR_OK;

    // We use a loop here so we can service Read() calls
    // occurred inside the ReadDone() callback
    while (m_pPendingReadBuf)
    {
        // Transfer ownership of the pending buffer to
        // this local variable. This prepares for the
        // possibility that Read() may be called from
        // ReadDone()
        UINT32 byteCount = m_lPendingByteCount;
        IHXBuffer* pBuffer = m_pPendingReadBuf;
        m_lPendingByteCount = 0;
        m_pPendingReadBuf = NULL;

        result = DoReadLoopEx(byteCount, pBuffer);
    }

    return result;
}


/****************************************************************************
 *  IHXFileObject::DoReadLoopEx
 *
 *  This routine is a hook method than can be overriden to perform the 
 *  Read() and ReadDone() functions. It is provided to allow for the
 *  possibility of retrying the Read() if it is unsuccessful. Here, we
 *  do not perform a retry function.
 */
STDMETHODIMP CHXMiniFileObject::DoReadLoopEx(UINT32 byteCount, 
                                             IHXBuffer* pBuffer)
{
    HX_RESULT result = HXR_OK;
#if defined(HELIX_FEATURE_MMF_DATASOURCE)	
    if(m_bAsyncReadSupported)
    {
        m_pPendingReadBufAsync = pBuffer;
    }
#endif	

    UINT32 actualCount = DoRead(pBuffer);

#if defined(HELIX_FEATURE_MMF_DATASOURCE)	
    if(!m_bAsyncReadSupported)
    {
#endif    
    if(IsReadError())
    {
        HX_RELEASE(pBuffer);
        ReadDoneError(HXR_READ_ERROR);
        return HXR_READ_ERROR;
    }
    result = pBuffer->SetSize(actualCount);
        if(result == HXR_OK)
        {
            // Notify the caller that the read is done
            HX_RESULT readResult = actualCount > 0 ? HXR_OK : HXR_READ_ERROR;
            result = DoReadDone(readResult, pBuffer);
        }
#if defined(HELIX_FEATURE_MMF_DATASOURCE)		
    }
#endif	

    return result;
}

#if defined(HELIX_FEATURE_MMF_DATASOURCE)

/****************************************************************************
 *  IHXMMFDataSourceObserver::ReadDone
 *
 *  This routine is called by DataSource whenever there is a request for 
 *  data from file object and requested amount of data available at Data
 *  source level.
 */
 
void CHXMiniFileObject::ReadDone(IHXBuffer* pBuffer, UINT32 byteCount)
{
    HX_RESULT result = HXR_OK;
    //Read interrupt Read is not allowed. In the case when Seek happens the second Read may come in
    //before the first ReadDone a ReadDone(HXR_CANCELLED, NULL)is sent to the File format when Seek starts.
    //We put a check here to see if the returned buffer from asynchronous Data Source is the one we are expecting 
    //here. If not we just drop it.
    if ( m_pPendingReadBufAsync != pBuffer && pBuffer )
    {
        HX_RELEASE(pBuffer);
        result = HXR_FAIL;
    }
    if(m_pPendingReadBufAsync != NULL && result == HXR_OK)
    {
        if(IsReadError())
        {
            result = HXR_READ_ERROR;
        }
        if( result == HXR_OK )
        {
            result = m_pPendingReadBufAsync->SetSize(byteCount);
        }
        if(result == HXR_OK)
        {
            HX_RESULT readResult = byteCount > 0 ? HXR_OK : HXR_READ_ERROR;
            result = DoReadDone(readResult, m_pPendingReadBufAsync);
            m_pPendingReadBufAsync = NULL;
            if(result == HXR_OK)
            {
                if(m_pPendingReadBuf != NULL )
                {
                    DoReadLoop();
                }
            }
        }
        else
        {
            HX_RELEASE(m_pPendingReadBufAsync);
        }
        if(result != HXR_OK)
        {
            ReadDoneError(result);
        }
    }
}

#endif
/****************************************************************************
 *  IHXFileObject::DoRead
 *
 *  This routine performs the actual read() operation and returns the 
 *  count of the actual bytes read.
 */
STDMETHODIMP_(UINT32) CHXMiniFileObject::DoRead(IHXBuffer* pBuffer)
{
    UINT32 actualCount = 0;

#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    if(m_pDataSource)
    {
        actualCount = m_pDataSource->Read2(pBuffer, this);
    }
    else
#endif
    {
        // Read from the file directly into the buffer object
        actualCount = fread(pBuffer->GetBuffer(),
                            sizeof(UCHAR),
                            pBuffer->GetSize(), m_pFile);
    }

    return actualCount;
}


/****************************************************************************
 *  IHXFileObject::DoReadDone
 *
 *  This routine performs the ReadDone callback and on return will
 *  release the buffer. It sets the m_bInReadDone flag to indicate
 *  when we are in the middle of the ReadDone invocation to account for
 *  the possibility of an Read() embedded in the call flow of a ReadDone().
 */
STDMETHODIMP CHXMiniFileObject::DoReadDone(HX_RESULT readResult,
                                           IHXBuffer* pBuffer)
{
    HX_RESULT result = HXR_OK;

    m_bInReadDone = TRUE;
    // If heap gets low, memory allocations for new file read
    // buffers is one of the first places we notice it, and
    // if we're not careful to report this error, we can get
    // into a state where the renderer is stuck trying to
    // rebuffer, but the front end can't get any new data
    // because we can't allocate any new buffers. If we replace
    // this with a caching buffer system, this issue goes away.
    result = m_pFileResponse->ReadDone(readResult, pBuffer);
    m_bInReadDone = FALSE;

    // Release our handle on the buffer
    HX_RELEASE(pBuffer);

    return result;
}


/****************************************************************************
 *  IHXFileObject::ReadDoneError
 *
 *  This routine queries for the IHXErrorMessages object and reports
 *  the given error to it. Then, it invokes the ReadDone callback.
 */
STDMETHODIMP_(void) CHXMiniFileObject::ReadDoneError(HX_RESULT theError)
{
    IHXErrorMessages* pErrMsg = NULL;
    m_pContext->QueryInterface(IID_IHXErrorMessages, (void**)&pErrMsg);
	
    if (pErrMsg)
    {
        pErrMsg->Report(HXLOG_ERR, theError, 0, NULL , NULL);
        HX_RELEASE(pErrMsg);
    }


    m_pFileResponse->ReadDone(theError, NULL);
}        

HXBOOL CHXMiniFileObject::IsReadError()
{
    HXBOOL bError = FALSE;

#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    if(m_pDataSource && FAILED(m_pDataSource->GetLastError2(this)) )
#else
    if (m_pFile && ferror(m_pFile)) 
#endif
    {
        bError = TRUE;
    }
    return bError;
}

/****************************************************************************
 *  IHXFileObject::Write
 *
 *  This routine writes a block of data to the file. When writing has
 *  completed, the caller is asynchronously notified via the File Response
 *  object associated with this File Object. This method called by the File
 *  Format plug-in when it needs to write to the file.
 */
STDMETHODIMP CHXMiniFileObject::Write(IHXBuffer* pDataToWrite)
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Write()\n"));

    UINT32 byteCount =  pDataToWrite->GetSize();

    UINT32 actualCount = 0;

#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    if(m_pDataSource)
    {
        actualCount = m_pDataSource->Write2(pDataToWrite->GetBuffer(), 
                                             sizeof(UCHAR), byteCount, this);
    }
    else
#endif
    {
        actualCount = fwrite(pDataToWrite->GetBuffer(), sizeof(UCHAR), byteCount, m_pFile);
    }

    // Notify the caller that the write is done
    HX_RESULT writeResult = (actualCount == byteCount) ? HXR_OK : HXR_FAILED;
    m_pFileResponse->WriteDone(writeResult);

    return HXR_OK;
}


/****************************************************************************
 *  IHXFileObject::Seek
 *
 *  This routine moves to a given position in the file. The move can be
 *  either from the beginning of the file (absolute), or relative to the
 *  current file position. When seeking has completed, the caller is
 *  asynchronously notified via the File Response object associated with this
 *  File Object. This method called by the File Format plug-in when it needs
 *  to seek to a location within the file.
 */
STDMETHODIMP CHXMiniFileObject::Seek( UINT32 offset, HXBOOL bIsRelative )
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Seek()\n"));

    // Cancel any pending reads
    HX_RELEASE(m_pPendingReadBuf);
#if defined(HELIX_FEATURE_MMF_DATASOURCE)	
    if(m_bAsyncReadSupported && m_pPendingReadBufAsync)
    {
        //if the File format has ReadDone Pending cancel it here
        if (!m_bInReadDone)
        {
            m_pFileResponse->ReadDone(HXR_CANCELLED, NULL);
        }
        m_pPendingReadBufAsync = NULL;
    }
#endif	

    int fromWhere = SEEK_SET; // absolute
    if (bIsRelative == TRUE)
    {
        fromWhere = SEEK_CUR; // relative
    }

    int seekResult = HXR_FAIL;
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    if(m_pDataSource)
    {
        seekResult = m_pDataSource->Seek2(offset, fromWhere, this);
    }
    else
#endif
    {
    if (m_pFile)
        seekResult = fseek(m_pFile, offset, fromWhere);
    }

    // Notify the caller that the seek is done
    HX_RESULT result = !seekResult ? HXR_OK : HXR_FAILED;
    HX_RESULT retVal = m_pFileResponse->SeekDone(result);
    // Memory allocations can be attempted in the execution of SeekDone, and we
    // want to catch these if they fail.
    if( retVal != HXR_OUTOFMEMORY )
    {
        retVal = result;
    }

    return (retVal);
}


/****************************************************************************
 *  IHXFileObject::Advise
 *
 *  This routine is passed information about the intended usage of this
 *  object. The useage will indicate whether sequential or random requests
 *  for information will be made. This may be useful, for example, in
 *  developing a caching scheme.
 */
STDMETHODIMP CHXMiniFileObject::Advise(UINT32 ui/* useage */)
{
    HX_RESULT retVal = HXR_FAIL;
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Advise()\n"));

    if (ui == HX_FILEADVISE_NETWORKACCESS)
    {
	retVal = HXR_ADVISE_LOCAL_ACCESS;
    }

    return retVal;
}


/****************************************************************************
 *  IHXFileObject::Close
 *
 *  This routine closes the file resource and releases all resources
 *  associated with the object. This routine is crucial and must be called
 *  before the File Object is destroyed.
 */
STDMETHODIMP CHXMiniFileObject::Close(void)
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Close()\n"));
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    if(m_pDataSource)
    {
        m_pDataSource->Close2( this );
		HX_RELEASE(m_pDataSource);
    }
    else
#endif

    if (m_pFile != NULL)
    {
        fclose(m_pFile);
        m_pFile = NULL;
    }

    if (m_pRequest != NULL)
    {
        m_pRequest->Release();
        m_pRequest = NULL;
    }

#if defined (_WINDOWS ) && defined (_WIN32)
    if (m_hFileHandle)
    {
        ::FindClose( m_hFileHandle );
        m_hFileHandle = 0;
    }
#endif

    /*
     * Store this in temp so that if calling CloseDone
     * causes our descructor to get called we will
     * have pCallCloseDone on the stack to safely release.
     */
    IHXFileResponse* pCallCloseDone = m_pFileResponse;
    if (m_pFileResponse != NULL)
    {
        m_pFileResponse = NULL;
        pCallCloseDone->CloseDone(HXR_OK);
        pCallCloseDone->Release();
    }

    return HXR_OK;
}




// IHXRequestHandler Interface Methods

/****************************************************************************
 *  IHXRequestHandler::SetRequest
 *
 *  This routine associates this File Object with the file Request object
 *  passed to it from the Helix core. This Request object is primarily used to
 *  obtain the requested URL. This method is called just after the File
 *  Object is created.
 */
STDMETHODIMP CHXMiniFileObject::SetRequest(IHXRequest* pRequest)
{
    HX_RESULT res = HXR_FAIL;

    DPRINTF(D_MINI_FO, ("CHXMiniFO::SetRequest()\n"));

    // Reset the current request object and associated filename
    HX_RELEASE(m_pRequest);
    m_pRequest = pRequest;
    HX_VECTOR_DELETE(m_pFilename);

    if (m_pRequest)
    {
        m_pRequest->AddRef();

        const char* pURL;
        if (m_pRequest->GetURL(pURL) == HXR_OK && pURL)
        {
            // Ensure that we are working with an unescaped URL
#ifdef _MAC_UNIX
            res = HXR_OUTOFMEMORY;
            UInt32 urlLength = strlen(pURL);
            m_pFilename = new char [urlLength + 1];
            if (m_pFilename)
            {
                CFURLRef kNoBaseURL = NULL;
                CFURLRef theCFURL = CFURLCreateWithBytes(kCFAllocatorDefault, (const UInt8*) pURL, urlLength, kCFStringEncodingUTF8, kNoBaseURL);
                if (theCFURL)
                {
                    Boolean kResolveAgainstBase = true;
                    if (CFURLGetFileSystemRepresentation(theCFURL, kResolveAgainstBase, (UInt8*) m_pFilename, (urlLength + 1)))
                    {
                        res = HXR_OK;
                    }
                    CFRelease( theCFURL );
                }
            }
#else
            // Unescaped URL is at most same size or smaller
            INT32 cchEscaped = strlen(pURL);
            char* pUnescaped = new char [cchEscaped + 1];
            if( pUnescaped)
            {
                INT32 cchNew = URLUnescapeBuffer(pURL, cchEscaped, pUnescaped);
                if(cchNew > 0)
                {
                    pUnescaped[cchNew] = '\0';
                    res = ConvertToPlatformPath(m_pFilename, pUnescaped);
                }
                delete[] pUnescaped;

            }
#endif
        }
    }

    return res;
}


/****************************************************************************
 *  IHXRequestHandler::GetRequest
 *
 *  This routine retrieves the Request object associated with this File
 *  Object. It is called just after the SetRequest() method.
 */
STDMETHODIMP CHXMiniFileObject::GetRequest(REF(IHXRequest*) pRequest)
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::GetRequest()\n"));

    pRequest = m_pRequest;
    if (pRequest != NULL)
    {
        pRequest->AddRef();
    }

    return HXR_OK;
}


/****************************************************************************
 *  IHXFileExists::DoesExist
 *
 *  This routine determines if the given file exists, and notifies the File
 *  Response object. It is called by the server after the File Object has
 *  been created to determine if the requested file does actually exist. If
 *  it does the File Object is handed off to the File Format object.
 */
STDMETHODIMP CHXMiniFileObject::DoesExist( const char* pFilePath, IHXFileExistsResponse* pFileResponse )
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::DoesExist()\n"));

    HXBOOL  bDoesExist = FALSE;
    char* pFilePathPlatform   = NULL;

    // Convert file path to platform specific file path
    HX_RESULT result = ConvertToPlatformPath(pFilePathPlatform, pFilePath);

    // Determine if the file can be opened
    if (result == HXR_OK)
    {
        struct stat statbuf;
        if (stat(pFilePathPlatform, &statbuf) == 0)
        {
            bDoesExist = TRUE;
        }
    }

    // Notify the caller if the file exists
    pFileResponse->DoesExistDone(bDoesExist);

    if (pFilePathPlatform != NULL)
    {
        delete [] pFilePathPlatform;
    }

    return HXR_OK;
}


// IUnknown COM Interface Methods

/****************************************************************************
 *  IUnknown::AddRef
 *
 *  This routine increases the object reference count in a thread safe
 *  manner. The reference count is used to manage the lifetime of an object.
 *  This method must be explicitly called by the user whenever a new
 *  reference to an object is used.
 */
STDMETHODIMP_(UINT32) CHXMiniFileObject::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


/****************************************************************************
 *  IUnknown::Release
 *
 *  This routine decreases the object reference count in a thread safe
 *  manner, and deletes the object if no more references to it exist. It must
 *  be called explicitly by the user whenever an object is no longer needed.
 */
STDMETHODIMP_(UINT32) CHXMiniFileObject::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


/****************************************************************************
 *  IUnknown::QueryInterface
 *
 *  This routine indicates which interfaces this object supports. If a given
 *  interface is supported, the object's reference count is incremented, and
 *  a reference to that interface is returned. Otherwise a NULL object and
 *  error code are returned. This method is called by other objects to
 *  discover the functionality of this object.
 */
STDMETHODIMP CHXMiniFileObject::QueryInterface(REFIID interfaceID, void** ppInterfaceObj)
{
    QInterfaceList qiList[] =
    {
	{ GET_IIDHANDLE(IID_IUnknown), (IUnknown*)(IHXFileObject*)this },
	{ GET_IIDHANDLE(IID_IHXFileObject), (IHXFileObject*) this },
	{ GET_IIDHANDLE(IID_IHXDirHandler), (IHXDirHandler*) this },
	{ GET_IIDHANDLE(IID_IHXRequestHandler), (IHXRequestHandler*) this },
	{ GET_IIDHANDLE(IID_IHXFileExists), (IHXFileExists*) this },
	{ GET_IIDHANDLE(IID_IHXFileStat), (IHXFileStat*) this },
	{ GET_IIDHANDLE(IID_IHXGetFileFromSamePool), (IHXGetFileFromSamePool*) this },
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
	{ GET_IIDHANDLE(IID_IHXMMFDataSourceObserver), (IHXMMFDataSourceObserver*) this },
#endif
    };
    return ::QIFind(qiList, QILISTSIZE(qiList), interfaceID, ppInterfaceObj);
}

/************************************************************************
 * Method:
 *      IHXFileObject::Stat
 * Purpose:
 *      Collects information about the file that is returned to the
 *      caller in an IHXStat object
 */
STDMETHODIMP CHXMiniFileObject::Stat(IHXFileStatResponse* pFileStatResponse)
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::Stat()\n"));

    struct stat StatBuffer;

    if((m_pFile && fstat((int)fileno(m_pFile), &StatBuffer) == 0) ||
        stat(m_pFilename, &StatBuffer) == 0)
    {
	pFileStatResponse->StatDone(HXR_OK,
				    StatBuffer.st_size,
				    StatBuffer.st_ctime,
				    StatBuffer.st_atime,
				    StatBuffer.st_mtime,
				    StatBuffer.st_mode);
	return HXR_OK;
    }
#if defined(HELIX_FEATURE_MMF_DATASOURCE)
    else if(m_pDataSource)
    {
        if(m_pDataSource->Stat(&StatBuffer) == HXR_OK)
        {
	        pFileStatResponse->StatDone(HXR_OK,
				    StatBuffer.st_size,
				    StatBuffer.st_ctime,
				    StatBuffer.st_atime,
				    StatBuffer.st_mtime,
				    StatBuffer.st_mode);
        }
	    return HXR_OK;
    }
#endif

    return HXR_FAIL;
}

STDMETHODIMP CHXMiniFileObject::InitDirHandler( IHXDirHandlerResponse* pDirResponse )
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::InitDirHandler()\n"));

    m_pDirResponse = pDirResponse;
    m_pDirResponse->AddRef();
    m_pDirResponse->InitDirHandlerDone(HXR_OK);

    return HXR_OK;
}

STDMETHODIMP CHXMiniFileObject::CloseDirHandler()
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::CloseDirHandler()\n"));

    m_pDirResponse->CloseDirHandlerDone(HXR_OK);
    m_pDirResponse->Release();
    m_pDirResponse = NULL;

    return HXR_OK;
}

STDMETHODIMP CHXMiniFileObject::MakeDir()
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::MakeDir()\n"));

#if defined (_WINDOWS ) || defined (_WIN32)

        
    if( ::CreateDirectory(OS_STRING(m_pFilename), NULL) == 0 )
        return HXR_FAIL;

#elif defined UNIX
    if(mkdir(m_pFilename, 0x644) < 0)
        return HXR_FAIL;
#else
    //XXXGH...don't know how to do this on Mac
    return HXR_FAIL;
#endif

    m_pDirResponse->MakeDirDone(HXR_OK);

    return HXR_OK;
}

STDMETHODIMP CHXMiniFileObject::ReadDir()
{
    DPRINTF(D_MINI_FO, ("CHXMiniFO::ReadDir()\n"));

    char pDirname[1024];

#if defined (_WINDOWS ) && defined (_WIN32)
    
    if (!m_hFileHandle)
    {
        strcpy(pDirname, m_pFilename);
        strcat(pDirname, "\\*");

        m_hFileHandle = ::FindFirstFile( OS_STRING(pDirname), &m_FileInfo );

        if ( INVALID_HANDLE_VALUE == m_hFileHandle )
        {
            m_hFileHandle = 0;
            return HXR_FAILED;
        }
    }
    else
    {
        if ( ::FindNextFile(m_hFileHandle, &m_FileInfo) == 0 )
        {
            ::FindClose( m_hFileHandle );
            m_hFileHandle = 0;
            m_pDirResponse->ReadDirDone(HXR_FILE_NOT_FOUND, 0);
            return HXR_OK;
        }
    }

    strcpy( pDirname, OS_STRING(m_FileInfo.cFileName) );
#else
    return HXR_FAIL;
#endif

    HX_RESULT result;

    IHXBuffer* pBuffer = 0;

    result = m_pClassFactory->CreateInstance(CLSID_IHXBuffer, (void**)&pBuffer);

    if (HXR_OK != result)
    {
        return result;
    }

    pBuffer->Set( (Byte*) pDirname, strlen(pDirname) + 1 );
    m_pDirResponse->ReadDirDone(HXR_OK, pBuffer);
    pBuffer->Release();

    return HXR_OK;
}

/************************************************************************
 *      Method:
 *          IHXFileObject::GetFileObjectFromPool
 *      Purpose:
 *      To get another FileObject from the same pool.
 */
STDMETHODIMP CHXMiniFileObject::GetFileObjectFromPool( IHXGetFileFromSamePoolResponse* response )
{

    DPRINTF(D_MINI_FO, ("CHXMiniFO::GetFileObjectFromPool()\n"));

    HX_RESULT lReturnVal = HXR_OUTOFMEMORY;

    if ( m_pFilename )
    {
        delete [] m_pBasePath;
        m_pBasePath = new char[strlen(m_pFilename) + 1];

        if (m_pBasePath)
        {
            // Chop off the current path to create a base path.
            strcpy(m_pBasePath, m_pFilename);
            char* pLastSeparator = strrchr(m_pBasePath, OS_SEPARATOR_CHAR);
            *pLastSeparator = '\0';

            CHXMiniFileObject* pFileObject = new CHXMiniFileObject(m_pClassFactory, m_pBasePath, m_pContext);

            if (pFileObject)
            {
                IUnknown* pUnknown = 0;
                lReturnVal = pFileObject->QueryInterface(IID_IUnknown, (void**)&pUnknown);
                response->FileObjectReady((lReturnVal == HXR_OK) ? HXR_OK : HXR_FAILED, pUnknown);
                HX_RELEASE(pUnknown);
            }
        }
    }

    return lReturnVal;
}
