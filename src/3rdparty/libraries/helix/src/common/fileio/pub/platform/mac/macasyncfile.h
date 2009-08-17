/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macasyncfile.h,v 1.6 2005/03/14 19:36:31 bobclark Exp $
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

/*

   Macintosh File buffer

   This is a class which pretends to be a file, yet that reads from the file asynchronously.

   It makes the file async calls which read the file, the callback of which spews into a 
   buffer. This is done by using a timer callback which gets called every 1/2 second to 
   spew more data into the buffer. 

   The global list is meant to be used for storage of all the different file objects
   which will get processed each time through the loop.  

   This will significantly enhance the Macintosh when it comes to local playback of files. 

   Each file acquires a seperate buffer of 64k which is used to buffer file data.
 */

#ifndef __MACASYNCFILE_H
#define __MACASYNCFILE_H

#include "hxtypes.h"
#include "cmacfile.h"
#include "hxslist.h"
#include "cbbqueue.h"
#include "hxthread.h"
#ifndef _MAC_MACHO
#include <Timer.h>
#include <OSUtils.h>
#endif

enum HXAsyncBufferState
{
    AB_STATE_EMPTY = 0,
    AB_STATE_FULL,
    AB_STATE_OUTOFCONTEXT,
    AB_STATE_IGNORED,
    AB_STATE_ASYNC_SEEK,	/* asynch seek */
    AB_STATE_ASYNC_INTERNAL_SEEK,	/* asynch internal seek */
    AB_STATE_WRITE
};

struct HXAsyncQueueBuffer
{
    HXAsyncQueueBuffer ()
    {
	buffer = NULL;
	size = 0;
	state = AB_STATE_EMPTY;
    current_buf_ptr	= 0;
    current_size = 0;
    position_in_file = 0;

    /* msAllocatedBufferCount++;
    char str[64];
    ::sprintf(str, "HXAsyncQueueBuffer constructor, Buffer Count = %d;g", (int) msAllocatedBufferCount);
    DebugStr(c2pstr(str)); */
    }

    ~HXAsyncQueueBuffer ()
    {
	if (buffer)
	{
	    delete[] buffer;
	}
	
    /* msAllocatedBufferCount--;
    char str[64];
    ::sprintf(str, "HXAsyncQueueBuffer destructor, Buffer Count = %d;g", (int) msAllocatedBufferCount);
    DebugStr(c2pstr(str)); */
    }

	// static ULONG32 msAllocatedBufferCount;
    char* buffer;
    char* current_buf_ptr;
    ULONG32 current_size;
    ULONG32 size;
    HXAsyncBufferState state;
    ULONG32 position_in_file;
};

struct HXParamBlockRec
{
    HXParamBlockRec()
    {
	param	= NULL;
	entry	= NULL;
    }

    ~HXParamBlockRec()
    {
    }

    ParamBlockRec io;
    void *param;		// Refcon type param.

    HXAsyncQueueBuffer *entry;	// databuffer

};

#define		kMacAsyncBuffer		256*1024
#define		kNeedToReadThreshhold	4*1024
#define		kMacMaxChunkToRead	32*1024

struct CMacAsyncFileResponse
{
    virtual HX_RESULT AsyncReadDone(HX_RESULT result, IHXBuffer* pBuffer) = 0;
    virtual HX_RESULT AsyncSeekDone(HX_RESULT result) = 0;
};

class CMacAsyncFile:public CMacFile
{
    public:
    CMacAsyncFile ();
    virtual ~ CMacAsyncFile ();
    
    STDMETHOD_(ULONG32, AddRef)  (THIS);
    
    STDMETHOD_(ULONG32, Release) (THIS);

    virtual ULONG32 Read (char *buf, ULONG32 count);
    virtual HX_RESULT Open (const char *filename, UINT16 mode, HXBOOL textflag = 0);
    virtual HX_RESULT Seek (ULONG32 offset, UINT16 fromWhere);
    virtual ULONG32 Tell (void);

