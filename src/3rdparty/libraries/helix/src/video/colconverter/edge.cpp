/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: edge.cpp,v 1.4 2007/07/06 20:53:51 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxcom.h"
#include "edge.h"

#include "hlxclib/math.h"

// Lookup tables to perform luma image enhancement
int soft_core_2d[LUT_LENGTH];
int clip_softcore_post[CLIP_TABLE_LENGTH];
int soft_core_2d88[LUT_LENGTH];

#define lut_soft_core_2d (soft_core_2d+OFFSET_SOFTCORE)
#define lut_soft_core_2d88 (soft_core_2d88+OFFSET_SOFTCORE)
#define lut_clip_softcore_post (clip_softcore_post+OFFSET_CLIP_SOFTCORE)

float g_fCurrentSharpness = 0.0;
INT16 g_nCurrentExpand = 0 ;

void HXEXPORT
ENTRYPOINT(SetSharpnessAdjustments)(float Sharpness, INT16 nExpand)
{

    //Initialize edge enhancemnt lookup table
    //adjust the Sharpness parameter input range [-1,1] to output range [0,MAX_B]
    g_fCurrentSharpness = Sharpness;
    g_nCurrentExpand = nExpand;

    Sharpness += 1.0;
    
    if(nExpand)
    {
	Sharpness *= (ZOOM_BOOST*MAX_B/2.0);
    }
    else
    {
	Sharpness *= (MAX_B/2.0);
    }

    Inittriangleluts(DEFT_C,DEFT_A,Sharpness);
   

}

void HXEXPORT
ENTRYPOINT(GetSharpnessAdjustments)(float* pSharpness, INT16* pnExpand)
{
    *pSharpness = g_fCurrentSharpness;
    *pnExpand	= g_nCurrentExpand;
}

