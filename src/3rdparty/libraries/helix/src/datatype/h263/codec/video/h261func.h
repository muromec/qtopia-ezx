/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: h261func.h,v 1.4 2007/07/06 22:00:33 jfinnecy Exp $
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

#ifndef _INC_H261FUNC
#define _INC_H261FUNC   1 

//  dct.c
extern void Dct2( PIXEL x[], int xdim, S16 y[8][8] );
extern void Dct2Diff( PIXEL x[], int xdim, PIXEL pred[], S16 y[64] );
extern void InitDctTables ( void );
extern void Dct8x8ssDiff( PIXEL x[], int xdim, PIXEL pred[], S16 y[8*8],
                          int hSize,    // 16: normal DCT; otherwise: mirror input horizontally
                          int vSize     // 16: normal DCT; otherwise: mirror input vertically
                          );

//  dct4x4.c
extern void Dct4x4( PIXEL x[], int xdim, S16 y[4][4]);
extern void Dct4x4Diff( PIXEL x[], int xdim, PIXEL pred[], S16 y[4][4]);

//  h261dec.c
extern int  H261Decode( H261Decoder * s );
extern int  BsDiff( BS_PTR bs1, BS_PTR bs2 );   // Compute # bits in segment bs1 - bs2
extern void InitFindSC( void );
extern int  checksym( SYMBOL sym, int type, char routine[] );

//  h261enc.c
extern int H261Encode( H261Encoder * s, H261PicDescriptor d, TIME32 time0[], S32 DCTStats[] );
extern int H261Recon( H261Encoder * s, H261PicDescriptor d );

//  h261err.c
extern void H261ErrMsg( char s[] );

//  h263dec.c
extern int  DecPicLayer263( BS_PTR * bs, int nbits, PICTURE_DESCR * pic,
                            GOB_DESCR * gob, int * decPtype );
extern void InitMvTabs( int trD, int trB,
                        int tabMvF[], int tabMvB[]  // [UMV_MIN:UMV_MAX]
                        );
extern int  DecGobLayer263( BS_PTR * bs, int nbits, GOB_DESCR * gob, int * gfid );
extern int  DecMbLayer263(  BS_PTR * bs,    // Bitstream pointer
                            int nbits,      // Bits to decode (incl. trailing startcode)
                            GOB_DESCR * gob,        // GOB descriptor
                            MACROBLOCK_DESCR mb[],  // Packed array of "gob->num_mb" MB descr.
                            int interFrame, // 0: ptype=INTRA, otherwise ptype=INTER
                            int PBframe,    // 0: not PB frame, otherwise PB frame
                            int unrestrictedMv, // 0: -16/+15.5 motion, otherwise +/- 31.5
                            int advancedIntraMode, // 0 off else on
                            SYMBOL sym[],   // symbol array
                            int maxsym      // size of symbol array
                            );
extern void GetMvBframe( MACROBLOCK_DESCR *mb, int unrestrictedMv,
                         int tabMvF[], int tabMvB[]  // [UMV_MIN:UMV_MAX]
                         );

//  idct.c
extern void InitReconTables( void );
extern void Idct2( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[], int clean );
extern void Idct2Sum( SYMBOL sym[], int nsym, PIXEL x[], int xdim, S16 recon[], int clean );
extern void Idct2_s16( int intra, SYMBOL sym[], int nsym, S16 x[], int xdim, S16 recon[] );

//  intercod.c
extern U8  ReconStep[];
extern int InterCode( PICTURE *pic, PICTURE *pred, MACROBLOCK_DESCR * mb, int masking,
                        SYMBOL sym[] );
extern int IntraCode( PICTURE *pic, MACROBLOCK_DESCR * mb, SYMBOL sym[], long mbDiff, int codingMethod );
extern void InitQuantTables( void );
extern int Code32x32( PICTURE *pic, PICTURE *pred, MACROBLOCK_DESCR * mb, int masking,
                        SYMBOL sym[], int intraFlag );


