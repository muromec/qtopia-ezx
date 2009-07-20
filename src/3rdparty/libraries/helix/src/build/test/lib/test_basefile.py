# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: test_basefile.py,v 1.2 2006/06/19 23:11:30 jfinnecy Exp $ 
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
"""Unit tests for Basefile class."""
import os
import shutil
import re
import string

import unittest

# Setup test data references.
from test import assertEquals, testRoot, testDataPath
caseOneFile   = os.path.join( testDataPath , 'basefile' , 'list1.txt' )
caseTwoFile   = os.path.join( testDataPath , 'basefile' , 'list2.txt' )

scratchFile   = os.path.join( testDataPath , 'basefile' , 'scratch.txt' )

# Setup module to test.
from basefile import Basefile, BasefileException

#
# Test Cases
#
#   NoDataLock:  try to use basefile without getting lock.
#   NoDataRead:  try to use basefile before reading data.
#   DataPresent: existing basefile - target dll data exists in file
#   DataOnly:    existing basefile - target dll data exists as only line
#   NotPresent:  existing basefile - target dll data not present
#   NewFile:     basefile not yet created
#   Highest:     dll is highest address in basefile
#
class Highest( unittest.TestCase ):
    """Getting the nextBaseAddress when current DLL is the highest.
    """
    def setUp( self ):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        shutil.copy( caseOneFile , scratchFile )
        
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'server\\access\\auth\\ntauth\\rel32\\svrntauth.dll'
        self.base     = 0x60000000
        self.newBase  = 0x601b0000
        self.basefile.lock()
        self.basefile.readData()
        
    def tearDown( self ):
        self.basefile.unlock()
        
    def testGetNextBaseAddress( self ):
        assertEquals( 'Next base address: ' , 
                      self.basefile.getNextBaseAddress( self.dllName ),
                      self.newBase )        
        
class NoDataLock(unittest.TestCase): 
    """Trying to use the class without getting a file lock.
    """
    def setUp(self):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        shutil.copy( caseOneFile , scratchFile )
        
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'common\\lang\\xml\\rel32\\hxxml.dll'
        self.size     = 0x20000
        self.newSize  = 0x40000
        self.base     = 0x60000000
        self.newBase  = 0x601c0000
        
    def testIsDLLRecorded( self ):
        self.assertRaises( BasefileException , self.basefile.isDLLRecorded , self.dllName )

    def testGetBaseAddressOfFile( self ):
        self.assertRaises( BasefileException , self.basefile.getBaseAddressOfFile , self.dllName )
                      
    def testGetSizeOfFile( self ):
        self.assertRaises( BasefileException , self.basefile.getSizeOfFile , self.dllName )
                      
    def testGetNextBaseAddress( self ):
        self.assertRaises( BasefileException , self.basefile.getNextBaseAddress , self.dllName ) 
       
    def testRecordDLL( self ):
        self.assertRaises( BasefileException , self.basefile.recordDLL , self.dllName , self.newBase , self.newSize ) 
        
    def testReadData( self ):
        self.assertRaises( BasefileException , self.basefile.readData )

class NoDataRead(unittest.TestCase): 
    """Trying to use the class without reading the data.
    """
    def setUp(self):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        shutil.copy( caseOneFile , scratchFile )
        
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'common\\lang\\xml\\rel32\\hxxml.dll'
        self.size     = 0x20000
        self.newSize  = 0x40000
        self.base     = 0x60000000
        self.newBase  = 0x601c0000
        self.basefile.lock()
        
    def tearDown( self ):
        self.basefile.unlock()
        
    def testIsDLLRecorded( self ):
        self.assertRaises( BasefileException , self.basefile.isDLLRecorded , self.dllName )

    def testGetBaseAddressOfFile( self ):
        self.assertRaises( BasefileException , self.basefile.getBaseAddressOfFile , self.dllName )
                      
    def testGetSizeOfFile( self ):
        self.assertRaises( BasefileException , self.basefile.getSizeOfFile , self.dllName )
                      
    def testGetNextBaseAddress( self ):
        self.assertRaises( BasefileException , self.basefile.getNextBaseAddress , self.dllName ) 
       
    def testRecordDLL( self ):
        self.assertRaises( BasefileException , self.basefile.recordDLL , self.dllName , self.newBase , self.newSize ) 
                
                      
