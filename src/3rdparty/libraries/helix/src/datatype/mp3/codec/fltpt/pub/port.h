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

#ifndef O_BINARY
#define O_BINARY 0
#endif


/*--- no kb function unless DOS ---*/

#ifndef KB_OK
    #ifdef __MSDOS__
    #define KB_OK
    #endif
    #ifdef _CONSOLE
    #define KB_OK
    #endif
#endif

#ifdef KB_OK
    #ifdef _MSC_VER
       #pragma warning(disable: 4032)
    #endif
    #include <conio.h>
#else
     static int kbhit()  { return 0; }
     static int getch()  { return 0; }
#endif

/*-- no pcm conversion to wave required 
 if short = 16 bits and little endian ---*/

/* mods 1/9/97 LITTLE_SHORT16 detect */

#ifndef LITTLE_SHORT16
    #ifdef __MSDOS__
        #undef LITTLE_SHORT16
        #define LITTLE_SHORT16
    #endif
    #ifdef WIN32
        #undef LITTLE_SHORT16
        #define LITTLE_SHORT16
    #endif
    #ifdef _M_IX86
        #undef LITTLE_SHORT16
        #define LITTLE_SHORT16
    #endif
#endif



#ifdef LITTLE_SHORT16
  #define cvt_to_wave_init(a)
  #define cvt_to_wave(a, b)  b
#else
  void cvt_to_wave_init(int bits);
  unsigned int cvt_to_wave(void *a, unsigned int b);
#endif
int cvt_to_wave_test(void);

