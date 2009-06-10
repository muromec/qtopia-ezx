/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: macaudio.cp,v 1.10 2008/01/25 01:32:23 qluo Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

//
//	macaudio.cp
// 

#include <stdio.h>
#include "macaudio.h"
#include "USound.h"		
#ifndef _MAC_MACHO
#include <AIFF.h>
#include <fixmath.h>
#endif

#include "hxtypes.h"		
#include "hxerrors.h"		
#include "hxcom.h"
#include "hxausvc.h"
#include "auderrs.h"

#include "hxaudev.h"
#include "hxslist.h"
#include "hxtick.h"		
#include "chxpckts.h"

#ifdef THREADS_SUPPORTED
#include "hxthread.h"
#endif

#include "hxmm.h"

//#define LOG_MULTIPLE_DEFERRED_TASKS 1

HXBOOL 				gSoundCallbackTime = FALSE;

#if defined( _CARBON ) || defined( _MAC_UNIX )
DeferredTaskUPP		CAudioOutMac::gDeferredTask = NewDeferredTaskUPP(CAudioOutMac::DeferredTaskCallback);
#else
DeferredTaskUPP		CAudioOutMac::gDeferredTask = NewDeferredTaskProc(CAudioOutMac::DeferredTaskCallback);
#endif

#ifdef THREADS_SUPPORTED
HXMutex*		CAudioOutMac::zm_pMutex = NULL;
#endif

UINT32       gActiveAudioDeviceCount = 0;			    


#if defined( _CARBON ) || defined( _MAC_UNIX )

typedef pascal Handle (*MacAudioNewHandleSysProcPtr)(Size);
CFragConnectionID gAudioInterfaceLibConnID = kInvalidID;
MacAudioNewHandleSysProcPtr gMacAudioNewHandleSysProc = nil;
bool gMacAudioTriedToInitialize = false;

void MacAudioInitInterfaceLibProcPtrs()
{
	if (gMacAudioTriedToInitialize) return;
	gMacAudioTriedToInitialize = true;

	if (gAudioInterfaceLibConnID == kInvalidID)
	{
		GetSharedLibrary("\pInterfaceLib", kCompiledCFragArch, kReferenceCFrag,
			&gAudioInterfaceLibConnID, nil, nil);
	}
	
	if (gAudioInterfaceLibConnID != kInvalidID)
	{
		OSErr err = noErr;
		
		err = FindSymbol(gAudioInterfaceLibConnID, "\pNewHandleSys", (Ptr*)&gMacAudioNewHandleSysProc, nil);
	}
}

#endif

/*--------------------------------------------------------------------------
|	CMacWaveFormat
|
|		Default ctor.
--------------------------------------------------------------------------*/
CMacWaveFormat::CMacWaveFormat (void)

	{ /* begin CMacWaveFormat */
	
		SetFormatDflt ();
	
	} /* end CMacWaveFormat */

/*--------------------------------------------------------------------------
|	~CMacWaveFormat
|
|		dtor.
--------------------------------------------------------------------------*/
CMacWaveFormat::~CMacWaveFormat (void)

	{ /* begin ~CMacWaveFormat */
	
	} /* end ~CMacWaveFormat */

/*--------------------------------------------------------------------------
|	SetUpSound
|
|		Formats a sound handle.
--------------------------------------------------------------------------*/
OSErr CMacWaveFormat::SetUpSound (

	SndListHandle	sndHandle,
	long			numBytes,
	short			*headerLen,
	long			*headerOffset)
	
	{ /* begin SetUpSound */
		
		OSErr	e = noErr;
		long	response;
		
		if (sampleSize > 8) {
			if (USound::CheckSMVersion () < 3) {
			    if ((::Gestalt(gestaltSoundAttr, &response) == noErr)
			    	&& ((response & (1L << gestalt16BitSoundIO)) == 0))
					return (noHardwareErr);
				} /* if */
				
			else {
			    if ((::Gestalt(gestaltSoundAttr, &response) == noErr)
			    	&& ((response & (1L << gestalt16BitAudioSupport)) == 0))
					return (noHardwareErr);
				} /* else */
			} /* if */
			
		if (noErr != (e = ::SetupSndHeader (sndHandle,
										   numChannels,
										   sampleRate,
										   sampleSize,
										   compressionType,
										   baseFrequency,
										   numBytes,
										   headerLen)))
			goto CleanUp;
							   
		if (noErr != (e = USound::GetSoundHeaderOffset (sndHandle, headerOffset)))
			goto CleanUp;
	
	CleanUp:
	
		return (e);
		
	} /* end SetUpSound */

/*--------------------------------------------------------------------------
|	SetHeaderLength [static]
|
|		Formats a sound header for a given byte count.
--------------------------------------------------------------------------*/

OSErr CMacWaveFormat::SetHeaderLength (

	SoundHeaderPtr	pSoundHeader,
	long			numBytes)
	
	{ /* begin SetHeaderLength */
	
		switch (pSoundHeader->encode) {
			case stdSH: 										/*standard sound header*/
				pSoundHeader->length = numBytes;
				break;
				
			case extSH: 										/*extended sound header*/
				{
                                ExtSoundHeaderPtr	eh = (ExtSoundHeaderPtr) pSoundHeader;
				eh->numFrames = numBytes / (eh->numChannels * (eh->sampleSize / 8));
                                }
				break;
				
			case cmpSH:											/*compressed sound header*/
				{
                                CmpSoundHeaderPtr	ch = (CmpSoundHeaderPtr) pSoundHeader;
				ch->numFrames = numBytes / (ch->numChannels * (ch->sampleSize / 8));
                                }
				break;
				
			default:
				return badFormat;
			} /* switch */		
		
		return noErr;
		
	} /* SetHeaderLength */

