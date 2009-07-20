/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: chxuuid.cpp,v 1.13 2005/05/03 16:14:29 albertofloyd Exp $
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
 * developer and/or licensor of the Original Code and owns the
 * copyrights in the portions it created.
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

//
//	CHXuuid class implementation
//
//
//  This code derives from uuid.c from OSF DCE source
/*
 * 
 * (c) Copyright 1989 OPEN SOFTWARE FOUNDATION, INC.
 * (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 * (c) Copyright 1989 DIGITAL EQUIPMENT CORPORATION
 * To anyone who acknowledges that this file is provided "AS IS"
 * without any express or implied warranty:
 *                 permission to use, copy, modify, and distribute this
 * file for any purpose is hereby granted without fee, provided that
 * the above copyright notices and this notice appears in all source
 * code copies, and that none of the names of Open Software
 * Foundation, Inc., Hewlett-Packard Company, or Digital Equipment
 * Corporation be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  Neither Open Software Foundation, Inc., Hewlett-
 * Packard Company, nor Digital Equipment Corporation makes any
 * representations about the suitability of this software for any
 * purpose.
 * 
 */
 
#include "hxtypes.h"
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN
#include "hlxclib/windows.h"	// for HX_GET_TICK_COUNT
#endif

#include "hlxclib/string.h"
// #include "hlxclib/stdio.h"
#include "safestring.h"

#include "hxstring.h"
#include "hxresult.h"
#include "hxassert.h"
#include "hxrand.h"
#include "chxuuid.h"
#include "hxtick.h"
#include "hxtime.h"
#include "netbyte.h"
#include "md5.h"

#include <ctype.h>
#if !defined(_VXWORKS) && !defined(_SYMBIAN) && !defined(_OPENWAVE)
#include <memory.h>
#endif
#include <stdarg.h>


#ifdef _WIN16
#include <ctype.h>
#include <memory.h>
#endif

#ifdef _WIN32
#include <objbase.h>
#endif

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#if defined(_WIN16)
// global definitons (scope: wsscanf.c)

#ifdef ALLOW_RANGE
   char szSBrackSet[] = " \t-\r]" ;  // use range-style list
#else
   char szSBrackSet[] = "\t\n\v\f\r]" ;  // chars defined by isspace 
#endif

char szCBrackSet[] = "]" ;

// constant definitions
#define ALLOW_RANGE                        // allow "%[a-z]" - style 
                                           // scansets

#define LEFT_BRACKET  ('[' | ('a' - 'A'))  // 'lowercase' version

#define ASCII         32                   // # of bytes needed to 
                                           // hold 256 bits
// macro definitions
#define INC()            (++nCharCount, Inc( (LPSTR FAR *) &lpPtrBuffer ))
#define UN_INC( chr )    (--nCharCount, UnInc( chr, (LPSTR FAR *) &lpPtrBuffer ))
#define EAT_WHITE()      WhiteOut( (int FAR *) &nCharCount, (LPSTR FAR *) &lpPtrBuffer )
#define HEXTODEC( chr )  HexToDec( chr )

#define MUL10(x)         ( (((x)<<2) + (x))<<1 )

typedef char _far *wva_list ;

#define wva_start( ap, v )  (ap = (wva_list) &v + sizeof( v ))
#define wva_arg( ap, t )    (((t _far *)(ap += sizeof( t )))[-1])
#define wva_end( ap )       (ap = NULL)

/************************************************************************
 *  char NEAR HexToDec( char chChar )
 *
 *  Description:
 *     HexToDec() convert hexadecimal to decimal equivalent.
 *
 ************************************************************************/

char NEAR HexToDec( char chChar )
{
   return (isdigit( chChar ) ? chChar : (char) ((chChar & ~('a' - 'A')) - 'A' + 10 + '0')) ;

} /* end of HexToDec() */

/************************************************************************
 *  char NEAR Inc( LPSTR FAR *lpPtrBuffer )
 *
 *  Description:
 *     "Pops" a character from the buffer.
 *
 ************************************************************************/

char NEAR Inc( LPSTR FAR *lpPtrBuffer )
{
   char  ch ;

   ch = *((LPSTR) *lpPtrBuffer) ;
   ((LPSTR&)(*lpPtrBuffer))++ ;

   return ( ch ) ;

} /* end of Inc() */

/************************************************************************
 *  VOID NEAR UnInc( char chChar, LPSTR FAR *lpPtrBuffer )
 *
 *  Description:
 *     "Pushes" a characters back into the buffer.
 *
 ************************************************************************/

VOID NEAR UnInc( char chChar, LPSTR FAR *lpPtrBuffer )
{
   ((LPSTR&)(*lpPtrBuffer))-- ;
   *((LPSTR) *lpPtrBuffer) = chChar ;
   
} /* end of UnInc() */

/************************************************************************
 *  char NEAR WhiteOut( int *lpnCounter, LPSTR FAR *lpPtrBuffer )
 *
 *  Description:
 *     Increments the string pointer while in white space.
 *
 ************************************************************************/

char NEAR WhiteOut( int FAR *lpnCounter, LPSTR FAR *lpPtrBuffer )
{
   char  ch ;

   while (isspace(ch = (++*lpnCounter, Inc( lpPtrBuffer )))) ;
   return ( ch ) ;

} /* end of WhiteOut() */

/************************************************************************
 *  int FAR _cdecl wsscanf( LPSTR lpBuffer, LPSTR lpFormat,
 *                          LPSTR lpParms, ... )
 *
 *  Description:
 *     Replacement function for sscanf().   Useable in DLLs (all
 *     parameters are passed as FAR pointers.
 *
 ************************************************************************/

