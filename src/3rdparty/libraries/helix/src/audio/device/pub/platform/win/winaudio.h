/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: winaudio.h,v 1.9 2007/11/02 09:28:09 lovish Exp $
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

#ifndef _WINAUDIO_H_
#define _WINAUDIO_H_

struct IHXCallback;

class CHXSimpleList;
class CPtrQueue;

#define	WND_CLASS   _T("AudioServicesInternal")

#if defined( _WIN32 )
typedef    LPWAVEFORMATEX	WAVEFMTPTR;
#elif defined( _WINDOWS )
typedef    LPWAVEFORMAT		WAVEFMTPTR;
#endif

typedef enum 
{
    HXAUDIO_UNKNOWN,
    HXAUDIO_BADDEVICE,
    HXAUDIO_GOODDEVICE
} audioDevice;

/****************************************************************************
 * 
 *  Class:	CWindowsWaveFormat
 * 
 *  Purpose:	Used to hide 16bit vs. 32bit bullsheet for the windows 
 *		wave format structs...
 */
class CWindowsWaveFormat
{
public:
    CWindowsWaveFormat()
		{
		    mpwf = NULL;
		    SetFormatDflt();
		}

    ~CWindowsWaveFormat()
		{
//    		    if (mpwf != NULL)
//			delete [] mpwf;
		}

    void SetFormatDflt()
		{
#if defined(_WIN32)
		    mwf.wFormatTag = WAVE_FORMAT_PCM;
		    mwf.nChannels = 1;
		    mwf.nSamplesPerSec = 8000; // 11025;
		    mwf.nAvgBytesPerSec = 16000; // 22050;
		    mwf.nBlockAlign = 2;
		    mwf.wBitsPerSample = 16;
		    mwf.cbSize = 0;
#else
		    mwf.wf.wFormatTag = WAVE_FORMAT_PCM;
		    mwf.wf.nChannels = 1;
		    mwf.wf.nSamplesPerSec = 8000; // 11025;
		    mwf.wf.nAvgBytesPerSec = 16000; // 22050;
		    mwf.wf.nBlockAlign = 2;
		    mwf.wBitsPerSample = 16;
#endif
		}

    void SetFormat
		(
		    ULONG32 inSampleRate,
		    UINT16  channels,
		    UINT16  bitsPerSample
		)
		{
#if defined(_WIN32)
		    mwf.wFormatTag = WAVE_FORMAT_PCM;
		    mwf.nChannels = channels;
		    mwf.nSamplesPerSec = inSampleRate;	//8000; // 11025;

		    // note this works for 1 or 2 channels only!!
		    mwf.nAvgBytesPerSec = bitsPerSample <= 8 ? inSampleRate * channels
													    : inSampleRate * channels * 2;
		    mwf.nBlockAlign = bitsPerSample <= 8 ? 1 * channels : channels * 2;
		    mwf.wBitsPerSample = bitsPerSample;
		    mwf.cbSize = 0;
#else
		    mwf.wf.wFormatTag = WAVE_FORMAT_PCM;
		    mwf.wf.nChannels = channels;
		    mwf.wf.nSamplesPerSec = inSampleRate; //8000; // 11025;
		    mwf.wf.nAvgBytesPerSec = bitsPerSample <= 8 ? inSampleRate * channels
													    : inSampleRate * channels * 2;
		    mwf.wf.nBlockAlign = bitsPerSample <= 8 ? 1 * channels : channels * 2;
		    mwf.wBitsPerSample = bitsPerSample;
#endif
		}

    WAVEFMTPTR GetWaveFormat()
		{
//		    if (mpwf != NULL)
//			return (mpwf);
		    return (WAVEFMTPTR(&mwf));
		}

    WORD GetWaveFormatSize ()
		{
//		    if (mpwf != NULL) return (mcbWaveFmt);
		    return (sizeof (mwf));
		}
private:

	WAVEFMTPTR	mpwf;				// ptr to wave format header
	WORD		mcbWaveFmt;			// size of wave format header

#if defined(_WIN32)
	WAVEFORMATEX	mwf;				// default PCM wave format header
#else
	PCMWAVEFORMAT	mwf;				// default PCM wave format header
#endif

};

