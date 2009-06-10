/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#include "sconceal.h"

#include "hlxclib/string.h" // memcpy()
#include "hlxclib/stdlib.h" // rand()
#include "hlxclib/float.h"  // FLT_MIN and friends
#include "hlxclib/math.h"   // sqrt, pow...

#ifdef _VXWORKS
#include "private/trigP.h"
#endif

#if !defined(_WINDOWS) && !defined(_OPENWAVE)
# define _copysign copysign
#endif

#ifndef max
#define max(a,b) ((a)>=(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<=(b)?(a):(b))
#endif

/*
  nLinesMax denotes the maximum number of spectral lines this
  object will ever be fed. No testing done!

  Currently, we use dynamic memory allocation in here which raises
  the question how we should fail if we run out of memory.

  It might make sense to allocate memory statically.
 */

#ifdef _CARBON
#pragma old_argmatch on
#endif

CConcealment::CConcealment(int nLinesMax)
: m_offset(0), m_nLinesMax(nLinesMax), lastConcealed(0)
{
  int i ;
  // allocate spectral history. Should really check if succeeded.
  m_history[0].spectrum = new float[m_nLinesMax * sizeofBuffer];

  // clear spectral history
  zeroFloat(m_history[0].spectrum, m_nLinesMax * sizeofBuffer) ;

  for (i = 0; i < sizeofBuffer ; i++)
  {
    m_history[i].spectrum        = m_history[0].spectrum + m_nLinesMax * i;
    m_history[i].spectrumPresent = 1 ;
    m_history[i].blockType       = 0 ;
    m_history[i].nLinesActive    = 0 ;
  }
}

CConcealment::~CConcealment()
{
  if (m_history[0].spectrum)
    delete[] m_history[0].spectrum ;
}

/*
  throw out old spectrum, making room for new. Implemented via a
  circular buffer of pointers to spectra.
 */

void CConcealment::rotateHistory()
{
  int i ;

  // kick time forward
  if (++m_offset == sizeofBuffer) m_offset = 0 ;

  for (i = 0 ; i < sizeofBuffer ; i++)
  {
    m_historyPtr[i] = &m_history[(i + m_offset) % sizeofBuffer] ;
  }
}

/*
  user-calleable function.
  insert spectrum (MDCT / MLT coefficients) into our delay line.
  Currently, blocktype is of Layer-3 flavor (2 == short block);
  this should be generalized.

  (Currently, spectra with block type of 2 are handed out unchanged,
  but concealed afterwards because their layout is different. For G2,
  this behaviour is unnecessary)

  nLines is the highest active spectral line in this block
  (i.e. the audio bandwidth)
 */
void CConcealment::insert(const float* s, int blocktype, int nLines)
{
  rotateHistory() ;

  /*
   * insert current information into the future
   */
  
  if (nLines == -1)        // if we were passed -1
    nLines = m_nLinesMax ; // it means "all"

  nLinesActive(sizeofFuture)    = nLines ;
  blockType(sizeofFuture)       = blocktype ;
  spectrumPresent(sizeofFuture) = (s != 0) ;

  // copy all lines containing energy
  if (s) memcpy(spectrum(sizeofFuture), s, sizeof(float)*nLines); /* Flawfinder: ignore */
  // zero out the rest

  if (!s) nLines = 0 ;

  zeroFloat(&(spectrum(sizeofFuture)[nLines]), m_nLinesMax - nLines);
}

/*
  no spectrum present, schedule this spectrum to be concealed.
  call this if you have a defect, or missing, spectrum.
 */
void CConcealment::insert()
{
  insert(0,0,0) ;
}

/*
  retrieve a valid spectrum, concealed if necessary
  returns index+1 of highest spectral line containing energy
  (i.e. audio bandwidth) and Layer-3 type blocktype.
 */
int CConcealment::retrieve(float* s, int &blocktype)
{
  int linesActive ;

  // if spectrum is present, return it as-is
  if (spectrumPresent(0))
  {
    memcpy(s, spectrum(0), sizeof(float)*m_nLinesMax); /* Flawfinder: ignore */
    blocktype = blockType(0) ;
    linesActive = nLinesActive(0) ;
  }

  // conceal this spectrum if it was a short block or damaged
  if (!spectrumPresent(0) ||
       blockType(0) == 2)
  {
    conceal(CONCEAL_PREDICTION);
    lastConcealed = 1 ;
  }
  else
  {
    lastConcealed = 0 ;
  }

  // if spectrum was damaged, return concealed version
  if (!spectrumPresent(0))
  {
    memcpy(s, spectrum(0), sizeof(float)*m_nLinesMax); /* Flawfinder: ignore */
    blocktype = blockType(0) ;
    linesActive = nLinesActive(0) ;
  }

  // if this spectrum was concealed because it was a short spectrum,
  // mark it as non-present (so it won't be included in the prediction)

  if (blockType(0) == 2)
  {
    spectrumPresent(0) = 0 ;
    blockType(0) = 0 ;
  }

  // this would be a good time to update predictor coefficients
  // (if we knew how to)

  // return number of lines containing energy
  return linesActive ;
}