int FAR _cdecl wsscanf( char* lpBuffer, char* lpFormat, ... )
{
   BYTE          bCoerceShort, bDoneFlag, bLongOne, bMatch, bNegative,
                 bReject, bSuppress ;
   BYTE FAR *    lpPtrScan ;
   DWORD         dwNumber ;
   LPSTR         lpPtrFormat, lpPtrBuffer ;
   LPVOID FAR *  lpArgList;
   LPVOID        lpPointer, lpStart ;
   WORD          wHoldSeg ;
   char          ch, chCom, chLast, chPrev, chRange, szTable[ ASCII ] ; /* Flawfinder: ignore */
   int           nCount, nCharCount, nWidth, nWidthSet, nStarted ;

   wva_list	 lpParms;
   
   wva_start(lpParms, lpFormat);

   lpArgList = (LPVOID FAR *) &lpParms ;

   nCount = nCharCount = 0 ;
   bMatch = FALSE ;
   lpPtrFormat = lpFormat ;
   lpPtrBuffer = lpBuffer ;

   while (*lpPtrFormat)
   {
      if (isspace( *lpPtrFormat ))
      {
         UN_INC( EAT_WHITE() ) ;  // put first non-space char back
         while (isspace(*++lpPtrFormat)) ;
      }
      if ('%' == *lpPtrFormat)
      {
         bCoerceShort = bDoneFlag = bLongOne = bNegative =
         bReject = bSuppress = FALSE ;
         nWidth = nWidthSet = nStarted = 0 ;
         dwNumber = 0 ;
         chPrev = 0 ;
   
         while (!bDoneFlag)
         {
            if (isdigit( chCom = *++lpPtrFormat ))
            {
               ++nWidthSet ;
               nWidth = MUL10( nWidth ) + (chCom - '0') ;
            }
            else
               switch (chCom)
               {
                  case 'F':
                  case 'N':
                     // FAR is only type of pointer in DLLs
                     break ;
   
                  case 'h':
                     ++bCoerceShort ;
                     break ;
            
                  case 'l':
                     ++bLongOne ;
                     break ;
   
                  case '*':
                     ++bSuppress ;
                     break ;
   
                  default:
                     ++bDoneFlag ;
                     break ;
               }
         }
   
         if (!bSuppress)
            /* ALL pointers are pushed as FAR */
            lpPointer = (LPVOID) *lpArgList++ ;
   
         bDoneFlag = FALSE ;
   
         if ('n' != (chCom = (char)(*lpPtrFormat | ('a' - 'A'))))
         {
            if ('c' != chCom && LEFT_BRACKET != chCom)
               ch = EAT_WHITE() ;
            else
               ch = INC() ;
         }
   
         if (!nWidthSet || nWidth)
         {
            switch (chCom)
            {
               case 'c':
                  if (!nWidthSet)
                  {
                     ++nWidthSet ;
                     ++nWidth ;
                  }
                  lpPtrScan = (BYTE FAR *) szCBrackSet  ;
                  --bReject ;
                  goto ScanIt2 ;
   
               case 's':
                  lpPtrScan = (BYTE FAR *) szSBrackSet ;
                  --bReject ;
                  goto ScanIt2 ;
   
               case LEFT_BRACKET:
                  lpPtrScan = (BYTE FAR *) (++lpPtrFormat) ;
                  if ('^' == *lpPtrScan)
                  {
                     ++lpPtrScan ;
                     --bReject ;
                  }
ScanIt2:
                  _fmemset( szTable, 0, ASCII ) ;
   
#ifdef ALLOW_RANGE
                  if (LEFT_BRACKET == chCom)
                     if (']' == *lpPtrScan)
                     {
                        chPrev = ']' ;
                        ++lpPtrScan ;
                        szTable[ ']' >> 3 ] = 1 << (']' & 7 ) ;
                     }
                  while (']' != *lpPtrScan)
                  {
                     chRange = *lpPtrScan++ ;
                     if ('-' != chRange || !chPrev || ']' == *lpPtrScan)
                        szTable[(chPrev = chRange) >> 3] |= 1 << (chRange & 7) ;
                     else
                     {
                        // handle a-z type set
                        chRange = *lpPtrScan++ ;
                        if (chPrev < chRange)
                           chLast = chRange ;
                        else
                        {
                           chLast = chPrev ;
                           chPrev = chRange ;
                        }
                        for (chRange = chPrev; chRange <= chLast; ++chRange)
                           szTable[chRange >> 3] |= 1 << (chRange & 7) ;
                        chPrev = 0 ;
                     }
                  }
#else
                  if (LEFT_BRACKET == chCom)
                     if (']'  == *lpPtrScan)
                     {
                        ++lpPtrScan;
                        szTable[(chPrev = ']') >> 3] |= 1 << (']' & 7) ;
                     }
                  while (']' != *lpPtrScan)
                     szTable[ *lpPtrScan >> 3] |= 1 << (*lpPtrScan & 7) ;
#endif
                  if (!*lpPtrScan)
                     goto ErrorReturn ;
         
                  if (LEFT_BRACKET == chCom)
                     lpPtrFormat = (LPSTR)lpPtrScan ;
            
                  lpStart = lpPointer ;
                  while ((!nWidthSet || nWidth--) &&
                         ((szTable[ ch >> 3 ] ^ bReject) & (1 << (ch & 7))))
                  {
                     if (!bSuppress)
                     {
                        *(LPSTR)lpPointer = (char) ch ;
                        ++((LPSTR&)lpPointer) ;
                     }
                     else
                        // just indicate a match
                        ++((LPSTR&)lpStart) ;
                     ch = INC() ;
                  }
                  UN_INC( ch )  ;

                  // make sure something has been match and, if assignment
                  // is not suppressed, null-terminate output string if
                  // chCom != c

                  if (lpStart != lpPointer)
                  {
                     if (!bSuppress)
                     {
                        nCount++ ;
                        if ('c' != chCom)
                           // NULL terminate strings
                           *(LPSTR) lpPointer = NULL ;
                     }
                  }
                  else
                     goto ErrorReturn ;

                  break ;

               case 'i':
                  chCom = 'd' ;  // use 'd' as default
         
               case 'x':
                  if ('-' == ch)
                  {
                     ++bNegative ;
                     goto XIncWidth ;
                  }
                  else if ('+' == ch)
                  {
XIncWidth:
                     if (!--nWidth && nWidthSet)
                        ++bDoneFlag ;
                     else
                        ch = INC() ;
                  }
                  if ('0' == ch)
                  {
                     if ('x' == ((char) (ch = INC())) || 'X' == (char) ch)
                     {
                        ch = INC() ;
                        chCom = 'x' ;
                     }
                     else
                     {
                        // scanning a hex number that starts with 0
                        // push back the character currently in ch
                        // and restore the 0
   
                        UN_INC( ch ) ;
                        ch = '0' ;
                     }
                  }
                  goto GetNum ;
   
               case 'p':
                  // ALL pointers are FAR
                  if (!bCoerceShort)
                  {
                     ++bLongOne ;
                     chCom = 'F' ;  // indicates FAR
                  }
   
               case 'o':
               case 'u':
               case 'd':
                  if ('-' == ch)
                  {
                     ++bNegative ;
                     goto dIncWidth ;
                  }
                  else if ('+' == ch)
                  {
dIncWidth:
                     if (!--nWidth && nWidthSet)
                        ++bDoneFlag ;
                     else
                        ch = INC() ;
                  }
GetNum:
                  while (!bDoneFlag)
                  {
                     if ('x' == chCom || 'p' == chCom || 'F' == chCom)
                     {
                        if (isxdigit(ch))
                        {
                           dwNumber = (dwNumber << 4) ;
                           ch = HEXTODEC( ch ) ;
                        }
                        else if ('F' == chCom)
                        {
                           if (nStarted)
                           {
                              if (':' == ch)
                              {
                                 wHoldSeg = LOWORD( dwNumber ) ;
                                 dwNumber = 0 ;
                                 nStarted = -1 ;
                                 chCom = 'p' ; // switch to offset
                                 ch = '0' ; // don't add ':'
                              }
                              else
                              {
                                 nStarted = 0 ;
                                 ++bDoneFlag ;
                              }
                           }
                           else
                              ++bDoneFlag ;
                        }
                        else
                           ++bDoneFlag ;
                     }
                     else if (isdigit( ch ))
                     {
                        if ('o' == chCom)
                        {
                           if ('8' > ch)
                              dwNumber = (dwNumber << 3) ;
                           else
                              ++bDoneFlag ;
                        }
                        else
                           // 'd' == chCom
                           dwNumber = MUL10( dwNumber ) ;
                     } 
                     else
                        ++bDoneFlag ;

                     if (!bDoneFlag)
                     {
                        ++nStarted ;
                        dwNumber += ch - '0' ;
                        if (nWidthSet && !--nWidth)
                           ++bDoneFlag ;
                        else
                           ch = INC() ;
                     }
                     else
                        UN_INC( ch ) ;

                  } // end of while
   
                  if ('p' == chCom && bLongOne)
                     dwNumber = (dwNumber & (DWORD) 0x0000FFFF) |
                                 ((DWORD) wHoldSeg) << 16 ;
#if _WIN16   
#pragma warning(disable : 4146) 
#endif   
                  if (bNegative)
                     dwNumber = -dwNumber ;
#if _WIN16   
#pragma warning(default : 4146) 
#endif
                  if ('F' == chCom)
                     nStarted = 0 ;
                  if (nStarted)
                  {
                     if (!bSuppress)
                     {
                        ++nCount ;
AssignNum:
                        if (bLongOne)
                           *(DWORD FAR *)lpPointer = dwNumber ;
                        else
                           *(WORD FAR *)lpPointer = LOWORD( dwNumber ) ;
                     }
                  }
                  else
                     goto ErrorReturn ;
   
                  break ;
   
               case 'n':
                  dwNumber = nCharCount ;
                  goto AssignNum ;
   
               default:
                  if ((char)*lpPtrFormat != ch)
                  {
                     UN_INC( ch ) ;
                     goto ErrorReturn ;
                  }
                  else
                     bMatch-- ;
   
                  if (!bSuppress)
                     // ALL pointers are FAR pointers
                     --lpArgList ;
   
            } // end of switch

            bMatch++ ;

         } // end of while
         else
         {
            UN_INC( ch ) ;
            goto ErrorReturn ;
         }
         ++lpPtrFormat ;
      }
      else
      {
         // ('%' != *lpPtrFormat)
         if ((char) *lpPtrFormat++ != (ch = INC()))
         {
            UN_INC( ch ) ;
            goto ErrorReturn ;
         }
      }

   } // end while (*lpPtrFormat)

ErrorReturn:

   wva_end(lpParms);

   return nCount ;

} /* end of wsscanf() */

