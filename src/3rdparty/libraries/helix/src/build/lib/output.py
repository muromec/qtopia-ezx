# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: output.py,v 1.6 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Override/tee standard output & standard error with style."""

import sys
import log

class OutReplacement:
    def __init__(self, fil):
        self.fil = fil
        self.block_count = 0
        self.fil_list = []

    def __getattr__(self, name):
        return getattr(self.fil, name)

    def Block(self):
        self.block_count = self.block_count + 1

    def UnBlock(self):
        self.block_count = self.block_count - 1

    def AddOutputFile(self, path):
        log.trace( 'entry' , [ path ] )
        fil = open(path, "w")
        self.fil_list.append(fil)
        log.trace( 'exit' )

    def RemoveOutputFile(self, path):
        log.trace( 'entry' , [ path ] )
        rm_fil = None
        log.debug( 'fil_list = %s' % self.fil_list )
        for fil in self.fil_list:
            if fil.name == path:
                rm_fil = fil
                break
        if rm_fil:
            log.debug( 'Removing %s from file list.' % rm_fil )
            self.fil_list.remove(rm_fil)
            rm_fil.close()
        log.trace( 'exit' )
    
    def write(self, text):
        if not self.block_count:
            self.fil.write(text)
            self.fil.flush()

        for fil in self.fil_list:
            fil.write(text)
            if hasattr(fil, "flush"):
                fil.flush()

    def write_blocked(self, text):
        for fil in self.fil_list:
            fil.write(text)
            if hasattr(fil, "flush"):
                fil.flush()

    def flush(self):
        for fil in self.fil_list:
            if hasattr(fil, "flush"):
                fil.flush()
        
## replace sys.stdout with the new class
if not hasattr(sys.stdout, "AddOutputFile"):
    sys.stdout.flush()
    sys.stdout = OutReplacement(sys.stdout)

## Backwards compatibility
OutReplacment = OutReplacement
