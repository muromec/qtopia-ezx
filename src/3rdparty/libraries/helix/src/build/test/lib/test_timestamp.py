# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: test_timestamp.py,v 1.2 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Unit tests for Timestamp class."""
import unittest

# Setup basic test module references.
from test import assertEquals, testRoot, testDataPath

# Setup module to test.
from timestamp import Timestamp, TimestampException

# Additional modules needed
import time

# Test Datasets
#
# These datasets assume tests are being run in PST/PDT timezone.
#
# DST ON (-7):
#
format1  = "2007-04-18 18:23:32"
utcF1    = "2007-04-19 01:23:32"
utcF1alt = "2007/04/19 01:23:32"
ticks1   = 1176945812
utc1     = 1176971012
day1     = 18
utcDay1  = 19
cvstime1 = "2007/04/18 18:23:32"

# DST OFF (-8):
format2  = "2007-12-18 22:19:00"
format2alt = "2007/12/18 22:19:00"
utcF2    = "2007-12-19 06:19:00"
ticks2   = 1198045140
utc2     = 1198073940
day2     = 18
utcDay2  = 19

# Date-only format
format3  = "2007-04-18"
ticks3   = 1176879600

class defaultInit( unittest.TestCase ):
    """Simply tests whether a plain Timestamp() gets the current time."""
    def testInitialization( self ):
        # This may "sometimes" fail if Timestamp() and time.time() don't 
        # complete within the same second.
        assertEquals( 'Ticks' ,
                      Timestamp().getLocalTicks(),
                      int(time.time()))
    
class dateOnlyInit( unittest.TestCase ):
    def setUp( self ):
        self.ts = Timestamp( format3 )
        
    def testGetLocalTicks( self ):
        assertEquals( 'Ticks' ,
                      self.ts.getLocalTicks(),
                      ticks3 )
                      
class utc_true_dst_on( unittest.TestCase ):
    """Tests that specifying the init as UTC works."""
    def setUp( self ):
        self.ts = Timestamp( utcF1 , True )
        
    def testGetUTCTicks( self ):
        assertEquals( 'UTC Ticks' ,
                      self.ts.getUTCTicks(),
                      utc1 )
                      
    def testGetLocalTicks( self ):
        assertEquals( 'Local Ticks' ,
                      self.ts.getLocalTicks() ,
                      ticks1 )
                          
    def testGetUTCString( self ):
        assertEquals( 'Date' ,
                      self.ts.getUTCString() ,
                      utcF1 )
                      
    def testGetLocalString( self ):
        assertEquals( 'Date' ,
                      self.ts.getLocalString() ,
                      format1 )
                      
    def testAltSeparator( self ):
        assertEquals( 'Date' ,
                      self.ts.getUTCString( '/' ) ,
                      utcF1alt )
                      
    def testGetUTCDay( self ):
        assertEquals( 'Day of Month' ,
                      self.ts.getUTCDay() ,
                      utcDay1 )
                      
    def testGetLocalDay( self ):
        assertEquals( 'Day of Month' ,
                      self.ts.getLocalDay() ,
                      day1 )
                      
    def testGetCVSTime( self ):
        assertEquals( 'CVS Time',
                      self.ts.getLocalCVSTime() ,
                      cvstime1 )

class utc_false_dst_off( unittest.TestCase ):
    """Tests that specifying the init as UTC works."""
    def setUp( self ):
        self.ts = Timestamp( format2 , False )
        
    def testGetUTCTicks( self ):
        assertEquals( 'UTC Ticks' ,
                      self.ts.getUTCTicks(),
                      utc2 )
                      
    def testGetLocalTicks( self ):
        assertEquals( 'Local Ticks' ,
                      self.ts.getLocalTicks() ,
                      ticks2 )
                          
    def testGetUTCString( self ):
        assertEquals( 'Date' ,
                      self.ts.getUTCString() ,
                      utcF2 )
                      
    def testGetLocalString( self ):
        assertEquals( 'Date' ,
                      self.ts.getLocalString() ,
                      format2 )
                      
    def testAltSeparator( self ):
        assertEquals( 'Date' ,
                      self.ts.getLocalString('/') ,
                      format2alt )
                      
    def testGetLocalDay( self ):
        assertEquals( 'Day of Month' ,
                      self.ts.getLocalDay() ,
                      day2 )
                      
    def testGetUTCDay( self ):
        assertEquals( 'Day of Month' ,
                      self.ts.getUTCDay() ,
                      utcDay2 )
                      
