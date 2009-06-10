/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudev.h,v 1.17 2009/01/22 21:27:30 sfu Exp $
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
#ifndef _HXAUDEV_H_
#define _HXAUDEV_H_

#include "hxprefs.h"

#if defined( _LINUX ) || defined ( _FREEBSD )
#  define kOSS               0
#  define kSupportForOldOSS  1
#  define kESound            2
#  define kALSA              3
#  define kUSound            4
#if defined ( ANDROID )
#  define kAndroidAudio      5
#endif 
#endif


// forward decls
struct IHXAudioDeviceResponse;
struct IHXScheduler;

class CHXAudioSession;

typedef struct _HXAudioFormat	HXAudioFormat;
typedef struct _HXAudioData	HXAudioData;

typedef enum device_state
{
    E_DEV_CLOSED,
    E_DEV_OPENED,
    E_DEV_PAUSED,
    E_DEV_RESUMED
} E_DEVICE_STATE;

/****************************************************************************
 *
 *  Class:
 *
 *      CHXAudioDevice
 *
 *  Purpose:
 *
 *      PN implementation of audio device object.
 *
 */
class CHXAudioDevice: public IHXAudioDevice, public IHXOpaqueAudioDevice
{
private:

	LONG32		m_lRefCount;
	HXBOOL		m_bBuffer;
	HX_RESULT	m_wLastError;
	IHXAudioDeviceResponse* m_pDeviceResponse;

protected:

	IUnknown*	m_pContext;		// context.
	IHXAudioDeviceHookManager* m_pAudioDevHook;	// audio session's IHXAudioDeviceHookManager interface.
	UINT16		m_uMinVolume;		// Client's min volume
	UINT16		m_uMaxVolume;		// Client's max volume
    	UINT16		m_uCurVolume;		// Current volume
    	UINT16		m_uSampFrameSize;	// Frame size.
	HXAudioFormat	m_AudioFmt;		// Device format
	HXBOOL		m_bPaused;		// Whether device is supposed to be paused!
	ULONG32		m_ulCurrentTime;
	ULONG32		m_ulLastSysTime;
	ULONG32		m_ulGranularity;
	ULONG32		m_ulBytesPerGran;
        IHXScheduler*	m_pScheduler;
	E_DEVICE_STATE	m_eState;
	char*		m_pdevName;

	virtual		HX_RESULT	_Imp_Init(IUnknown* pContext) = 0;
	virtual		HX_RESULT	_Imp_Open(const HXAudioFormat* pAudioFormat ) = 0;
	virtual		HX_RESULT	_Imp_OpaqueOpen(const HXAudioFormat* pAudioFormat, const char *pszOpaqueType, IHXBuffer *pOpaqueData ) { return HXR_FAIL; }
	virtual		HX_RESULT	_Imp_Close(void) = 0;
	virtual		HX_RESULT	_Imp_Seek(ULONG32 ulSeekTime) = 0;
	virtual		HX_RESULT	_Imp_Pause(void) = 0;
	virtual		HX_RESULT	_Imp_Resume(void) = 0;
	virtual		HX_RESULT	_Imp_Write(const HXAudioData* pAudioData) = 0;
	virtual		HXBOOL    	_Imp_SupportsVolume(void) = 0;
	virtual		HX_RESULT	_Imp_SetVolume(const UINT16 uVolume) = 0;
	virtual		UINT16		_Imp_GetVolume(void) = 0;
	virtual		HX_RESULT	_Imp_Reset(void) = 0;
	virtual		HX_RESULT	_Imp_Drain(void) = 0;
	virtual		HX_RESULT	_Imp_CheckFormat(const HXAudioFormat* pAudioFormat) = 0;
	virtual		HX_RESULT	_Imp_GetCurrentTime(ULONG32& ulCurrentTime) = 0;
	virtual		INT16		_Imp_GetAudioFd(void) = 0;
	virtual		UINT16		_NumberOfBlocksRemainingToPlay(void) = 0;
	virtual		HXBOOL		_IsWaveOutDevice(void);

