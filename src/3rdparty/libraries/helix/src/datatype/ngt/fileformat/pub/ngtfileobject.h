/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ngtfileobject.h,v 1.3 2006/08/17 01:03:34 milko Exp $
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

#ifndef _NGTFILEOBJECT_H_
#define _NGTFILEOBJECT_H_

/****************************************************************************
 *  Defines
 */
#define NGTFF_INVALID_TIMESTAMP 0xFFFFFFFF


/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxformt.h"
#include "hxfiles.h"
#include "hxerror.h"
#include "hxmeta.h"

#include "unkimp.h"
#include "baseobj.h"

#include "ngtmetafile.h"


/****************************************************************************
 *  Globals
 */


/****************************************************************************
 * 
 *  Class:
 *	CNGTFileObject
 *
 *  Purpose:
 *	Implements Media Nugget File Object abstraction
 */
class CNGTFileObject :	public CUnknownIMP,
			public IHXFileObject,
			public IHXFileStat,
			public IHXFileResponse,
			public IHXFileStatResponse
{
    DECLARE_UNKNOWN(CNGTFileObject)

public:
    /*
     *	Constructor/Destructor
     */
    CNGTFileObject(void);

    ~CNGTFileObject();


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
    STDMETHOD(Init)     (THIS_
                        ULONG32            /*IN*/  ulFlags,
                        IHXFileResponse*   /*IN*/  pFileResponse);

    /************************************************************************
     *  Method:
     *      IHXFileObject::GetFilename
     *  Purpose:
     *      Returns the filename (without any path information) associated
     *      with a file object.
     *
     *      Note: The returned pointer's lifetime expires as soon as the
     *      caller returns from a function which was called from the RMA
     *      core (i.e. when you return control to the RMA core)
     *
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
    STDMETHOD(Close)    (THIS);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Read
     *  Purpose:
     *      Reads a buffer of data of the specified length from the file
     *      and asynchronously returns it to the caller via the
     *      IHXFileResponse interface passed in to Init.
     */
    STDMETHOD(Read)     (THIS_
                        ULONG32 ulCount);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Write
     *  Purpose:
     *      Writes a buffer of data to the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     */
    STDMETHOD(Write)    (THIS_
                        IHXBuffer* pBuffer);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Seek
     *  Purpose:
     *      Seeks to an offset in the file and asynchronously notifies
     *      the caller via the IHXFileResponse interface passed in to Init,
     *      of the completeness of the operation.
     *      If the bRelative flag is TRUE, it is a relative seek; else
     *      an absolute seek.
     */
    STDMETHOD(Seek)     (THIS_
                        ULONG32 ulOffset,
                        HXBOOL    bRelative);

    /************************************************************************
     *  Method:
     *      IHXFileObject::Advise
     *  Purpose:
     *      To pass information to the File Object advising it about usage
     *      heuristics.
     */
    STDMETHOD(Advise)   (THIS_
                        ULONG32 ulInfo);


    /*
     *  IHXFileStat methods
     */
    /************************************************************************
     *	Method:
     *	    Stat
     *	Purpose:
     *	    Collects information about the file that is returned to the
     *      caller in an IHXStat object
     */
    STDMETHOD(Stat)		(THIS_
				IHXFileStatResponse* pFileStatResponse);


    /*
     *  IHXFileResponse methods
     */
    /************************************************************************
     *  Method:
     *      IHXFileResponse::InitDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      initialization of the file is complete. If the file is not valid
     *      for the file system, the status HXR_FAILED should be
     *      returned.
     */
    STDMETHOD(InitDone)                 (THIS_
                                        HX_RESULT           status);

    /************************************************************************
     *  Method:
     *      IHXFileResponse::CloseDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      close of the file is complete.
     */
    STDMETHOD(CloseDone)                (THIS_
                                        HX_RESULT           status);

    /************************************************************************
     *  Method:
     *      IHXFileResponse::ReadDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      last read from the file is complete and a buffer is available.
     */
    STDMETHOD(ReadDone)                 (THIS_
                                        HX_RESULT           status,
                                        IHXBuffer*          pBuffer);

    /************************************************************************
     *  Method:
     *      IHXFileResponse::WriteDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      last write to the file is complete.
     */
    STDMETHOD(WriteDone)                (THIS_
                                        HX_RESULT           status);

    /************************************************************************
     *  Method:
     *      IHXFileResponse::SeekDone
     *  Purpose:
     *      Notification interface provided by users of the IHXFileObject
     *      interface. This method is called by the IHXFileObject when the
     *      last seek in the file is complete.
     */
    STDMETHOD(SeekDone)                 (THIS_
                                        HX_RESULT           status);


    /*
     *  IHXFileStatResponse method
     */
    STDMETHOD(StatDone)         (THIS_
                                 HX_RESULT status,
                                 UINT32 ulSize,
                                 UINT32 ulCreationTime,
                                 UINT32 ulAccessTime,
                                 UINT32 ulModificationTime,
                                 UINT32 ulMode);

    /*
     *	Public Meta-file methods
     */
    HX_RESULT InitNugget(IUnknown* pContext,
			 IHXFileObject* pFileObject, 
			 UINT32 ulFlags, 
			 IHXMetaFileFormatResponse* pResponse,
			 HXBOOL bForceNew = FALSE);
    void CloseNugget(void);

    HXBOOL IsInitializedForCreation(void)	{ return m_eMetaFileState == NGTFO_InitializedForCreation; }
    UINT16 GetNuggetVersion(void)		{ return m_metaFileHeader.m_uFileVersion; }
    UINT32 GetNuggetExpiration(void)		{ return m_metaFileBody.m_ulExpiration; }
    UINT32 GetNuggetConnectTime(void)		{ return m_metaFileBody.m_ulConnectTime; }
    UINT32 GetNuggetLocalDuration(void)		{ return m_metaFileBody.m_ulLocalDuration; }
    UINT32 GetNuggetOverallDuration(void)	{ return m_metaFileBody.m_ulOverallDuration; }
    const char* GetNuggetLocalMimeType(void)	{ return m_metaFileBody.m_pLocalMimeType; }
    const char* GetNuggetRemoteSourceURL(void)	{ return m_metaFileBody.m_pRemoteSourceURL; }
    HXBOOL IsExpired(void);
    HXBOOL IsKnownVersion(void)			{ return (m_metaFileHeader.m_uFileVersion <= NGTMETAFILE_CURRENT_VERSION); }

    void SetNuggetLocalDuration(UINT32 ulLocalDuration);
    HXBOOL ConfigureNugget(UINT32 ulExpiration,
			   UINT32 ulConnectTime,
			   UINT32 ulLocalDuration,
			   UINT32 ulOverallDuration,
			   const char* pLocalMimeType,
			   const char* pRemoteSourceURL);
        
protected:
    // Order of states below matters
    typedef enum
    {
	NGTFO_Offline,
	NGTFO_Closing,
	NGTFO_Initializing,
	NGTFO_ReadingHeader,
	NGTFO_ReadingBody,
	NGTFO_InitializedForCreation,
	NGTFO_InitializedForUpdate,
    } NGTFileObjectState;

    typedef enum
    {
	NGTFOIMPL_Initializing,
	NGTFOIMPL_Initialized,
	NGTFOIMPL_Closing,
	NGTFOIMPL_Closed
    } NGTFileObjectImplementationState;

    HX_RESULT DelegatedQI(REFIID riid, void** ppvObj);

    HXBOOL IsMetaFileInitialized(void)	
    { 
	return (m_eMetaFileState >= NGTFO_InitializedForCreation); 
    }
    HX_RESULT StartClosingFileObject(void);
    HX_RESULT WriteMetaFile(HX_RESULT entryStatus);
    void CloseCoreAttributes(void);

    IUnknown* m_pContext;
    IHXFileObject* m_pFileObject;
    IHXFileStat* m_pFileStat;
    NGTFileObjectImplementationState m_eFileObjectState;
    IHXBuffer* m_pDeferredInitialMediaWrite;
    
    IHXMetaFileFormatResponse* m_pMetaFileResponse;
    IHXFileResponse* m_pFileResponse;
    IHXFileStatResponse* m_pFileStatResponse;

    CNGTMetaFileHeader m_metaFileHeader;
    CNGTMetaFileBody m_metaFileBody;

    UINT32 m_ulFlags;
    UINT32 m_ulMetaFileSize;
    HXBOOL m_bMetaFileUpdateNeeded;
    NGTFileObjectState m_eMetaFileState;
};

#endif	// _NGTFILEOBJECT_H_
