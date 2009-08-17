#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: launcher.py,v 1.2 2006/07/06 19:28:04 jfinnecy Exp $ 
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
#  Contributor(s):  
#  
#  ***** END LICENSE BLOCK ***** 
#
"""Centralized launcher for all Ribosome tools. Sets up logging subsystem,
enforces Ribosome requirements, performs platform-specific checks/fixes, logs 
the initial state of the system, and then runs the requested tool inside a 
unified exception handler.
"""
import os
import sys

buildRoot = os.environ.get('BUILD_ROOT')
if not buildRoot:
    print ('You must set BUILD_ROOT in your environment.')
    sys.exit(1)        
sys.path.insert(0, os.path.join(buildRoot, 'lib'))
sys.path.insert(1, os.path.join(buildRoot, 'umake'))

import log

class Launcher:
    def __init__( self , tool ):
        self.toolName = tool

        
    def run( self ):
        self.__setupLogging()
        
        import version
        log.info( 'Ribosome v%s' % version.getVersionString() )
        
        version.enforcePythonVersion()
        log.info( 'Using Python v%s' % version.Version( sys.version ) )        
        
        self.__platformChecks()

        self.__logInitialState()

        self.__runTool()               

        
    def __logInitialState( self ):
        log.info( 'Log dir: %s' % os.environ.get('RIBOSOME_LOGDIR') )
        log.debug( 'Import path: %s' % sys.path )

        
    def __platformChecks( self ):    
       ## fix broken RedHat Python           
       if sys.platform == 'linux-i386':
            sys.platform = 'linux2'

            
    def __runTool( self ):
        import err
        
        try:
            log.info( 'Running tool: %s' % self.toolName )
            tool = __import__("%s_exe" % self.toolName)
            tool.run()
            
        except KeyboardInterrupt:
            log.info( 'Received keyboard interrupt - aborting.' )
            sys.exit(0)
            
        except SystemExit:
            sys.exit(0)
            
        except:
            e = err.Error()
            e.Set( "You have found a Ribosome bug." )
            e.SetTraceback( sys.exc_info() )
            log.error( e.Text() )
            raise

        else:
            sys.exit(0)            

            
    def __setupLogging( self ):
        os.environ['RIBOSOME_LOGDIR'] = self.getLogDir()
        self.initLogging()
        

    def initLogging( self ):
        log.initLogging( self.getLogDir() , self.toolName )

        
    def getLogDir( self ):
        logDir = os.environ.get('RIBOSOME_LOGDIR')        
        if not logDir:
            logDir = os.path.join( os.getcwd() , 'ribosome_logs' )
        return logDir
        
        
class UnittestLauncher( Launcher ):
    """class UnittestLauncher
    
    Handles setup specific to unittest utilities.
    """
    def __init__( self , tool ):
        # Perform the general setup.
        Launcher.__init__( self , tool )
        
        # Test-specific setup
        self.testRoot = os.environ.get('TEST_ROOT')
        if not self.testRoot:
            print 'Please set TEST_ROOT as your desired working dir for running tests.'
            sys.exit(1)

        sys.path.insert(2, os.path.join(buildRoot, 'test', 'lib'))  

        
    def getLogDir( self ):
        """getLogDir() --> string
        
        Returns the logDir with behavior specific to unittest tools.
        """
        return os.path.join( self.testRoot , 'ribosome_logs' )
        
    
    def initLogging( self ):
        if len(sys.argv) > 1:
            # testdriver <module>
            identifier = 'test_%s' % sys.argv[1]
        else:
            # testsuite
            identifier = 'testsuite'
            
        log.initLogging( self.getLogDir() , identifier )