class mathOps( unittest.TestCase ):
    def setUp( self ):
        self.ts1 = Timestamp( format1 )
        self.ts2 = Timestamp()
        self.ts3 = Timestamp( format1 )
        
    def testAdd( self ):
        target = self.ts1.getLocalTicks() + 29000
        self.ts1 = self.ts3 + 29000        
        assertEquals( 'Ticks' ,
                      self.ts1.getLocalTicks() ,
                      target )
        # Make sure ts3 was unchanged by the operation.
        assertEquals( 'Ticks' ,
                      self.ts3.getLocalTicks() ,
                      ticks1 )
                  
    def testSelfIncrement( self ):
        target = self.ts1.getLocalTicks() + 29000
        self.ts1 += 29000
        assertEquals( 'Ticks' ,
                      self.ts1.getLocalTicks() ,
                      target )
                      
    def testSubtract( self ):
        ticks = self.ts1.getLocalTicks() + 30000
        self.ts2 = Timestamp( ticks )
        self.ts1 = self.ts2 - 30000
        assertEquals( 'Ticks' ,
                      self.ts1.getLocalTicks(),
                      ticks1 )
        # Make sure ts2 was unchanged by the operation.
        assertEquals( 'Ticks' ,
                      self.ts2.getLocalTicks() ,
                      ticks )
                      
    def testSelfDecrement( self ):
        ticks = self.ts1.getLocalTicks() + 30000
        self.ts2 = Timestamp( ticks )
        self.ts2 -= 30000
        assertEquals( 'Ticks',
                      self.ts2.getLocalTicks() ,
                      self.ts1.getLocalTicks() )
                      
    def testLTOne( self ):
        assertEquals( '<' ,
                      self.ts1 < self.ts2,
                      True )
    
    def testLTTwo( self ):
        assertEquals( '<' ,
                      self.ts2 < self.ts3,
                      False )
                      
    def testLEOne( self ):
        assertEquals( '<=' ,
                      self.ts1 <= self.ts2,
                      True )
    
    def testLETwo( self ):
        assertEquals( '<=' ,
                      self.ts2 <= self.ts3,
                      False )
                      
    def testGTOne( self ):
        assertEquals( '>' ,
                      self.ts2 > self.ts1,
                      True )
    
    def testGTTwo( self ):
        assertEquals( '>' ,
                      self.ts3 > self.ts2,
                      False )                      

    def testGEOne( self ):
        assertEquals( '>=' ,
                      self.ts2 >= self.ts1,
                      True )
    
    def testGETwo( self ):
        assertEquals( '>=' ,
                      self.ts3 >= self.ts2,
                      False ) 
                      
    def testEQOne( self ):
        assertEquals( '==' ,
                      self.ts2 == self.ts1,
                      False )
    
    def testEQTwo( self ):
        assertEquals( '==' ,
                      self.ts1 == self.ts3,
                      True ) 
                      
    def testNEOne( self ):
        assertEquals( '==' ,
                      self.ts2 != self.ts1,
                      True )
    
    def testNETwo( self ):
        assertEquals( '==' ,
                      self.ts1 != self.ts3,
                      False ) 


# Add tests for new methods. utcticks, localticks, etc.
                      
def getSuites():
    suite1 = unittest.makeSuite( defaultInit , 'test' )
    suite2 = unittest.makeSuite( utc_true_dst_on , 'test' )
    
    suite4 = unittest.makeSuite( utc_false_dst_off , 'test' )
    suite5 = unittest.makeSuite( dateOnlyInit , 'test' )
    suite6 = unittest.makeSuite( mathOps , 'test' )
   
    
    suites = unittest.TestSuite( ( suite1 ,
                                   suite2 , 

                                   suite4 ,
                                   suite5 ,
                                   suite6
                                  )
                               ) 
     
    return suites
