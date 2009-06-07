# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: test_datelib.py,v 1.2 2007/04/30 22:51:13 jfinnecy Exp $ 
#   
#  Copyright Notices: 
#   
#  Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
#   
#  Patent Notices: This file may contain technology protected by one or  
#  more of the patents listed at www.helixcommunity.org 
#   
#  1.   The contents of this file, and the files included with this file, 
#  are protected by copyright controlled by RealNetworks and its  
#  licensors, and made available by RealNetworks subject to the current  
#  version of the RealNetworks Public Source License (the "RPSL")  
#  available at  * http://www.helixcommunity.org/content/rpsl unless  
#  you have licensed the file under the current version of the  
#  RealNetworks Community Source License (the "RCSL") available at 
#  http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
#  will apply.  You may also obtain the license terms directly from 
#  RealNetworks.  You may not use this file except in compliance with 
#  the RPSL or, if you have a valid RCSL with RealNetworks applicable 
#  to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
#  the rights, obligations and limitations governing use of the 
#  contents of the file. 
#   
#  2.  Alternatively, the contents of this file may be used under the 
#  terms of the GNU General Public License Version 2 (the 
#  "GPL") in which case the provisions of the GPL are applicable 
#  instead of those above.  Please note that RealNetworks and its  
#  licensors disclaim any implied patent license under the GPL.   
#  If you wish to allow use of your version of this file only under  
#  the terms of the GPL, and not to allow others 
#  to use your version of this file under the terms of either the RPSL 
#  or RCSL, indicate your decision by deleting Paragraph 1 above 
#  and replace them with the notice and other provisions required by 
#  the GPL. If you do not delete Paragraph 1 above, a recipient may 
#  use your version of this file under the terms of any one of the 
#  RPSL, the RCSL or the GPL. 
#   
#  This file is part of the Helix DNA Technology.  RealNetworks is the 
#  developer of the Original Code and owns the copyrights in the 
#  portions it created.   Copying, including reproducing, storing,  
#  adapting or translating, any or all of this material other than  
#  pursuant to the license terms referred to above requires the prior  
#  written consent of RealNetworks and its licensors 
#   
#  This file, and the files included with this file, is distributed 
#  and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
#  WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
#  AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
#  WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
#  FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
#   
#  Technology Compatibility Kit Test Suite(s) Location:  
#     http://www.helixcommunity.org/content/tck 
#   
# Contributor(s):
# 
# ***** END LICENSE BLOCK *****
# 
"""Unit tests for Version class."""
import os
import unittest
import log

# Setup test data references.
from test import assertEquals, testRoot, testDataPath
# dataSource = os.path.join( testDataPath , 'version' , 'test.bif' )

# Setup module to test.
import datelib

# Test Datasets
#
# Coming up with boundry dates will be joyous thanks to the new laws,
# so don't try to hit it exactly.

# DST ON (-7):
#
format1  = "2007-04-18 11:23:32"
ticks1   = 1176920612.0
utc1     = 1176945812.0
asctime1 = "2007-04-18 11:23:32"
cvstime1 = "2007/04/18 11:23:32"
cvs_server_time1 = "2007-04-18 18:23:32"
day1     = 18

format2  = "2007/04/18 11:23:32"
ticks2   = 1176920612.0
utc2     = 1176945812.0

format3  = "2007-04-18"
ticks3   = 1176879600.0
utc3     = 1176904800.0
asctime3 = "2007-04-18 00:00:00"
cvstime3 = "2007/04/18 00:00:00"
cvs_server_time3 = "2007-04-18 07:00:00"
day3     = 18

# DST OFF (-8):
#
format4  = "2007-12-18 22:19:00"
ticks4   = 1198045140.0
utc4     = 1198073940.0
asctime4 = "2007-12-18 22:19:00"
cvstime4 = "2007/12/18 22:19:00"
cvs_server_time4 = "2007-12-19 06:19:00"
day4     = 18

class date_to_ticks( unittest.TestCase ):
    def testFormat1( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_ticks( format1 ),
                      ticks1 )
                      
    def testFormat2( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_ticks( format2 ),
                      ticks2 )

    def testFormat3( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_ticks( format3 ),
                      ticks3 )
                      
    def testFormat4( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_ticks( format4 ),
                      ticks4 )
              
class date_to_gmticks( unittest.TestCase ):
    def testFormat1( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_gmticks( format1 ),
                      utc1 )
                      
    def testFormat2( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_gmticks( format2 ),
                      utc2 )

    def testFormat3( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_gmticks( format3 ),
                      utc3 )
                      
    def testFormat4( self ):
        assertEquals( 'Ticks',
                      datelib.date_to_gmticks( format4 ),
                      utc4 )                     
                      
class asctime( unittest.TestCase ):
    def testFormat1( self ):
        assertEquals( 'String',
                      datelib.asctime( ticks1 ),
                      asctime1 )
                      
    def testFormat3( self ):
        assertEquals( 'String',
                      datelib.asctime( ticks3 ),
                      asctime3 )
                      
    def testFormat4( self ):
        assertEquals( 'String',
                      datelib.asctime( ticks4 ),
                      asctime4 )
                      
class cvstime( unittest.TestCase ):
    def testFormat1( self ):
        assertEquals( 'String',
                      datelib.cvstime( ticks1 ),
                      cvstime1 )
                      
    def testFormat3( self ):
        assertEquals( 'String',
                      datelib.cvstime( ticks3 ),
                      cvstime3 )
                      
    def testFormat4( self ):
        assertEquals( 'String',
                      datelib.cvstime( ticks4 ),
                      cvstime4 )

class cvs_server_time( unittest.TestCase ):
    def testFormat1( self ):
        assertEquals( 'String',
                      datelib.cvs_server_time( ticks1 ),
                      cvs_server_time1 )
                      
    def testFormat3( self ):
        assertEquals( 'String',
                      datelib.cvs_server_time( ticks3 ),
                      cvs_server_time3 )
                      
    def testFormat4( self ):
        assertEquals( 'String',
                      datelib.cvs_server_time( ticks4 ),
                      cvs_server_time4 )

class get_day( unittest.TestCase ):
    def testFormat1( self ):
        assertEquals( 'Day',
                      datelib.get_day( ticks1 ),
                      day1 )
                      
    def testFormat3( self ):
        assertEquals( 'Day',
                      datelib.get_day( ticks3 ),
                      day3 )
                      
    def testFormat4( self ):
        assertEquals( 'Day',
                      datelib.get_day( ticks4 ),
                      day4 )

class asctime_from_seconds( unittest.TestCase ):
    def testOne( self ):
        assertEquals( 'asctime',
                     datelib.asctime_from_seconds( 134 ),
                     '0:02.14' )
         
    def testTwo( self ):
        assertEquals( 'asctime',
                     datelib.asctime_from_seconds( 3452345 ),
                     '958:59.05' )
   
def getSuites():
    suite1 = unittest.makeSuite( date_to_ticks , 'test' )
    suite2 = unittest.makeSuite( date_to_gmticks , 'test' )
    suite3 = unittest.makeSuite( asctime , 'test' )
    suite4 = unittest.makeSuite( cvstime , 'test' )
    suite5 = unittest.makeSuite( cvs_server_time , 'test' )
    suite6 = unittest.makeSuite( get_day , 'test' )
    suite7 = unittest.makeSuite( asctime_from_seconds , 'test' )

    
    
    suites = unittest.TestSuite( ( suite1 ,
                                   suite2 ,
                                   suite3 ,
                                   suite4 ,
                                   suite5 ,
                                   suite6 ,
                                   suite7 )
                               ) 
     
    return suites
