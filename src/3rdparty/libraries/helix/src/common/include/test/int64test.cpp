/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: int64test.cpp,v 1.4 2007/07/06 20:43:45 jfinnecy Exp $
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

#include <stdio.h>
#include <stdlib.h>

#include "hxtypes.h"

int main()
{
    //Test default ctors
    INT64 a;

    //Test assignment.
#ifndef _SYMBIAN
    a = -0x11223344556677;
#else
    //symbian doesn't have a native 64-bit type so we can assign
    //a large constant like above;
    a = -0x11223344;
    a = a<<2;
#endif    
    
    //Test copy ctors
    INT64 c = a;

    //Test compares
    if( c!=a )
    {
        fprintf( stderr, "c!=a  FAIL\n" );
        exit(1);
    }
    if( c==a )
    {
    }
    else
    {
        fprintf( stderr, "c==a   FAIL\n" );
        exit(1);
    }
    

    //Test some math stuff.
    INT64 foo = 0xFFFFFFFF; //32-bits of stuff.
    INT64 bar = foo*2; //Too many for 32-bits.
    bar = bar>>1;
    if( bar != foo )
    {
        fprintf( stderr, "bar!=foo  FAIL\n" );
        exit(1);
    }
    bar = foo*2;
    foo = foo<<1;
    if( bar<foo || bar>foo)
    {
        fprintf( stderr, "bar<foo || bar>foo  FAIL\n" );
        exit(1);
    }

    foo = 0xFFFFFFFF;
    bar = foo;
    bar >>= 1;
    bar *= 2;
    bar = bar | 0x3;
    bar = bar & 0xFFFFFFFF;
    if( bar!=foo)
    {
        fprintf( stderr, "and or test failed.  FAIL\n" );
        exit(1);
    }

    if( bar==foo )
    {
    }
    else
    {
        fprintf( stderr, "Should be the same FAIL\n" );
        exit(1);
    }

    //Only 31 because we are testing signed 64-bit nums...
    foo = 0xFFFFFFFF;
    bar = 0xFFFFFFFF;
    bar <<=31;
    foo = foo << 31;
    if( bar != foo )
    {
        fprintf( stderr, "High bit test FAIL\n" );
        exit(1);
    }

    bar = bar >> 31;
    foo = 0xFFFFFFFF;
    if( bar != foo )
    {
        fprintf( stderr, "big shift test  FAIL\n" );
        exit(1);        
    }
    

    foo = 0xFFFFFF;
    bar = 0xFFFFFF;

    bar = bar << 24; //0xFFFFFF000000
    fprintf( stderr, "bar should be 0xFFFFFF000000 is %p%p\n", bar.High(), bar.Low() ); 
    bar = bar & 0xFFFFFF; //0x0
    fprintf( stderr, "bar should be 0x0 is %p%p\n", bar.High(), bar.Low() ); 
    if( bar != 0 )
    {
        fprintf( stderr, "shift-or test FAIL\n" );
        exit(1);
    }
    
    bar = bar + 4;

    bar = 0xABCDEF;
    int nInt = INT64_TO_UINT32(bar);
    if( nInt != 0xABCDEF )
    {
        fprintf( stderr, "Casting failed....\n" ); 
    }
    
    bar = 0x0f;
    fprintf( stderr, "bar should be 0x0f is %p\n", bar.Low()  ); 
    foo = 0xf0;
    fprintf( stderr, "foo should be 0xf0 is %p\n", foo.Low()  ); 
    bar = bar|foo;
    fprintf( stderr, "bar should now be 0xff is %p\n", bar.Low() ); 
    if( bar != 0xff )
    {
        fprintf( stderr, "small OR failed....\n" );
        exit(1);
    }
    
    bar = 0xFF;
    foo = 0xFF00;
    bar = bar << 32; //0xFF 00000000
    bar = foo | bar; //0xFF 0000FF00
    if( bar.Low() != 0xFF00 )
    {
        fprintf( stderr, "Low or failed....\n" );
        exit(1);
    }
    if( bar.High() != 0xFF )
    {
        fprintf( stderr, "High or failed....\n" );
        exit(1);
    }
    
    
    
    fprintf( stderr, "Int 64 test PASSED\n" ); 
    return 0;
}
