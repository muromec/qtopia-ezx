/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: RAExactResampler.h,v 1.2 2004/07/09 18:37:27 hubbe Exp $
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

#ifndef _RAEXACTRESAMPLER_H_
#define _RAEXACTRESAMPLER_H_

#include "hxresult.h"

class RAExactResampler
{
public:
    // resampler quality
    enum {
        qualityVeryLow= 0, // as low a quality as we dare
        qualityLow    = 1, // somewhat better
        qualityMedium = 2, // probably sufficient for the player
        qualityHigh   = 3, // the default, giving CD quality,
        qualityHyper  = 4  // better than CD quality (think DSD)
    } ;

    // Create a resampler with a given quality level.
    static HX_RESULT Create(RAExactResampler** pRes, int inrate, int outrate, int chans, int intype, int quality = qualityHigh) ;

    // If you know the specs your resampler is required to have, use one of these functions.
    static HX_RESULT Create(RAExactResampler** pRes, int inrate, int outrate, int chans, int intype, float atten, float trans, float dcgain) ;

    static HX_RESULT Create(RAExactResampler** pRes, int inrate, int outrate, int chans, int intype, float atten, float passband, float stopband, float dcgain) ;

    virtual ~RAExactResampler() ;

    // this is the input data type.
    enum {
        _INT16 = 0,
        _INT32 = 1,
        _FLOAT = 2
    } ;

    /*
     * insamps is the number of samples (not sample frames) you are handing the resampler.
     * Note that in the case of stereo, this must be divisible by two.
     */
    virtual int Resample(void *inbuf, int insamps, signed short *outbuf) = 0 ;
    virtual int Resample(void *inbuf, int insamps, signed int *outbuf) = 0 ;
    /*
     * The maximum output, in samples, that could be produced by
     * insamps input samples.
     */
    virtual int GetMaxOutput(int insamps) = 0 ;
    /*
     * The minimum input, in samples, that will produce at least
     * outsamps output samples.
     */
    virtual int GetMinInput(int outsamps) = 0 ;
    /*
     * The delay of this filter, in frames of output frequency.
     */
    virtual int GetDelay() = 0 ;

protected:
    RAExactResampler() ; // make this protected -- use the Create() function!
};

#endif // _RAEXACTRESAMPLER_H_
