/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fastfile.h,v 1.5 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _FASTFILE_H_
#define _FASTFILE_H_

_INTERFACE IHXFileObject;
_INTERFACE IHXFileObjectExt;
_INTERFACE IHXCallback;
_INTERFACE IHXRequestHandler;
_INTERFACE IHXGetFileFromSamePool;
_INTERFACE IHXGetFileFromSamePoolResponse;
_INTERFACE IHXFileStat;
_INTERFACE IHXFilePlacementRead;
_INTERFACE IHXRegistry;
//_INTERFACE IHXMutex;
_INTERFACE IHXFastAlloc;

#include "mutex.h"

class FastFile;
class FastFileResponse;
class FastFileBlock;
class FastFileBlockManager;
class FastFileBuffer;
class FastFileStats;
class FastFileFactory;
class CDistMIIStatistics;

#ifdef xprintf
#undef xprintf
#endif
#ifdef xxprintf
#undef xxprintf
#endif
//#define xprintf(x) printf x, fflush(0)
#define xprintf(x) /**/
//#define xxprintf(x) printf x, fflush(0)
#define xxprintf(x) /**/

#define USE_FAST_CACHE_MEM

#if 1
#define LOCK2(x,y)   HXMutexLock(&x); \
                      xprintf(("LOCK %s=%p at %s:%d\n", \
		             (y), (&x), __FILE__, __LINE__))
#define UNLOCK2(x,y) HXMutexUnlock(&x); \
                      xprintf(("UNLOCK %s=%p at %s:%d\n", \
		             (y), (&x), __FILE__, __LINE__))
#else
#define LOCK2(x,y) /**/
#define UNLOCK2(x,y) /**/
#endif


//////////////////////////////////////////////////////////////////////
// FastFile
//////////////////////////////////////////////////////////////////////
class FastFile : public IHXFileObject
               , public IHXCallback
               , public IHXRequestHandler
               , public IHXGetFileFromSamePool
	       , public IHXFileStat
{
public:
    /*
     *	IUnknown methods
     */
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID		  /*IN*/  riid,
    	    	    	    	void**		  /*OUT*/ ppvObj);

    /*
     *	IHXFileObject methods
     */
    STDMETHOD(Init)		(THIS_
				ULONG32		  /*IN*/  ulFlags,
				IHXFileResponse* /*IN*/  pFileResponse);

    STDMETHOD(Close)	    	(THIS);

    STDMETHOD(Read)		(THIS_
    	    	    	    	ULONG32	    	  /*IN*/  ulCount);

    STDMETHOD(Write)	    	(THIS_
    	    	    	    	IHXBuffer*	  /*IN*/  pBuffer);

    STDMETHOD(Seek)		(THIS_
    	    	    	    	 ULONG32	  /*IN*/  ulOffset,
				 BOOL             /*IN*/  bRelative);

    STDMETHOD(Advise)		(THIS_
				ULONG32 	  /*IN*/  ulInfo);

    STDMETHOD(GetFilename)      (THIS_
                                 REF(const char*) /*OUT*/ pFilename);

    /*
     *	IHXCallback methods
     */
    STDMETHOD(Func)(THIS);

    /*
     *  IHXRequestHandler methods
     */
    STDMETHOD(SetRequest)       (THIS_
                                 IHXRequest*      /*IN*/  pRequest);

    STDMETHOD(GetRequest)       (THIS_
                                 REF(IHXRequest*) /*OUT*/ pRequest);

    /*
     * IHXGetFileFromSamePool methods
     */
    STDMETHOD(GetFileObjectFromPool) (THIS_
                                      IHXGetFileFromSamePoolResponse* pResp);

    /*
     * IHXFileStat methods
     */
    STDMETHOD(Stat)             (THIS_
                                 IHXFileStatResponse* /*IN*/ pResp);

    /*
     *	FastFileResponse callback methods
     */
    HX_RESULT InitDone  (HX_RESULT /*IN*/ status);
    HX_RESULT CloseDone (HX_RESULT /*IN*/ status);
    HX_RESULT ReadDone  (HX_RESULT /*IN*/ status, IHXBuffer* /*IN*/ pBuffer);
    HX_RESULT WriteDone (HX_RESULT /*IN*/ status);
    HX_RESULT SeekDone  (HX_RESULT /*IN*/ status);
    HX_RESULT FileObjectReady (HX_RESULT /*IN*/ status, IUnknown* /*IN*/ pUnk);
    HX_RESULT StatDone  (HX_RESULT /*IN*/ status,
                         UINT32    /*IN*/ ulSize,
                         UINT32    /*IN*/ ulCreationTime,
                         UINT32    /*IN*/ ulAccessTime,
                         UINT32    /*IN*/ ulModificationTime,
                         UINT32    /*IN*/ ulMode);

    /*
     *  other public methods
     */
    FastFile			(IUnknown*	  /*IN*/  pFob,
				 const char*      /*IN*/  pURL,
                         	 IUnknown*        /*IN*/  pContext,
				 UINT32           /*IN*/  ulBlockSize,
				 FastFileFactory* /*IN*/  pFastFileFactory,
				 Dict*            /*IN*/  pFastFileDict,
				 CDistMIIStatistics* /*IN*/ pMIIStats,
                                 BOOL             /*IN*/  bAlignReads = FALSE,
                                 BOOL             /*IN*/  bCacheStats = TRUE,
                                 UINT32           /*IN*/  ulMaxBlockSize = 0,
                                 UINT32           /*IN*/  ulMaxMemUse = 0);

    FastFileBlockManager* GetBlockManager(const char* /*IN*/ pURL,
                                          UINT32*     /*IN*/ pBlockCount,
                                          UINT32*     /*IN*/ pInUseBlockCount);
