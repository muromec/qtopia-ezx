/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxuuid.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef __CHXUUID_
#define __CHXUUID_

//#include "hxtypes.h"
//#include "hxrand.h"

class CMultiplePrimeRandom;

#define MACHINEID_SIZE	6
typedef struct tag_uuid_tt
{
    ULONG32      time_low;
    UINT16      time_mid;
    UINT16      time_hi_and_version;
    UCHAR       clock_seq_hi_and_reserved;
    UCHAR       clock_seq_low;
    UCHAR       node[MACHINEID_SIZE];
} uuid_tt, *uuid_p_t;

typedef struct tag_uuid_ttime_t
{
    ULONG32  lo;
    ULONG32  hi;
} uuid_ttime_t, *uuid_ttime_p_t;


typedef struct tag_unsigned64_t
{
    ULONG32  lo;
    ULONG32  hi;
} unsigned64_t, *unsigned64_p_t;

/*
 * the number of elements returned by sscanf() when converting
 * string formatted uuid's to binary
 */
#define UUID_ELEMENTS_NUM       11

/*
 * local defines used in uuid bit-diddling
 */
#define HI_WORD(w)                  ((w) >> 16)
#define RAND_MASK                   0x3fff      /* same as CLOCK_SEQ_LAST */

#define TIME_MID_MASK               0x0000ffff
#define TIME_HIGH_MASK              0x0fff0000
#define TIME_HIGH_SHIFT_COUNT       16

#define MAX_TIME_ADJUST             0x7fff

#define CLOCK_SEQ_LOW_MASK          0xff
#define CLOCK_SEQ_HIGH_MASK         0x3f00
#define CLOCK_SEQ_HIGH_SHIFT_COUNT  8
#define CLOCK_SEQ_FIRST             1
#define CLOCK_SEQ_LAST              0x3fff      /* same as RAND_MASK */

#define CLOCK_SEQ_BUMP(seq)         ((*seq) = ((*seq) + 1) & CLOCK_SEQ_LAST)

#define UUID_VERSION_BITS           (1 << 12)
#define UUID_RESERVED_BITS          0xE0

#define IS_OLD_UUID(uuid) (((uuid)->clock_seq_hi_and_reserved & 0xc0) != 0x80)

/*
 * Max size of a uuid string: tttttttt-tttt-cccc-cccc-nnnnnnnnnnnn
 * Note: this includes the implied '\0'
 */
#define UUID_C_UUID_STRING_MAX          37

class CHXString;

enum uuid_compval_t
{
    uuid_e_less_than, uuid_e_equal_to, uuid_e_greater_than
};

#ifdef _WIN16
/************************************************************************
 *  int FAR _cdecl wsscanf( LPSTR lpBuffer, LPSTR lpFormat,
 *                          LPSTR lpParms, ... )
 *
 *  Description:
 *     Replacement function for sscanf().   Useable in DLLs (all
 *     parameters are passed as FAR pointers.
 *
 ************************************************************************/
int STDMETHODCALLTYPE wsscanf( char* lpBuffer, char* lpFormat, ... );

#endif /* _WIN16 */

class CHXuuid
{
protected:
/*
 * declarations used in UTC time calculations
 */
    uuid_ttime_t     m_time_now;     /* utc time as of last query        */
    uuid_ttime_t     m_time_last;    /* 'saved' value of time_now        */
    UINT16	    m_time_adjust;  /* 'adjustment' to ensure uniqness  */
    UINT16	    m_clock_seq;    /* 'adjustment' for backwards clocks*/
    UCHAR	    m_machineID[MACHINEID_SIZE];
    CMultiplePrimeRandom* m_pRand;

    void NewClockSeq(UINT16& clock_seq);
    void GetOSTime(uuid_ttime_t * uuid_ttime);
    void GenerateMachineID();
    uuid_compval_t TimeCmp(uuid_ttime_p_t time1, uuid_ttime_p_t time2);
    void UnsignedExtendedMultiply(ULONG32 u,ULONG32 v, unsigned64_t* prodPtr);

    UINT16 TrueRandom();

public:
    CHXuuid();
    CHXuuid(UCHAR theMachineID[MACHINEID_SIZE]);
    virtual ~CHXuuid();

    UCHAR* GetMachineID() {return m_machineID;}

    HX_RESULT GetUuid(uuid_tt* uuid);
    HX_RESULT GetUuid(uuid_tt* uuid, const UCHAR* pBuffer, UINT32 ulBufferSize);
    static HX_RESULT HXUuidToString(const uuid_tt* uuid, CHXString* uuid_string);
    static HX_RESULT HXUuidFromString(const char* uuid_string, uuid_tt* uuid);
    static HXBOOL HXIsEqual(uuid_p_t uuid1, uuid_p_t uuid2);
    static HX_RESULT HXPack(UINT8* pBuffer, uuid_p_t uuid);
    static HX_RESULT HXUnpack(uuid_p_t uuid, UINT8* pBuffer);
};

#endif /*__CHXUUID_*/