/*
  a dispatcher between different methods of concealment. This can
  probably go away once I've settled on one method.
 */
void CConcealment::conceal(int method)
{
  switch(method)
  {
  case CONCEAL_PREDICTION:
    concealByPrediction();
    break ;
  }
}

/*
  magic numbers for concealment-by-prediction
 */

#define alpha 1.0f // weighting factor old vs. newer spectra
#define kk    1.0f // max allowed deviation of prediction from precious values

void CConcealment::concealByPrediction()
{
  prediction() ;

  /* adapt energy */

  adaptEnergy() ;
}

void CConcealment::prediction()
{
  int i ;
  int line ;
  int linesActive ;
  int finalLinesActive = 0;

  float weight = (float)pow(1.0f/alpha,sizeofHistory-2) ;
  if (!lastConcealed)
  {
    zeroFloat(g0,  CONCEALLIMIT) ;
    zeroFloat(g0p, CONCEALLIMIT) ;
    zeroFloat(g1,  CONCEALLIMIT) ;
    zeroFloat(g1p, CONCEALLIMIT) ;
    zeroFloat(g2,  CONCEALLIMIT) ;
    zeroFloat(ma,  CONCEALLIMIT) ;

    const float* spec0, *spec1, *spec2 ;

    spec1 = spectrum(-sizeofHistory) ;
    spec2 = spectrum(-sizeofHistory+1) ;

    linesActive = max(nLinesActive(-sizeofHistory),nLinesActive(-sizeofHistory+1));
    for (line = 0; line < linesActive ; line++)
    {
      ma[line] = (fabs(spec1[line]) >= fabs(spec2[line])) ?
           (float)fabs(spec1[line]) : (float)fabs(spec2[line]) ;
    }

    for (i = -sizeofHistory+2; i < 0; i++)
    {
      spec0 = spec1;
      spec1 = spec2;
      spec2 = spectrum(i) ;

      if (spectrumPresent(i))
      {
        linesActive = max(nLinesActive(i),nLinesActive(i-1));
        linesActive = max(linesActive,nLinesActive(i-2));

        if (linesActive > finalLinesActive)
          finalLinesActive = linesActive ;

        for (line = 0; line < linesActive ; line++)
        {
          float s0 = spec0[line] ;
          float s1 = spec1[line] ;
          float s2 = spec2[line] ;

          if ((float)fabs(s2) > ma[line]) ma[line] = (float)fabs(s2) ;

          g0[line]  += s1 * s1 * weight;
          g0p[line] += s0 * s0 * weight;
          g1[line]  += s0 * s1 * weight;
          g1p[line] += s1 * s2 * weight;
          g2[line]  += s0 * s2 * weight;
        } // for (line)
      } // if spectrumPresent
      weight *= alpha ;
    } // for (i over history)

    // now calculate the prediction coefficients
    calculateCoefficients(finalLinesActive,g0,g0p,g1,g1p,g2,a0,a1);
  }
  else
  {
    finalLinesActive = nLinesActive(-1) ;
  }

  predict(finalLinesActive, spectrum(-2),spectrum(-1),a0,a1,ma,spectrum(0)) ;

  if (!lastConcealed)
  {
    // take the future into account!
    weight = 1.0f; // make it important!
    if (spectrumPresent(1))
    {
      const float* spec0, *spec1, *spec2 ;

      spec0 = spectrum(-1);
      spec1 = spectrum(0);
      spec2 = spectrum(1);
      linesActive = max(nLinesActive(1),nLinesActive(0));
      linesActive = max(linesActive,nLinesActive(-1));

      if (linesActive > finalLinesActive)
        finalLinesActive = linesActive ;

      for (line = 0; line < linesActive ; line++)
      {
        float s0 = spec0[line] ;
        float s1 = spec1[line] ;
        float s2 = spec2[line] ;

        g0[line]  += s1 * s1 * weight;
        g0p[line] += s0 * s0 * weight;
        g1[line]  += s0 * s1 * weight;
        g1p[line] += s1 * s2 * weight;
        g2[line]  += s0 * s2 * weight;

        if (fabs(s2) > ma[line]) ma[line] = (float)fabs(s2) ;
      } // for (line)
    } // if spectrumPresent

    // calculate the prediction coefficients again
    calculateCoefficients(finalLinesActive,g0,g0p,g1,g1p,g2,a0,a1);

    // the actual prediction, redone
    predict(finalLinesActive,spectrum(-2),spectrum(-1),a0,a1,ma,spectrum(0)) ;
  }

  // clear spectrum above highest predicted line
  zeroFloat(spectrum(0) + finalLinesActive, m_nLinesMax - finalLinesActive) ;

  nLinesActive(0) = finalLinesActive ;
}

