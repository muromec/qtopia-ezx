# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: buildtarget.py,v 1.3 2007/06/13 01:12:50 jfinnecy Exp $ 
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
"""Class hierarchy for Build Targets (such as 'exe', 'dll', etc.). Uses a
factory method to return the appropriate object, and then subclasses behavior
to determine how to do various things, like determining the ouptutName, getting
the appropriate rules for the 'clean:' target, etc.

Most likely, lots of methods in here will be delegates to linker or compiler
implemented methods, or back to generator methods, to get the platform
functionality necessary.

These classes should walk a very fine line, and encapsulate only those
behaviors which will vary according to build target (exe vs. dll, etc.).

Current methods:
    getCleanFunction()  - returns a function that returns a list of lines to
                            run as the Clean: rule.
    getOutputName()     - returns the output name of the target as modified
                            for the build target type.
    getTargetType()     - returns the buildTarget's type ('exe', 'dll', etc.)    
"""
# A lot of this stuff is not only BuildTarget specific but also platform and/or
# generator specific. Eventually, there will need to be a good way to couple
# the various objects together, potentially using a mediator pattern?
import os

import log
log.debug( 'Imported: $Id: buildtarget.py,v 1.3 2007/06/13 01:12:50 jfinnecy Exp $' )

import umake_lib

def getValidTypes():
    return [ 'dll', 'exe', 'lib' , 'nonobj' ]

def createBuildTarget( targetType , platform ):
    """Factory method to create BuildTarget objects."""
    log.trace( 'entry' , [ targetType ] )
    
    target = targetType.lower()    
    if 'exe' == target:
        obj = ExeTarget( platform )        
    elif 'dll' == target:
        obj = DLLTarget( platform )        
    elif 'lib' == target:
        obj = LibTarget( platform )
    elif 'nonobj' == target:
        obj = NonObjectTarget( platform )        
    else:
        # Use getValidTypes() first to avoid this branch.
        obj = None
        log.error( 'Unsupported targetType: %s' % targetType )
        
    log.trace( 'exit' , [ obj ] )
    return obj
        
class BuildTarget:
    def __init__ ( self , platform ):
        log.trace( 'entry' )
        self.platform   = platform
        # Must set targetType in the subclasses below.
        self.targetType = ''
        self.canSign = False
        log.trace( 'exit' )
        
    # Abstract methods. Must implement.
    def getOutputName( self , targetName ):        
        umake_lib.fatal( 'getOutputName() not implemented' )

    def getCleanFunction( self ):
        umake_lib.fatal( 'getCleanFunction() not implemented' )                

    # Public methods.
    def getAllLine( self , objectDir , targets ):
        return 'all: %s %s' % (objectDir, targets)

    def getOutputPath( self , dir , name ):
        return os.path.join( dir , name )
        
    def getTargetType( self ):
        return self.targetType
        
    def isSignable( self ):
        return self.canSign

        
class LibTarget( BuildTarget ):
    def __init__( self , platform ):
        BuildTarget.__init__( self , platform )
        self.targetType = 'lib'
        
    def getCleanFunction( self ):
        return self.platform.link.CleanLIB
        
    def getOutputName( self , targetName ):
        return "%s%s.%s" % (self.platform.library_prefix,
                            targetName,
                            self.platform.library_suffix)                                   
        
        
class ExeTarget( BuildTarget ):
    def __init__( self , platform ):
        BuildTarget.__init__( self , platform )
        self.targetType = 'exe'
        self.canSign = True
        
    def getCleanFunction( self ):
        return self.platform.link.CleanEXE
        
    def getOutputName( self , targetName ):
        if len(self.platform.exe_suffix):
            return "%s.%s" % ( targetName, self.platform.exe_suffix)
        else:
            return targetName
                
        
class DLLTarget( BuildTarget ):
    def __init__( self , platform ):
        BuildTarget.__init__( self , platform )
        self.targetType = 'dll'
        self.canSign = True
        
    def getCleanFunction( self ):
        return self.platform.link.CleanDLL
        
    def getOutputName( self , targetName ):
        return self.platform.versioning.create_dll_name( targetName )
        
        
class NonObjectTarget( BuildTarget ):
    def __init__( self , platform ):
        BuildTarget.__init__( self , platform )
        self.targetType = 'nonobj'
        
    def getCleanFunction( self ):
        return self.platform.cleaner
        
    def getOutputName( self , targetName ):
        return targetName
        
    def getOutputPath( self , dir , name ):
        """getOutputPath( d, n ) --> string
        
        NonObjectTargets want to land in the same directory as where their
        source files exist. So, ignore the dir aspect of the deal.
        """
        return name

    def getAllLine( self , objectDir , targets ):
        return 'all: %s' % targets

   