void HXEXPORT
ENTRYPOINT(Enhance)(UCHAR *yuv420in, INT32 inheight, INT32  inwidth, INT32 pitchSrc, float Sharpness)
{
    int     row, curBuf, prevBuf;
    unsigned char  *pLuma;
    static int  helper[2][2][HELPER_MAXLEN];    


    static int first_fl=1;
    float tolerance=(float)0.1;
    int tmp1;
	
    if ((inwidth > HELPER_MAXLEN) || (inheight <16))
	{
		return ;
	}

    //if Sharpness is -1.0 (or close) do not do any edge enhancement
    if(((Sharpness+1.0)<tolerance) && ((Sharpness+1.0)>-tolerance))
    {
	return ;
    }

    if(first_fl==1)
    {		
	//Initialize constant edge enhancemnt lookup table to defaults for the first time
	/*Inittriangleluts(DEFT_C,DEFT_A,DEFT_B);*/
	Inittrianglelutsconst();
	//initialize clipping table
	Initcliplut();
	first_fl=0;
	}

    curBuf = 0;         // Point to "current" line buffer
    pLuma = yuv420in;    // Point to beginning of current line

    DiffNonLin2Dconst( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
    DiffNonLin2Dconst( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	

    for (row = 1; row < inheight-9; row=row+8) {

	//(1)
	prevBuf = curBuf;   // Point to previous line buffer
	curBuf ^= 1;        // Add 1 modulo 2
	pLuma +=pitchSrc;   // Advance pointer to next row
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
        DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(2)
	prevBuf = curBuf;   
        curBuf ^= 1;       
	pLuma +=pitchSrc;  
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(3)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;  
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(4)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;   
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(5)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;   
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(6)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//2edge pixels------------------

	//(7)
        prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;
	DiffNonLin2Dconst( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Dconst( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
        Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(8)
        prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;
	DiffNonLin2Dconst( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Dconst( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
        Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    }

    tmp1=(inheight-((inheight>>3)<<3))-1;
    switch(tmp1)
    {
    case 6:
	 //(1)
	prevBuf = curBuf;   
	curBuf ^= 1;        
	pLuma +=pitchSrc;		
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 5:
	//(2)
	prevBuf = curBuf;  
	curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 4:
	//(3)
	prevBuf = curBuf;  
	curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 3:
	//(4)
	prevBuf = curBuf;   
	curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );
    
    case 2:
	//(5)
	prevBuf = curBuf;   
	curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 1:
	//(6)
	prevBuf = curBuf;   
	curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2D( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2D( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 0:
	break;
    }

    return ;
}

void HXEXPORT
ENTRYPOINT(EnhanceUniform)(UCHAR *yuv420in, INT32 inheight, INT32  inwidth, INT32 pitchSrc, float Sharpness)
{
    int     row, curBuf, prevBuf;
    unsigned char  *pLuma;
    static int  helper[2][2][HELPER_MAXLEN];    

    static int first_fl=1;
    float tolerance=(float)0.1;
    int tmp1;
	
    
    if ((inwidth > HELPER_MAXLEN) || (inheight <16))
    {
	return ;	
    }

    //if Sharpness is -1.0 (or close) don not do any edge enhancement
    if(((Sharpness+1.0)<tolerance) && ((Sharpness+1.0)>-tolerance))
    {
	return ;
    }

    if(first_fl==1)
    {		
	//Initialize edge enhancemnt lookup table to defaults for the first time
	Inittriangleluts(DEFT_C,DEFT_A,DEFT_B);
	//initialize clipping table
	Initcliplut();
	first_fl=0;
    }

    curBuf = 0;         // Point to "current" line buffer
    pLuma = yuv420in;    // Point to beginning of current line

    DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
    DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	

    for (row = 1; row < inheight-9; row=row+8) {

	//(1)
	prevBuf = curBuf;   // Point to previous line buffer
        curBuf ^= 1;        // Add 1 modulo 2
	pLuma +=pitchSrc;   // Advance pointer to next row
        DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
        DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );


	//(2)
	prevBuf = curBuf;   
	curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(3)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );


	//(4)
	prevBuf = curBuf;   
        curBuf ^= 1;       
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(5)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(6)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );


	//2edge pixels------------------

	//(7)
        prevBuf = curBuf;  
        curBuf ^= 1;        
	pLuma +=pitchSrc;
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
        Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	//(8)
        prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
        Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

	}

    tmp1=(inheight-((inheight>>3)<<3))-1;
    switch(tmp1)
    {
    case 6:
	//(1)
        prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc;	
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
        DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 5:
	//(2)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 4:
	//(3)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 3:
	//(4)
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 2:
	//(5)
	prevBuf = curBuf;   
        curBuf ^= 1;       
	pLuma +=pitchSrc;
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 1:
	//(6)	
	prevBuf = curBuf;   
        curBuf ^= 1;        
	pLuma +=pitchSrc; 
	DiffNonLin2Duniform( &pLuma[pitchSrc], &pLuma[1], helper[curBuf][0], inwidth-1 );
	DiffNonLin2Duniform( &pLuma[pitchSrc+1], &pLuma[0], helper[curBuf][1], inwidth-1 );
	Add2DHelper( (int *)helper[curBuf], (int *)helper[prevBuf], &pLuma[1], inwidth-2 );

    case 0:
	break;
    }

    return ;
}

int Add2DHelper( int *cur, int *prev, unsigned char *pic, int n )
{
    int i;
    int tmp=n>>1;
    int tmp1=n-((n>>1)<<1);

    if(n<2) 
    {
	return 1;
    }

    for (i = 0; i < n-1; i=i+2)
    {
	pic[i] = clip_softcore_post[OFFSET_CLIP_SOFTCORE + pic[i]
                        + prev[i+1] 
			+ prev[i + HELPER_MAXLEN] 
                        - cur[i] 
			- cur[i+1 + HELPER_MAXLEN] ]; 
	pic[i+1] = clip_softcore_post[OFFSET_CLIP_SOFTCORE + pic[i+1]
                        + prev[i+2] 
			+ prev[i+1+ HELPER_MAXLEN] 
                        - cur[i+1] 
			- cur[i+2 + HELPER_MAXLEN] ]; 
    }

    if(tmp1)
    {
	pic[n-1] = clip_softcore_post[OFFSET_CLIP_SOFTCORE + pic[n-1]
                      + prev[n] 
		      + prev[n-1+ HELPER_MAXLEN]
		      - cur[n-1] 
		      - cur[n + HELPER_MAXLEN] ];
    }

    return 1;
}



int DiffNonLin2D( unsigned char* pos, unsigned char* neg, int* out, int n )
{
    int i;
    int tmpv;
   
    if(n<16)
    {
	return 1;
    }

    out[0]=lut_soft_core_2d[pos[0] - neg[0]];
    
    for (i = 1; i < n-9; i=i+8)
    {	
	register int tmp0, tmp1;

	tmp0 = pos[i] - neg[i];
	tmp1 = pos[i+1] - neg[i+1];
	out[i]=lut_soft_core_2d[tmp0];
	out[i+1]=lut_soft_core_2d[tmp1];

	tmp0 = pos[i+2] - neg[i+2];
	tmp1 = pos[i+3] - neg[i+3];
	out[i+2]=lut_soft_core_2d[tmp0];
	out[i+3]=lut_soft_core_2d[tmp1];

			
	tmp0 = pos[i+4] - neg[i+4];
	tmp1 = pos[i+5] - neg[i+5];
	out[i+4]=lut_soft_core_2d[tmp0];
	out[i+5]=lut_soft_core_2d[tmp1];

	tmp0 = pos[i+6] - neg[i+6];
	tmp1 = pos[i+7] - neg[i+7];
	out[i+6]=lut_soft_core_2d88[tmp0];
	out[i+7]=lut_soft_core_2d88[tmp1];
    }

	
    tmpv=(n-((n>>3)<<3))-1;
	
    switch(tmpv)
    {
    case 6:
	out[n-6]=lut_soft_core_2d[pos[n-6]-neg[n-6]];
    case 5:
	out[n-5]=lut_soft_core_2d[pos[n-5]-neg[n-5]];
    case 4:
	out[n-4]=lut_soft_core_2d[pos[n-4]-neg[n-4]];
    case 3:
	out[n-3]=lut_soft_core_2d[pos[n-3]-neg[n-3]];
    case 2:
	out[n-2]=lut_soft_core_2d[pos[n-2]-neg[n-2]];
    case 1:
	out[n-1]=lut_soft_core_2d[pos[n-1]-neg[n-1]];
    case 0:
	break;
    }

		
    return 1;
}

int DiffNonLin2Duniform( unsigned char* pos, unsigned char* neg, int* out, int n )
{
    int i;
    int tmpv;

    if(n<16)
    {
	return 1;
    }

    out[0]=lut_soft_core_2d[pos[0] - neg[0]];

    for (i = 1; i < n-9; i=i+8)
	{	
	    register int tmp0, tmp1;

	    tmp0 = pos[i] - neg[i];
	    tmp1 = pos[i+1] - neg[i+1];
	    out[i]=lut_soft_core_2d[tmp0];
	    out[i+1]=lut_soft_core_2d[tmp1];

	    tmp0 = pos[i+2] - neg[i+2];
	    tmp1 = pos[i+3] - neg[i+3];
	    out[i+2]=lut_soft_core_2d[tmp0];
	    out[i+3]=lut_soft_core_2d[tmp1];

			
	    tmp0 = pos[i+4] - neg[i+4];
	    tmp1 = pos[i+5] - neg[i+5];
	    out[i+4]=lut_soft_core_2d[tmp0];
	    out[i+5]=lut_soft_core_2d[tmp1];

	    tmp0 = pos[i+6] - neg[i+6];
	    tmp1 = pos[i+7] - neg[i+7];
	    out[i+6]=lut_soft_core_2d[tmp0];
	    out[i+7]=lut_soft_core_2d[tmp1];
	}

    tmpv=(n-((n>>3)<<3))-1;
	
    switch(tmpv)
    {
    case 6:
	out[n-6]=lut_soft_core_2d[pos[n-6]-neg[n-6]];
    case 5:
	out[n-5]=lut_soft_core_2d[pos[n-5]-neg[n-5]];
    case 4:
	out[n-4]=lut_soft_core_2d[pos[n-4]-neg[n-4]];
    case 3:
	out[n-3]=lut_soft_core_2d[pos[n-3]-neg[n-3]];
    case 2:
	out[n-2]=lut_soft_core_2d[pos[n-2]-neg[n-2]];
    case 1:
	out[n-1]=lut_soft_core_2d[pos[n-1]-neg[n-1]];
    case 0:
	break;
    }
	    
    return 1;
}

int DiffNonLin2Dconst( unsigned char* pos, unsigned char* neg, int* out, int n )
{
    int i;
    int tmpv;

    if(n<16)
    {
	return 1;
    }

    out[0]=lut_soft_core_2d88[pos[0] - neg[0]];


    for (i = 1; i < n-9; i=i+8)
	{	
	    register int tmp0, tmp1;

	    tmp0 = pos[i] - neg[i];
	    tmp1 = pos[i+1] - neg[i+1];
	    out[i]=lut_soft_core_2d88[tmp0];
	    out[i+1]=lut_soft_core_2d88[tmp1];

	    tmp0 = pos[i+2] - neg[i+2];
	    tmp1 = pos[i+3] - neg[i+3];
	    out[i+2]=lut_soft_core_2d88[tmp0];
	    out[i+3]=lut_soft_core_2d88[tmp1];

			
	    tmp0 = pos[i+4] - neg[i+4];
	    tmp1 = pos[i+5] - neg[i+5];
	    out[i+4]=lut_soft_core_2d88[tmp0];
	    out[i+5]=lut_soft_core_2d88[tmp1];

	    tmp0 = pos[i+6] - neg[i+6];
	    tmp1 = pos[i+7] - neg[i+7];
	    out[i+6]=lut_soft_core_2d88[tmp0];
	    out[i+7]=lut_soft_core_2d88[tmp1];
	}
	

    tmpv=(n-((n>>3)<<3))-1;
	
    switch(tmpv)
    {
    case 6:
	out[n-6]=lut_soft_core_2d88[pos[n-6]-neg[n-6]];
    case 5:
	out[n-5]=lut_soft_core_2d88[pos[n-5]-neg[n-5]];
    case 4:
	out[n-4]=lut_soft_core_2d88[pos[n-4]-neg[n-4]];
    case 3:
	out[n-3]=lut_soft_core_2d88[pos[n-3]-neg[n-3]];
    case 2:
	out[n-2]=lut_soft_core_2d88[pos[n-2]-neg[n-2]];
    case 1:
	out[n-1]=lut_soft_core_2d88[pos[n-1]-neg[n-1]];
    case 0:
	break;
    }
	    	    
		
    return 1;
}



int Inittriangleluts(float c, float a, float b)
{
	int i;
	
	//nonlinearity triangle lut
        for (i=1; i<256; i++)
	    soft_core_2d[i+255] = (int)soft_triangle_lut_2d((float)i,c,a,b);

	soft_core_2d[255+(int)(c/2)] =int(-a);
	soft_core_2d[511-int((255-c)/2)]=int(b);


        for (i=0; i<255; i++)
            soft_core_2d[i] = -soft_core_2d[510-i];

	soft_core_2d[255]=0;
	soft_core_2d[0]=0;
	soft_core_2d[510]=0;

	return 1;

}

int Inittrianglelutsconst(void)
{

	int i;

	//initialize a second lut which has c=50,a=0,b=40 this will be used for 8x8 block boundaries always
        for (i=1; i<256; i++)
	    soft_core_2d88[i+255] = (int)soft_triangle_lut_2d((float)i,(float)CONST_C,(float)CONST_A,(float)CONST_B);

	soft_core_2d88[255+(int)(CONST_C/2)] =-CONST_A;
	soft_core_2d88[511-int((255-CONST_C)/2)]=40;


        for (i=0; i<255; i++)
            soft_core_2d88[i] = -soft_core_2d88[510-i];

	soft_core_2d88[255]=0;
	soft_core_2d88[0]=0;
	soft_core_2d88[510]=0;

	return 1;

}

int soft_triangle_lut_2d(float input, float c, float a, float b)
{
    float y;
    /*assumes the ranges of input,a,b,c are already clipped*/

    if(input>=0 && input<(c/2))
	y=-(input*a)/(c/2);
    else if(input>=c/2 && input<c)
	y=-((c-input)*a)/(c/2);
    else if(input>=c && input<((255+c)/2))
	y=((input-c)*b)/((255-c)/2);
    else if(input>=((255+c)/2) && input <=255)
	y=((255-input)*b)/((255-c)/2);


    return((int)y);

}



void Initcliplut(void)
{

	int i;
	//clipping lut
	//assumes a and b can have a max value 60	

    for (i=0; i<319; i++)
	clip_softcore_post[i] = 0;

	clip_softcore_post[319] = 0;
    for (i=1; i<256; i++)
	clip_softcore_post[i+319] = i;     
   
    for (i=256; i<511; i++)
	clip_softcore_post[i+319] = 255;     
	
}