#endif	// _WIN16

 /*
 * Internal structure of universal unique IDs (UUIDs).
 *
 * There are three "variants" of UUIDs that this code knows about.  The
 * variant #0 is what was defined in the 1989 HP/Apollo Network Computing
 * Architecture (NCA) specification and implemented in NCS 1.x and DECrpc
 * v1.  Variant #1 is what was defined for the joint HP/DEC specification
 * for the OSF (in DEC's "UID Architecture Functional Specification Version
 * X1.0.4") and implemented in NCS 2.0, DECrpc v2, and OSF 1.0 DCE RPC.
 * Variant #2 is defined by Microsoft.
 *
 * This code creates only variant #1 UUIDs.
 * 
 * The three UUID variants can exist on the same wire because they have
 * distinct values in the 3 MSB bits of octet 8 (see table below).  Do
 * NOT confuse the version number with these 3 bits.  (Note the distinct
 * use of the terms "version" and "variant".) Variant #0 had no version
 * field in it.  Changes to variant #1 (should any ever need to be made)
 * can be accomodated using the current form's 4 bit version field.
 * 
 * The UUID record structure MUST NOT contain padding between fields.
 * The total size = 128 bits.
 *
 * To minimize confusion about bit assignment within octets, the UUID
 * record definition is defined only in terms of fields that are integral
 * numbers of octets.
 *
 * Depending on the network data representation, the multi-octet unsigned
 * integer fields are subject to byte swapping when communicated between
 * dissimilar endian machines.  Note that all three UUID variants have
 * the same record structure; this allows this byte swapping to occur.
 * (The ways in which the contents of the fields are generated can and
 * do vary.)
 *
 * The following information applies to variant #1 UUIDs:
 *
 * The lowest addressed octet contains the global/local bit and the
 * unicast/multicast bit, and is the first octet of the address transmitted
 * on an 802.3 LAN.
 *
 *  NOTE:  In the PN version the node field may not be the 802.3 NIC ID
 *	But is a machine ID generated on construction if not specified.
 *	The method for generating the machine ID may vary and is documented
 *	in the GenerateMachineID method.
 *
 * The adjusted time stamp is split into three fields, and the clockSeq
 * is split into two fields.
 *
 * |<------------------------- 32 bits -------------------------->|
 *
 * +--------------------------------------------------------------+
 * |                     low 32 bits of time                      |  0-3  .time_low
 * +-------------------------------+-------------------------------
 * |     mid 16 bits of time       |  4-5               .time_mid
 * +-------+-----------------------+
 * | vers. |   hi 12 bits of time  |  6-7               .time_hi_and_version
 * +-------+-------+---------------+
 * |Res|  clkSeqHi |  8                                 .clock_seq_hi_and_reserved
 * +---------------+
 * |   clkSeqLow   |  9                                 .clock_seq_low
 * +---------------+----------...-----+
 * |            node ID               |  8-16           .node
 * +--------------------------...-----+
 *
 * --------------------------------------------------------------------------
 *
 * The structure layout of all three UUID variants is fixed for all time.
 * I.e., the layout consists of a 32 bit int, 2 16 bit ints, and 8 8
 * bit ints.  The current form version field does NOT determine/affect
 * the layout.  This enables us to do certain operations safely on the
 * variants of UUIDs without regard to variant; this increases the utility
 * of this code even as the version number changes (i.e., this code does
 * NOT need to check the version field).
 *
 * The "Res" field in the octet #8 is the so-called "reserved" bit-field
 * and determines whether or not the uuid is a old, current or other
 * UUID as follows:
 *
 *      MS-bit  2MS-bit  3MS-bit      Variant
 *      ---------------------------------------------
 *         0       x        x       0 (NCS 1.5)
 *         1       0        x       1 (DCE 1.0 RPC)
 *         1       1        0       2 (Microsoft)
 *         1       1        1       unspecified
 *
 *  NOTE:   To identify the PN version from other versions we use the 
 *	111 unspecified variant.
 *
 * --------------------------------------------------------------------------
 *
 */