/*--------------------------------------------------------------------------
|	SetFormatDflt
|
|		Sets up the default Sound header information.
--------------------------------------------------------------------------*/
void CMacWaveFormat::SetFormatDflt (void)

	{ /* begin SetFormatDflt */
		
		//mwf.wf.nAvgBytesPerSec = 22050;
		//mwf.wf.nBlockAlign = 2;
		
		numChannels = 1;					//mwf.wf.nChannels = 1;
		sampleRate = Long2Fix (8000);		//mwf.wf.nSamplesPerSec = 8000;
		//sampleRate = rate11025hz;			//mwf.wf.nSamplesPerSec = 11025;
		sampleSize = 16;					//mwf.wBitsPerSample = 16;
		compressionType = NoneType;			//mwf.wf.wFormatTag = WAVE_FORMAT_PCM;
		baseFrequency = kMiddleC;
		
	} /* end SetFormatDflt */

/*--------------------------------------------------------------------------
|	SetFormatDflt
|
|		Sets up the default Sound header information.
--------------------------------------------------------------------------*/
void CMacWaveFormat::SetFormat (
	
	ULONG32	inSampleRate,
	UINT16 	channels,
	UINT16  bitsPerSample)

	{ 
		
		numChannels = channels;					
		sampleRate = inSampleRate << 16L;		
		sampleSize = bitsPerSample;					
		compressionType = NoneType;			
		baseFrequency = kMiddleC;
		
	}

/*--------------------------------------------------------------------------
|	CWaveHeader
--------------------------------------------------------------------------*/

CWaveHeader::CWaveHeader (void)

	: soundA5 (0)
	, state (kFreeState)
	, waveOut (NULL)
	, mMaxPlay (0)
	, sndHandle (nil)
	, mHeaderLen (0)
	, mSoundHeader (nil)

	{ /* begin CWaveHeader */
		
		soundA5 = SetCurrentA5 ();
		
	} /* end CWaveHeader */
	
/*--------------------------------------------------------------------------
|	~CWaveHeader
--------------------------------------------------------------------------*/
CWaveHeader::~CWaveHeader (void)

	{ /* begin ~CWaveHeader */
		
		if (sndHandle) ::DisposeHandle ((Handle) sndHandle);
		sndHandle = nil;
		
	} /* end ~CWaveHeader */
	
/*--------------------------------------------------------------------------
|	CWaveHeader
--------------------------------------------------------------------------*/
const	Size	CWaveHeader::kSndHeaderSize = 512;
		UINT16	CWaveHeader::WAVE_BLOCKSIZE = 4096;

OSErr CWaveHeader::Allocate (
	
	UINT16	inMaxPlay,
	float   sampleRate,
	UINT16  channels,
	UINT16  bitsPerSample)
	
	{ /* begin Allocate */
		
		OSErr	e = badFormat;
		
		long	offset;
		
		if (!waveOut)
			goto CleanUp;
		
		OSErr theError;
		sndHandle = (SndListHandle) ::TempNewHandle (kSndHeaderSize + inMaxPlay, &theError);
		if (!sndHandle)
			goto CleanUp;

        check_noerr (theError);
		waveOut->wf.SetFormat ((long)sampleRate,channels,bitsPerSample);
		
		if (noErr != (e = waveOut->wf.SetUpSound (sndHandle, inMaxPlay, &mHeaderLen, &offset)))
			goto CleanUp;
		
		::SetHandleSize ((Handle) sndHandle, mHeaderLen + inMaxPlay);
		if (noErr != (e = MemError ()))
			goto CleanUp;
			
		mMaxPlay = inMaxPlay;

		::MoveHHi ((Handle) sndHandle);
		::HLock ((Handle) sndHandle);
		mSoundHeader = (SoundHeaderPtr) ((*(Handle) sndHandle) + offset);
		
	CleanUp:
		
		return (e);
		
	} /* end Allocate */

/*--------------------------------------------------------------------------
|	Release
--------------------------------------------------------------------------*/

void CWaveHeader::Release (short	releaseCode, HXBOOL bSendTimeSync /*= TRUE*/)

{ /* begin Release */
	
    if (kFreeState != state) 
    {
	state = kFreeState;
	if ((CWaveHeader::kResetRelease != releaseCode) &&
		(CWaveHeader::kAbortRelease != releaseCode)) //cz 5/7/96
	if (waveOut) waveOut->DonePlaying (cbPlaying, timeEnd,releaseCode == kCallBackRelease, bSendTimeSync);
    } /* if */
	
	
	
    if (kAbortRelease == releaseCode) 
    {
        
	if (sndHandle) 
	{
	    ::DisposeHandle ((Handle) sndHandle);
	    sndHandle=NULL;
	}
	
	sndHandle = nil;
	mMaxPlay = 0;
	mSoundHeader = nil;
    } /* if */
		
} /* end Release */



/*--------------------------------------------------------------------------
|	PlayBuffer
--------------------------------------------------------------------------*/