class DataPresent(unittest.TestCase): 
    """When there is an existing file and the target dll data is mingled in
    with other data.
    """
    def setUp(self):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        shutil.copy( caseOneFile , scratchFile )
        
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'common\\lang\\xml\\rel32\\hxxml.dll'
        self.size     = 0x20000
        self.newSize  = 0x40000
        self.base     = 0x60000000
        self.newBase  = 0x601c0000
        self.basefile.lock()
        self.basefile.readData()

    def tearDown( self ):
        self.basefile.unlock()

    def testIsDLLRecorded( self ):
        assertEquals( 'isDLLRecorded: ',
                      self.basefile.isDLLRecorded( self.dllName ),
                      1 )

    def testGetBaseAddressOfFile( self ):
        assertEquals( 'Base address: ',
                      self.basefile.getBaseAddressOfFile( self.dllName ),
                      self.base )
                      
    def testGetSizeOfFile( self ):
        assertEquals( 'DLL Size: ',
                      self.basefile.getSizeOfFile( self.dllName ),
                      self.size )
                      
    def testGetNextBaseAddress( self ):
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( self.dllName ) ,
                      self.newBase )
       
    def testRecordDLL( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( 'bogus' ),
                      self.newBase + self.newSize )
                      
    def testNumberOfTimesInFile( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        count = 0
        for line in open( scratchFile , "r" ).readlines():            
            l = string.split(line,";")[0]        
            l = string.replace(l,"\t"," ")
            if l == 'common\\lang\\xml\\rel32\\hxxml.dll 0x601c0000 0x40000 ':
                count += 1                                
        assertEquals( '# of occurences of dll found in file: ',
                      count,
                      1 )
                                      
class DataOnly(unittest.TestCase): 
    """When there is an existing file and the target dll data is the only data.
    """
    def setUp(self):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        shutil.copy( caseTwoFile , scratchFile )
        
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'server\\fs\\adminfs\\rel32\\adminfs.dll'
        self.size     = 0x20000
        self.newSize  = 0x40000
        self.base     = 0x600e0000
        self.newBase  = 0x60000000
        self.basefile.lock()
        self.basefile.readData()
        
    def tearDown( self ):
        self.basefile.unlock()
        
    def testIsDLLRecorded( self ):
        assertEquals( 'isDLLRecorded: ',
                      self.basefile.isDLLRecorded( self.dllName ),
                      1 )

    def testGetBaseAddressOfFile( self ):
        assertEquals( 'Base address: ',
                      self.basefile.getBaseAddressOfFile( self.dllName ),
                      self.base )
                      
    def testGetSizeOfFile( self ):
        assertEquals( 'DLL Size: ',
                      self.basefile.getSizeOfFile( self.dllName ),
                      self.size )
                      
    def testGetNextBaseAddress( self ):
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( self.dllName ) ,
                      self.newBase )
       
    def testRecordDLL( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( 'bogus' ),
                      self.newBase + self.newSize )
                      
    def testNumberOfTimesInFile( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        count = 0
        for line in open( scratchFile , "r" ).readlines():            
            l = string.split(line,";")[0]        
            l = string.replace(l,"\t"," ")
            if l == 'server\\fs\\adminfs\\rel32\\adminfs.dll 0x60000000 0x40000 ':
                count += 1                                
        assertEquals( '# of occurences of dll found in file: ',
                      count,
                      1 )

class NotPresent(unittest.TestCase): 
    """When there is an existing file but the target dll is not present.
    """
    def setUp(self):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        shutil.copy( caseOneFile , scratchFile )
        
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'server\\fs\\adminfs\\rel32\\bogus.dll'
        self.newSize  = 0x40000
        self.newBase  = 0x601c0000
        self.basefile.lock()
        self.basefile.readData()

    def tearDown( self ):
        self.basefile.unlock()
        
    def testIsDLLRecorded( self ):
        assertEquals( 'isDLLRecorded: ',
                      self.basefile.isDLLRecorded( self.dllName ),
                      0 )

    def testGetBaseAddressOfFile( self ):
        self.assertRaises( BasefileException, self.basefile.getBaseAddressOfFile, self.dllName )
                      
    def testGetSizeOfFile( self ):
        self.assertRaises( BasefileException, self.basefile.getSizeOfFile, self.dllName )
                      
    def testGetNextBaseAddress( self ):
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( self.dllName ) ,
                      self.newBase )
       
    def testRecordDLL( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( 'bogus' ),
                      self.newBase + self.newSize )
                      
    def testNumberOfTimesInFile( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        count = 0
        for line in open( scratchFile , "r" ).readlines():            
            l = string.split(line,";")[0]        
            l = string.replace(l,"\t"," ")
            if l == 'server\\fs\\adminfs\\rel32\\bogus.dll 0x601c0000 0x40000 ':
                count += 1                                
        assertEquals( '# of occurences of dll found in file: ',
                      count,
                      1 )                      

class NewFile(unittest.TestCase): 
    """When there is no existing file.
    """
    def setUp(self):
        if os.path.exists( scratchFile ):
            os.remove( scratchFile )
            
        self.basefile = Basefile( scratchFile , 0x60000000 )
        self.dllName  = 'server\\fs\\adminfs\\rel32\\bogus.dll'
        self.newSize  = 0x30000
        self.newBase  = 0x60000000
        self.basefile.lock()
        self.basefile.readData()

    def tearDown( self ):
        self.basefile.unlock()
        
    def testIsDLLRecorded( self ):
        assertEquals( 'isDLLRecorded: ',
                      self.basefile.isDLLRecorded( self.dllName ),
                      0 )

    def testGetBaseAddressOfFile( self ):
        self.assertRaises( BasefileException, self.basefile.getBaseAddressOfFile, self.dllName )
                      
    def testGetSizeOfFile( self ):
        self.assertRaises( BasefileException, self.basefile.getSizeOfFile, self.dllName )
                      
    def testGetNextBaseAddress( self ):
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( self.dllName ) ,
                      self.newBase )
       
    def testRecordDLL( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        assertEquals( 'Base address: ',
                      self.basefile.getNextBaseAddress( 'bogus' ),
                      self.newBase + self.newSize )
                      
    def testNumberOfTimesInFile( self ):
        self.basefile.recordDLL( self.dllName , self.newBase , self.newSize )
        count = 0
        for line in open( scratchFile , "r" ).readlines():            
            l = string.split(line,";")[0]        
            l = string.replace(l,"\t"," ")
            if l == 'server\\fs\\adminfs\\rel32\\bogus.dll 0x60000000 0x30000 ':
                count += 1                                
        assertEquals( '# of occurences of dll found in file: ',
                      count,
                      1 )                     
                      
def getSuites():
    suite0 = unittest.makeSuite( NoDataLock  , 'test' )
    suite1 = unittest.makeSuite( DataPresent , 'test' )
    suite2 = unittest.makeSuite( DataOnly ,    'test' )
    suite3 = unittest.makeSuite( NotPresent ,  'test' )
    suite4 = unittest.makeSuite( NewFile ,     'test' )
    suite5 = unittest.makeSuite( NoDataRead ,  'test' )
    suite6 = unittest.makeSuite( Highest    ,  'test' )
    
    suites = unittest.TestSuite( ( suite0 ,
                                   suite1 , 
                                   suite2 , 
                                   suite3 ,
                                   suite4 ,
                                   suite5 ,
                                   suite6 ,
                               ) )
     
    return suites