//  predsel.c
extern void InitPred( void );
extern int PredSelect( H261PicDescriptor *d, PICTURE *oldOut, PICTURE *newIn,
                       int mbhor, int mbvert, MACROBLOCK_DESCR mb[], GOB_DESCR gob[],
                       PICTURE *pred, int dct_thresh, H261Stats *stats,
                       S32 weight_1, S32 weight_2, S32 sendGobHeaders );
extern int var16( PIXEL pic[], int xdim, int phase, long var[4] );
extern int err16( PIXEL new[], PIXEL old[], int xdim, int phase, int err[4] );
extern int  LoopFilter( MACROBLOCK_DESCR *mb, PICTURE *prev_pic, PICTURE *pic );

// mcomp.c
extern int  MotionComp( MACROBLOCK_DESCR *mb, PICTURE *prev_pic, PICTURE *pic );
extern void MotionComp263( MACROBLOCK_DESCR * mb, // Describes block to be motion-compensated
                            PICTURE * prevPic,  // Describes previous picture used to form MC
                            PICTURE * pic       // Output picture where MC block is placed
                            );
extern void OverlapMC( MACROBLOCK_DESCR * mb,   // Describes block to be motion-compensated
                        int     PBframe,    // Non-zero if PB frame
                        PICTURE * prevPic,  // Describes previous picture used to form MC
                        PICTURE * pic,      // Output picture where MC block is placed
                        int     mbWidth,    // Macroblocks per row
                        int     mbOffset,   // Row offset; (mb-mbOffset) is neighbor on top
                        int     overlap[4]  // Returns YES or NO to indicate whether overlap
                                            // was done in each 8x8 subblock
                        );
extern int PointingOutside( int col1, int col2, // First and last column of block
                            int row1, int row2, // First and last row of block
                            int mvX, int mvY,   // Motion vector; one fractional bit
                            int nCols, int nRows    // Picture size
                            );
extern void PredBframe( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        );
extern void PredBdist( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        );

//  mest.c
extern int  MotionEst( PICTURE *new_in, PICTURE *old_in, int mbhor, int mbvert,
					   MACROBLOCK_DESCR Mb[], int unrestrictedMv, int advancedPred, S32 *mvSum );
extern int  RefineMotionVector( MACROBLOCK_DESCR *mb,   // MB descriptor; contains initial MV
                                PIXEL new[],    // Upper left pixel of new 16x16 block
                                PIXEL old[],    // Old block (no motion comp)
                                int xdim,       // Line offset for "new" and "old"
                                int phase,      // Phase used for computing "mc_err"
                                int mc_err,     // Error for initial motion vector
                                int err[4],     // Error for each 8x8 block; submit values for
                                                // initial mv; returns values for chosen mv
                                int horPredOnly );   // 1-D prediction for mv?
extern int  TryInter4V( MACROBLOCK_DESCR mb[],  // MB descriptor array; mb[0] is current mb;
                                                // use surrounding mb's to get mv candidates
                                                // Note: "past" mv's have a fractional bit
                        PIXEL new[],    // Upper left pixel of new 16x16 block
                        PIXEL old[],    // Old block (no motion comp)
                        int xdim,       // Line offset for "new" and "old"
                        int phase,      // Phase used for computing "mc_err"
                        int mc_err,     // Error for initial motion vector
                        int errMc[4],   // Error for each 8x8 block with INTER mv (undefined
                                        //  if mv=0); returns values for chosen mv/mv's
                        int err[4],     // Error for each 8x8 block with mv=0
                                        //  We are assuming that sum(err) > sum(errMc)
                        int sqQuant     // QUANT^2; needed for INTER4V decision
                        );
extern int UmvErr( PIXEL new[], PIXEL old[], int xdim, int phase, int errvec[4],
                   int size, int col, int row, int mvX, int mvY );
extern int RefineMvReducedResMode( MACROBLOCK_DESCR *mb,// MB descriptor; contains initial MV
                                   int blk,     // Subblock (0,1,2,or 3)
                                   PIXEL new[], // Upper left pixel of new 16x16 or 8x8 block
                                   PIXEL old[], // Old block (no motion comp)
                                   int xdim,    // Line offset for "new" and "old"
                                   int phase,   // Phase used for computing "mc_err"
                                   int col, int row // (x,y) position for upper left corner
                                   );

