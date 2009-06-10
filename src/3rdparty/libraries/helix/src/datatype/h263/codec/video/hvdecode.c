/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hvdecode.c,v 1.5 2007/07/06 22:00:33 jfinnecy Exp $
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

/*  Header File Includes        */
#include "machine.h"
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "hlxclib/malloc.h"

#include "dllindex.h"
#include "h261func.h"
#include "h261defs.h"
#include "hvscodes.h"

/*  Defines, Typedefs, & Enums  */
//#define DEBUG_VVDECODE(a) a
#define DEBUG_VVDECODE(a)

/*  Globals                     */
extern H261Decoder *DecoderRegistry[MAX_NUM_DECODERS];

/*  File-wide Statics           */
/******************************************************************************
* clearChromaFrame (PICTURE *newIn)
*
*
*/
static void clearChromaComponent (COMPONENT *c)
{
   PIXEL    *p;
   S32      nRows, nCols;
    
    /* clear the chroma image component */  
    p = c->ptr;
    nRows = c->nvert;  
    while (nRows--) {
        nCols = c->nhor;
        while (nCols--) {
         *p++ = 0x80;
        }
        p += (c->hoffset - c->nhor);
    }
}

static void clearChromaFrame ( PICTURE *newIn)
{
    /* clear both image components  */
    clearChromaComponent (&newIn->cr);
    clearChromaComponent (&newIn->cb);

    return;
}
/*-----------------------------------------------------------------------------
 *  Function:   VvDecode
 *
 *  DESCRIPTION
 *      Starts bitstream decoding.
 *
 *  CALLING SYNTAX
 *      VvDecode    S16 index
 *                  U32 PBframeCap
 *                  U32 picdesc
 *                  U32 bsStart
 *                  U32 bsEnd
 *                  U32 nextGOB
 *                  U32 newPic
 *                  U32 status
 *                  U32 newBS
 *
 *      index:      Indicates which decoder is being called.
 *      PBframeCap  If 0: no B-frame reconstruction; otherwise, reconstruct B-frame
 *      picdesc:    Pointer to a PICTURE DESCR struct used for the decoded picture layer info
 *      bsStart:    Offset into bistream indicating where to begin decoding:
 *                      high word indicates offset to a byte,lo word indicates bit number.
 *      bsEnd:      Offset into bistream indicating where to stop decoding (worst case):
 *                      high word indicates offset to a byte,lo word indicates bit number.
 *      nextGOB:    Pointer to S16 indicating which GOB to decode next. If zero, the routine
 *                  skips all GOBs until it finds a PSC (Picture Start Code ).
 *      newPic:     Pointer to a PICTURE struct used for the decoded image
 *      status:     Pointer to an array containing status info
 *      newBs:      Pointer to offset into bitstream indicating where the decoder actually stopped decoding 
 *                      high word indicates offset to a byte,lo word indicates bit number.
 * 
 *  RETURNS
 *      Returns type U32 containing status; YES = completed decoding an entire image
 *
 *  Author:     Mary Deshon     7/21/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/

#define NUM_STATUS_WORDS    4


extern U32 VvDecode ( S16 index, U32 PBframeCap, PICTURE_DESCR* picdesc,
                          U32 bsStart, U32 bsEnd, S16* nextGOB, PICTURE* newPic, 
                          VvDecoderStats* status, U32 newBs )
{
    H261Decoder     *dec;
    PICTURE_DESCR   *far32_picdesc;
    PICTURE         *far32_newPic;
    PICTURE         temp_pic;
    U32             *far32_newBs;
    VvDecoderStats  *far32_status;
    S16             *far32_nextGOB;
    int             retval, oldFormat, numMBs;
    DEBUG_VVDECODE  (char   buf[128];) /* Flawfinder: ignore */
    static int      StaticOldColor=1;
    int             resetChroma = 0;

    dec = DecoderRegistry[index-1];

    // Get pointers we can use
    far32_picdesc   =  picdesc; 
    far32_status    =  status;  
    far32_nextGOB   =  nextGOB; 
    far32_newPic    =  newPic;  
    far32_newBs     =  (void*) newBs;

    if ( !far32_picdesc->decodingInProgress ) {
        far32_picdesc->rows = ((far32_picdesc->rows + 15)>>4)<<4;
        far32_picdesc->cols = ((far32_picdesc->cols + 15)>>4)<<4;
#ifdef BIG_ENDIAN	// Mac
        // Setup H261Decoder structure
        dec->bsStart.byteptr = dec->bsAbs.byteptr + (bsStart & 0xffff) ;
        dec->bsStart.bitptr =  (bsStart >> 16) & 0xff;
        dec->bsEnd.byteptr = dec->bsAbs.byteptr + (bsEnd & 0xffff) ;
        dec->bsEnd.bitptr =  (bsEnd >> 16) & 0xff;
#else
        // Setup H261Decoder structure
        dec->bsStart.byteptr = dec->bsAbs.byteptr + (bsStart >> 16) ;
        dec->bsStart.bitptr =  bsStart & 0xff ;
        dec->bsEnd.byteptr = dec->bsAbs.byteptr + (bsEnd >> 16) ;
        dec->bsEnd.bitptr =  bsEnd & 0xff ;
#endif
        // Set nextGOB
        dec->next_gob = *far32_nextGOB;
        // Set picture size (if ANYSIZE), coding method, and PB capability
        dec->pic_layer.rows = far32_picdesc->rows;
        dec->pic_layer.cols = far32_picdesc->cols;
        dec->pic_layer.decodingInProgress = FALSE;
//#define VTEL_M261
#ifdef VTEL_M261
        dec->codingMethod   = H261_CODING;
#else
        dec->codingMethod   = H263_CODING; 
#endif
        dec->PBframeCap     = PBframeCap;    // Enable B-frame reconstruction
        //dec->maxComp  = 0;    // No time-out
        dec->maxComp  = 396;    // Set time-out limit
        // Set oldFormat
        oldFormat = dec->decMB.format;
        // Are arrays big enough?
        numMBs = ((dec->pic_layer.rows + 15) >> 4) *
                 ((dec->pic_layer.cols + 15) >> 4);
        if (numMBs > dec->maxMbnum) {
            // Reallocate arrays
            FreeVarStructsDecoder( dec );
            retval = AllocVarStructsDecoder( dec, (short)numMBs );
            if (retval)  return NO;
        }
        // Initialize structures according to color parameter passed in.
        dec->newOut.color = far32_newPic->color;
        dec->oldOut.color = far32_newPic->color;
        dec->B_Out.color  = far32_newPic->color;
    }
    // Call Decoder
    retval = H261Decode( dec );
    
    // Check if we are done
    far32_picdesc->decodingInProgress = dec->pic_layer.decodingInProgress;
    if ( dec->pic_layer.decodingInProgress )
        return 0;   // Not done yet

    // Copy to return areas
    memcpy ( far32_picdesc, (void *)&dec->pic_layer, sizeof ( PICTURE_DESCR ) ); /* Flawfinder: ignore */
    memcpy ( far32_status, (void *)&dec->status, sizeof (VvDecoderStats) ); /* Flawfinder: ignore */
    memcpy ( far32_nextGOB, (void *)&dec->next_gob, sizeof ( S32 ) ); /* Flawfinder: ignore */

    // Return updated bitstream