private:
    /*
     *	other private methods
     */

    ~FastFile(void);

    HX_RESULT DoRead(void);

    friend class FastFileResponse;

    /*
     *	private member variables
     */
    ULONG32		m_ulRefCount;
    IUnknown* 		m_pContext;
    IHXThreadSafeScheduler* m_pScheduler;
    IHXRegistry*	m_pRegistry;
    IHXFileObject*    	m_pFileObject;
    IHXFileObjectExt*	m_pFileObjectExt;
    IHXFilePlacementRead* m_pFileObjectPlacementRead;
    IHXFileStat*    	m_pFileStat;
    IHXFastFileStats*  m_pFastFileStatReport;
    IHXRequestHandler* m_pRequestHandler;
    IHXGetFileFromSamePool* m_pPool;
    IHXGetFileFromSamePoolResponse* m_pPoolResponse;
    char*               m_pURL;
    FastFileBlockManager* m_pBlockMgr;
    UINT32		m_ulChunkSize;
    UINT32		m_ulExtraSlopSize;
    UINT32		m_ulMaxAddressSpaceUsed;
    UINT32		m_ulCurrentAddressSpaceUsed;
    UINT32		m_ulMaxRecursionLevel;
    FastFileResponse*	m_pFastFileResponse;
    IHXFileResponse*   m_pRealResponse;
    IHXFileStatResponse* m_pRealFileStatResponse;
    BOOL                m_bRealFileStatPending;
    UINT32		m_ulRealReadPos;
    UINT32		m_ulRealReadSize;
    BOOL		m_bRealReadPending;
    UINT32		m_ulInternalReadPos;
    BOOL		m_bInternalReadPending;
    BOOL		m_bInternalSeekPending;
    UINT32		m_ulReadRecursionLevel;
    UINT32		m_ulFileSize;
    UINT32		m_ulFileCTime;
    UINT32		m_ulFileATime;
    UINT32		m_ulFileMTime;
    UINT32		m_ulFileMode;
    UINT32              m_ulAlignmentReadAmount;
    UINT32              m_ulAlignmentReadOffset;
    char*               m_pAlignmentReadBuf;
    BOOL                m_bAlignReads;
    BOOL		m_bClosed;
    FastFileBlock*      m_pLastBlock;

    UINT32*		m_pFobCount;
    UINT32*		m_pBlockCount;
    UINT32*		m_pInUseBlockCount;
    UINT32*		m_pAggFastBytesRead;
    UINT32*		m_pAggSlowBytesRead;
    UINT32*		m_pAggInternalBytesRead;

    UINT32		m_ulFastBytesRead;
    UINT32		m_ulSlowBytesRead;

    UINT32		m_ulDefaultChunkSize;
    FastFileFactory*	m_pFastFileFactory;
    Dict*		m_pFastFileDict;
    BOOL                m_bCacheStats;
    IMalloc*            m_pIMalloc;
    ULONG32             m_ulSectorAlignment;
    ULONG32             m_ulStartReadPos;
    CDistMIIStatistics* m_pMIIStats;
    BOOL                m_bReadPending;
    BOOL                m_bReadCancelled;
    BOOL                m_bReportedLimitReached;
    HXTime              m_ReadStart;
    HXTime              m_ReadEnd;
    double              m_ReadDuration;
    UINT32              m_ulBytesSinceLastRead;
    UINT32              m_ulMaxBlockSize;
    UINT32              m_ulMaxMemUse;
    UINT32*             m_pMemUse;
    BOOL                m_bDynamicReadSizing;
};


