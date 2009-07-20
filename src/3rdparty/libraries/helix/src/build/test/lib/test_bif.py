# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: test_bif.py,v 1.3 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Unit tests for BIF class."""
import os
import unittest
import log

# Setup test data references.
from test import assertEquals, testRoot, testDataPath
dataSource = os.path.join( testDataPath , 'bif' , 'test.bif' )

# Setup module to test.
from bif import BIF, BIFException


#
# Test Cases
#
# 1. Using the object before it is setup.
# 2. Trying to setup the object properly.
# 3. Improperly setting up the object.
# 4. Using the object once properly set up.
#
class NoInitTestCase(unittest.TestCase): 
    """This handles the case where people try to use the object before it is
    fully initialized.
    """
    def setUp(self):    
        self.bif = BIF()
               
    def testGetBIFVersion( self ):
        """NoInitTestCase::getBIFVersion()"""
        self.assertRaises( BIFException, self.bif.getBIFVersion )
 
    def testGetBuildID( self ):
        """NoInitTestCase::getBuildID()"""
        self.assertRaises( BIFException, self.bif.getBuildID )
 
    def testGetDataAsFormattedString( self ):
        """NoInitTestCase::getDataAsFormattedString()"""
        self.assertRaises( BIFException, self.bif.getDataAsFormattedString )
        
    def testGetDataAsString( self ):
        """NoInitTestCase::getDataAsString()"""
        self.assertRaises( BIFException, self.bif.getDataAsString )
        
    def testGetDataSource( self ):
        """NoInitTestCase::getDataSource()"""
        assertEquals( 'Data source' , self.bif.getDataSource() , None )
        
    def testGetDefaultCVSDate( self ):
        """NoInitTestCase::getDefaultCVSDate()"""
        self.assertRaises( BIFException, self.bif.getDefaultCVSDate )
        
    def testGetDefaultCVSRoot( self ):
        """NoInitTestCase::getDefaultCVSRoot()"""
        self.assertRaises( BIFException, self.bif.getDefaultCVSRoot )
        
    def testGetDefaultCVSTag( self ):
        """NoInitTestCase::getDefaultCVSTag()"""
        self.assertRaises( BIFException, self.bif.getDefaultCVSTag )

    def testGetDefaultCVSTagType( self ):        
        """NoInitTestCase::getDefaultCVSTagType()"""
        self.assertRaises( BIFException, self.bif.getDefaultCVSTagType )
        
    def testGetDefaultOptions( self ):
        """NoInitTestCase::getDefaultOptions()"""
        self.assertRaises( BIFException, self.bif.getDefaultOptions )
        
    def testGetDefaultProfile( self ):
        """NoInitTestCase::getDefaultProfile()"""
        self.assertRaises( BIFException, self.bif.getDefaultProfile )
        
    def testGetDefaults( self ):
        """NoInitTestCase::getDefaults()"""
        self.assertRaises( BIFException, self.bif.getDefaults )
        
    def testGetDefaultTarget( self ):
        """NoInitTestCase::getDefaultTarget()"""
        self.assertRaises( BIFException, self.bif.getDefaultTarget )
        
    def testGetExpiration( self ):
        """NoInitTestCase::getExpiresTicks()"""
        self.assertRaises( BIFException, self.bif.getExpiration )
        
    def testGetModuleName( self ):
        """NoInitTestCase::getModuleName()"""
        self.assertRaises( BIFException, self.bif.getModuleName, 'module1' )
        
    def testGetModules( self ):
        """NoInitTestCase::getModules()"""
        self.assertRaises( BIFException, self.bif.getModules )
        
    def testAddModule( self ):
        """NoInitTestCase::addModule()"""
        self.assertRaises( BIFException, self.bif.addModule , 'fake_module' )    
            
    def testLoadData( self ):
        """NoInitTestCase::loadData()"""
        self.assertRaises( BIFException, self.bif.loadData )   
        
    def testRemoveModule( self ):
        """NoInitTestCase::removeModule()"""
        self.assertRaises( BIFException, self.bif.removeModule, 'module1' )   
        
    def testSaveData( self ):
        """NoInitTestCase::saveData()"""
        self.assertRaises( BIFException, self.bif.saveData )   
        
    def testSetBuildID( self ):
        """NoInitTestCase::setBuildID()"""
        self.assertRaises( BIFException, self.bif.setBuildID, 'fake_id' )   
        
    def testSetDefaultCVSRoot( self ):
        """NoInitTestCase::setDefaultCVSRoot()"""
        self.assertRaises( BIFException, self.bif.setDefaultCVSRoot , 'fake_root' )   
        
    def testSetDefaultCVSTag( self ):
        """NoInitTestCase::setDefaultCVSTag()"""
        self.assertRaises( BIFException, self.bif.setDefaultCVSTag , 'fake_tag' )   
        
    def testSetDefaultCVSTagType( self ):
        """NoInitTestCase::setDefaultCVSTagType()"""
        self.assertRaises( BIFException, self.bif.setDefaultCVSTagType , 'fake_type' )   
        