OSErr CWaveHeader::PlayBuffer (
	
	char		*pData,
	UINT16		cbPlayingArg,
	ULONG32		timeEndArg)
	
	{ /* begin PlayBuffer */
		
		OSErr		e = noErr;
		
		SndCommand	cmd;
		
		
		if (!waveOut)
			return (badFormat);
			
		if (kFreeState != state)
			return (badFormat);
			
		if (!sndHandle) 
			return (nilHandleErr);
		
		if (cbPlayingArg > mMaxPlay)
			return (memFullErr);
		
		if (noErr != (e = waveOut->wf.SetHeaderLength (mSoundHeader, cbPlayingArg)))
			goto CleanUp;
		
		if(pData != NULL)	
			::BlockMove (pData, (*(Handle) sndHandle) + mHeaderLen, cbPlayingArg);
		
		cmd.cmd = bufferCmd;
		cmd.param1 = 0;
		cmd.param2 = (long) mSoundHeader;
		if (noErr != (e = ::SndDoCommand (waveOut->chan, &cmd, TRUE))) goto CleanUp;
			
		state = kQueuedState;
		cbPlaying = cbPlayingArg;
		timeEnd = timeEndArg;

		waveOut->StartedPlaying (cbPlaying);
		
		cmd.cmd = callBackCmd;
		cmd.param1 = kCallBackRelease;
		cmd.param2 = (long) this;
		if (noErr != (e = ::SndDoCommand (waveOut->chan, &cmd, TRUE))) goto CleanUp;
		
		waveOut->m_bAudioCallbacksAreAlive = TRUE;
		
	CleanUp:
	
		return (e);
		
	} /* end PlayBuffer */
	
/*--------------------------------------------------------------------------
|	NewChannel
--------------------------------------------------------------------------*/
SndChannelPtr CWaveHeader::NewChannel (void)

	{ /* begin NewChannel */
		
		SndChannelPtr		chan = nil;
#if defined( _CARBON ) || defined( _MAC_UNIX )
		SndCallBackUPP		userProc = NewSndCallBackUPP (Callback);
#else
		SndCallBackUPP		userProc = NewSndCallBackProc (Callback);
#endif
		
//		if (noErr != ::SndNewChannel (&chan, sampledSynth, initStereo + initNoDrop + initNoInterp, userProc))
		if (noErr != ::SndNewChannel (&chan, sampledSynth, initStereo, userProc))
			chan = nil;
		
		return chan;
		
	} /* end NewChannel */

/*--------------------------------------------------------------------------
|	DisposeChannel
--------------------------------------------------------------------------*/
OSErr CWaveHeader::DisposeChannel (
	
	SndChannelPtr	chan)
	
	{ /* begin DisposeChannel */
	OSErr e;
	
		if(chan)
		{
			SndCallBackUPP		userProc = chan->callBack;
#if defined( _CARBON ) || defined( _MAC_UNIX )
			if (userProc) DisposeSndCallBackUPP (userProc);
#else
			if (userProc) DisposeRoutineDescriptor (userProc);
#endif
		
			e = ::SndDisposeChannel (chan, TRUE);
		
		}
		return e;
		
	} /* end DisposeChannel */

/*--------------------------------------------------------------------------
|	Callback
--------------------------------------------------------------------------*/

pascal void CWaveHeader::Callback (

	SndChannelPtr	chan,
	SndCommand		*cmd)
	
	{ /* begin Callback */
		
#ifndef _MAC_UNIX
		HXMM_INTERRUPTON();
#endif
	
//    if (HXMM_RAHUL_CHECK())
//    {
//        DebugStr("\pCWaveHeader Deferred Task ENTER;g");
//    }
    
		DeferredTask*	dtrec = NULL;
		CWaveHeader*    wh    = NULL;
                SndCommand*	  newCommand;
                
		if (cmd)
		{
			wh = (CWaveHeader *) cmd->param2;
			
			if ( wh )
			{
			    CAudioOutMac* theWaveOut = wh->waveOut;
			    
			    if ( theWaveOut )
			    {
				CMacWaveFormat wf = theWaveOut->wf;
				
				UINT32 ulBytesPerSecond = 
					wf.numChannels *
					( (ULONG32)wf.sampleRate >> 16 ) *
					( wf.sampleSize / 8 );
				
				theWaveOut->m_millisecondsIntoClip +=
					(double)wh->cbPlaying * 1000.0 / (double)ulBytesPerSecond;
			    }
			}
			
			dtrec=wh->waveOut->mDeferredTaskRec;
		}
		
		if (!dtrec)
		{
			goto CleanUp;
		}
		
		newCommand=new SndCommand;
		HX_ASSERT(newCommand);
		
		if (!newCommand)
		{
			delete dtrec;
			goto CleanUp;
		}
		
		if (cmd)
		{
			*newCommand=*cmd;
		}
		else
		{
			delete newCommand;
			delete dtrec;
			goto CleanUp;
		}
		
		wh->waveOut->AddToThePendingList(newCommand);
	
		if (!wh->waveOut->m_bDeferredTaskPending)
		{
		    wh->waveOut->m_bDeferredTaskPending = TRUE;
		    
		    short err = 0;
		    
		    if ( wh->waveOut->m_bIsQuitting )
		    {
			err = -1; // just so it's nonzero
		    }
		    else
		    {
			err = DTInstall(dtrec);
		    }
		    if ( err != noErr )
		    {
			wh->waveOut->m_bDeferredTaskPending = FALSE;
			wh->waveOut->m_bAudioCallbacksAreAlive = FALSE;
			delete dtrec;
			delete newCommand;
			goto CleanUp;
		    }
		}
		
	/*	
		if (wh) {
			long	saveA5 = SetA5 (wh->soundA5);
			wh->Release (cmd->param1);
			SetA5 (saveA5);
			} 
	*/
	
	
	CleanUp:
		
#ifndef _MAC_UNIX
		HXMM_INTERRUPTOFF();
#endif
		
//    if (HXMM_RAHUL_CHECK())
//    {
//        DebugStr("\pCWaveHeader Deferred Task LEAVE;g");
//    }
		return;
		
	} /* end Callback */
	

