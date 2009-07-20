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

#ifndef __CONCEALMENT_H__
#define __CONCEALMENT_H__

#include "hlxclib/assert.h"

/*
 * class CConcealment keeps a history of spectral lines and tries
 * to conceal missing spectra.
 * Instantiate one per channel.
 */

class CConcealment
{
public:
  // instantiate a spectrum concealer. nLinesMax is the maximum number
  // of lines that we'll ever take or hand out.
  CConcealment(int nLinesMax) ;
  ~CConcealment() ;

  // insert spectrum into our delay line. Currently, blocktype is of
  // Layer-3 flavor (2 == short block); this will be generalized.
  // nLines is the highest active spectral line in this block
  // (i.e. the audio bandwidth)
  void insert(const float *spec, int blocktype, int nLines = -1) ;

  // no spectrum present, schedule this spectrum to be concealed.
  // call this if you have a defect, or missing, spectrum.
  void insert() ;

  // retrieve a valid spectrum, concealed if necessary
  // returns index+1 of highest spectral line containing energy
  // and Layer-3 type blocktype.
  int retrieve(float *s, int &blocktype) ;

private:
  enum
  {
    CONCEAL_REPETITION    = 0,
    CONCEAL_INTERPOLATION = 1,
    CONCEAL_PREDICTION    = 2
  };

  //
  // the most likely candidates for concealment. See source code
  // for documentation
  //

  void conceal(int method) ;
  void concealByPrediction() ;
  void adaptEnergy() ;
  void prediction() ;
  void predict(int          nLines,
               const float *spec0,
               const float *spec1,
               const float *a0,
               const float *a1,
               const float *ma,
               float       *current);
  void calculateCoefficients(/* input */
                             int          nLines,
                             const float *g0,
                             const float *g0p,
                             const float *g1,
                             const float *g1p,
                             const float *g2,
                             /* output */
                             float       *a0,
                             float       *a1);

  void poleLimitMagnitude(float mag, float *a0, float *a1) ;
  void rotateHistory() ;
  void zeroFloat(float *f, int n) ;

  //
  // inline helper functions to make access to history buffer
  // transparent
  //

  unsigned char& spectrumPresent(int time)
  {
    return m_historyPtr[time + sizeofHistory]->spectrumPresent;
  }

  int& blockType(int time)
  {
    return m_historyPtr[time + sizeofHistory]->blockType;
  }

  int& nLinesActive(int time)
  {
    return m_historyPtr[time + sizeofHistory]->nLinesActive;
  }

  float* spectrum(int time)
  {
    assert(sizeofHistory+time >= 0);
    assert(sizeofHistory+time < sizeofBuffer);
    return m_historyPtr[time + sizeofHistory]->spectrum;
  }

  //
  // member variables ...
  //

  enum
  {
    sizeofBuffer = 8,
    sizeofFuture = 2,
    sizeofHistory= sizeofBuffer-sizeofFuture-1,
    CONCEALLIMIT = 576 // can handle at most 576 lines.
  };

  int m_offset ;
  int m_nLinesMax ;

  struct
  {
    float * spectrum ;
    int     nLinesActive ;
    unsigned char    spectrumPresent ;
    int     blockType ;
  } m_history[sizeofBuffer], *m_historyPtr[sizeofBuffer] ;

  unsigned char lastConcealed ;

  // last frame's predictor coefficients

  float a0[CONCEALLIMIT] ;
  float a1[CONCEALLIMIT] ;
  float ma[CONCEALLIMIT] ;

  float g0[CONCEALLIMIT], g0p[CONCEALLIMIT];
  float g1[CONCEALLIMIT], g1p[CONCEALLIMIT];
  float g2[CONCEALLIMIT];

};

#endif /* ifdef __CONCEALMENT_H__ */
