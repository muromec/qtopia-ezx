# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: signclient.py,v 1.10 2006/04/24 23:34:02 jfinnecy Exp $ 
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
import os
import sys
import socket
import time

import mainloop
import protocol


class SignClient:
    def __init__(self, host, user, passwd):
        self.host = host
        self.user = user
        self.passwd = passwd
        self.args = {}
        self.signType = 'sign'

    def GetSignedComponent(self, component):
        ## create the sign request message
        message = protocol.CreateMessage( self.signType )
        message.user = self.user
        message.passwd = self.passwd
        message.component = component
        message.args = self.args    
        
        ## connect to signing server
        self.connect()

        ## send message
        self.message_pipe.send(message)

        ## enter the mainloop, which is exited if the connection is
        ## broken or the result message is recieved
        mainloop.main()

        if hasattr(self, "component") and hasattr(self, "status"):
            return self.status, self.component

        return None, None

    def addOption( self , option , value ):
        self.args[option] = value
        
    def setSignType( self , signType ):
        self.signType = signType
        
    def connect(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        ## try to connect 'forever'
        for x in range(0,50):
            try:
                sock.connect((self.host, 6501))
                break
            except socket.error, inst:
                print "SignClient.connect: retrying connection to sign host: '%s'" % (self.host)
		print "Socket Error: %s" % (inst)
                time.sleep(5)


        ## create a message pipe bound to the socket, and add callbacks
        self.message_pipe = protocol.CreateSocketMessagePipe(sock)
        self.message_pipe.set_message_cb(self.message_cb)
        self.message_pipe.set_close_cb(self.close_cb)

    def message_cb(self, message_pipe, message):
        if message.type == "sign return":
            self.component = message.component
            self.status = message.status
            mainloop.done()
        else:
            print "SignClient.message_cb: bad message=\"%s\"" % (message.type)

    def close_cb(self, message_pipe):
        mainloop.done()


## testing
if __name__ == "__main__":
    component = open("/vmlinuz", "rb").read()

    
    sc = SignClient("somewhere", "jpaint", "crap")
    result_component = sc.GetSignedComponent(component)

    if result_component != component:
        print "ERROR!"

    print len(component)