pascal void CAudioOutMac::DeferredTaskCallback(long param)
{
    if (!param)
    {
        return;
    }
    
#ifndef _MAC_UNIX
	HXMM_INTERRUPTON();
#endif
	
	CAudioOutMac* pAudioOut = (CAudioOutMac*) param;
	
	pAudioOut->m_bAudioCallbacksAreAlive = FALSE;
	
#ifdef THREADS_SUPPORTED
	if (zm_pMutex)
	{
	    zm_pMutex->Lock();
	}
#endif

#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	    if ( !pAudioOut->m_bAudioCallbacksAreAlive )
	    {
		DebugStr( "\pDeferredTaskCallback -- m_bAudioCallbacksAreAlive not true!;g" );
	    }

#endif
	while (gActiveAudioDeviceCount &&
	       pAudioOut->m_pPendingCallbackList && 
	      !pAudioOut->m_pPendingCallbackList->IsEmpty())
	{
		SndCommand*	cmd = (SndCommand*) pAudioOut->m_pPendingCallbackList->RemoveHead();
		HXBOOL bIsEmpty = pAudioOut->m_pPendingCallbackList->IsEmpty();
		
#ifdef THREADS_SUPPORTED
		// we used to deadlock here; we'd grab the audio mutex before the core mutex,
		// which is not how the rest of this class operates
		if (zm_pMutex)
		{
		    zm_pMutex->Unlock();
		}
#endif
		/* Send time sync for ONLY the last pending audio callback */
		pAudioOut->ProcessCmd(cmd, bIsEmpty);
				
     	delete cmd;
#ifdef THREADS_SUPPORTED
		if (zm_pMutex)
		{
		    zm_pMutex->Lock();
		}
#endif
	}
	
	/* It is possible that on a timesync, a renderer may issue a hyper navigate request.
	 * This may result in leaving the currently active page (thereby unloading 
	 * the embedded player and the core. We would have destructed the CMacAudio class
	 * in this case.
	 */ 
	if (!gActiveAudioDeviceCount)
	{
	    goto cleanup;
	}
	
	pAudioOut->m_bDeferredTaskPending = FALSE;
	
cleanup:	

#ifdef THREADS_SUPPORTED
	if (zm_pMutex)
	{
	    zm_pMutex->Unlock();
	}
#endif

#ifndef _MAC_UNIX
	HXMM_INTERRUPTOFF();
#endif
}

void
CAudioOutMac::ProcessCmd(SndCommand* cmd, HXBOOL bSendTimeSync /*= TRUE*/)
{
	CWaveHeader	*wh = (CWaveHeader *) cmd->param2;
	if (wh)
	{
		wh->Release(cmd->param1, bSendTimeSync);
	}
}

	
/*--------------------------------------------------------------------------
|	CAudioOutMac
--------------------------------------------------------------------------*/
UINT16 CAudioOutMac::NUM_WAVEHDRS = 12;	// was 30

CAudioOutMac::CAudioOutMac ()
	: chan (nil)
	, mlpWaveHdrs (NULL)
	, mcbPlaying (0)
	, mPaused (TRUE)
	, m_uNumBlocksInDevice(0)
	, m_bFirstPacket(TRUE)
	, m_ulTimeOfFirstPacket(0L)
	, m_pPendingCallbackList(NULL)
	, m_bDeferredTaskPending(FALSE)
	, m_bAudioCallbacksAreAlive(FALSE)
	, m_bIsQuitting(FALSE)
	, m_millisecondsIntoClip(0.0)
	, m_pCallback(NULL)
	, m_pInterruptState(NULL)
{ /* begin CAudioOutMac */
	mDeferredTaskRec=(DeferredTask*)NewPtrClear(sizeof(DeferredTask));
	
	mDeferredTaskRec->qType=dtQType;
	mDeferredTaskRec->dtAddr=CAudioOutMac::gDeferredTask;
    mDeferredTaskRec->dtParam= (long) this;
    
	// Gestalt-based code that remembers potential deferred tasks for emergency removal
	
#ifndef _MAC_UNIX
	Handle dtGestaltHandle = nil;
	OSErr err = Gestalt( kLetInterruptsFinishBeforeQuittingGestalt, (long*)&dtGestaltHandle );
	if ( err != noErr )
	{
#if defined( _CARBON )
	    MacAudioInitInterfaceLibProcPtrs();
	    if (gMacAudioNewHandleSysProc)
	    {
		dtGestaltHandle = (*gMacAudioNewHandleSysProc)( 0 );
	    }
	    if (!dtGestaltHandle)
	    {
		dtGestaltHandle = NewHandle( 0 );
	    }
#else
	    dtGestaltHandle = NewHandleSys( 0 );
#endif
	    if ( dtGestaltHandle )
	    {
		NewGestaltValue( kLetInterruptsFinishBeforeQuittingGestalt, (long)dtGestaltHandle );
	    }
	}

	if ( dtGestaltHandle )
	{
	    GestaltDeferredStruct gds;
	    gds.quitting = &m_bIsQuitting;
	    gds.pending = &m_bAudioCallbacksAreAlive;
	    GetCurrentProcess(&(gds.psn));
	    PtrAndHand( &gds, dtGestaltHandle, sizeof(gds) );
	}
#endif
	

#ifdef THREADS_SUPPORTED
	if (!zm_pMutex)
	{
	    HXMutex::MakeMutex(zm_pMutex);
	}
#endif
    gActiveAudioDeviceCount++;
} /* end CAudioOutMac */