#ifdef BIG_ENDIAN	// e.g. Mac
    *far32_newBs = (( dec->newBs.byteptr - dec->bsAbs.byteptr ) && 0xffff) |
                    ( (dec->newBs.bitptr << 16) & 0xff );
#else
    *far32_newBs = (( dec->newBs.byteptr - dec->bsAbs.byteptr ) << 16) |
                    ( dec->newBs.bitptr & 0xff );
#endif

    /* check to see if chroma needs to be cleared  */
    if ( dec->newOut.color == FALSE && StaticOldColor == TRUE )
        resetChroma = 1;
    StaticOldColor = dec->newOut.color;
//    if ( ( oldFormat == CIF && dec->formatCap == QCIF ) && ( dec->newOut.color == FALSE ) )
    if ( (oldFormat == CIF && dec->decMB.format == QCIF) && (dec->newOut.color == FALSE) )
        resetChroma = 1;
    /* reset the chroma image components if necessary   */
    if ( resetChroma ) {
        clearChromaFrame (&dec->newOut);
        clearChromaFrame (&dec->B_Out);
        clearChromaFrame (&dec->oldOut);
        clearChromaFrame (&dec->prevOldOut);
    }        

    // If retval is YES, then the decoder has completed decoding of an entire frame.
    // The new frame becomes the old.  Aliases are set up for return.
    // For PB-frames, return the B-frame first, and the P-frame on next call.
    // Doublebuffered output: prevOldOut holds previous output frame
    if ( retval == YES ) {
        temp_pic = dec->prevOldOut;         // Reuse oldest frame
        if ( dec->PBframeCap  &&  dec->pic_layer.PBframeMode ) {
            // We have reconstructed B-frame and P-frame
            dec->prevOldOut = dec->B_Out;   // Will become PrevOut after next call
            dec->B_Out      = temp_pic;     // Reuse oldest frame
            temp_pic        = dec->oldOut;
            dec->oldOut     = dec->newOut;  // New P-frame becomes old
            dec->newOut     = temp_pic;     // Reuse prev. output (after it's been displayed)
            dec->pPrevPic = &dec->newOut;       // Old P-frame (reuse after display)
            dec->pNewPic  = &dec->prevOldOut;   // Return B-frame
            dec->pendingFrame = 1;              // Hold on to P-frame
        } else {
            dec->prevOldOut = dec->oldOut;  // Previous output frame
            dec->oldOut     = dec->newOut;  // New P-frame becomes old
            dec->newOut     = temp_pic;     // Reuse oldest frame
            dec->pPrevPic   = &dec->prevOldOut; // Previous output frame
            dec->pNewPic    = &dec->oldOut;     // Output frame
            dec->pendingFrame = 0;              // No frame left awaiting output
        }
        *far32_newPic  = *dec->pNewPic;
        //*far32_prevPic = *dec->pPrevPic;
    } else if ( dec->pendingFrame ) {
        // P-frame is intact even if next frame is being decoded
        dec->pPrevPic  = dec->pNewPic;      // B-frame was previous output
        dec->pNewPic   = &dec->oldOut;      // Return P-frame
        *far32_newPic  = *dec->pNewPic;
        //*far32_prevPic = *dec->pPrevPic;
        dec->pendingFrame = 0;          // No frame left awaiting output
        retval = YES;                   // We have a frame to return
    }
    // Report to higher layer whether we are returning a B-frame
    far32_status->dwBframe = dec->pendingFrame;

    /*if ( retval == YES ) {

          // Exchange pointers to newOut and oldOut
        temp_pic    = dec->oldOut;
        dec->oldOut  = dec->newOut; // This is the output P (or I) frame
        dec->newOut  = temp_pic;
        if ( dec->PBframeCap  &&  dec->pic_layer.PBframeMode ) {
            // We have reconstructed a B-frame
            *far32_newPic = dec->B_Out; // Return B-frame
            dec->pendingFrame = 1;      // Hold on to P-frame
        } else {
            *far32_newPic = dec->oldOut;    // Return P (or I) frame
            dec->pendingFrame = 0;          // No frame left awaiting output
        }
    } else if ( dec->pendingFrame ) {
        // P-frame is intact even if next frame is being decoded
        *far32_newPic = dec->oldOut;    // Return P (or I) frame
        dec->pendingFrame = 0;          // No frame left awaiting output
        retval = YES;                   // We have a frame to return
    }*/

    // Return status; YES = completed decoding a picture, otherwise N0.
    return ( (U32) retval );
}

