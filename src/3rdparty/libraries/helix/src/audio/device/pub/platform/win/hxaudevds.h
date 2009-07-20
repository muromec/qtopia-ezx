/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxaudevds.h,v 1.14 2006/02/16 23:04:51 ping Exp $
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

#ifndef _NEWRMAUDEVDS_H_
#define _NEWRMAUDEVDS_H_

#include "dsound.h"
#include "ks.h"
#include "ksmedia.h"

#include "tsconvrt.h"

#include "hxstring.h"

class CHXAudioSession;
class CHXAudioDevice;

interface IHXAudioDeviceHookDMO;

typedef struct _HXAudioFormat	HXAudioFormat;
typedef struct _HXAudioData	HXAudioData;

/****************************************************************************
 *
 *  Class:
 *
 *      CHXAudioDeviceDS
 *
 *  Purpose:
 *
 *      PN implementation of audio device object.
 *
 */

class CHXAudioDeviceDS: public CHXAudioDevice
{
protected:
	IDirectSound8*	m_pDSDev;
	IDirectSoundBuffer *m_pPrimaryBuffer, *m_pSecondaryBuffer ;
	DWORD		defaultChannelMapping(UINT32 ulChannels) const ;

	HXBOOL		m_bOpaqueFormat;
	WAVEFORMATEX*	m_pWaveFormat;
	UINT32		m_ulLastPlayCursor;
	UINT32		m_ulLastWriteCursor;
	UINT32		m_ulCurrPlayTime;
	UINT32		m_ulCurrLoopTime;
	UINT32		m_ulTotalBuffer;
	HWND		m_hwnd;
	void*		m_pAudioPtrStart;
	UINT32		m_ulLoops;
	double		m_ulLoopTime;
	int		m_nBlocksPerBuffer;
	HINSTANCE	m_hSoundDll;
	HANDLE		m_hDSNotifyEvent;
	HANDLE		m_hWaitThread;
	HXBOOL		m_bExitThread;
	UINT32		m_ulOriginalThreadId;
	CTSConverter	m_TSConverter;
	IHXAudioDeviceHookDMO*	m_pAudioHookDMO;

	HX_RESULT	SetPrimaryBufferFormat() ;

	inline UINT32 CalcMs(UINT32 ulNumBytes);
	HANDLE GetEventHandle() { return m_hDSNotifyEvent; };
	HXBOOL GetExitCode() { return m_bExitThread; };
	void PostTimeSyncMessage();
	void KillThreadAndEvent();
	virtual HX_RESULT LoadDirectSoundFilter();
	HX_RESULT RegisterDirectSoundFilter();
	HX_RESULT GetFilterPathFromRegistry(CHXString & rstrPath);
	HX_RESULT InternalClose();

        static DWORD WINAPI EventThreadProc(LPVOID pVoid) ;

public:

    virtual ~CHXAudioDeviceDS();

    CHXAudioDeviceDS();

    HX_RESULT		    _Imp_Init(IUnknown* pContext) {return HXR_OK;};
    HX_RESULT   	    _DoOpen( const HXAudioFormat* pFormat );
    HX_RESULT   	    _Imp_Open( const HXAudioFormat* pFormat );
    HX_RESULT   	    _Imp_OpaqueOpen( const HXAudioFormat* pFormat, const char *pszOpaqueType, IHXBuffer *pOpaqueData );
    HX_RESULT   	    _Imp_Close( void );
    HX_RESULT		    _Imp_Seek(ULONG32 ulSeekTime) {     return HXR_OK; };
    HX_RESULT   	    _Imp_Pause( void );
    HX_RESULT   	    _Imp_Resume( void );
    HX_RESULT   	    _Imp_Write( const HXAudioData* pAudioOutData );
    HX_RESULT   	    _Imp_Reset( void );
    HX_RESULT  		    _Imp_Drain( void );
    HXBOOL 		    _Imp_SupportsVolume( void ) { return TRUE; };
    UINT16   		    _Imp_GetVolume( void );
    HX_RESULT   	    _Imp_SetVolume( const UINT16 uVolume );
    HX_RESULT   	    _Imp_CheckFormat( const HXAudioFormat* pFormat);
    HX_RESULT 		    _Imp_GetCurrentTime( ULONG32& ulCurrentTime);
    INT16		    _Imp_GetAudioFd(void) {return 0;};
    UINT16		    _NumberOfBlocksRemainingToPlay(void){return 0;};
    HXBOOL		    _IsWaveOutDevice(void) { return FALSE; };

    static UINT	    zm_uDestroyMessage;
};

#endif /* _NEWRMAUDEVDS_H_ */