//////////////////////////////////////////////////////////////////////
// FastFileResponse
//////////////////////////////////////////////////////////////////////
class FastFileResponse : public IHXFileResponse
                       , public IHXFileStatResponse
                       , public IHXGetFileFromSamePoolResponse
{
public:
    /*
     *	IUnknown methods
     */
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID		/*IN*/  riid,
    	    	    	    	void**		/*OUT*/ ppvObj);

    /*
     *	IHXFileResponse methods
     */
    STDMETHOD(InitDone)     	(THIS_
                            	HX_RESULT	/*IN*/  status);

    STDMETHOD(CloseDone)        (THIS_
                                HX_RESULT	/*IN*/  status);

    STDMETHOD(ReadDone)         (THIS_
                                HX_RESULT	/*IN*/  status,
                                IHXBuffer*	/*IN*/  pBuffer);

    STDMETHOD(WriteDone)        (THIS_
                                HX_RESULT	/*IN*/  status);

    STDMETHOD(SeekDone)         (THIS_
                                HX_RESULT	/*IN*/  status);

    /*
     *  IHXFileStatResponse methods
     */
     STDMETHOD(StatDone)        (THIS_
                                 HX_RESULT status,
                                 UINT32 ulSize,
                                 UINT32 ulCreationTime,
                                 UINT32 ulAccessTime,
                                 UINT32 ulModificationTime,
                                 UINT32 ulMode);

    /*
     * IHXGetFileFromSamePoolResponse
     */
    STDMETHOD(FileObjectReady) (THIS_
                                HX_RESULT status,
				IUnknown* ppUnknown);

    /*
     *	other public methods
     */
    FastFileResponse		(FastFile*	   /*IN*/ pFastFile);


private:
    /*
     *	other private methods
     */
    ~FastFileResponse(void);

    friend class FastFile;
    inline void SetFastFile (FastFile* pFastFile) { m_pFastFile = pFastFile; };

    /*
     *	private member variables
     */
    ULONG32			m_ulRefCount;
    FastFile*			m_pFastFile;
};


//////////////////////////////////////////////////////////////////////
// FastFileBlock
//////////////////////////////////////////////////////////////////////
class FastFileBlock
{
public:
    /*
     *  Misc public methods
     */
#ifdef USE_FAST_CACHE_MEM
    FAST_CACHE_MEM
#endif
    FastFileBlock(UINT32*                 /*IN*/ pBlockCount,
                  UINT32*                 /*IN*/ pInUseBlockCount,
                  IHXCommonClassFactory* /*IN*/ pClassFactory,
                  UINT32*                 /*IN*/ pMemUse);

    inline void AddRef()
    {
        INT32 ref = InterlockedIncrement(&m_ulRefCount);

        // if we just incremented from 1 to 2 then the underlying block
        // data is being accessed
        if (ref == 2)
            HXAtomicIncUINT32(m_pInUseBlockCount);
    };

    inline void Release()
    {
        INT32 ref = InterlockedDecrement(&m_ulRefCount);
        if (ref > 0)
	{
            // if (we just decremented from 2 to 1 then the underlying block
            // is no longer being accessed
            if (ref == 1)
                HXAtomicDecUINT32(m_pInUseBlockCount);
	    return;
	}
	delete this;
    };

    friend class FastFileBlockManager;

private:
    ~FastFileBlock();

    inline ULONG32 GetRefCount() { return m_ulRefCount; };

    /*
     *  Misc private variables
     */
    ULONG32	m_ulRefCount;
    IHXBuffer* m_pData;
    UINT32	m_ulOffset;
    UINT32	m_ulSize;
    UINT32*	m_pBlockCount;
    UINT32*	m_pInUseBlockCount;
    BOOL        m_bEOF;
    UINT32*	m_pMemUse;
};


//////////////////////////////////////////////////////////////////////
// FastFileBlockManager
//////////////////////////////////////////////////////////////////////
class FastFileBlockManager
{
public:
    /*
     *  Misc public methods
     */
    FastFileBlockManager(const char* /*IN*/ pURL,
                         IUnknown*   /*IN*/ pContext,
			 UINT32*     /*IN*/ pBlockCount,
			 UINT32*     /*IN*/ pInUseBlockCount,
			 Dict*       /*IN*/ pDict,
                         UINT32*     /*IN*/ pMemUse);

