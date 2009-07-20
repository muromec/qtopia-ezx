# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: testsuite_exe.py,v 1.3 2007/07/17 00:17:48 jfinnecy Exp $ 
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
"""Driver to run all unit tests in Ribosome project and report summary. If
errors are found, use the testdriver.py driver to test specific modules and
get more verbose reporting.
"""
import os
import re

def run():
    buildRoot = os.environ.get('BUILD_ROOT')
    libPath = os.path.join( buildRoot , 'lib' )
    umakePath = os.path.join( buildRoot , 'umake' )
    binPath = os.path.join( buildRoot , 'bin' )
    testPath = os.path.join( buildRoot , 'test' , 'lib' )
    testDriver = os.path.join( binPath , 'testdriver.py' )
        
    # Get list of all modules in lib and umake.
    modules = os.listdir( libPath )
    modules.extend(os.listdir( umakePath ))
    
    # Reporting variables for end of run.
    errorList = []
    modCount  = 0
    testCount = 0
    
    for module in modules:
        # Check only *.py files.    
        if re.match( '^.*\.py$' , module ):
            modCount += 1
            print ('%-35s' %  ( 'test_' + module + '...' )),
            # If a test module exists for a particular .py.
            if os.path.exists( os.path.join( testPath , 'test_%s' % module )):
                testCount += 1
                # Run the test.
                modName = re.sub( '\.py' , '' , module )
                cmd     = "%s %s" % ( testDriver , modName )
                proc    = os.popen( cmd , 'r')
                results = proc.close()
                if results:
                   print( '* FAILED *' )
                   errorList.append( module )
                else:
                   print( 'ok' )
            else:
                # Report no test module.
                print ( 'skip' )        
    
    print
    print ('%s of %s modules tested.' % ( testCount , modCount ) ),
    
    # Re-report on failed modules with number of errors.
    if errorList:
        print  
        print
        print ('***')
        print ('*' )
        print ('*  UNIT TESTS FAILED!' )
        print ('*' )
        print ('***')
        print
        print ('The following modules had errors:' )
        print
        for item in errorList:
            print item
        print
    else:
        print ('All unit tests passed.')    
