/*****************************************************************************
 * hxsym_dprintf.h
 * ---------------
 *
 *
 * Target:
 * Symbian OS
 *
 *
 * (c) 1995-2003 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *****************************************************************************/

#if !defined(HXSYM_DPRINTF_H__)
#define HXSYM_DPRINTF_H__

#include "hxcom.h"
_INTERFACE IHXPreferences;

#if defined(HELIX_FEATURE_DPRINTF)

# include "globals/hxglobals.h"

// flags controlling dprint output
enum PrintFlags
{
    // prolog
    PRINT_DATE                  = 0x01, // include date
    PRINT_TIME                  = 0x02, // include time
    PRINT_TIME_INCLUDE_MS       = 0x04, // incude ms when writing timestamp
    PRINT_TID                   = 0x08  // include thread id
};

struct DPrintfData
{
    DPrintfData();
    UINT32      printFlags;
    UINT32      mask;
    UINT32      funcTraceMask;
    CHXString   sinkName;
};

inline DPrintfData::DPrintfData()
: printFlags(PRINT_TIME | PRINT_TIME_INCLUDE_MS | PRINT_TID)
, mask(0)
, funcTraceMask(0)
{}

DPrintfData* dprintfGetData();
UINT32 dprintfGetMask();
void dprintfInit(IHXPreferences* pPrefs);

// for compatibility with older code (prefer DprintfData)
UINT32& debug_level();
UINT32& debug_func_level();


void dprintf(const char *, ... );
#define DPRINTF(mask,x) if (dprintfGetMask() & (mask)) dprintf x; else

#else // HELIX_FEATURE_DPRINTF

inline void dprintfInit(IHXPreferences* pPrefs) { /* compile out*/}

#define	DPRINTF(mask,x)

#endif // HELIX_FEATURE_DPRINTF

#define DPRINTF_INIT(pPrefs) dprintfInit(pPrefs)

#endif //HXSYM_DPRINTF_H__
