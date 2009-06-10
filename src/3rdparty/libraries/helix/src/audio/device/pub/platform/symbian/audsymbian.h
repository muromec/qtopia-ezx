/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: audsymbian.h,v 1.21 2009/02/27 22:56:59 shivnani Exp $
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

#ifndef _AUDSYMBIAN
#define _AUDSYMBIAN
#include <stdio.h>
#include <e32base.h>
#include <mmf/common/Mmfbase.h>
#include "hxengin.h"
#include "hxcom.h"
#include "hxausvc.h"
#include "auderrs.h"
#include "hxaudply.h"
#include "chxmapptrtoptr.h"
#include "AudDevStatusObserver.h"
#include "hxmon.h"


struct IHXAudioDeviceResponse;
class CHXSimpleList;
class MHXSymbianAudioDevice;
class HXSymbianAudioServerContext;
_INTERFACE IUnknown;
_INTERFACE IHXErrorMessages;

class CHXAudioDevice : public IHXAudioDevice
                       ,public CActive
                       ,public CHXAudDevStatusObserver
                       ,public IHXPropWatchResponse
{
  public:
    CHXAudioDevice();

    static const int MAX_VOLUME;

    //IUnkown methods.
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    //IHXAudioDevice methods
    HX_RESULT Open( const HXAudioFormat* pFormat,
                    IHXAudioDeviceResponse* );
    HX_RESULT Close( const HXBOOL bFlush);
    HX_RESULT Write( const HXAudioData* pAudioOutHdr );
    HX_RESULT Reset( void );
    HX_RESULT Drain( void );
    HX_RESULT SetVolume( const UINT16 uVolume );
    HX_RESULT GetCurrentAudioTime( ULONG32& ulCurrentTime);
    HXBOOL      SupportsVolume( void );
    UINT16    GetVolume( void );
    short     GetAudioFd( void );
    HX_RESULT Seek(ULONG32 ulSeekTime);
    HX_RESULT Resume( void );
    HX_RESULT Pause( void );
    HX_RESULT CheckFormat( const HXAudioFormat* pFormat);
    UINT16    NumberOfBlocksRemainingToPlay(void);
    HXBOOL      InitVolume( const UINT16 uMinVolume, const UINT16 uMaxVolume);

    void Init(IUnknown* pContext);
    inline HXBOOL IsWaveOutDevice() { return TRUE; }
    inline void SetGranularity(ULONG32 ulGranularity,
			       ULONG32 ulBytesPerGran) {}
    inline static CHXAudioDevice* Create(IHXPreferences* pPrefs)
	{ return new CHXAudioDevice; }

    // CActive
    virtual void RunL();
    virtual void DoCancel();

    // CHXAudDevStatusObserver methods
    void OnAudDevStatusChange(TInt status);

    //IHXPropWatchResponse methods
	STDMETHOD(AddedProp)	(THIS_ const UINT32 ulId, const HXPropType propType, const UINT32 ulParentID);
	STDMETHOD(ModifiedProp)	(THIS_ const UINT32	ulId, const HXPropType propType, const UINT32 ulParentID);
	STDMETHOD(DeletedProp)	(THIS_ const UINT32	ulId, const UINT32 ulParentID);

  protected:

    //--------------------------------------------------
    // DEVICE SPECIFIC METHODS
    //--------------------------------------------------
    virtual ~CHXAudioDevice();
    HX_RESULT OpenDevice();
    HX_RESULT InitDevice(const HXAudioFormat* pFormat);

  private:

    //protect the un-intentional copy ctor and assignment.
    CHXAudioDevice( const CHXAudioDevice& );
    CHXAudioDevice& operator=(const CHXAudioDevice& it );

    //helpers
    void DoSetTimer();
    void GetPrioritySettings();

    HX_RESULT RegisterAudDeviceErrNotification();
    void CancelAudDeviceErrNotification();

    LONG32                          m_lRefCount;
    bool                            m_deviceOpen;
    IHXAudioDeviceResponse*         m_pDeviceResponse;
    MHXSymbianAudioDevice*          m_pAudioStream;
    bool                            m_paused;
    UINT16                          m_uMinPlayerVolume;
    UINT16                          m_uMaxPlayerVolume;
    UINT32                          m_uMinDevVolume;
    UINT32                          m_uMaxDevVolume;
    TMMFPrioritySettings*           m_pPrioritySettings;
    RTimer                          m_iTimer;
    IUnknown*                       m_pContext;
    IHXErrorMessages*               m_pErrorMessages;

	HXBOOL							m_bSecureOutputChanged;
	UINT32							m_uSecureOutputChangeTime;
	IHXPropWatch*					m_pPropWatch;
    INT32               			m_lSecureOutputSetting;

};

#endif  //_AudioOutSYMBIAN
