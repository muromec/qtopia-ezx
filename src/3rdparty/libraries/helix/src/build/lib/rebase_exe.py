#!/usr/bin/env python
# -*- Mode: Python -*-
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: rebase_exe.py,v 1.3 2007/07/17 00:17:48 jfinnecy Exp $ 
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
"""Tool to recursively rebase all the DLL's in a given node so that there are
no overlapping DLL's. This tool will also check if a file has DRM signing, and
will attempt to re-sign after rebasing (since rebasing will invalidate the
signature).
"""
import sys
import os
import re
    
import shell
import dll
import basefile
import log

class App:
    """Class to implement rebase and resign behavior.
    
    Methods:
        run()
    """
    def __init__( self , base , targetDir , outputDir , outputFile ):
        """__init__(b,t,o,f)
        
        Instantiates the app object.
            b: base address (in hex)
            t: target dir (root node to process)
            o: output dir (where to drop log files)
            f: output file (file to drop address/size info - goes in output dir)
        """
        self.__base           = eval(base) # Converts hex string to number.
        self.__targetDir      = targetDir
        self.__outputFile     = os.path.join( outputDir, outputFile )
        self.__signVerifyTool = os.path.join( os.environ.get('BUILD_ROOT') , 'bin' , 'gethash.exe' )
        self.__signTool       = os.path.join( os.environ.get('BUILD_ROOT') , 'bin' , 'sign.py' )
        # MS tool, we need to let the environment provide the pathing.
        self.__rebaseTool     = 'rebase.exe'

        # Set up the basefile object.        
        if os.path.exists( self.__outputFile ):
            os.remove( self.__outputFile )            
        self.__bf = basefile.Basefile( self.__outputFile , self.__base )
        self.__bf.lock()
        self.__bf.readData()

                
    def run( self ):
        """run()
        
        Runs the main application logic.
        """        
        self.__buildFileLists()
        self.__rebase()
        self.__resign()
        
        
    def __buildFullFileList( self ):
        """__buildFullFileList()

        Builds the list of files to rebase. Picks up all dll's that are in or
        below the root node.
        """        
        self.__files = []
        # Get full list of files at targetDir location.
        ( files , dummy ) = shell.find( self.__targetDir )
        for file in files:
            # Now filter for only dll's.
            if re.search( '\.dll$' , file.lower() ):
                self.__files.append( file )
        
                
    def __buildFileLists( self ):
        """__buildFileLists()
        
        Builds the lists of files to operate on.
        """
        self.__buildFullFileList()            
        self.__buildSignedFileList()
              
        
    def __buildSignedFileList( self ):
        """__buildSignedFileList()
        
        Builds a list of all files in the file list that are signed.
        """
        self.__signedFiles = []        
        for file in self.__files:
            if self.__isSigned( file ):
                self.__signedFiles.append( file )
               
        
    def __getSizeOfItem( self, item ):
        """__getSizeOfItem(i) --> integer
        
        Returns the size of the dll i.
        """
        myDLL = dll.DLL( item )
        return myDLL.getRoundedMemorySize()
        
    
    def __getNextBase( self , item ):
        """__getNextBase(i) --> integer
        
        Returns the base address to use for dll i.
        """
        return self.__bf.getNextBaseAddress( item )
        
        
    def __isSigned( self , file ):
        """__isSigned(f) --> boolean
        
        Returns true if dll f is signed.
        """
        log.trace( 'entry' , [file] )
        cmd = '%s %s -check' % ( self.__signVerifyTool , file )
        ( ret, dummy ) = shell.run( cmd )
        # The tool returns 0 if the file is properly signed, which shell.run()
        # returns as None. The tool returns 1 if it is not signed.
        if None == ret:
            isSigned = True
        elif 1 == ret:
            isSigned = False
        else:
            log.error( 'Got unexpected result from %s: %s' % 
                    (self.__signVerifyTool , ret ))
        log.trace( 'exit' , [isSigned] )            
        return isSigned
                 
        
    def __rebase( self ):
        """__rebase()
        
        Rebase all the files previously found.
        """
        for item in self.__files:
            self.__rebaseItem( item )
                
            
    def __rebaseFile( self , file , base ):
        """__rebaseFile(f,b)
        
        Execute the command to rebase file f to base address b.
        """
        cmd = '%s -v -b 0x%x %s' % ( self.__rebaseTool , base , file )
        shell.run( cmd )

    
    def __rebaseItem( self , item ):
        """__rebaseItem(i)
        
        Perform steps necessary to rebase dll i.
        """
        size  = self.__getSizeOfItem( item )
        base  = self.__getNextBase( item )
        log.info( "Rebasing %s to address 0x%x (size: 0x%x)" % ( item , base , size ) )
        self.__rebaseFile( item , base )
        self.__recordItem( item , base , size )

        
    def __recordItem( self , item , base , size ):
        """__recordItem(i,b,s)
        
        Record that we rebased dll i with base b and size s.
        """
        self.__bf.recordDLL( item , base , size )
        
        
    def __resign( self ):
        """__resign()
        
        Resign all files that were signed before we rebased them.
        """
        if self.__signedFiles:
            for file in self.__signedFiles:
                log.info( "Signing file %s" % file )
                self.__sign( file )
        

    def __sign( self , file ):
        """__sign(f)
        
        Execute the command to sign dll f.
        """
        cmd = '%s %s' % ( self.__signTool , file )
        (ret, output) = shell.run( cmd )
        if ret:
            log.error("Failed to sign %s: %s" % ( file , output ) )
                  
# End class App.



# Check for error conditions before running.
def run():        
    if len( sys.argv ) < 5:
        print
        print "rebase.py: "
        print
        print "   Recursively processes a target directory, rebasing and,"
        print "   if necessary, resigning, dll's it finds along the way."
        print
        print '\nUsage: rebase.py <base> <target_dir> <output_dir> <list_file>'
        print
        print '     base      : base address to start assigning to dlls; specify'
        print '                  in hex format ("0x<address>")'
        print '     target_dir: root node to recursively process'
        print '     output_dir: location for output logs'
        print '     list_file : name of file to record results (in output_dir)'
        sys.exit(1)
                  
    # Parse command line args.
    base       = sys.argv[1]
    targetDir  = sys.argv[2]
    outputDir  = sys.argv[3]
    outputFile = sys.argv[4]
    
    # Create and run the app.
    app = App( base , targetDir , outputDir , outputFile )
    app.run()