/****************************************************************************
 *
 * global data declarations
 *
 ****************************************************************************/

static const uuid_tt uuid_g_nil_uuid = { 0, 0, 0, 0, 0, {0,0,0,0,0,0} };
static const uuid_tt uuid_nil = { 0, 0, 0, 0, 0, {0,0,0,0,0,0} };

/*
 * Check the reserved bits to make sure the UUID is of the known structure.
 */

/*
 * defines for time calculations
 */
#ifndef UUID_C_100NS_PER_SEC
#define UUID_C_100NS_PER_SEC            10000000
#endif

#ifndef UUID_C_100NS_PER_MSEC
#define UUID_C_100NS_PER_MSEC           10000
#endif

#define HX_GET_PID()			0xC0C0

 /*
 * UADD_UVLW_2_UVLW - macro to add two unsigned 64-bit long integers
 *                      (ie. add two unsigned 'very' long words)
 *
 * Important note: It is important that this macro accommodate (and it does)
 *                 invocations where one of the addends is also the sum.
 *
 * This macro was snarfed from the DTSS group and was originally:
 *
 * UTCadd - macro to add two UTC times
 *
 * add lo and high order longword separately, using sign bits of the low-order
 * longwords to determine carry.  sign bits are tested before addition in two
 * cases - where sign bits match. when the addend sign bits differ the sign of
 * the result is also tested:
 *
 *        sign            sign
 *      addend 1        addend 2        carry?
 *
 *          1               1            TRUE
 *          1               0            TRUE if sign of sum clear
 *          0               1            TRUE if sign of sum clear
 *          0               0            FALSE
 */