/*--------------------------------------------------------------------------
|	~CAudioOutMac
--------------------------------------------------------------------------*/
CAudioOutMac::~CAudioOutMac ()

{ /* begin ~CAudioOutMac */ 
	
	Abort ();

#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	HXBOOL bWaitedForPending = FALSE;
	if ( m_bAudioCallbacksAreAlive )
	{
	    DebugStr( "\pCAudioOutMac dtor -- m_bAudioCallbacksAreAlive still true!;g" );
	    bWaitedForPending = TRUE;
	}
#endif
    
    m_bIsQuitting = TRUE;
    // sit-n-spin awaiting interrupts to finish.
    for ( int i = 0; i < 10; i++ )
    {
	unsigned long dummyTix;
	Delay( 6, &dummyTix );
	if ( !m_bAudioCallbacksAreAlive )
	{
	    i = 10;
	}
    }
    
#if defined(_DEBUG) && defined(LOG_MULTIPLE_DEFERRED_TASKS)
	if ( bWaitedForPending )
	{
	    if ( m_bAudioCallbacksAreAlive )
	    {
		DebugStr( "\ptasks STILL pending! This is gonna hurt...;g" );
	    }
	    else
	    {
		DebugStr( "\pSuccessfully purged pending callbacks;g" );
	    }
	}
#endif

	if (mDeferredTaskRec)
	{


	// first ensure that the Gestalt handle doesn't think it's
	// here any more.
	
#ifndef _MAC_UNIX
	Handle dtGestaltHandle = nil;
	Gestalt( kLetInterruptsFinishBeforeQuittingGestalt, (long*)&dtGestaltHandle );
	if ( dtGestaltHandle )
	{
	    // XXXNH: only look at tasks for this process
	    ProcessSerialNumber psn;
	    GetCurrentProcess(&psn);
	
	    // zip through and if an entry equals this deferred task,
	    // simply zero it out. We won't worry about shuffling
	    // the handle size at this juncture.
	    long hSize = GetHandleSize( dtGestaltHandle );
	    GestaltDeferredStruct* currentPtr = (GestaltDeferredStruct*)*dtGestaltHandle;
	    for ( int i = 0; i < hSize / sizeof(GestaltDeferredStruct); i++ )
	    {
	        unsigned char bSameProcess = FALSE;
	        SameProcess(&(currentPtr[i].psn), &psn, &bSameProcess);
	        
		if (bSameProcess && 
		    currentPtr[i].quitting == &m_bIsQuitting && 
		    currentPtr[i].pending == &m_bAudioCallbacksAreAlive )
		{
		    currentPtr[i].quitting = NULL;
		    currentPtr[i].pending = NULL;
		}
	    }    
	}
#endif



		::DisposePtr	((Ptr)mDeferredTaskRec);
	}    	
	
	CleanupPendingLists();
	
	HX_DELETE(m_pPendingCallbackList);
	
	gActiveAudioDeviceCount--;
	
	HX_RELEASE(m_pScheduler);
	HX_RELEASE(m_pInterruptState);
} /* end ~CAudioOutMac */

UINT16 CAudioOutMac::m_uzVolume = 50;

