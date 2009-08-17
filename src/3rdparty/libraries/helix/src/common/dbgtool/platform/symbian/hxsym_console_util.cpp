/*****************************************************************************
 * hxsym_console_util.cpp
 * ---------------
 *
 *
 * utilities for printing dprintf output to RDebug (console)
 *
 *
 * Target:
 * Symbian OS
 *
 *
 * (c) 1995-2003 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *****************************************************************************/

// Symbian includes... 
#include <e32base.h>
#include <e32def.h>
#include <e32svr.h>
#include <string.h>

#include "hxassert.h"
#include "hxsym_console_util.h"

#if !defined (HELIX_FEATURE_DPRINTF) && defined(HELIX_FEATURE_LOGLEVEL_NONE)
#error file should not be included in build
#endif


namespace
{

void EnsureSafeChunk(TPtrC& ptrChunk);
HBufC* AllocEscapedBuffer(const char* pszText);

//
// Ensure that chunk doesn't end in middle of escape (%X) sequence. If it does,
// resize chunk slightly smaller so that it does not.
// 
void EnsureSafeChunk(TPtrC& ptrChunk)
{
    TUint32 cchChunk = ptrChunk.Length();
    HX_ASSERT(cchChunk > 0);

    // update chunk if last char is '%' (may or may not be beginning of escape sequence)
    TUint32 idx = cchChunk - 1;
    if( ptrChunk[idx] == '%' )
    {
        // scan back before all '%' chars at end of chunk
        while( ptrChunk[idx] == '%' && idx > 0 )
        {
            --idx;
        };
    
        // scan forward
        bool bInEscape = false;
        for (/**/; idx < cchChunk; ++idx)
        {
            if( !bInEscape && ptrChunk[idx] == '%' )
            {
                bInEscape = true;
            }
            else
            {
                bInEscape = false;
            }
        }

        if(bInEscape)
        {
            // last char was escape char (would split chunk in middle of '%X')
            ptrChunk.Set( ptrChunk.Left(cchChunk - 1));
        }

        HX_ASSERT(ptrChunk.Length() > 0);
    }
}

//
// escape possible '%' chars in text
//
HBufC* AllocEscapedBuffer(const char* pszText)
{
    HBufC* pBuff = 0;

    TUint32 cchIn = pszText ? strlen(pszText) : 0;
    if (cchIn > 0)
    {
        // determine length needed in order to escape '%'
        TUint32 cch = cchIn;
        for(TUint32 idx = 0; idx < cchIn; ++idx)
        {
            if( pszText[idx] == '%' )
            {
                ++cch;
            }
        }

        // allocate new buffer
	pBuff = HBufC::NewMax(cch);
        if( pBuff)
        {
            // escape '%' with '%%'
	    TPtr ptr = pBuff->Des();

            TUint32 idxOut = 0;
            for(TUint32 idx = 0; idx < cchIn; ++idx)
            {
	        if( pszText[idx] == '%' )
                {
                    ptr[idxOut++] = '%';
                }
                ptr[idxOut++] = pszText[idx];
            }
        }
    }
    return pBuff;	  
}

} // locals

//
// print text to console (via RDebug)
//
void PrintConsole(const char* pszText)
{
    // allocate new buffer
    HBufC* pBuff = AllocEscapedBuffer(pszText);
    if(pBuff)
    {
        // print out in chunks to work around RDebug limit
	TUint32 cchLeft = pBuff->Des().Length();
	TUint32 idxOffset = 0;
	const TUint32 CHUNK_SIZE = 128;
	while (cchLeft > 0)
	{
	    TPtrC ptrChunk(pBuff->Ptr() + idxOffset, Min(cchLeft, CHUNK_SIZE));
            
            EnsureSafeChunk(ptrChunk);
	    RDebug::Print(ptrChunk);

	    idxOffset += ptrChunk.Length();
	    cchLeft -= ptrChunk.Length();
	}

	delete pBuff;
    }
}
