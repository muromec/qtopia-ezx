/* ***** BEGIN LICENSE BLOCK *****
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

#ifndef FAKEAUDIODEVICE_H
#define FAKEAUDIODEVICE_H

// Includes
#include "hxausvc.h"
#include "hxengin.h"

// Defines
typedef enum _HXFakeAudioDeviceState
{
    HXFakeAudioDeviceStateClosed,
    HXFakeAudioDeviceStateOpened,
    HXFakeAudioDeviceStatePaused,
    HXFakeAudioDeviceStateResumed
}
HXFakeAudioDeviceState;


class CHXFakeAudioDevice : public IHXAudioDevice,
                           public IHXCallback
{
public:
    CHXFakeAudioDevice(IUnknown* pContext, UINT32 ulGranularity);
    virtual ~CHXFakeAudioDevice();

    // IUnknown methods
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    // IHXAudioDevice methods
    STDMETHOD(Open)                (THIS_ const HXAudioFormat* pAudioFormat, IHXAudioDeviceResponse* pStreamResponse);
    STDMETHOD(Close)               (THIS_ const HXBOOL bFlush);
    STDMETHOD(Resume)              (THIS);
    STDMETHOD(Pause)               (THIS);
    STDMETHOD(Write)               (THIS_ const HXAudioData* pAudioData);
    STDMETHOD_(HXBOOL,InitVolume)  (THIS_ const UINT16 uMinVolume, const UINT16 uMaxVolume);
    STDMETHOD(SetVolume)           (THIS_ const UINT16 uVolume);
    STDMETHOD_(UINT16,GetVolume)   (THIS);
    STDMETHOD(Reset)               (THIS);
    STDMETHOD(Drain)               (THIS);
    STDMETHOD(CheckFormat)         (THIS_ const HXAudioFormat* pAudioFormat);
    STDMETHOD(GetCurrentAudioTime) (THIS_ REF(ULONG32) ulCurrentTime);

    // IHXCallback methods
    STDMETHOD(Func) (THIS);

    // CHXFakeAudioDevice methods
    static HX_RESULT STDAPICALLTYPE HXCreateFakeAudioDevice(IUnknown*            pContext,
                                                            UINT32               ulGranularity,
                                                            REF(IHXAudioDevice*) rpAudioDevice);
    HXFakeAudioDeviceState          GetFakeAudioDeviceState() { return m_eState; }
protected:
    IUnknown*               m_pContext;
    IHXScheduler*           m_pScheduler;
    INT32                   m_lRefCount;
    HXAudioFormat           m_AudioFormat;
    IHXAudioDeviceResponse* m_pResponse;
    UINT16                  m_usVolume;
    UINT16                  m_usMinVolume;
    UINT16                  m_usMaxVolume;
    Timeval*                m_pCallbackTime;
    UINT32                  m_ulLastCallbackTime;
    UINT32                  m_ulLastCallbackTick;
    UINT32                  m_ulGranularity;
    CallbackHandle          m_CallbackID;
    HXFakeAudioDeviceState  m_eState;

    void ClearCallback();
};

#endif /* #ifndef FAKEAUDIODEVICE_H */
