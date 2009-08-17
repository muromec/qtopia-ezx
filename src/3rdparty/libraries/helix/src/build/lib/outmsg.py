# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: outmsg.py,v 1.3 2006/04/24 23:34:02 jfinnecy Exp $ 
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
"""Global message output system.  This is really a little messy, but
its hard to fix at this point."""


class OutputHook:
    def __init__(self):
        self.block_output_count = 0
        self.hook = None


    def set_hook(self, hook):
        self.hook = hook
        

    def block(self):
        self.block_output_count = self.block_output_count + 1


    def unblock(self):
        self.block_output_count = self.block_output_count + 1


    def write(self, text):
        self.P_write(text)


    def P_write(self, text):
        ## return if output is blocked
        if self.block_output_count:
            return
        
        if self.hook:
            self.hook(text)
        else:
            print text


class SendOutputHook(OutputHook):
    pass


class ErrorOutputHook(OutputHook):
    def write(self, text):
        self.P_write('ERROR: %s' % (text))


class DebugOutputHook(OutputHook):
    def write(self, text):
        self.P_write('DEBUG: %s' % (text))


class VerboseOutputHook(OutputHook):
    pass


## outmsg instance
_send = SendOutputHook()
_error = ErrorOutputHook()
_debug = DebugOutputHook()
_verbose = VerboseOutputHook()


## entrypoints
def set_send_hook(hook):
    _send.set_hook(hook)


def set_error_hook(hook):
    _error.set_hook(hook)


def set_debug_hook(hook):
    _debug.set_hook(hook)


def set_verbose_hook(hook):
    _verbose.set_hook(hook)
    

def send(*args):
    if len(args) == 1:
        _send.write(args[0])
    elif len(args) == 2:
        mode = args[0]
        text = args[1]
        
        if mode == 'e':
            _error.write(text)
        elif mode == 'd':
            _debug.write(text)
        elif mode == 'v':
            _verbose.write(text)
        else:
            _send.write(text)
        

def error(text):
    _error.write(text)


def debug(text):
    _debug.write(text)


def verbose(text):
    _verbose.write(text)