/*-----------------------------------------------------------------------------
 *  Function:   VvDecGetMBMap
 *
 *  DESCRIPTION
 *      Get decoder macroblock map.
 *
 *  CALLING SYNTAX
 *      VvDecGetMBMap   S16 index
 *                      U32 mapType
 *                      U32 outMap
 *
 *      index:      Input. Indicates which decoder is being called.
 *      mapType:    Input. Indicates which map to ouput.
 *      outMap:     Output. 16:16 pointer to output map.
 * 
 *  Author:         Mary Deshon             02/23/94
 *  Inspected:      <<not inspected yet>>
 *  Revised:
 -----------------------------------------------------------------------------*/
extern U32 VvDecGetMBMap ( S16 index, U32 mapType, U32 outMap )
{
    H261Decoder *dec;
    U16			*fpOutMap;
/*    int   mapHorz = 11;
    int mapVert = 9;*/
  
    dec = DecoderRegistry[index-1];
   
    // Get a pointer we can work with
    fpOutMap = (U16 *)(void *)outMap;
  
/*  if (dec->decMB.format == CIF) { 
        mapHorz = 22;
        mapVert = 18;
    }*/
    
    switch ( mapType ) {
    case DECODER_MAP:
        /* Copy map to pointed at area */
        memcpy ( (void *)fpOutMap, (void *)&dec->decMB, sizeof (MBMap)); /* Flawfinder: ignore */
        break;
    default:
        break;
    }
    return ( OK );    

}