class ObjectSetupTestCase( unittest.TestCase ):
    """This tests the setup process of the object."""
    def setUp( self ):
        global dataSource
        self.dataSource = dataSource
        self.bif = BIF()
        
    def testSetDataSource( self ):
        """ObjectSetupTestCase::Data Source Handling"""
        self.bif.setDataSource( self.dataSource )
        assertEquals( 'Data source' ,
                      self.bif.getDataSource() ,
                      self.dataSource )
                      
    def testLoadData( self ):
        """ObjectSetupTestCase::loadData()"""
        # No real check / assert here. Just see if it raises an exception.
        # We could check against the number of modules, but then failure could
        # be happening in getModules() instead of loadData().
        self.bif.setDataSource( self.dataSource )
        self.bif.loadData()
    
                      
class BadDataSourceTestCase( unittest.TestCase ):
    """This handles the case where the object is used with an invalid data
    source attribute.
    """
    def setUp( self ):
        self.bif = BIF()        
        self.bif.setDataSource( 'bogus.bif' )
        
    def testLoadData( self ):
        """BadDataSourceTestCase::loadData()"""
        self.assertRaises( BIFException , self.bif.loadData )
        

class SuccessfulSetupTestCase( unittest.TestCase ):
    """This loads the test.bif data and looks for expected responses."""
    def setUp( self ):
        global dataSource
        self.dataSource = dataSource
        self.bif = BIF()
        self.bif.setDataSource( self.dataSource )
        self.bif.loadData()
    
    def testGetBIFVersion( self ):
        """SuccessfulSetupTestCase::getBifVersion()"""
        assertEquals( 'BIF Version' , self.bif.getBIFVersion() , '1.05' )
        
    def testGetBuildID( self ):
        """SuccessfulSetupTestCase::getBuildID()"""
        assertEquals( 'Build ID' , self.bif.getBuildID() , 'test_build_id' )
        
    def testGetDataAsFormattedString( self ):
        """SuccessfulSetupTestCase::getDataAsFormattedString()"""
        assertEquals( 'Formatted string length' , 
                      len( self.bif.getDataAsFormattedString().split('\n') ) , 
                      32 )
        
    def testGetDataAsString( self ):
        """SuccessfulSetupTestCase::getDataAsString()"""
        assertEquals( 'Data string length' ,
                      len( self.bif.getDataAsString().split('\n') ) ,
                      28 )

    def testGetDataSource( self ):
        """SuccessfulSetupTestCase::getDataSource()"""
        assertEquals( 'Data source' , 
                      self.bif.getDataSource() , 
                      self.dataSource )

    def testGetDefaultCVSDate( self ):
        """SuccessfulSetupTestCase::getDefaultCVSDate()"""
        assertEquals( 'Default CVS date' , 
                      self.bif.getDefaultCVSDate() , 
                      '2006-04-05' )
 
    def testGetDefaultCVSRoot( self ):
        """SuccessfulSetupTestCase::getDefaultCVSRoot()"""
        assertEquals( 'Default CVS root' , 
                      self.bif.getDefaultCVSRoot() , 
                      'test_root' )
        
    def testGetDefaultCVSTag( self ):
        """SuccessfulSetupTestCase::getDefaultCVSTag()"""
        assertEquals( 'Default CVS tag' , 
                      self.bif.getDefaultCVSTag() , 
                      'test_tag' )
        
    def testGetDefaultCVSTagType( self ):        
        """SuccessfulSetupTestCase::getDefaultCVSTagType()"""
        assertEquals( 'Default CVS tag type' , 
                      self.bif.getDefaultCVSTagType() ,
                      'tag' )
        
    def testGetDefaultOptions( self ):
        """SuccessfulSetupTestCase::getDefaultOptions()"""
        assertEquals( 'Default options' , 
                      self.bif.getDefaultOptions() ,
                      'test_option' )
                      
    def testGetDefaultProfile( self ):
        """SuccessfulSetupTestCase::getDefaultProfile()"""
        assertEquals( 'Default profile' ,
                      self.bif.getDefaultProfile() ,
                      'test_profile' )
    
    def testGetDefaults( self ):
        """SuccessfulSetupTestCase::getDefaults()"""
        assertEquals( 'Default object list count' ,
                      len( self.bif.getDefaults() ), 
                      1 )
                      
    def testGetDefaultTarget( self ):
        """SuccessfulSetupTestCase::getDefaultTarget()"""
        assertEquals( 'Default target' , 
                      self.bif.getDefaultTarget() , 
                      'test_default_target' )
                      
    def testGetExpiration( self ):
        """SuccessfulSetupTestCase::getExpiration()"""
        # Invokes a method of the Timestamp class.
        assertEquals( 'Expires ticks' ,
                      self.bif.getExpiration().getLocalTicks() ,
                      1167638400 )

    def testGetModuleName( self ):
        """SuccessfulSetupTestCase::getModuleName()"""
        assertEquals( 'Module name' ,
                      self.bif.getModuleName( 'module2' ) , 
                      'test_name' )

    def testGetModules( self ):
        """SuccessfulSetupTestCase::getModules()"""
        assertEquals( 'Number of modules' ,
                      len( self.bif.getModules() ),
                      3 )
        
    def testSetBuildID( self ):
        """SuccessfulSetupTestCase::setBuildID()"""
        self.bif.setBuildID( 'new_id' )
        assertEquals( 'Setting Build ID' , self.bif.getBuildID() , 'new_id' )
    
    def testSetDefaultCVSRoot( self ):
        """SuccessfulSetupTestCase::setDefaultCVSRoot()"""
        self.bif.setDefaultCVSRoot( 'new_root' )
        assertEquals( 'Setting CVS root' , 
                      self.bif.getDefaultCVSRoot() , 
                      'new_root' )
              
    def testSetDefaultCVSTag( self ):
        """SuccessfulSetupTestCase::setDefaultCVSTag()"""
        self.bif.setDefaultCVSTag( 'new_tag' )
        assertEquals( 'Setting CVS tag' , 
                      self.bif.getDefaultCVSTag() , 
                      'new_tag' )
                      
    def testSetDefaultCVSTagType( self ):
        """SuccessfulSetupTestCase::setDefaultCVSTagType()"""
        self.bif.setDefaultCVSTagType( 'branch' )
        assertEquals( 'Setting CVS type' ,
                      self.bif.getDefaultCVSTagType() ,
                      'branch' )
                                            
    def testAddModule( self ):
        """SuccessfulSetupTestCase::addModule()"""
        modules   = self.bif.getModules()
        newModule = modules[ 0 ]
        newModule.id = 'new_module'
        self.bif.addModule( newModule )
        assertEquals( 'Number of modules' ,
                      len( self.bif.getModules() ),
                      4 )

    def testRemoveModule( self ):
        """SuccessfulSetupTestCase::removeModule()"""
        self.bif.removeModule( 'module1' )
        assertEquals( 'Number of modules' ,
                      len( self.bif.getModules() ),
                      2 )

    def testSaveData( self ):
        """SuccessfulSetupTestCase::saveData()"""
        tempSource = os.path.join( testRoot , 'test-out.bif' )
        self.bif.setDataSource( tempSource )
        self.bif.saveData()
        self.bif.removeModule( 'module1' )
        assertEquals( 'Modules in memory' , 
                      len( self.bif.getModules() ),
                      2 )
        self.bif.loadData()
        # Only 2 modules in test-out because it is deleting the shadow module.
        # Is this desired behavior?
        assertEquals( 'Modules in test-out.bif' ,
                      len( self.bif.getModules() ),
                      2 )
              
                      
