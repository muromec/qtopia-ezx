# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: dll.py,v 1.2 2006/06/19 23:11:27 jfinnecy Exp $ 
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
"""Module to handle processing related to dll output files.

Classes:
    DLL                - deals with processes related to manipulating a dll.
    DLLException       
"""

import os
import re
import string

import log
log.debug( 'Imported: $Id: dll.py,v 1.2 2006/06/19 23:11:27 jfinnecy Exp $' )
import shell


class DLLException( Exception ):
    def __init__( self , value ):
        self.value = value
        
    def __str__( self ):
        return repr( self.value )        
        
        
class DLL:
    """Class to abstract dll output files and various manipulations that may
    be performed on them.
    
    Methods:
        getBaseAddress()         - get the preferred load address of the dll
        getMemorySize()          - get memory size of dll
        getRoundedMemorySize()   - get memory size rounded up to next 0x10000
    """
    def __init__( self , file ):
        """__init__( f )
        
        Initializes the object with filename f.
        """
        self.__filename = file
        # We set this to true as soon as we know for sure file is a dll.
        self.__isDLL = 0 
               
    def getBaseAddress( self ):
        """getBaseAddress() --> integer
        
        Returns the preferred base address for loading the dll into memory, or
        0 if file is not a dll.
        
        Preconditions: 
            File exists ( or raises DLLException )
        """
        self.__checkObject()
        
        (status, output) = shell.run( 'dumpbin /headers %s' % self.__filename )        
        output = output.split('\n')  
        
        address = 0        
        for l in output:
            if not self.__isDLL:
                if re.match( "File Type: DLL", l):
                    self.__isDLL = 1
            else:
                m = re.match( " *([0-9A-Fa-f]*) image base", l)
                if m:
                    address = string.atoi( m.group(1), 16 )                    
        return address           
        
    def getMemorySize( self ):
        """getMemorySize() --> integer
        
        Returns the memory size of the target file, or 0 if file is not dll.

        Preconditions: 
            File exists ( or raises DLLException )
        """
        self.__checkObject()
        
        (status, output) = shell.run( 'dumpbin %s' % self.__filename )        
        output = output.split('\n')
        
        size  = 0
        for l in output:
            if not self.__isDLL:
                if re.match( "File Type: DLL", l):
                    self.__isDLL = 1                    
            else:
                m=re.match(" *([0-9a-fA-F]*) \\.", l)
                if m:
                    size = size + string.atoi(m.group(1), 16)                                    
        return size
        
    def getRoundedMemorySize( self ):
        """getRoundedMemorySize() --> integer
        
        Returns the memory size of the target file rounded up to the next
        highest 0x10000, or 0 if file is not a dll. This gives a size that
        is in line with the MS linker's requirements of having base addresses
        be multiples of 64k.

        Preconditions: 
            File exists ( or raises DLLException )               
        """
        self.__checkObject()
        size = self.getMemorySize()        
        # Round up to next 0x10000.
        size = (size + 0xffff) & ~0xffff;                
        return size        
        
    def __checkObject( self ):
        if not os.path.exists( self.__filename ):
            error = 'Could not find "%s" in dir "%s"' % ( self.__filename ,
                                                          os.getcwd() )
            log.error( error )
            raise DLLException( error )