/****************************************************************************
 * 
 *  Class:	CWaveHeader
 * 
 *  Purpose:	Manages a WAVEHDR struct, includes info about usage, etc.
 * 
 */
class CWaveHeader
{
public:
	ULONG32		    m_ulTimeEnd;
	void*		    m_pUser;	    // General Purpose user object.

protected:
	WAVEHDR		    m_WAVEHDR;
	HXBOOL		    m_bAvail;
	HXBOOL		    m_pPrepared;

	CWaveHeader()
	    {
		memset( &m_WAVEHDR, 0, sizeof(m_WAVEHDR) );
		m_bAvail = FALSE;
		m_pPrepared = FALSE;
 		m_ulTimeEnd = 0;
		m_pUser = NULL;
	    }
	friend class CAudioOutWindows;
};

class CHXAudioDevice;
class HXMutex;

/****************************************************************************
 * 
 *  Class:	CAudioOutWindows
 * 
 *  Purpose:	Windows implementation of audio out
 * 
 */
class CAudioOutWindows : public CHXAudioDevice
{
public:
	CAudioOutWindows();

#if defined(_WIN32)
	static LRESULT __declspec(dllexport) CALLBACK 
		WaveOutWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	#else
	static LRESULT CALLBACK __export 
		WaveOutWndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
	#endif


protected:
	virtual ~CAudioOutWindows();

	HX_RESULT	    _Imp_Init(IUnknown* pContext);
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
#ifdef DONT_WRITE_WHILE_PAUSE_AT_RESET
	UINT16		    _NumberOfBlocksPlaying(void);
#endif
	HXBOOL		    _IsWaveOutDevice(void) { return TRUE; };

private:

	static HXBOOL	    zm_bVolSupport;
	static HXBOOL	    zm_bLRVolSupport;
	static WORD	    zm_uMaxVolume;

	static HXBOOL	    zm_bMixerVolSupport;
	static HXBOOL	    zm_bMixerVolSupportChecked;

	CWindowsWaveFormat  m_WaveFormat;

	HWAVEOUT	    m_hWave;
	WAVEOUTCAPS	    m_waveOutCap;

	UINT16		    m_unAllocedBufferCnt;
	UINT16		    m_unAllocedBuffSize;

        UCHAR**		    m_ppAllocedBuffers;
	CWaveHeader*	    m_pWaveHdrs;

	CPtrQueue	    m_rAvailBuffers;
	CHXSimpleList	    m_UsedBuffersList;
	HXBOOL		    m_bInitialized;
	HXBOOL		    m_bResetting;
	IHXMutex*	    m_pMutex;
	
	HXBOOL		    m_bIsFirstPacket;
#ifdef DONT_WRITE_WHILE_PAUSE_AT_RESET
	HXBOOL		    m_bPausedAtReset;
	HXBOOL		    m_bPausedForRestart;
#endif //DONT_WRITE_WHILE_PAUSE_AT_RESET

	HXBOOL		    AllocateBuffers(UINT16 unNumBuffers, UINT16 unBufSize);

	HINSTANCE	    m_hInst;
	HWND		    m_hWnd;
	HXBOOL		    m_bClassRegistered;
#if defined(_WIN32)
	UINT32		    m_ulOriginalThreadId;
#endif

#if defined(_WIN32)
	void		    CheckForVolumeSupport(void);

        HMIXER               m_hMixer;
        MIXERCONTROLDETAILS  m_VolumeControlDetails;
#endif 

	HX_RESULT	    Register();
	void		    UnRegister();

	INT64		    m_llDeviceBytesPlayed;
	INT64		    m_llDeviceSamplesPlayed;

	UINT32		    m_ulLastDeviceBytesPlayed;
	UINT32		    m_ulLastDeviceSamplesPlayed;
	UINT32		    m_ulDevPosRollOver;

	static audioDevice  zm_audioDevice;
	static HXBOOL	    zm_bClosed;
	static UINT	    zm_uDestroyMessage;
};


#endif 	// _WINAUDIO_H_