HX_RESULT   	
CAudioOutMac::_Imp_Open( const HXAudioFormat* pFormat )
{

    /*
    **	Open the sound channel.
    */

    NUM_WAVEHDRS = 45; //params->numBufs;

    CWaveHeader::WAVE_BLOCKSIZE = pFormat->uMaxBlockSize;
	
    if (chan) CWaveHeader::DisposeChannel (chan);
    m_bAudioCallbacksAreAlive = FALSE;

	
    chan = CWaveHeader::NewChannel ();
    if (!chan) 
	return (HX_AUDIO_DRIVER_ERROR);
	
	if (!m_pPendingCallbackList)
	{
	    m_pPendingCallbackList = new CHXSimpleList;
	}
	
    ULONG32 nSampleRate = pFormat->ulSamplesPerSec;
    ULONG32 nBlockSize = pFormat->uMaxBlockSize;

#if powerc && FORCE_RATE_CONVERSION
	// if we are on a PowerPC we will do our own sample rate conversion
	// since the Apple version gives us a lot of pops and aliasing
    ULONG32 sysSampleRate = ::USound::GetSystemSampleRate(chan);
	
	// Note: we currently have to HACK 11227 and 22254 sample rates on
	// old macs to 11025 and 22050
	
    switch(sysSampleRate)
    {
	case 11227:
	    sysSampleRate = 11025;
	    break;
			
	case 22254:
	    sysSampleRate = 22050;
	    break;
    }	
	
	// we only have a problem if the sample rate is <= 32000 Hz. (sigh!)
    if(nSampleRate <= 32000)
    {
	if(sysSampleRate > 0 && nSampleRate != sysSampleRate)
	{
	    UINT16 bytesPerSample = pFormat->wBitsPerSample > 8 ? 2 : 1;
	    UINT16 frameSize = bytesPerSample * pFormat->wChannels;
	    ULONG32 bytesPerSecond = (ULONG32) (nSampleRate * frameSize);
			
	    float bufDuration = (float)pFormat->uMaxBlockSize/(float)bytesPerSecond;
			
	    // calculate the bytesPerSecond of the system sample rate
			
	    bytesPerSecond = (ULONG32) (sysSampleRate * frameSize);
			
	    // update the buffer size to handle the new sample rate
	    nBlockSize = bytesPerSecond * bufDuration;
			
	    // round the buffersize to a framesize
	    nBlockSize = ((nBlockSize/frameSize) * frameSize) + frameSize;
	    nSampleRate = sysSampleRate;
	    //pFormat->ulSamplesPerSec = sysSampleRate;
	}
    }

#ifndef MAC_RELEASE
#if 0
	char s[256];
	::sprintf(s,"sample rate %ld",sysSampleRate);
	::c2pstr(s);
	DebugStr((UCHAR *)s);
#endif
#endif // MAC_RELEASE
#endif // powerc && FORCE_RATE_CONVERSION


//    SetVolume(pFormat->volume);
    _Imp_SetVolume(m_uzVolume);
    
	/*
	**	Allocate memory for wave block headers and for wave data.
	*/
	
    if (NULL == mlpWaveHdrs) 
    {
	if (NULL == (mlpWaveHdrs = new CWaveHeader [NUM_WAVEHDRS])) 
	    return (HX_MEMORY_ERROR);
		
	for (short iwh = 0; iwh < NUM_WAVEHDRS; iwh++) 
	{
	    CWaveHeader	*pwh = GetWaveHeader (iwh);
	    if (pwh) 
	    {
		pwh->SetOutput (this);
		switch (pwh->Allocate (nBlockSize, nSampleRate,
			pFormat->uChannels,pFormat->uBitsPerSample)) 
		{
		    case noErr:
			break;
				
		    case memFullErr:
			return (HX_MEMORY_ERROR);
				
		    default:
			return (HX_AUDIO_DRIVER_ERROR);
		} 
	    } 
	} 
    } 
	
	/*
	**	Indicate that no audio blocks are currently playing.
	*/
    mcbPlaying = 0;
    mPaused = FALSE;
    
    if (!m_pScheduler && m_pContext)
    {
	m_pContext->QueryInterface(IID_IHXScheduler, (void**) &m_pScheduler);
	m_pContext->QueryInterface(IID_IHXInterruptState, (void**) &m_pInterruptState);
    }
    
    return (HX_NO_ERROR);
}

HX_RESULT   	
CAudioOutMac::_Imp_Close( void )
{
    Abort();
    return HXR_OK;
}

HX_RESULT   	
CAudioOutMac::_Imp_Pause( void )
{
    mPaused = TRUE;
    
    if (OkToPauseResume(FALSE))
    {
	// Stop playback
	if (chan != NULL) 
	DoImmediate (pauseCmd);
    }
    	
    return HXR_OK;
}

HX_RESULT   	
CAudioOutMac::_Imp_Resume( void )
{
    mPaused = FALSE;

    if (OkToPauseResume(TRUE))
    {
	if (chan != NULL) 
	{
	DoImmediate (resumeCmd);
	}	

	this->OnTimeSync();
    }
    
    return HXR_OK;    
}

HX_RESULT   	
CAudioOutMac::_Imp_Write( const HXAudioData* pAudioOutHdr )
{
    HX_RESULT err = HX_NO_ERROR;
        
//    CWaveHeader	*pwh = GetWaveHeader(0);;
    CWaveHeader	*pwh=NULL;
	
    Boolean found = FALSE;
    void* pBuffer = NULL;
		
    for (short iwh = 0; iwh < NUM_WAVEHDRS && !found; iwh++) 
    {
	pwh = GetWaveHeader(iwh);
	found = pwh && pwh->Available ();
    }
	
    if (found)
    {
	pBuffer = pwh->get_buffer();
    }
    else
    {
	return playError;
    }

    if(pwh == 0)
    {
	return (playError);
    }
	
    char* pData = 0;
    pData = (char*)pAudioOutHdr->pData->GetBuffer();
    ULONG32 nLen = pAudioOutHdr->pData->GetSize();
    if (pBuffer)
    {
    	memcpy(pBuffer, pData, nLen);
    	
	if (noErr != pwh->PlayBuffer ((char*)pBuffer, nLen, pAudioOutHdr->ulAudioTime))
	    err = (playError);
    }

	if (!err)
	{
		m_uNumBlocksInDevice++;
		if(m_bFirstPacket)
		{
			m_bFirstPacket = FALSE;
			m_ulTimeOfFirstPacket = pAudioOutHdr->ulAudioTime;
		}		
	}

    return err;
}

HX_RESULT   	
CAudioOutMac::_Imp_Reset( void )
{
    // Stop playback
    if (chan != NULL) {
	DoImmediate (pauseCmd);
	DoImmediate (flushCmd);
	DoImmediate (quietCmd);
    } /* if */

    m_ulCurrentTime = 0;
    m_bFirstPacket = TRUE;
    
    // Mark blocks as available.
    ReleaseBlocks (CWaveHeader::kResetRelease);
    m_uNumBlocksInDevice = 0;
    
    CleanupPendingLists();		

    return HX_NO_ERROR;
}
HX_RESULT  	
CAudioOutMac::_Imp_Drain( void )
{
    return HX_NO_ERROR;
}

HXBOOL 		
CAudioOutMac::_Imp_SupportsVolume( void )
{
    return TRUE;
}

