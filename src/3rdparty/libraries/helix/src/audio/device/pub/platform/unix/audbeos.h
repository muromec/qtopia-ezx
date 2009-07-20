/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audbeos.h,v 1.5 2006/02/16 23:04:50 ping Exp $
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

/*******************************************************************
 *
 *	audbeos.h
 *
 *	CLASS: CAudioOutBeOS
 *	
 *	DESCRIPTION: Class definition for BeOS-specific audio devices 
 *
 *******************************************************************/

#ifndef _AUDBEOS_H_
#define _AUDBEOS_H_

#include <GameSoundDefs.h>

struct IHXCallback;
class BPushGameSound;

#define _BEOS_AUDIODEV_CALLBACK		0

class CAudioOutBeOS : public CHXAudioDevice
{
public:
	CAudioOutBeOS();

#if _BEOS_AUDIODEV_CALLBACK
	/* Create friend class for scheduled playback callback.
	*/
	class HXPlaybackCountCb : public IHXCallback
	{
	private:
		LONG32			m_lRefCount;
		~HXPlaybackCountCb();


	public:
		HXPlaybackCountCb(HXBOOL timed = TRUE);

		HXBOOL			m_timed;
		CAudioOutBeOS	*m_pAudioDeviceObject;

		/*
		*  IUnknown methods
		*/
		STDMETHOD(QueryInterface) (THIS_
							REFIID riid,
							void** ppvObj);

		STDMETHOD_(ULONG32,AddRef) (THIS);

		STDMETHOD_(ULONG32,Release) (THIS);
		/*
		*  IHXCallback methods
		*/
		STDMETHOD(Func) (THIS);
	};
	friend HXPlaybackCountCb;
#endif

protected:
	virtual ~CAudioOutBeOS();

	HX_RESULT	_Imp_Init(IUnknown* pContext) {return HXR_OK;};
	HX_RESULT   	    _Imp_Open( const HXAudioFormat* pFormat );
	HX_RESULT   	    _Imp_Close( void );
	HX_RESULT	    _Imp_Seek(ULONG32 ulSeekTime);
	HX_RESULT   	    _Imp_Pause( void );
	HX_RESULT   	    _Imp_Resume( void );
	HX_RESULT   	    _Imp_Write( const HXAudioData* pAudioOutData );
	HX_RESULT   	    _Imp_Reset( void );
	HX_RESULT  	    _Imp_Drain( void );
	HXBOOL 		    _Imp_SupportsVolume( void );
	UINT16   	    _Imp_GetVolume( void );
	HX_RESULT   	    _Imp_SetVolume( const UINT16 uVolume );
	HX_RESULT   	    _Imp_CheckFormat( const HXAudioFormat* pFormat);
	HX_RESULT 	    _Imp_GetCurrentTime( ULONG32& ulCurrentTime);
	INT16		    _Imp_GetAudioFd(void) {return 0;};
	UINT16		    _NumberOfBlocksRemainingToPlay(void);

private:
    void		    SetFormat(const HXAudioFormat* pFormat);
	UINT32			CalculateElapsedBytes(UINT32 ulLastBytePos, UINT32 ulCurrentBytePos);
    UINT32		    CalcMs(ULONG32 ulNumBytes);
#if _BEOS_AUDIODEV_CALLBACK
	void			DoTimeSyncs(void);
	HX_RESULT		ReschedPlaybackCheck(void);
#endif

	BPushGameSound			*m_player;
	gs_audio_format			m_gameSoundFormat;
    CHXSimpleList			*m_pPendingBufferList;
    UINT32					m_ulLastPlayCursor;
    UINT32					m_ulCurrentPlayTime;
    UINT32					m_ulLastTimeSync;
	UINT32					m_ulBufferSize;
	UINT32					m_ulFrameSize;
	UINT32					m_ulNextWriteOffset;
	UINT32					m_ulOldBytesLeft;
	UINT32					m_ulBlockSize;
	HX_BITFIELD				m_bGotAWrite : 1;
    HX_BITFIELD				m_bFirstWrite : 1;
    HX_BITFIELD				m_bPlaying : 1;
#if _BEOS_AUDIODEV_CALLBACK
	Timeval					*m_pPlaybackCountCBTime;
	HXBOOL					m_bCallbackPending;
	ULONG32					m_PendingCallbackID;
#endif
};


#endif 	// _AUDBEOS_H_