    inline void AddRef()
    {
	m_ulRefCount++;
    };

    inline void Release()
    {
	m_ulRefCount--;
	if (m_ulRefCount == 0) delete this;
    };

    static FastFileBlockManager* GetManager (const char* /*IN*/ pURL,
                                             IUnknown*   /*IN*/ pContext,
			                     UINT32*     /*IN*/ pBlockCount);

    HX_RESULT GetBuffer (UINT32              /*IN*/     ulOffset,
                         UINT32              /*IN*/     ulCount,
                         BOOL                /*IN*/     bShortBufferOK,
			 REF(FastFileBuffer*)/*OUT*/    pBuffer,
			 REF(FastFileBlock*) /*IN/OUT*/ pLastBlock);

    HX_RESULT AddBlock (UINT32      /*IN*/ ulOffset,
                        IHXBuffer* /*IN*/ pBuffer,
                        BOOL        /*IN*/ bEOF,
                        UINT32*     /*OUT*/ pMemUse);

    HX_RESULT RemoveFromDictionary();
    HX_RESULT VerifyFileStats(size_t ulSize,
                              UINT32 ulCreationTime,
                              UINT32 ulModificationTime);
private:
    ~FastFileBlockManager();

    UINT32 IsInBlock (UINT32         /*IN*/  ulOffset,
                      UINT32         /*IN*/  ulCount,
                      UINT32         /*IN*/  ulBlockOffset,
                      UINT32         /*IN*/  ulBlockSize,
	              BOOL           /*IN*/  bShortBufferOK);
    /*
     *  Misc private variables
     */
    ULONG32		m_ulRefCount;
    FastFileBlock**	m_pBlockList;
    UINT32		m_ulBlockListSize;
    UINT32		m_ulBlockListEnd;
    char*               m_pURL;
    IHXFastAlloc*      m_pFastAlloc;
    UINT32*             m_pBlockCount;
    UINT32*             m_pInUseBlockCount;
    Dict*		m_pFastFileDict;
    IHXCommonClassFactory* m_pClassFactory;
    BOOL                m_bRemoved;
    size_t              m_ulSize;
    UINT32              m_ulCreationTime;
    UINT32              m_ulModificationTime;
        
//    IHXMutex*		m_pFastFileDictMutex;
//    IHXMutex*		m_pBlockListMutex;
};


//////////////////////////////////////////////////////////////////////
// FastFileBuffer
//////////////////////////////////////////////////////////////////////
class FastFileBuffer : public IHXBuffer
{
public:
    /*
     *  IUnknown methods
     */
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID		  /*IN*/  riid,
    	    	    	    	void**		  /*OUT*/ ppvObj);

    /*
     *  IHXBuffer methods
     */
    STDMETHOD(Get)              (THIS_
                                REF(UCHAR*)       /*OUT*/ pData,
                                REF(ULONG32)      /*OUT*/ ulLength);
    STDMETHOD(Set)              (THIS_
                                const UCHAR*      /*IN*/  pData,
                                ULONG32           /*IN*/  ulLength);
    STDMETHOD(SetSize)          (THIS_
                                ULONG32           /*IN*/  ulLength);
    STDMETHOD_(ULONG32,GetSize) (THIS);
    STDMETHOD_(UCHAR*,GetBuffer)(THIS);

    /*
     *  Misc public methods
     */
#ifdef USE_FAST_CACHE_MEM
    FAST_CACHE_MEM
#endif
    FastFileBuffer(UINT32                  /*IN*/ ulInitialRefCount,
                   IHXCommonClassFactory* /*IN*/ pFactory);

    friend class FastFile;
    friend class FastFileBlockManager;

private:
    /*
     *  Misc private methods
     */
    ~FastFileBuffer(void);

    inline ULONG32 GetRefCount() { return m_ulRefCount; };

    inline FastFileBlock* GetBlock() { return m_pBlock; };

    HX_RESULT SetBlock (FastFileBlock* /*IN*/ pBlock,
                        Byte*          /*IN*/ pData,
			UINT32         /*IN*/ ulDataSize);
 
    /*
     *  Misc private variables
     */
    ULONG32        m_ulRefCount;
    FastFileBlock* m_pBlock;
    Byte*          m_pData;
    UINT32         m_ulCurrentSize;
    BOOL           m_bPrivateData;
};

#endif //_FASTFILE_H_