UINT16   	
CAudioOutMac::_Imp_GetVolume()
{
#if 0
    unsigned long	volume=0;
    SndCommand		theCmd;

    theCmd.cmd = getVolumeCmd;
    theCmd.param1 = 0;
    theCmd.param2 = volume;

	// queue volume command in channel
    ::SndDoImmediate(chan,&theCmd);

    return  (UINT16) volume;

#endif
    return m_uzVolume;
}

HX_RESULT   	
CAudioOutMac::_Imp_SetVolume( const UINT16 uVolume )
{
    SndCommand		theCmd;
    unsigned long	volume;
    unsigned long	leftVolume, rightVolume;
	
//    if(uVolume < 0) uVolume = 0;
    
    if(chan)
    {
#if defined( _CARBON ) || defined( _MAC_UNIX )
	leftVolume = rightVolume = (long) uVolume * 5; //uVolume is between 0 and 100
	volume = (rightVolume << 16) + leftVolume;
	theCmd.cmd = volumeCmd;
	theCmd.param1 = 0;
	theCmd.param2 = volume;

	// queue volume command in channel
	::SndDoImmediate(chan,&theCmd);
#else
#if OLDROUTINENAMES && !GENERATINGCFM
	if (USound::CheckSMVersion () < 3)
		::SetSoundVol (uVolume);
#else
	leftVolume = rightVolume = (long) uVolume * 5; //uVolume is between 0 and 100
	volume = (rightVolume << 16) + leftVolume;
	theCmd.cmd = volumeCmd;
	theCmd.param1 = 0;
	theCmd.param2 = volume;

	// queue volume command in channel
	::SndDoImmediate(chan,&theCmd);
#endif
#endif
    }
    
    m_uzVolume = uVolume;

    return HXR_OK;	
}

HX_RESULT   	
CAudioOutMac::_Imp_CheckFormat( const HXAudioFormat* pFormat)
{
    if (pFormat->uChannels > 2)
    {
        // multi-channel not supported via Sound Manager; need to
        // upgrade to a CoreAudio-based solution.
        return HXR_AUDIO_DRIVER;
    }
    return HX_NO_ERROR;
}

HX_RESULT 
CAudioOutMac::_Imp_Seek(ULONG32 ulSeekTime)
{
    m_ulCurrentTime = 0;
    return HX_NO_ERROR;
}

/*---------------------------------------------------------------------------
|	ReleaseBlocks
|
|		Frees the blocks manually
--------------------------------------------------------------------------*/
void CAudioOutMac::ReleaseBlocks (
	
	short	releaseCode)
	
	{ /* begin ReleaseBlocks */
	
		// Mark blocks as available.
		for (short iwh = 0; iwh < NUM_WAVEHDRS; iwh++) {
			CWaveHeader	*pwh = GetWaveHeader (iwh);
				
//			if (pwh && !pwh->Available ())
			if (pwh)
				pwh->Release (releaseCode);
			} /* for */
			
		m_millisecondsIntoClip = 0.0;
	} /* end ReleaseBlocks */

/*---------------------------------------------------------------------------
|	DoImmediate
|
|		Executes a sound command
--------------------------------------------------------------------------*/
OSErr CAudioOutMac::DoImmediate (
	
	short	cmd,
	short	param1/*= 0*/,
	long	param2/*= 0L*/)
	
	{ /* begin DoImmediate */
		
		SndCommand	sndCmd;
		
		sndCmd.cmd = cmd;
		sndCmd.param1 = param1;
		sndCmd.param2 = param2;
		
		return ::SndDoImmediate (chan, &sndCmd);
			
	} /* end DoImmediate */

/*---------------------------------------------------------------------------
|	Abort
|
|		Halts playback and shuts down
--------------------------------------------------------------------------*/
void CAudioOutMac::Abort (void)
	
	{ /* begin Abort */
	
		// Stop playback
		if (chan != NULL) {
			//	Kludge around usual SM unreliabilityÉ
			//	rmgw 3/12/95
			DoImmediate (pauseCmd);
			DoImmediate (flushCmd);
			DoImmediate (quietCmd);
			
			CWaveHeader::DisposeChannel (chan);
			m_bAudioCallbacksAreAlive = FALSE;

			chan = NULL;
			mPaused = TRUE;
			} /* if */
		
		// Mark blocks as available.
		ReleaseBlocks (CWaveHeader::kAbortRelease);
			
		/*
		**	Free allocated buffers.
		*/
		
		if (mlpWaveHdrs) delete [] mlpWaveHdrs;
		mlpWaveHdrs = NULL;
		m_uNumBlocksInDevice = 0;
		m_ulCurrentTime = 0;
		
		CleanupPendingLists();	
		
		// remove any pending callback
		if (m_pCallback && m_pCallback->m_CallbackHandle && m_pScheduler)
    		{
		    m_pScheduler->Remove(m_pCallback->m_CallbackHandle);
		    HX_RELEASE(m_pCallback);
		}

	} /* end Abort */

