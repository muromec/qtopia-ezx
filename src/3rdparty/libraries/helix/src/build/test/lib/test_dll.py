# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: test_dll.py,v 1.2 2006/06/19 23:11:30 jfinnecy Exp $ 
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
"""Unit tests for DLL class."""
import os
import unittest

# Setup test data references.
from test import assertEquals, testRoot, testDataPath
goodDataSource = os.path.join( testDataPath , 'dll' , 'some.dll' )
badDataSource  = os.path.join( testDataPath , 'dll' , 'some.txt' )
nullDataSource = os.path.join( testDataPath , 'dll' , 'bogus' )

# Setup module to test.
from dll import DLL, DLLException

#
# Test Cases
#
# 1. Improper target (non-dll)
# 2. Proper target (dll)
# 3. NullTarget (bad filename)
#
class ImproperTarget(unittest.TestCase): 
    """This handles the case where the filename provided to DLL is not a dll.
    All queries against a non-dll should return 0.
    """
    def setUp(self):    
        self.myDll = DLL( badDataSource )
               
    def testGetMemorySize( self ):
        """ImproperTarget::getMemorySize()"""
        assertEquals( 'Memory size: ', 
                      self.myDll.getMemorySize(),
                      0 )
                              
    def testGetRoundedMemorySize( self ):
        """ImproperTarget::getRoundedMemorySize()"""
        assertEquals( 'Rounded memory size: ',
                      self.myDll.getRoundedMemorySize(),
                      0 )

    def testGetBaseAddress( self ):
        """ProperTarget::getBaseAddress()"""
        assertEquals( 'Base address: ',
                      self.myDll.getBaseAddress(),
                      0 )
 
class ProperTarget( unittest.TestCase ):
    """This handles the case where the file is a proper dll.
    """
    def setUp( self ):
        self.myDll = DLL( goodDataSource )
        
    def testGetMemorySize( self ):
        """ProperTarget::getMemorySize()"""
        assertEquals( 'Memory size: ' ,
                      self.myDll.getMemorySize(),
                      8192 )
                      
    def testGetRoundedMemorySize( self ):
        """ProperTarget::getRoundedMemorySize()"""
        assertEquals( 'Rounded size: ' ,
                      self.myDll.getRoundedMemorySize(),
                      65536 )
          
    def testGetBaseAddress( self ):
        """ProperTarget::getBaseAddress()"""
        assertEquals( 'Base address: ',
                      '%x' % self.myDll.getBaseAddress(),
                      '73dc0000' )
        
class NullTarget( unittest.TestCase ):
    """This handles the case where a filename was provided but references no
    file. All queries should raise an exception.
    """
    def setUp( self ):
        self.myDll = DLL( nullDataSource )
        
    def testGetMemorySize( self ):
        """ImproperTarget::getMemorySize()"""
        self.assertRaises( DLLException, self.myDll.getMemorySize )
        
    def testGetRoundedMemorySize( self ):
        """ImproperTarget::getRoundedMemorySize()"""
        self.assertRaises( DLLException , self.myDll.getRoundedMemorySize )

    def testGetBaseAddress( self ):
        """ProperTarget::getBaseAddress()"""
        self.assertRaises( DLLException , self.myDll.getBaseAddress )
                      
def getSuites():
    suite1 = unittest.makeSuite( ImproperTarget , 'test' )
    suite2 = unittest.makeSuite( ProperTarget , 'test' )
    suite3 = unittest.makeSuite( NullTarget , 'test' )
    
    suites = unittest.TestSuite( ( suite1 , suite2 , suite3 ) )
     
    return suites
