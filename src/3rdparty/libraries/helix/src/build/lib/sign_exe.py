#!/usr/bin/env python
# -*- Mode: Python -*-
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: sign_exe.py,v 1.4 2006/08/22 20:58:47 jfinnecy Exp $ 
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

import os
import sys
import string
import getopt    

try:
    drmsign_host = os.environ["DRMSIGN_HOST"]
except KeyError:
    print "[ERROR] set DRMSIGN_HOST."
    sys.exit(1)

import signclient

args = {}
args['options'] = []

def parseArgs():
    global args
    
    argList = []
    try:
        opts , argList = getopt.getopt( sys.argv[1:] , 'o:t:z:' )
    except getopt.GetoptError, e:
        print 'Bad options specified: %s' % e
        usage()
        
    for o, a in opts:
        if '-o' == o:
            args[ 'outFile' ] = a
        if '-t' == o:
            args[ 'signType' ] = a
        if '-z' == o:
            args[ 'options' ].append( a )
        
    if 1 != len( argList ):
        print 'Need to specify target file.'
        print 'argList: %s' % argList
        usage()
    else:
        args[ 'targetFile' ] = argList[0]    
          
    # Set the default arguments.
    if not args.get('outFile'):
        args['outFile'] = args['targetFile']
        
    if not args.get('signType'):
        args['signType'] = 'drmsign'
        
def usage():
    print
    print 'sign [-o output-file] [-t sign-type]'
    print '     [-z name1:value1] [-z name2:value2]... <file>'
    sys.exit(1)
    
def addOptionsToSignClient( sc ):
    global args
    
    for item in args[ 'options' ]:
        ( key , value ) = item.split( ':', 1 )
        sc.addOption( key , value )
    
        
def run():    
    parseArgs()
      
    sourceFile = args['targetFile']
    outputFile = args['outFile']
    signType   = args['signType']
    
    try:
        if os.path.isdir(sourceFile):
            ## Assume mac bundle
            b=os.path.basename(sourceFile)
            if not b:
                b=os.path.basename(os.path.dirname(sourceFile))
            b=string.split(b,".")
            sourceFile = os.path.join(
                sourceFile,
                "Contents",
                "MacOS",
                string.join(b[:-1],".") )

            ## This is ugly and should be fixed
            outputFile = sourceFile
                
        component = open(sourceFile, "rb").read()
    except IOError:
        print "[ERROR]: cannot read file=\"%s\"" % (sourceFile)
        sys.exit(1)
    
    sc = signclient.SignClient(drmsign_host, "", "")
    sc.setSignType( signType )
    addOptionsToSignClient( sc )
    result_status, result_component = sc.GetSignedComponent(component)

    ## check status
    if result_status != 0:
        print "[ERROR]: exit status=\"%s\"" % (str(result_status))
        sys.exit(1)

    ## write signed component
    try:
        open(outputFile, "wb").write(result_component)
    except IOError:
        print "[ERROR]: cannot write file=\"%s\"" % (outputFile)
        sys.exit(1)

    sys.exit(0)