    virtual HX_RESULT Close (void);

    HX_RESULT SetAsyncResponse (CMacAsyncFileResponse * pResponse);
    HX_RESULT SafeOpen (const char *filename, UINT16 mode, HXBOOL textflag = 0, HXBOOL bAtInterrupt = FALSE);
    HX_RESULT SafeRead (ULONG32 count, HXBOOL bAtInterrupt = FALSE);
	ULONG32   SafeWrite(IHXBuffer* pBuffer, HXBOOL bAtInterrupt);
    HX_RESULT SafeSeek (ULONG32 offset, UINT16 fromWhere, HXBOOL bAtInterrupt = FALSE);

    static CHXDataFile *Construct ();

private:

    static IOCompletionUPP zmIOCallbackUPP;
    static pascal void zmIOCallback (HXParamBlockRec * pb);

    void EmptyAsyncQueue ();
    void EnqueueAsyncBuffers ();
    void DeleteAsyncQueueBuffer(HXAsyncQueueBuffer* x, LISTPOSITION pos);
    HXBOOL FillBufferFromPQ(UINT32 ulSeekPosition);
    void PerformInternalSeek(HXBOOL ASYNC);
    


    OSErr ReadData(HXParamBlockRec* pb, HXBOOL ASYNC);
	OSErr WriteData(HXParamBlockRec* pb, HXBOOL ASYNC);
    OSErr SeekData(HXParamBlockRec* pb, HXBOOL ASYNC);
    void  ProcessPendingRead();

    CBigByteQueue *mReadQueue;

    ULONG32 mSeekFromWhere;
    ULONG32 mSeekPos;

    ULONG32		    mFilePos;
    
    /* absolute position of look ahead buffer read in the file*/
    ULONG32		    m_ulReadPositionInFile; 
    
    CMacAsyncFileResponse*  m_pResponse;
    UINT32		    m_ulPendingReadCount;
    DeferredTask	m_DeferredTaskStruct;
    UINT16 			m_uNumDeferredTask;
    
    HXMutex*	    m_pMutex;


// Make the booleans use only one bit
private:
	
	LONG32			m_lRefCount;
	
	HX_BITFIELD		m_bSeekPending : 1; 
	HX_BITFIELD		m_bFileDone : 1;
	HX_BITFIELD		m_bReadPending : 1;
public:
	HX_BITFIELD		m_bDeferredTaskPending : 1;
	HX_BITFIELD		m_bIsQuitting : 1;
	HX_BITFIELD		m_bInEnqueueAsyncBuffers : 1;
	HX_BITFIELD		m_bPendingAsyncSeekCompleted : 1;
	HX_BITFIELD		m_bAllCallbacksCompleted : 1;
	HX_BITFIELD		m_bInProcessPendingCallbacks : 1;
	HX_BITFIELD		m_bSettingSeekState : 1;
	HX_BITFIELD		m_bCheckFromPQ : 1;
	HX_BITFIELD		m_bInternalSeekNeeded : 1;
	

public:
    static pascal void		DeferredTaskProc(long param);

    void ReadAsyncData (HXBOOL ASYNC);
	ULONG32 WriteAsyncData(IHXBuffer* pBuffer, HXBOOL ASYNC);
    void UpdateFilePos (HXBOOL bASync = TRUE, HXBOOL bInternal = FALSE);

    void PendingAsyncSeekDone();
    void AllPendingCallbacksDone();
    void ProcessPendingCallbacks();
    
   	void AddToThePendingList(void* pNode);
   	void ProcessBlock(HXParamBlockRec* pb);


    CHXSimpleList *mAsyncQueue;
    
	CHXSimpleList*	m_pPendingCallbackList;
	CHXSimpleList*  m_pTimedPQList;
	CHXSimpleList*  m_pLocationPQList;
	UINT32		m_ulTotalPQSize;
	

    LONG32  mOutStandingCallbacks;
};

#endif