//  enc
extern void MvPred( MACROBLOCK_DESCR mb[],
                    int blk,        // specify block: UL, UR, LL, LR, or WHOLE 
                    int mbhor,      // offset from previous row of MBs
                    int horPredOnly, 
                    int *mvX, int *mvY );

//  vld.c
extern U8 Get8Bits( BS_PTR bs );
extern void IncBsPtr( BS_PTR * bs, int incr );
extern int FLDecSymbol( BS_PTR * bs,        // Bitstream pointer; incremented by this routine
                        int bits,           // Symbol length
                        int * numBits,      // Max # bits to decode; decremented by this routine
                        int * value         // Returns decoded symbol
                        );
extern int VLDecSymbol( BS_PTR * bs,        // Bitstream pointer; incremented by this routine
                        int tabNum,         // Use dectable[tabNum] for decoding
                        int * numBits,      // Max # bits to decode; decremented by this routine
                        SYMBOL * sym        // Returns decoded symbol
                        );
extern void InitDecodeTable( void );
//  quant2.c
extern int QuantSelect ( S32 codedBlkCnt, H261CodingControl * cntrl, int num_mb,
                         MACROBLOCK_DESCR mb[], MbInfo mbStats[], long *logSum, S32 color,
                         int reducedResUpdate );
extern void InitLogTab ( void );

                      
// getImgParms().
extern S16 getImgParms( short format, short *numGOBS, short *numMBs, long *imgSize,
            long *lumaSize, long *chromaLineLength, long *chromaRows, long *maxsym);


// vdecopcl.c
extern int AllocVarStructsDecoder( H261Decoder *dec, short formatCap_or_numMBs );
extern void FreeVarStructsDecoder( H261Decoder *dec );

// recongob.c
extern int ReconGob( GOB_DESCR * gob, MACROBLOCK_DESCR mb[],
                     PICTURE * prev_pic, PICTURE * pic, PICTURE * Bpic, 
                     int advancedPred, int PBframe, int PBframeCap, int reducedResUpdate,
                     int advancedIntraMode, H261DecState *state, int maxComp );
extern int ConcealGob( GOB_DESCR * gob, MACROBLOCK_DESCR mb[], int reducedResUpdate,
                       PICTURE * prev_pic, PICTURE * pic );
extern void ReconIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean);
extern void ReconAdvancedIntra( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean);
extern void ReconInter( MACROBLOCK_DESCR * mb, PICTURE * pic, int clean );
extern S8 ReducedResMvComponent( S8 x );
extern void MotionComp32x32( MACROBLOCK_DESCR * mb, // Describes block to be motion-compensated
                            PICTURE * prevPic,  // Describes previous picture used to form MC
                            PICTURE * pic       // Output picture where MC block is placed
                            );
extern void Overlap32x32( MACROBLOCK_DESCR * mb,   // Describes block to be motion-compensated
                        PICTURE * prevPic,  // Describes previous picture used to form MC
                        PICTURE * pic,      // Output picture where MC block is placed
                        int     mbWidth,    // Macroblocks per row
                        int     mbOffset,   // Row offset; (mb-mbOffset) is neighbor on top
                        int     overlap[4]  // Returns YES or NO to indicate whether overlap
                                            // was done in each 8x8 subblock
                        );
extern void Fill32x32( MACROBLOCK_DESCR * mb, PICTURE * pic, PIXEL value );
extern void ReconReducedResMb( MACROBLOCK_DESCR * mb,   // Macroblock to be reconstructed
                               PICTURE * pic,   // Input: motioncomp. prediction;
                                                // output: reconstr. picture
                               int intra,       // INTER block if zero, otherwise INTRA
                               PICTURE * tempPic// Use for temporary storage
                               );

//  Routines used for debugging
extern void printsym( SYMBOL sym );
extern void sprintsym( SYMBOL sym, char s[], int sBufLen );
extern void sprinttype( int symtype, char s[] , int sBufLen);
extern void printstate( int state );

#endif