#define UADD_UVLW_2_UVLW(add1, add2, sum)                               \
    if (!(((add1)->lo&0x80000000UL) ^ ((add2)->lo&0x80000000UL)))           \
    {                                                                   \
        if (((add1)->lo&0x80000000UL))                                    \
        {                                                               \
            (sum)->lo = (add1)->lo + (add2)->lo ;                       \
            (sum)->hi = (add1)->hi + (add2)->hi+1 ;                     \
        }                                                               \
        else                                                            \
        {                                                               \
            (sum)->lo  = (add1)->lo + (add2)->lo ;                      \
            (sum)->hi = (add1)->hi + (add2)->hi ;                       \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (sum)->lo = (add1)->lo + (add2)->lo ;                           \
        (sum)->hi = (add1)->hi + (add2)->hi ;                           \
        if (!((sum)->lo&0x80000000UL))                                    \
            (sum)->hi++ ;                                               \
    }


/*
 * UADD_ULW_2_UVLW - macro to add a 32-bit unsigned integer to
 *                   a 64-bit unsigned integer
 *
 * Note: see the UADD_UVLW_2_UVLW() macro
 *
 */
#define UADD_ULW_2_UVLW(add1, add2, sum)                                \
{                                                                       \
    (sum)->hi = (add2)->hi;                                             \
    if ((*add1) & (add2)->lo & 0x80000000UL)                              \
    {                                                                   \
        (sum)->lo = (*add1) + (add2)->lo;                               \
        (sum)->hi++;                                                    \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (sum)->lo = (*add1) + (add2)->lo;                               \
        if (!((sum)->lo & 0x80000000UL))                                  \
        {                                                               \
            (sum)->hi++;                                                \
        }                                                               \
    }                                                                   \
}


/*
 * UADD_UW_2_UVLW - macro to add a 16-bit unsigned integer to
 *                   a 64-bit unsigned integer
 *
 * Note: see the UADD_UVLW_2_UVLW() macro
 *
 */
#define UADD_UW_2_UVLW(add1, add2, sum)                                 \
{                                                                       \
    (sum)->hi = (add2)->hi;                                             \
    if ((add2)->lo & 0x80000000UL)                                        \
    {                                                                   \
        (sum)->lo = (*add1) + (add2)->lo;                               \
        if (!((sum)->lo & 0x80000000UL))                                  \
        {                                                               \
            (sum)->hi++;                                                \
        }                                                               \
    }                                                                   \
    else                                                                \
    {                                                                   \
        (sum)->lo = (*add1) + (add2)->lo;                               \
    }                                                                   \
}

CHXuuid::CHXuuid()
{
    uuid_ttime_t         t;
    UINT16          *seedp, seed=0;
    /*
     * Generating our 'seed' value
     *
     * We start with the current time, but, since the resolution of clocks is
     * system hardware dependent (eg. Ultrix is 10 msec.) and most likely
     * coarser than our resolution (10 usec) we 'mixup' the bits by xor'ing
     * all the bits together.  This will have the effect of involving all of
     * the bits in the determination of the seed value while remaining system
     * independent.  Then for good measure to ensure a unique seed when there
     * are multiple processes creating UUID's on a system, we add in the PID.
     */
    GetOSTime(&t);
    seedp = (UINT16 *)(&t);
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed += HX_GET_PID();
    /*
     * init the random number generator
     */
    m_pRand = new CMultiplePrimeRandom(seed);

    GetOSTime (&m_time_last);

    m_time_adjust = 0;
    m_clock_seq = TrueRandom();

    GenerateMachineID();
}

CHXuuid::CHXuuid(UCHAR machineID[MACHINEID_SIZE])
{
    uuid_ttime_t         t;
    UINT16          *seedp, seed=0;
    /*
     * Generating our 'seed' value
     *
     * We start with the current time, but, since the resolution of clocks is
     * system hardware dependent (eg. Ultrix is 10 msec.) and most likely
     * coarser than our resolution (10 usec) we 'mixup' the bits by xor'ing
     * all the bits together.  This will have the effect of involving all of
     * the bits in the determination of the seed value while remaining system
     * independent.  Then for good measure to ensure a unique seed when there
     * are multiple processes creating UUID's on a system, we add in the PID.
     */
    GetOSTime(&t);
    seedp = (UINT16 *)(&t);
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed ^= *seedp++;
    seed += HX_GET_PID();
    /*
     * init the random number generator
     */
    m_pRand = new CMultiplePrimeRandom(seed);

    GetOSTime (&m_time_last);

    m_time_adjust = 0;
    m_clock_seq = TrueRandom();

    memcpy(m_machineID, machineID, sizeof(machineID)); /* Flawfinder: ignore */
}

CHXuuid::~CHXuuid()
{
    if (m_pRand != NULL)
    {
	delete m_pRand;
    }
}


//
// Changed this routine to avoid unaligned access errors on RISC
// processors. -dbrumley 10-15-98 
//
void CHXuuid::GenerateMachineID()
{
    ULONG32 rand;
    UINT16  tick;

    rand = m_pRand->GetRandomNumber();
    tick = (UINT16)HX_GET_TICKCOUNT();
    memcpy(m_machineID, (UCHAR*)&rand, sizeof rand); /* Flawfinder: ignore */
    memcpy(&m_machineID[4], (UCHAR*)&tick, sizeof tick); /* Flawfinder: ignore */
}


/////////////////////////////////////////////////////////////////////////
// Method:
//	CHXuuid::GetUuid
// Purpose:
//      Loose implementation of version 3 UUID (creation of name-based UUID).  This
//      algorithm outputs the result of an MD5 hash of an internally generated 
//      namespace (GUID) and a name (input buffer).  Unlike the version 3 UUID algorithm, 
//      the namespace is essentially generated at random, so UUIDs created from 
//      the same name at any point of time have a very high probability of being different.

HX_RESULT CHXuuid::GetUuid(uuid_tt* pUuid, const UCHAR* pBuffer, UINT32 ulBufferSize)
{
    // Validate params
    HX_ASSERT(pUuid && pBuffer && ulBufferSize);
    if (!pUuid || !pBuffer || !ulBufferSize)
        return HXR_INVALID_PARAMETER;
    
    // Get a UUID to use as a namespace ID
    HX_RESULT res = GetUuid(pUuid);
    
    if (SUCCEEDED(res))
    {
        md5_state_t ctx;
        md5_init(&ctx);
        UCHAR aHashBuffer[20];
        memset(aHashBuffer, 0, 20);

        // MD5-hash the namespace ID with the name buffer
        md5_append(&ctx, (const UCHAR*)pUuid, sizeof(uuid_tt));
        md5_append(&ctx, (const UCHAR*)pBuffer, ulBufferSize);                
        md5_finish(aHashBuffer, &ctx);

        // Copy the 128-bit hash result into the UUID.  Memcpy one var at a time
        // in case the struct is padded.
        memcpy(&pUuid->time_low, aHashBuffer, sizeof(pUuid->time_low)); /* Flawfinder: ignore */
        memcpy(&pUuid->time_mid, aHashBuffer+4, sizeof(pUuid->time_mid)); /* Flawfinder: ignore */
        memcpy(&pUuid->time_hi_and_version, aHashBuffer+6, sizeof(pUuid->time_hi_and_version)); /* Flawfinder: ignore */
        memcpy(&pUuid->clock_seq_hi_and_reserved, aHashBuffer+8, sizeof(pUuid->clock_seq_hi_and_reserved)); /* Flawfinder: ignore */
        memcpy(&pUuid->clock_seq_low, aHashBuffer+9, sizeof(pUuid->clock_seq_low)); /* Flawfinder: ignore */
        memcpy(&pUuid->node, aHashBuffer+10, sizeof(pUuid->node)); /* Flawfinder: ignore */
    }

    return res;
}

HX_RESULT CHXuuid::GetUuid(uuid_tt* uuid)
{
    HX_RESULT theErr = HXR_OK;

#if defined(_WIN32) && !defined(WIN32_PLATFORM_PSPC) && !defined(HELIX_FEATURE_SERVER)
    if(CoCreateGuid((GUID*)uuid) != S_OK)
	theErr = HXR_FAILED;
#else

    HXBOOL got_no_time = FALSE;

    do
    {
        /*
         * get the current time
         */
        GetOSTime (&m_time_now);

        /*
         * do stuff like:
         *
         *  o check that our clock hasn't gone backwards and handle it
         *    accordingly with clock_seq
         *  o check that we're not generating uuid's faster than we
         *    can accommodate with our time_adjust fudge factor
         */
        switch (TimeCmp(&m_time_now, &m_time_last))
        {
            case uuid_e_less_than:
                NewClockSeq(m_clock_seq);
                m_time_adjust = 0;
		got_no_time = FALSE;
                break;
            case uuid_e_greater_than:
                m_time_adjust = 0;
		got_no_time = FALSE;
                break;
            case uuid_e_equal_to:
                if (m_time_adjust == MAX_TIME_ADJUST)
                {
                    /*
                     * spin your wheels while we wait for the clock to tick
                     */
                    got_no_time = TRUE;
                }
                else
                {
                    m_time_adjust++;
		    got_no_time = FALSE;
                }
                break;
            default:
                theErr = HXR_FAILED;
                return theErr;
        }
    } while (got_no_time);

    m_time_last.lo = m_time_now.lo;
    m_time_last.hi = m_time_now.hi;

    if (m_time_adjust != 0)
    {
        UADD_UW_2_UVLW (&m_time_adjust, &m_time_now, &m_time_now);
    }

    /*
     * now construct a uuid with the information we've gathered
     * plus a few constants
     */
    uuid->time_low = m_time_now.lo;
    uuid->time_mid = (UINT16)(m_time_now.hi & TIME_MID_MASK);

    uuid->time_hi_and_version =	(UINT16)
        ((m_time_now.hi & TIME_HIGH_MASK) >> TIME_HIGH_SHIFT_COUNT);
    uuid->time_hi_and_version |= UUID_VERSION_BITS;

    uuid->clock_seq_low = HX_SAFEINT(m_clock_seq & CLOCK_SEQ_LOW_MASK);
    uuid->clock_seq_hi_and_reserved =
        HX_SAFEINT((m_clock_seq & CLOCK_SEQ_HIGH_MASK) >> CLOCK_SEQ_HIGH_SHIFT_COUNT);

    uuid->clock_seq_hi_and_reserved |= UUID_RESERVED_BITS;

    memcpy (uuid->node, m_machineID, sizeof (m_machineID)); /* Flawfinder: ignore */

#endif // _WIN32

    return theErr;
}

/*
**++
**
**  ROUTINE NAME:       uuid_to_string
**
**--
**/

HX_RESULT CHXuuid::HXUuidToString(const uuid_tt* uuid, CHXString* uuid_string)
{
    HX_RESULT theErr = HXR_OK;

    /*
     * don't do anything if the output argument is NULL
     */
    if (uuid_string == NULL)
    {
        return theErr;
    }

    char *theBuff = uuid_string->GetBuffer(UUID_C_UUID_STRING_MAX);

    SafeSprintf(theBuff, UUID_C_UUID_STRING_MAX,
                "%.4x%.4x-%.4x-%.4x-%.2x%.2x-%.2x%.2x%.2x%.2x%.2x%.2x",
	HIWORD(uuid->time_low), LOWORD(uuid->time_low), uuid->time_mid, 
	uuid->time_hi_and_version, uuid->clock_seq_hi_and_reserved, uuid->clock_seq_low,
	(UCHAR) uuid->node[0], (UCHAR) uuid->node[1],
	(UCHAR) uuid->node[2], (UCHAR) uuid->node[3],
	(UCHAR) uuid->node[4], (UCHAR) uuid->node[5]);

    uuid_string->ReleaseBuffer();

    return theErr;
}

/*
**++
**
**  ROUTINE NAME:       uuid_from_string
**
**--
**/

static int ParseIIDString(
    const char* uuid_string,
    long&       time_low,
    int&        time_mid,
    int&        time_hi_and_version,
    int&        clock_seq_hi_and_reserved,
    int&        clock_seq_low,
    int*        node /*[6]*/)
{
    int count = 0;
#if defined(_OPENWAVE)
// XXXSAB Untested...

    // No sscanf()...
    const char* pCur = uuid_string;
    char* pEnd = NULL;
    unsigned long curVal;

    // Skip leading white space
    pCur += strspn(pCur, " \t");

    curVal = strtoul(pCur, &pEnd, 16);
    if (pEnd == (pCur + 8) && *pEnd == '-') time_low = curVal;
    else return count;
    pCur += 8; ++count;

    curVal = strtoul(pCur, &pEnd, 16);
    if (pEnd == (pCur + 4) && *pEnd == '-') time_mid = curVal;
    else return count;
    pCur += 4; ++count;

    curVal = strtoul(pCur, &pEnd, 16);
    if (pEnd == (pCur + 4) && *pEnd == '-') time_hi_and_version = curVal;
    else return count;
    pCur += 4; ++count;

    curVal = strtoul(pCur, &pEnd, 16);
    if (pEnd == (pCur + 4) && *pEnd == '-')
    {
        clock_seq_hi_and_reserved = (curVal & 0xff00) >> 8;
        clock_seq_low = curVal & 0xff;
    }
    else return count;
    pCur += 4; count += 2;

    curVal = strtoul(pCur, &pEnd, 16);
    if (pEnd == (pCur + 6) && *(pEnd+strspn(pEnd, " \t")) == 0)
    {
        for (int i = 5; i >= 0; ++i, curVal = curVal >> 8)
        {
            node[i] = curVal & 0xff;
        }
    }
    else return count;
    pCur += 4; count += 6;

#else
    count = sscanf(uuid_string, "%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
                   &time_low,
                   &time_mid,
                   &time_hi_and_version,
                   &clock_seq_hi_and_reserved,
                   &clock_seq_low,
                   &node[0], &node[1], &node[2], &node[3], &node[4], &node[5]);
#endif
    return count;
}

HX_RESULT CHXuuid::HXUuidFromString(const char* uuid_string, uuid_tt* uuid)
{
    HX_RESULT theErr = HXR_OK;

    uuid_tt             uuid_new;       /* used for sscanf for new uuid's */
    uuid_p_t            uuid_ptr=0;     /* pointer to correct uuid (old/new) */
    int                 i;

    /*
     * If a NULL pointer or empty string, give the nil UUID.
     */
    if (uuid_string == NULL || *uuid_string == '\0')
    {
	memcpy (uuid, &uuid_g_nil_uuid, sizeof *uuid); /* Flawfinder: ignore */
	return theErr;
    }

    /*
     * check to see that the string length is right at least
     */
    if (strlen ((char *) uuid_string) != UUID_C_UUID_STRING_MAX - 1)
    {
        theErr = HXR_FAILED;
        return theErr;
    }

    /*
     * check for a new uuid
     */
    if (uuid_string[8] == '-')
    {
        long    time_low;
        int     time_mid;
        int     time_hi_and_version;
        int     clock_seq_hi_and_reserved;
        int     clock_seq_low;
        int     node[6];

	// VC1.5's runtime library(lddcew.lib) doesn't have sscanf()!!
#if defined(_WIN16)
	i = wsscanf((char *) uuid_string, "%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
	    (long FAR *)&time_low,
	    (int FAR *)&time_mid,
	    (int FAR *)&time_hi_and_version,
	    (int FAR *)&clock_seq_hi_and_reserved,
	    (int FAR *)&clock_seq_low,
	    (int FAR *)&node[0], (int FAR *)&node[1], (int FAR *)&node[2], 
	    (int FAR *)&node[3], (int FAR *)&node[4], (int FAR *)&node[5]);
#else
        i = ParseIIDString(
            uuid_string,
            time_low,
            time_mid,
            time_hi_and_version,
            clock_seq_hi_and_reserved,
            clock_seq_low,
            node);

#ifdef Commented_out_code_20030421_114057
        i = sscanf((char *) uuid_string, "%8lx-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
            &time_low,
            &time_mid,
            &time_hi_and_version,
            &clock_seq_hi_and_reserved,
            &clock_seq_low,
            &node[0], &node[1], &node[2], &node[3], &node[4], &node[5]);
#endif /* Commented_out_code_20030421_114057 */
#endif

        /*
         * check that sscanf worked
         */
        if (i != UUID_ELEMENTS_NUM)
        {
            theErr = HXR_FAILED;
            return theErr;
        }

        /*
         * note that we're going through this agony because scanf is defined to
         * know only to scan into "int"s or "long"s.
         */
        uuid_new.time_low                   = time_low;
        uuid_new.time_mid                   = time_mid;
        uuid_new.time_hi_and_version        = time_hi_and_version;
        uuid_new.clock_seq_hi_and_reserved  = clock_seq_hi_and_reserved;
        uuid_new.clock_seq_low              = clock_seq_low;
        uuid_new.node[0]                    = node[0];
        uuid_new.node[1]                    = node[1];
        uuid_new.node[2]                    = node[2];
        uuid_new.node[3]                    = node[3];
        uuid_new.node[4]                    = node[4];
        uuid_new.node[5]                    = node[5];

        /*
         * point to the correct uuid
         */
        uuid_ptr = &uuid_new;
    }

    /*
     * copy the uuid to user
     */
    memcpy(uuid, uuid_ptr, sizeof (uuid_tt)); /* Flawfinder: ignore */

    return theErr;
}


/*
**++
**
**  ROUTINE NAME:       uuid_from_string
**
**--
**/

HX_RESULT CHXuuid::HXPack(UINT8* pBuffer, uuid_p_t uuid)
{
    uuid_p_t uuidPacket = (uuid_p_t) pBuffer;

    *uuidPacket = *uuid;
    uuidPacket->time_low = DwToNet(uuid->time_low);
    uuidPacket->time_mid = WToNet(uuid->time_mid);
    uuidPacket->time_hi_and_version = WToNet(uuid->time_hi_and_version);

    return HXR_OK;
}


/*
**++
**
**  ROUTINE NAME:       uuid_from_string
**
**--
**/

HX_RESULT CHXuuid::HXUnpack(uuid_p_t uuid, UINT8* pBuffer)
{
    uuid_p_t uuidPacket = (uuid_p_t) pBuffer;

    *uuid = *uuidPacket;
    uuid->time_low = DwToHost(uuidPacket->time_low);
    uuid->time_mid = WToHost(uuidPacket->time_mid);
    uuid->time_hi_and_version = WToHost(uuidPacket->time_hi_and_version);

    return HXR_OK;
}

/*
**++
**
**  ROUTINE NAME:       uuid_equal
**
**--
**/

HXBOOL CHXuuid::HXIsEqual(uuid_p_t uuid1, uuid_p_t uuid2)
{
    /*
     * Note: This used to be a memcmp(), but changed to a field-by-field compare
     * because of portability problems with alignment and garbage in a UUID.
     */
    if ((uuid1->time_low == uuid2->time_low) && 
	(uuid1->time_mid == uuid2->time_mid) &&
	(uuid1->time_hi_and_version == uuid2->time_hi_and_version) && 
	(uuid1->clock_seq_hi_and_reserved == uuid2->clock_seq_hi_and_reserved) &&
	(uuid1->clock_seq_low == uuid2->clock_seq_low) &&
	(memcmp(uuid1->node, uuid2->node, 6) == 0))
    {
	return ( TRUE );
    }
    else
    {
        return (FALSE);
    }
}


/*****************************************************************************
 *
 *  LOCAL MATH PROCEDURES - math procedures used internally by the UUID module
 *
 ****************************************************************************/

/*
** T I M E _ C M P
**
** Compares two UUID times (64-bit UTC values)
**/
uuid_compval_t CHXuuid::TimeCmp(uuid_ttime_p_t time1, uuid_ttime_p_t time2)
{
    /*
     * first check the hi parts
     */
    if (time1->hi < time2->hi) return (uuid_e_less_than);
    if (time1->hi > time2->hi) return (uuid_e_greater_than);

    /*
     * hi parts are equal, check the lo parts
     */
    if (time1->lo < time2->lo) return (uuid_e_less_than);
    if (time1->lo > time2->lo) return (uuid_e_greater_than);

    return (uuid_e_equal_to);
}

/*
**  UnsignedExtendedMultiply
**
**  Functional Description:
**        32-bit unsigned quantity * 32-bit unsigned quantity
**        producing 64-bit unsigned result. This routine assumes
**        long's contain at least 32 bits. It makes no assumptions
**        about byte orderings.
**
**  Inputs:
**
**        u, v       Are the numbers to be multiplied passed by value
**
**  Outputs:
**
**        prodPtr    is a pointer to the 64-bit result
**
**  Note:
**        This algorithm is taken from: "The Art of Computer
**        Programming", by Donald E. Knuth. Vol 2. Section 4.3.1
**        Pages: 253-255.
**--
**/
void CHXuuid::UnsignedExtendedMultiply(ULONG32 u,ULONG32 v, unsigned64_t* prodPtr)
{
    /*
     * following the notation in Knuth, Vol. 2
     */
    ULONG32      uuid1, uuid2, v1, v2, temp;

    uuid1 = u >> 16;
    uuid2 = u & 0xffff;
    v1 = v >> 16;
    v2 = v & 0xffff;

    temp = uuid2 * v2;
    prodPtr->lo = temp & 0xffff;
    temp = uuid1 * v2 + (temp >> 16);
    prodPtr->hi = temp >> 16;
    temp = uuid2 * v1 + (temp & 0xffff);
    prodPtr->lo += (temp & 0xffff) << 16;
    prodPtr->hi += uuid1 * v1 + (temp >> 16);
}

UINT16 CHXuuid::TrueRandom()
{
    ULONG32 nRand = m_pRand->GetRandomNumber(); 
    return (UINT16)(HI_WORD(nRand) ^ (nRand & RAND_MASK));
}


/*****************************************************************************
 *
 *  LOCAL PROCEDURES - procedures used staticly by the UUID module
 *
 ****************************************************************************/

/*
** N E W _ C L O C K _ S E Q
**
** Ensure *clkseq is up-to-date
**
** Note: clock_seq is architected to be 14-bits (unsigned) but
**       I've put it in here as 16-bits since there isn't a
**       14-bit unsigned integer type (yet)
**/

void CHXuuid::NewClockSeq(UINT16& clkseq)
{
    /*
     * A clkseq value of 0 indicates that it hasn't been initialized.
     */
    if (clkseq == 0)
    {
        /*
         * with a volatile clock, we always init to a random number
         */
        clkseq = (UINT16) m_pRand->GetRandomNumber();
    }

    CLOCK_SEQ_BUMP (&clkseq);
    if (clkseq == 0)
    {
        clkseq = clkseq + 1;
    }
}


/*
 *  Define constant designation difference in Unix and DTSS base times:
 *  DTSS UTC base time is October 15, 1582.
 *  Unix base time is January 1, 1970.
 */
#define uuid_c_os_base_time_diff_lo     0x13814000
#define uuid_c_os_base_time_diff_hi     0x01B21DD2

/*
 * U U I D _ _ G E T _ O S _ T I M E
 *
 * Get OS time - contains platform-specific code.
 */
 
void CHXuuid::GetOSTime(uuid_ttime_t * uuid_ttime)
{
    HXTime		tv;
    unsigned64_t        utc,
                        usecs,
                        os_basetime_diff;

    /*
     * Fill out the HXTime struct with the current time
     */
    gettimeofday(&tv, NULL);

    /*
     * Multiply the number of seconds by the number clunks 
     */
    UnsignedExtendedMultiply ((long) tv.tv_sec, UUID_C_100NS_PER_SEC, &utc);

    /*
     * Multiply the number of milliseconds by the number clunks 
     * and add to the seconds
     */
    UnsignedExtendedMultiply ((long) tv.tv_usec / 1000, UUID_C_100NS_PER_MSEC, &usecs);
    UADD_UVLW_2_UVLW (&usecs, &utc, &utc);

    /*
     * Offset between DTSS formatted times and Unix formatted times.
     */
    os_basetime_diff.lo = uuid_c_os_base_time_diff_lo;
    os_basetime_diff.hi = uuid_c_os_base_time_diff_hi;
    UADD_UVLW_2_UVLW (&utc, &os_basetime_diff, uuid_ttime);
}
