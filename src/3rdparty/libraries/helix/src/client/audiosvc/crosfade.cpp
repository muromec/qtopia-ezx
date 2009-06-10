/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: crosfade.cpp,v 1.6 2007/07/06 21:57:40 jfinnecy Exp $
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

/****************************************************************************
 *
 * Fixed-point crossfade.
 * Uses arbitrary lookup table, with table interpolation.
 *
 */

#include "hxtypes.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hlxclib/stdio.h"
#include "hlxclib/math.h"

#include "crosfade.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

//#define GENERATE_TABLE	1
#define TABLE_FILE_NAME	    "table.cpp"


CrossFader::CrossFader()
    : tabstep(0)
    , tabacc(0)
    , tabint(0)
    , m_uNumChannels(0)
    , m_bInitialized(FALSE)
{
}

CrossFader::~CrossFader()
{
}

HX_RESULT	
CrossFader::Initialize(UINT16 uNumSamplesToFadeOn, UINT16 uNumChannels)
{
    HX_ASSERT(uNumSamplesToFadeOn > 0 && uNumChannels > 0);

    if (uNumSamplesToFadeOn == 0 || uNumChannels == 0)
    {
	return HXR_INVALID_PARAMETER;
    }

    tabacc = 0;		/* accumulator */
    tabint = 0;		/* table index */
    tabstep = (NALPHA << FRACBITS) / uNumSamplesToFadeOn;

    m_uNumChannels = uNumChannels;
    m_bInitialized  = TRUE;

    return HXR_OK;
}

/*
 * Crossfades over nfade samples, operating in-place on sampnew.
 * alpha values are generated from table via linear interpolation.
 */
void
CrossFader::CrossFade(INT16* sampold, INT16* sampnew, UINT16 uNumSamples)
{
    INT32 alerp, sdelta;
    UINT16 n;

    if (!m_bInitialized)
    {
	return;
    }

	/*  if we are doing more samples than the table is set up for  */
	/*  just keep tail of sampnew as is (i.e. reduce uNumSamples we are mixing over) */
	if( ((tabacc + (uNumSamples-1)*tabstep)>>FRACBITS) >= NALPHA)
	{
		uNumSamples = ((1+((((INT32)NALPHA)<<FRACBITS)-tabacc))/tabstep)-1;
	}
 
    /* crossfade, stepping thru table in fixed point */
    for (n = 0; n < uNumSamples; n++) 
    {
	/* interpolate new alpha */
	alerp = alpha[tabint];
	alerp += (adelta[tabint] * (tabacc & FRACMASK)) >> FRACBITS;
	/* next table step */
	tabacc += tabstep;
	tabint = tabacc >> FRACBITS;

	for (UINT16 uNumChannels = 0; uNumChannels < m_uNumChannels; uNumChannels++)
	{
	    /* crossfade, using interpolated alpha */
	    sdelta = alerp * (*sampold - *sampnew);
	    *sampnew += int((sdelta + FRACROUND) >> FRACBITS);

	    sampold++;
	    sampnew++;
	}
    }
}


// XXXNH: CrossFadeInit() *only* appears to be used from the main() in this
// file, which in turn is only used when GENERATE_TABLE is defined.  For
// ordinary circumstances I think it is therefore acceptable to completely
// #ifdef this out.
#ifdef GENERATE_TABLE

/*
 * Initialize crossfade tables.
 * Currently using linear dB, but could be anything...
 * This code is ONLY used to generate static tables.
 */
void
CrossFader::CrossFadeInit(void)
{
    double dbval, dbstep;
    int n;

//#ifdef GENERATE_TABLE
    FILE* fd = fopen(TABLE_FILE_NAME, "w+");
    fprintf(fd, "#ifdef GENERATE_TABLE\n");
    fprintf(fd, "static INT32 alpha[NALPHA+1] = {\n");
    fprintf(fd, "#else\n");
    fprintf(fd, "static const INT32 alpha[NALPHA+1] = {\n");
    fprintf(fd, "#endif\n");
    fprintf(fd, "\n\t\t\t\t");
//#endif /*GENERATE_TABLE*/

    /* generate alpha table */
    dbstep = (DBEND - DBSTART) / (NALPHA - 1);
    for (n = 0; n < NALPHA; n++) 
    {
	/* linear steps in dB */
	dbval = DBSTART + (dbstep * n);
	/* store as gains, in fixed point */
	alpha[n] = (int) (DB2GAIN(dbval) * (1<<FRACBITS) + 0.5);

//#ifdef GENERATE_TABLE
	fprintf(fd, "%d, ", alpha[n]);
	if ((n+1) % 5 == 0)
	{
	    fprintf(fd, "\n\t\t\t\t");
	}
//#endif /*GENERATE_TABLE*/
    }

    alpha[n] = alpha[n-1];	/* in case roundoff oversteps */

//#ifdef GENERATE_TABLE
    fprintf(fd, "%d\n", alpha[n]);
    fprintf(fd, "};\n\n");

    fprintf(fd, "#ifdef GENERATE_TABLE\n");
    fprintf(fd, "static INT32 adelta[NALPHA] = {\n");
    fprintf(fd, "#else\n");
    fprintf(fd, "static const INT32 adelta[NALPHA] = {\n");
    fprintf(fd, "#endif\n");
    fprintf(fd, "\n\t\t\t\t");
//#endif /*GENERATE_TABLE*/

    /* generate delta table, for fast interpolate */
    for (n = 0; n < NALPHA-1; n++)
    {
	adelta[n] = alpha[n+1] - alpha[n];

//#ifdef GENERATE_TABLE
	fprintf(fd, "%d, ", adelta[n]);
	if ((n+1)%5 == 0)
	{
	    fprintf(fd, "\n\t\t\t\t");
	}
//#endif /*GENERATE_TABLE*/
    }

    adelta[n] = 0;	/* in case roundoff oversteps */

//#ifdef GENERATE_TABLE
    fprintf(fd, "%d\n", adelta[n]);
    fprintf(fd, "};\n\n");
    fclose(fd);
//#endif /*GENERATE_TABLE*/
}

#endif /*GENERATE_TABLE*/



/*
 * test app
 */

#ifdef GENERATE_TABLE

#define NMAX 4096
short testold[NMAX];
short testnew[NMAX];
int
main(void)
{
	int n;

	for (n = 0; n < NMAX; n++) {
		testold[n] = -32768;
		testnew[n] = 32767;
	}

	CrossFader cf;
	cf.CrossFadeInit();
	cf.CrossFade(testold, testnew, 1024);

	return 0;
}
#endif /*GENERATE_TABLE*/