class BadDataTestCase( unittest.TestCase ):
    """"Test the methods that take parameters with bad data."""
    def setUp( self ):
        global dataSource
        self.dataSource = dataSource
        self.bif = BIF()
        self.bif.setDataSource( self.dataSource )
        self.bif.loadData()
        
    def testAddModule1( self ):
        """BadDataTestCase::addModule() - type mismatch"""
        module = 'this is a string'
        self.assertRaises( BIFException, self.bif.addModule, module )
        
    def testAddModule2( self ):
        """BadDataTestCase::addModule() - duplicate module"""
        # Try to add a module that's already in the BIF.
        modules = self.bif.getModules()
        module  = modules[0]
        self.assertRaises( BIFException ,
                           self.bif.addModule,
                           module )
        
    def testRemoveModule( self ):
        """BadDataTestCase::removeModule() - no such module"""
        modules = self.bif.getModules()
        module = modules[0]
        module.id = 'bogus_module'
        self.assertRaises( BIFException, self.bif.removeModule , module.id )
        
    def testSetDefaultCVSTagType( self ):
        """BadDataTestCase::setDefaultCVSTagType - invalid cvs tag type"""
        self.assertRaises( BIFException, 
                           self.bif.setDefaultCVSTagType, 
                           'bad_type' )
        

def getSuites():
    suite1 = unittest.makeSuite( NoInitTestCase , 'test' )
    suite2 = unittest.makeSuite( ObjectSetupTestCase , 'test' )
    suite3 = unittest.makeSuite( BadDataSourceTestCase , 'test' )
    suite4 = unittest.makeSuite( SuccessfulSetupTestCase , 'test' )
    suite5 = unittest.makeSuite( BadDataTestCase , 'test' )
    
    suites = unittest.TestSuite( (suite1 , suite2 , suite3 , suite4 , suite5) )
     
    return suites