	virtual		~CHXAudioDevice();

        //This give the audio devices a chance to init themselves after
        //the context has been set. This is called from Init() and defaults
        //to a noop.
        virtual void _initAfterContext();
        

public:

	char*	getDevName(void)
				{
				    return m_pdevName;
				}

	static		CHXAudioDevice	*Create(IHXPreferences* pPrefs);

	CHXAudioDevice()
			: 	m_lRefCount(0)
			,	m_pDeviceResponse(0)
		        ,	m_pContext(0)
		        ,	m_pAudioDevHook(0)
			,	m_uMinVolume(0)
			,	m_uMaxVolume(0)
			,	m_uCurVolume(0)
			,	m_uSampFrameSize(0)
                        ,	m_bPaused(0)
			,	m_ulCurrentTime(0)
			,	m_ulLastSysTime(0)
			,	m_ulGranularity(0)
			,	m_ulBytesPerGran(0)
		        ,	m_pScheduler(0)
			, 	m_eState(E_DEV_CLOSED)
			, 	m_pdevName(0)
			{
			};

	/* This function calls GetCurrentAudioTime() to get the
	 * an accurate time from the audio playback system.
	 */
	HX_RESULT            OnTimeSync();

	/* A HACK until GetCurrentTime() is implemented.
	 */
	void            SetGranularity(ULONG32 ulGranularity, ULONG32 ulBytesPerGran)
			{ 
			   m_ulGranularity  = ulGranularity; 
			   m_ulBytesPerGran = ulBytesPerGran; 
			};

		/*
	 	 *	IUnknown methods
	 	 */
	STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

	STDMETHOD_(ULONG32,AddRef)  (THIS);

	STDMETHOD_(ULONG32,Release) (THIS);

	void	Init(IUnknown* pContext);
		/*
		 *      IHXAudioDevice methods
		 */

	STDMETHOD(Open)			(THIS_
					const HXAudioFormat*  pAudioFormat,
					      IHXAudioDeviceResponse* pDeviceResponse
					);

	STDMETHOD(Close)		(THIS_
					const HXBOOL      bFlush ) ;

	STDMETHOD(Pause)		(THIS);
	STDMETHOD(Resume)		(THIS);

	STDMETHOD(Write)		(THIS_
					const HXAudioData*	pAudioData
					);

	STDMETHOD(SetBuffering)		(THIS_
					const HXBOOL      bSetBuffering) ;

	STDMETHOD_(HXBOOL,InitVolume)	(THIS_
					const UINT16    uMinVolume,
					const UINT16    uMaxVolume) ;

	STDMETHOD(SetVolume)		(THIS_
					const UINT16    uVolume) ;
	STDMETHOD_(UINT16,GetVolume)		(THIS);
	STDMETHOD(Reset)		(THIS);
	STDMETHOD(Drain)		(THIS);
	STDMETHOD(CheckFormat)		(THIS_
					 const HXAudioFormat* pAudioFormat);

	/*
	*  IHXOpaqueAudioDevice methods
	*/

	STDMETHOD(Open)(THIS_
			const HXAudioFormat*    /*IN*/ pAudioFormat, 
			IHXAudioDeviceResponse* /*IN*/ pDeviceResponse, 
			const char*		    /*IN*/ pszOpaqueType, 
			IHXBuffer*		    /*IN*/ pOpaqueData);

	/* This function returns the most accurate audio playback time 
  	 * that can be ascertained from platform specific calls.
	 */
	STDMETHOD(GetCurrentAudioTime)	(THIS_
					 REF(ULONG32)	ulCurrentTime);
	STDMETHOD(GetAudioFd)		(THIS);

	HX_RESULT Seek( UINT32  ulSeekTime);
	
	E_DEVICE_STATE	GetState(void)
					{
					    return m_eState;
					};

	UINT16	    NumberOfBlocksRemainingToPlay();
	HXBOOL	    IsWaveOutDevice();
};

#endif /* HXAUDEV_H_ */
