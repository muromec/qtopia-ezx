# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: test_version.py,v 1.3 2007/04/30 22:51:13 jfinnecy Exp $ 
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
from version import Version, VersionException


# Test Cases:

# Queries

# Compare: (major , minor , patch)
# 1st Set:
#   == , == , ==
#   == , == , ++
#   == , == , --
#   == , ++ , ==
#   == , ++ , ++
#   == , ++ , --
#   == , -- , ==
#   == , -- , ++
#   == , -- , --
#   
# 2nd Set, Major = ++
# 3rd Set, Major = --
#  

# - Proper setup
# 

class BadVersions( unittest.TestCase ):
    def testBadVersionString( self ):
        self.assertRaises( VersionException , Version, ('blee') )
        
    def testBadVersionZero( self ):
        self.assertRaises( VersionException , Version, ('0.0.0'))
        
    def testBadVersionZero2( self ):
        self.assertRaises( VersionException , Version , ( '0' ))
        
    def testBadVersionFourZero( self ):
        self.assertRaises( VersionException , Version , ( '0.0.0.3' ))
        
    def testBadVersionFour( self ):
        self.assertRaises( VersionException, Version , ( '1.2.3.5-devel' ))
        
    def testNegMajor( self ):
        self.assertRaises( VersionException, Version , ( '-1.3.2' ))
        
    def testMixedNotLast( self ):
        self.assertRaises( VersionException, Version , ( '2.4c1.4' ))
                
class FirstDigitZero( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '0.0.1' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      0 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      0 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      1 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      None )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '0.0.1' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '0.0.1' )
            
class TwoDashAdd( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '3.2.1-devel-test-twodash' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      3 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      2 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      1 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      'devel-test-twodash' )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '3.2.1-devel-test-twodash' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '3.2.1' )
            
class OneDigit( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '2' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      2 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      0 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      0 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      None )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '2.0' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '2.0' )
                      
class TwoDigit( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '6.4-foob' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      6 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      4 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      0 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      'foob' )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '6.4-foob' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '6.4' )
                           
class ThreeDigitPatchZero( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '3.2.0' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      3 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      2 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      0 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      None )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '3.2' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '3.2' )
                               
class MixedVersionNumber( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '2.4.4c1' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      2 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      4 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      4 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      'c1' )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '2.4.4-c1' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '2.4.4' )
                                       
class QueryTestCase( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '2.4.3-devel' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      2 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      4 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      3 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      'devel' )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '2.4.3-devel' )
                      
    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '2.4.3' )
                                   
class QueryTestCaseNoAddendum( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '3.5.2' )
        
    def testGetMajor( self ):
        assertEquals( 'Major version number' ,
                      self.base.getMajor(),
                      3 )
      
    def testGetMinor( self ):
        assertEquals( 'Minor version number' ,
                      self.base.getMinor(),
                      5 )
                      
    def testGetPatch( self ):
        assertEquals( 'Patch level' ,
                      self.base.getPatch(),
                      2 )
              
    def testGetAddendum( self ):
        assertEquals( 'Addendum' ,
                      self.base.getAddendum(),
                      None )
                      
    def testPrintVersion( self ):
        verString = "%s" % self.base
        assertEquals( 'Version string' ,
                      verString,
                      '3.5.2' )

    def testGetVersionNoAddendum( self ):
        verString = self.base.getVersionNoAddendum()
        assertEquals( 'Version string - no add' ,
                      verString,
                      '3.5.2' )
                      
class CompareTestCase( unittest.TestCase ):
    def setUp( self ):
        self.base = Version( '2.4.3-devel' )

    def testEquals( self ):
        assertEquals( 'self == y' , self.base == Version( '2.4.3' ), True )
        
    def testNotEquals( self ):
        assertEquals( 'self != y' , self.base != Version( '2.7.1'), True )
        
    def testEEE( self ):
        assertEquals( 'self < y' , self.base < Version( '2.4.3-test'), False )
       
    def testEEG( self ):
        assertEquals( 'self < y' , self.base < Version( '2.4.5-foobar' ), True )
        
    def testEEL( self ):
        assertEquals( 'self < y' , self.base < Version( '2.4.2' ), False )
        
    def testEGE( self ):
        assertEquals( 'self < y' , self.base < Version( '2.5.3' ), True )
        
    def testEGG( self ):
        assertEquals( 'self < y' , self.base < Version( '2.7.9' ) , True )
        
    def testEGL( self ):
        assertEquals( 'self < y' , self.base < Version( '2.12.1' ) , True )
        
    def testELE( self ):
        assertEquals( 'self < y' , self.base < Version( '2.2.3' ) , False )
        
    def testELG( self ):
        assertEquals( 'self < y' , self.base < Version( '2.2.13' ) , False )
        
    def testELL( self ):
        assertEquals( 'self < y' , self.base < Version( '2.2.0' ) , False )
        
    def testGEE( self ):
        assertEquals( 'self < y' , self.base < Version( '3.4.3-bogus'), True )
        
    def testGEG( self ):
        assertEquals( 'self < y' , self.base < Version( '12.4.12' ), True ) 
                      
    def testGEL( self ):
        assertEquals( 'self < y' , self.base < Version( '12.4.1-blee' ), True ) 
                      
    def testGGE( self ):
        assertEquals( 'self < y' , self.base < Version( '12.8.3' ), True ) 
        
    def testGGL( self ):
        assertEquals( 'self < y' , self.base < Version( '6.8.1' ), True ) 
    
    def testLGG( self ):
        assertEquals( 'self < y' , self.base < Version( '0.8.9' ) , False )

    def testLLG( self ):
        assertEquals( 'self < y' , self.base < Version( '0.1.26'), False ) 
        
    def testLLL( self ):
        assertEquals( 'self < y' , self.base < Version( '0.0.1' ) , False )

def getSuites():
    suite1 = unittest.makeSuite( QueryTestCase , 'test' )
    suite2 = unittest.makeSuite( QueryTestCaseNoAddendum , 'test' )
    suite3 = unittest.makeSuite( CompareTestCase , 'test' )
    suite4 = unittest.makeSuite( OneDigit , 'test' )
    suite5 = unittest.makeSuite( TwoDigit , 'test' )
    suite6 = unittest.makeSuite( ThreeDigitPatchZero , 'test' )
    suite7 = unittest.makeSuite( FirstDigitZero , 'test' )
    suite8 = unittest.makeSuite( BadVersions , 'test' )
    suite9 = unittest.makeSuite( TwoDashAdd , 'test' )
    suite10 = unittest.makeSuite( MixedVersionNumber , 'test' )

    
    
    suites = unittest.TestSuite( (suite1, 
                                  suite2, 
                                  suite3, 
                                  suite4, 
                                  suite5, 
                                  suite6,
                                  suite7,
                                  suite8,
                                  suite9,
                                  suite10) )
     
    return suites
