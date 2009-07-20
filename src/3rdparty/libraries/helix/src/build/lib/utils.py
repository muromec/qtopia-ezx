# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: utils.py,v 1.4 2007/07/17 00:17:48 jfinnecy Exp $ 
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
"""Miscellaneous static utility methods, not bound to any particular ADT.

    appendStringToFile()   - Appends a string to a file.
    formatHash()           - Formats a hash of data into an attractive string.
    globNoMetaPath()       - glob.glob wrapper that ignores meta chars in path.
    indentString()         - Prepends spaces to a string.
    mkdirTree()            - Makes a directory node, and all parent nodes that
                             don't exist.
    readFile()             - Return the contents of a file as a list of strings.
    readLineFromFile()     - Returns the first line of a file as a string.
    writeStringToFileNow() - Writes a list of strings to a file with explicit
                             flushing (appends '\n' to each line).
"""
import os
import glob

import log
log.debug( 'Imported: $Id: utils.py,v 1.4 2007/07/17 00:17:48 jfinnecy Exp $' )



def appendStringToFile( line , filename ):
    """appendStringToFile(l,f)
    
    Appends the string l to file f.
    """
    fileHandle = None
    try:
        mkdirTree( os.path.dirname( filename ) )
        fileHandle = open( filename , 'a' )
        fileHandle.write( line )
    finally:
        if fileHandle:
            fileHandle.close()
        

def formatHash( name, hash, indent = 0, step = 3, string = ''):
    """formatHash(n, h, indent=0, step=3, string='') --> string
    
    Recursively formats a hash (h) with name (n) into an attractive string.
    The initial indentation can be specified using indent. The amount of
    indentation can be specified using step. String will be prepended with 
    string if it is provided.
    
    Limitations:
    The hash can contain other hashes, lists, tuples, or anything, but hashes
    inside lists or tuples will not be recursively expanded. The root data
    item *must* be a hash.

    TODO: Expand this function to recurse into all built-in data structures.
    """        
    string += indentString( '%s:\n' % name, indent )
    string += indentString( '{\n', indent)            
    indent += step

    for key in hash.keys():
        value = hash[key]
        # If it's a hash, increase the indent by recursing into formatHash.
        if isinstance( value, type( {} ) ):            
            string = formatHash( key , value , indent=indent, string=string )
        else:
            string += indentString( '%s = %s\n' % ( key, value ), indent)

    # We finished this hash, so decrease the indent level and pop off the
    # recursion stack.
    indent -= step
    string += indentString( '}\n', indent )
    
    return string
    

def globNoMetaPath( target ):
    """globNoMetaPath(t) --> list
    
    Wrapper to built-in glob.glob that ignores meta characters in the path
    portion of the search string (like "[" and "]", which are common in 
    internal branch names - and hence, BIF paths and archive paths).
    """
    # Save the working dir.
    origDir   = os.getcwd()
    
    # Extract the path and chdir to it. These functions don't view "[" and "]"
    # as special characters. If it doesn't exist, than glob returns empty list.
    targetDir = os.path.dirname( target )
    if not os.path.exists( targetDir ):
        return []   
        
    os.chdir( targetDir )
    
    # Now glob with the file pattern.
    globList = glob.glob( os.path.basename( target ) )

    # Restore the working dir.
    os.chdir( origDir )

    # Piece together the result files with the directory we peeled off.
    results = []
    for item in globList:
        results.append( os.path.join( targetDir, item ) )
        
    return results
    
    
def indentString( string , indent ):
    """indentString(s, i) --> string
    
    Returns s with an indent of i spaces prepended.
    """
    return  (' ' * indent) + string 

    
def mkdirTree(dir):
    """mkdirTree(d)
    
    Makes the target directory, and any parent node directories required.
    """
    if not dir or os.path.isdir(dir):
        return
        
    mkdirTree(os.path.dirname(dir))
    
    if not os.path.isdir(dir):
        try:
            os.mkdir(dir)
        except OSError:
            if not os.path.isdir(dir):
                raise    
    
                
def readFile( filename ):
    """readFile(f) --> list
    
    Reads the file f and returns the contents as a list of strings.
    """
    contents = []        
    try:
        fileHandle = open( filename , "r" )
        contents = fileHandle.readlines()
    finally:
        if fileHandle:
            fileHandle.close() 
        
    return contents    

    
def readLineFromFile( filename ):
    """readLineFromFile(f) --> string
    
    Returns the first line from file f.
    """
    fileHandle = None
    try:
        fileHandle = open( filename , 'r' )
        line = fileHandle.readline()
    finally:
        if fileHandle:
            fileHandle.close()
        
    return line        
        
    
def writeStringsToFileNow( lines , filename ):
    """writeStringsToFileNow(l,f)
    
    Write the list of strings l to file f, overwriting if f previously existed. 
    Writes one string per line, adds '\n' to end of strings. Uses explicit 
    flushing to get the contents out to disk ASAP. 
    """
    # Uses as many options for flushing the buffer as it can think of. See 
    # Python's fsync() documentation for fd objects returned by os.open().
    try:
        mkdirTree( os.path.dirname( filename ) )
        fileHandle = open( filename , 'w' )
        data = '\n'.join( lines )
        fileHandle.write( data )
        fileHandle.flush()
        os.fsync( fileHandle.fileno() )
    finally:
        if fileHandle:
            fileHandle.close()
