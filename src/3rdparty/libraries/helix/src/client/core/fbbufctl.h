/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: fbbufctl.h,v 1.6 2007/01/11 19:53:31 milko Exp $
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

#ifndef FBBUFCTL_H
#define FBBUFCTL_H

#include "hxbufctl.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxsmbw.h"
#include "hxerror.h"
#include "hxprefs.h" // IHXPreferences
#include "hxpends.h" // IHXPendingStatus

class HXFeedbackControl
{
public:
    HXFeedbackControl();
    
    void Init(double c, double wn, double Kv, double delt);

    void Reset(UINT32 ulSetPoint, 
	       INT32 e1, INT32 e2,
	       UINT32 c1, UINT32 c2);

    void SetLimits(UINT32 ulMin, UINT32 ulMax);

    UINT32 Control(UINT32 ulCurrentBits);

    double SamplePeriod() const { return m_delt;}
    UINT32 SetPoint() const {return m_ulSetPoint;}
    UINT32 Min() const { return m_ulMin;}
    UINT32 Max() const { return m_ulMax;}

private:
    UINT32 RoundAndClip(double value) const;
    void Enqueue(INT32 error, UINT32 ulBandwidth);

    double m_delt;		// Sample period sec
    double m_a1;		// Weights in bandwith computation
    double m_a2;
    double m_b1;
    double m_b2;

    UINT32 m_ulSetPoint;        // Target value to stablize at

                                // Output clipping values
    UINT32 m_ulMin;             // Minimum output value
    UINT32 m_ulMax;             // Maximum output value

    INT32 m_e1;                 // Error history
    INT32 m_e2;
    UINT32 m_c1;                // Control history
    UINT32 m_c2;

};

class HXFeedbackBufferControl : public IHXBufferControl,
				public IHXCallback
{
public:
    HXFeedbackBufferControl();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)   (THIS_
                                REFIID riid,
                                void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)  (THIS);

    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     * IHXBufferControl method
     */

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::Init
     *	Purpose:
     *      Initialize the buffer control object with a context
     *      so it can find the interfaces it needs to do buffer
     *      control
     */
    STDMETHOD(Init) (THIS_ IUnknown* pContext);

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnBuffering
     *	Purpose:
     *      Called while buffering
     */
    STDMETHOD(OnBuffering) (THIS_ UINT32 ulRemainingInMs,
			    UINT32 ulRemainingInBytes);

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnBufferingDone
     *	Purpose:
     *      Called when buffering is done
     */
    STDMETHOD(OnBufferingDone)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnResume
     *	Purpose:
     *      Called when playback is resumed
     */
    STDMETHOD(OnResume) (THIS);
    
    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnPause
     *	Purpose:
     *      Called when playback is paused
     */
    STDMETHOD(OnPause) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnSeek
     *	Purpose:
     *      Called when a seek occurs
     */
    STDMETHOD(OnSeek) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::OnClipEnd
     *	Purpose:
     *      Called when we get the last packet in the clip
     */
    STDMETHOD(OnClipEnd) (THIS);

    /************************************************************************
     *	Method:
     *	    IHXBufferControl::Close()
     *	Purpose:
     *      Called when the owner of this object wishes to shutdown
     *      and destroy this object. This call causes the buffer control
     *      object to release all it's interfaces references.
     */
    STDMETHOD(Close)(THIS);

    /************************************************************************
     *	Method:
     *	    IHXCallback::Func
     *	Purpose:
     *	    This is the function that will be called when a callback is
     *	    to be executed.
     */
    STDMETHOD(Func)		(THIS);

private:
    typedef enum {csError,
		  csNotInitialized,
		  csInitialized,
		  csBuffering,
		  csPlaying,
		  csPaused,
		  csSeeking,
		  csClipEnd} ControlState;

    ~HXFeedbackBufferControl();

    void ChangeState(ControlState newState);
    HXBOOL Initialized() const { return (csInitialized <= m_state);}

    void ScheduleCallback();
    void StartCallback();
    void StopCallback();

    HX_RESULT GetBwInfo(REF(UINT32) ulClipBw,
			REF(UINT16) nControlStream, REF(UINT32) ulControlBw);

    HX_RESULT GetStreamBw(UINT16 uStreamNumber, REF(UINT32) ulBandwidth);
    HXBOOL IsAudioStream(UINT16 uStreamNumber);
    
    void GetControlData(REF(UINT32) ulTotalBits,
			REF(UINT32) ulControlBits);

    HX_RESULT Control(UINT32 ulControlBits, REF(UINT32) ulNewBandwidth);
    void SetBandwidth(UINT32 ulBandwidth);

    HX_RESULT ReadPrefSettings();
    void SetTransportByteLimits(UINT32 ulClipBw);

    HXBOOL IsBuffering();

    UINT32 ControlToTotal(UINT32 ulValue);
    UINT32 TotalToControl(UINT32 ulValue);
    UINT32 MulDiv(UINT32 ulValue, UINT32 ulMult, UINT32 ulDiv);
    
    UINT32 ClampBandwidth(UINT32 ulBandwidth) const;

    // Used to initialize functionallity related to testing
    HX_RESULT InitTesting(IUnknown* pContext);

    ULONG32 m_lRefCount;

    IHXScheduler* m_pScheduler;
    CallbackHandle m_callbackHandle;
    HXTimeval m_lastTime;

    IHXStreamSource* m_pSource;
    IHXSourceBufferingStats3* m_pBufferStats;
    IHXThinnableSource* m_pThin;
    IHXPreferences* m_pPrefs;
    IHXPendingStatus* m_pStatus;

    IHXErrorMessages* m_pErrMsg;
    
    HXFeedbackControl m_control; // control algorithm

    UINT32 m_ulClipBw;           // Clip bandwidth

    // Control stream values
    UINT16 m_nControlStream;     // Control stream index
    UINT32 m_ulControlBw;        // Control stream bandwidth

    HXBOOL m_bControlTotal;        // Control total buffer?

    HXBOOL m_bPaused;
    ControlState m_state;
};

#endif /* FBBUFCTL_H */