void CAudioOutMac::CleanupPendingLists()
{
#ifdef THREADS_SUPPORTED
    if (zm_pMutex)
    {
	zm_pMutex->Lock();
    }
#endif
    while(m_pPendingCallbackList && !m_pPendingCallbackList->IsEmpty())
    {
	SndCommand*	cmd = (SndCommand*)m_pPendingCallbackList->RemoveHead();	
	delete cmd;		
    }
#ifdef THREADS_SUPPORTED
    if (zm_pMutex)
    {
	zm_pMutex->Unlock();
    }
#endif
}
/*---------------------------------------------------------------------------
|	DonePlaying
|
--------------------------------------------------------------------------*/
void CAudioOutMac::DonePlaying ( UINT16	cbPlayedArg, ULONG32 timeEndArg, 
 							    short waveTask, HXBOOL bSendTimeSync /*= TRUE*/)
{ 
    mcbPlaying -= cbPlayedArg;
//    if (cbPlayedArg)
    {
		if (m_uNumBlocksInDevice > 0)
		{
			m_uNumBlocksInDevice--;
		}

    	gSoundCallbackTime = TRUE;
    	
	HX_ASSERT(timeEndArg >= m_ulTimeOfFirstPacket);
	if(timeEndArg >= m_ulTimeOfFirstPacket)
	{
        /* Never go back in time */
	    if (m_ulCurrentTime < m_millisecondsIntoClip)
	    {
		    m_ulCurrentTime = m_millisecondsIntoClip;
	    }
	}
	else
	{
    		m_ulCurrentTime = 0L;
	}
	

    if (bSendTimeSync)
    {
	    this->OnTimeSync();
	}
	
	gSoundCallbackTime = FALSE;
	
    }
} 


/************************************************************************
 *  Method:
 *              CAudioOutMac::_Imp_GetCurrentTime
 *      Purpose:
 *              Get the current time from the audio device.
 */
HX_RESULT CAudioOutMac::_Imp_GetCurrentTime
( 
        ULONG32& ulCurrentTime
)
{
    
    ulCurrentTime = m_ulCurrentTime;
    return HX_NO_ERROR;
}
	
UINT16	CAudioOutMac::_NumberOfBlocksRemainingToPlay(void)
{
	return m_uNumBlocksInDevice;
}

void CAudioOutMac::AddToThePendingList(void* pNode)
{
#ifdef THREADS_SUPPORTED
    if (zm_pMutex)
    {
	zm_pMutex->Lock();
    }
#endif

    m_pPendingCallbackList->AddTail(pNode);

#ifdef THREADS_SUPPORTED
    if (zm_pMutex)
    {
	zm_pMutex->Unlock();
    }
#endif
}		

HXBOOL 
CAudioOutMac::OkToPauseResume(HXBOOL bToBeResumed/* TRUE - Resume. FALSE - Pause */)
{
    if (m_pInterruptState && m_pInterruptState->AtInterruptTime())
    {
         if (!m_pCallback)
         { 
             m_pCallback = new HXPauseResumeCb(this);
             m_pCallback->AddRef();
         }
         
         m_pCallback->Enter(bToBeResumed);
         
         if (m_pCallback->m_CallbackHandle == 0 && m_pScheduler)
         {
            m_pCallback->m_CallbackHandle = m_pScheduler->RelativeEnter(m_pCallback, 0);
         }
               
         return FALSE;
    }
    else if (m_pCallback && m_pCallback->m_CallbackHandle && m_pScheduler)
    {
        // rememebr the callback handle since it will be set to 0 in func()
        CallbackHandle handle = m_pCallback->m_CallbackHandle;
        m_pCallback->Func();
        m_pScheduler->Remove(handle);
        m_pCallback->m_CallbackHandle = 0;
    }

    return TRUE;
}

// CAudioOutMac::HXPauseResumeCb

CAudioOutMac::HXPauseResumeCb::HXPauseResumeCb(CAudioOutMac* pAudioObject) :
     m_lRefCount (0)
    ,m_CallbackHandle(0) 
    ,m_pAudioDeviceObject (pAudioObject)
    ,m_pCommandList(NULL)
{
}

CAudioOutMac::HXPauseResumeCb::~HXPauseResumeCb()
{
    HX_DELETE(m_pCommandList);
}

/*
 * IUnknown methods
 */
/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::QueryInterface
//      Purpose:
//              Implement this to export the interfaces supported by your 
//              object.
//
STDMETHODIMP CAudioOutMac::HXPauseResumeCb::QueryInterface
(	REFIID riid, void** ppvObj )
{
    if (IsEqualIID(riid, IID_IHXCallback))
    {
        AddRef();
        *ppvObj = (IHXCallback*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = this;
        return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::AddRef
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) CAudioOutMac::HXPauseResumeCb::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

/////////////////////////////////////////////////////////////////////////
//      Method:
//              IUnknown::Release
//      Purpose:
//              Everyone usually implements this the same... feel free to use
//              this implementation.
//
STDMETHODIMP_(ULONG32) CAudioOutMac::HXPauseResumeCb::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

/*
 *      IRMAPauseResumeCb methods
 */
STDMETHODIMP CAudioOutMac::HXPauseResumeCb::Func(void)
{
    m_CallbackHandle = 0;
    
    if (m_pAudioDeviceObject)
    {
        while (m_pCommandList && m_pCommandList->GetCount() > 0)
        {
            HXBOOL bResume = (HXBOOL) m_pCommandList->RemoveHead();
	    if(bResume)
	    {
	        m_pAudioDeviceObject->_Imp_Resume();
	    }
	    else
	    {
	        m_pAudioDeviceObject->_Imp_Pause();
	    }
	}
	
	HX_DELETE(m_pCommandList);
    }

    return HXR_OK;
}

void
CAudioOutMac::HXPauseResumeCb::Enter(HXBOOL bResume)
{
    if (!m_pCommandList)
    {
        m_pCommandList = new CHXSimpleList;
    }
    
    m_pCommandList->AddTail((void*) bResume);
}



