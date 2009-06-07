/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxrsmp2.h,v 1.2 2004/07/09 18:37:27 hubbe Exp $
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

#ifndef _HXRSMP2_H_
#define _HXRSMP2_H_

#include "growingq.h"
#include "hxcomm.h"
#include "RAResampler.h"

typedef enum _AudioChannelConversion
{
    AUDIO_CHANNEL_NONE,
    AUDIO_CHANNEL_UP,
    AUDIO_CHANNEL_DOWN
} AudioChannelConversion;

typedef enum _AudioSampleConversion
{
    AUDIO_SAMPLE_NONE,
    AUDIO_SAMPLE_NEEDED    
} AudioSampleConversion;

class HXCDQualityResampler : public IHXAudioResampler
{
protected:
    LONG32		    m_lRefCount;
    RAExactResampler*	    m_pResampler;
    AudioChannelConversion  m_audioChannelConversion;
    AudioSampleConversion   m_audioSampleConversion;
    HXAudioFormat	    m_inAudioFormat;
    HXAudioFormat	    m_outAudioFormat;

    UINT32		    m_ulSamplesSaved;
    UINT32		    m_ulSamplesFixed;
    UINT32		    m_ulBytesFixed;
    short*		    m_pBPS8To16Out;

    void    Downmix16(INT16* pIn, UINT32 nSamplesIn);
    void    BPS8To16(INT16* pIn, UINT32 ulBytesIn, INT16* pOut, UINT32& ulBytesOut);
    void    Close(void);
    void    CopyAudioFormat(HXAudioFormat from, REF(HXAudioFormat) to);

public:

    HXCDQualityResampler();
    ~HXCDQualityResampler();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    /*
     *  IHXResampler methods
     */

    /*
     *  IHXResampler methods
     */

    /**********************************************************************
     *  Method:
     *      IHXAudioResampler::Resample
     *  Purpose:
     *  Will produce 1 output frame for every (upFactor/downFactor) inputs
     *  frames, straddling if not an integer.  Works down to 1 sample/call.
     * 
     *  Returns actual number of output frames.
     ***********************************************************************/

    STDMETHOD_(UINT32, Resample)    (   THIS_
                                        UINT16*	pInput, 
                                        UINT32	ulInputBytes, 
                                        UINT16*	pOutput);

    /**********************************************************************
     *  Method:
     *      IHXAudioResampler::Requires
     *  Purpose:
     *  Returns number of input frames required to produce this number
     *  of output frames, given the current state of the filter.
     */

    STDMETHOD_(UINT32, Requires)    (   THIS_
                                        UINT32 ulOutputFrames);

    HX_RESULT	Init(HXAudioFormat	    inAudioFormat,
		     REF(HXAudioFormat)    outAudioFormat);
};

#endif //_HXRSMP2_H_