/*
  tile the spectrum into equally-sized bands, adapting the energy
  within to be equal to either
  - the energy in the last frame
  - geometric mean of last and future frame energies if available
 */

void CConcealment::adaptEnergy()
{
  enum
  {
    nBands = 100
  };
  int bWidth = m_nLinesMax / nBands;
  int band ;

  for (band = 0 ; band < nBands ; band++)
  {
    float nrgm1 ;
    float nrg0  ;
    float nrgp1 ;

    int j, jstart = band * bWidth, jstop = min(m_nLinesMax, jstart + bWidth) ;
    float nrg;

    /* energy per band of last spectrum */
    nrg = 0.0f ;
    for (j = jstart ; j < jstop ; j++)
    {
      nrg += spectrum(-1)[j]*spectrum(-1)[j] ;
    }
    nrgm1 = nrg ;
    /* energy per band of current (concealed/predicted) spectrum */
    nrg = 0.0f ;
    for (j = jstart ; j < jstop ; j++)
    {
      nrg += spectrum(0)[j]*spectrum(0)[j] ;
    }
    nrg0 = nrg ;

    /* energy per band of next spectrum, if present */
    int future = spectrumPresent(1) ? 1 : (spectrumPresent(2) ? 2 : 0) ;
    if (future)
    {
      nrg = 0.0f ;
      for (j = jstart ; j < jstop ; j++)
      {
        nrg += spectrum(future)[j]*spectrum(future)[j] ;
      }
      nrgp1 = nrg ;
    }
    else
    {
      nrgp1 = nrgm1 ;
    }

    /* adapt spectral energy. If we have two spectra, we adapt to the geometric
       mean of signal energies. If there is just one (the last), we slowly fade
       out
    */

    if (nrg0 > FLT_MIN)
      /* if energy is zero already, don't adjust energy */
    {
      double templ  = pow(nrgm1, ((float)future)/(future+1)) * pow(nrgp1, ((float)1.0)/(future+1)) ;
      double factor = sqrt(templ / nrg0) ;

      if (factor > 10.0) factor = 10.0 ;

      for (j = jstart ; j < jstop ; j++)
      {
        spectrum(0)[j] *= (float)factor ;
      }
    }
  }
}

/*
  Given prediction coefficients, do the actual prediction.
 */

void CConcealment::predict(/* input */
                           int          nLines,
                           const float *spec0,
                           const float *spec1,
                           const float *a0,
                           const float *a1,
                           const float *ma,
                           /* output */
                           float       *result)
{
  int line ;

  for (line = 0 ; line < nLines ; line++)
  {
    float predict = spec0[line]*a1[line] + spec1[line]*a0[line] ;

    /* sanity check on prediction: Do not let predicted value stray from interval
      spanned by past mdct values */
    if (fabs(predict) > kk*ma[line]) predict = (float)_copysign(kk*ma[line], predict) ;

    result[line] = predict ;
  }
}

/*
  derive prediction coefficients from intermediate calculated values
 */
void CConcealment::calculateCoefficients(/* input */
                                         int          nLines,
                                         const float *g0,
                                         const float *g0p,
                                         const float *g1,
                                         const float *g1p,
                                         const float *g2,
                                         /* output */
                                         float       *a0,
                                         float       *a1)
{
  int line ;
  for (line = 0; line < nLines ; line++)
  {
    float den = (g0[line] *g0p[line] - g1[line]*g1[line] ) ;
    float _a0 = (g1p[line]*g0p[line] - g2[line]*g1[line] ) ;
    float _a1 = (g2[line] *g0[line]  - g1p[line]*g1[line]) ;

    if (fabs(den) > 1.0f / FLT_MAX)
    {
      den = 1.0f / den ;
      _a0 *= den ;
      _a1 *= den ;
    }
    else
    {
      _a0 = 1.0f; _a1 = 0.0f; // repetition if no estimate possible
    }

    // limit the magnitude of the pole(s) to less than 2
    poleLimitMagnitude(2.0f,&_a0,&_a1);

    a0[line] = _a0 ; a1[line] = _a1 ; // update predictors
  }
}

/*
  limit the magnitudes of the two poles of the error concealment
  prediction filter to make it stable.
  Only works for two-pole filters.
 */

void CConcealment::poleLimitMagnitude(float mag, float *a0, float *a1)
{
  if (fabs(*a1) > mag)
  {
    *a0 /= (float)sqrt(fabs(*a1/mag));
    *a1  = -mag ;
  }
}

/*
  clear (set to zero) a float vector
 */
void CConcealment::zeroFloat(float *f, int n)
{
  int i ;
  for (i = 0 ; i < n ; i++) f[i] = 0.0f ;
}

#ifdef _CARBON
#pragma old_argmatch reset
#endif
