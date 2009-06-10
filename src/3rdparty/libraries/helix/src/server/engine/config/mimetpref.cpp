/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mimetpref.cpp,v 1.5 2005/04/14 19:04:55 bgoldfarb Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
 *       
 * The contents of this file, and the files included with this file, 
 * are subject to the current version of the RealNetworks Public 
 * Source License (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the current version of the RealNetworks Community 
 * Source License (the "RCSL") available at 
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
 * will apply. You may also obtain the license terms directly from 
 * RealNetworks.  You may not use this file except in compliance with 
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable 
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
 * the rights, obligations and limitations governing use of the 
 * contents of the file. 
 *   
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the 
 * portions it created. 
 *   
 * This file, and the files included with this file, is distributed 
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY 
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS 
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET 
 * ENJOYMENT OR NON-INFRINGEMENT. 
 *  
 * Technology Compatibility Kit Test Suite(s) Location:  
 *    http://www.helixcommunity.org/content/tck  
 *  
 * Contributor(s):  
 *   
 * ***** END LICENSE BLOCK ***** */  

#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxresult.h"
#include "hxmon.h"
#include "proc.h"
#include "servreg.h"
#include "mimetpref.h"
#include "dict.h"
#include "hxstrutl.h"

MimeTypesPrefController::MimeTypesPrefController(Process* p)
: m_pProc(p)
{
    m_pProc->pc->registry->SetAsActive("config.MimeTypes", this,
        m_pProc);
}

MimeTypesPrefController::~MimeTypesPrefController()
{
}

/************************************************************************
* IHXActivePropUser::SetActiveStr
*
*    Async request to set string pName to string in pBuffer.
*/
STDMETHODIMP
MimeTypesPrefController::SetActiveStr(const char* pName,
                        IHXBuffer* pBuffer,
                        IHXActivePropUserResponse* pResponse)
{
    Dict* mime_types = m_pProc->pc->mime_type_dict;
    const char* pKey;
    char* pValue;
    const char* pc = pName;
    /*
     * Get to the val under the MimeTypes.  This will be the mimetype and will
     * be our value in the mime_types dict.
     *
     * XXXTDM: what are inputs other than "config.MimeTypes.*"?  Any?
     */
    while (strncasecmp(pc, "MimeTypes.", 10) != 0)
    {
        pc = strchr(pc, '.');
        if (!pc)
        {
            pResponse->SetActiveStrDone(HXR_FAIL, pName, pBuffer, 0, 0);
            return HXR_OK;
        }
        pc++;
    }
    // Found it, advance to next segment
    pc += 10;   // == strlen("MimeTypes.")

    // Should have "<mimetype>.Ext_<n>".  Find end of mimetype.
    const char* pc2 = strrchr(pc, '.');
    if (!pc2)
    {
        pResponse->SetActiveStrDone(HXR_FAIL, pName, pBuffer, 0, 0);
        return HXR_OK;
    }

    /*
     * If we are replacing and old name's value, we need to look up the
     * old value in the registry and use that as a delete key first.
     */
    IHXBuffer* pKeyBuff = 0;
    char* pDelete;
    m_pProc->pc->registry->GetStr(pName, pKeyBuff, m_pProc);
    if (pKeyBuff)
    {
        pKey = (const char*)pKeyBuff->GetBuffer();
        pDelete = (char*)mime_types->remove(pKey);
        delete[] pDelete;
        pKeyBuff->Release();
    }

    /*
     * Now our new value will be what was after MimeTypes in the pName.
     */
    pValue = new char[pc2 - pc + 1];
    memcpy(pValue, pc, pc2 - pc);
    pValue[pc2 - pc] = 0;

    /*
     * Our key for the dict is the value for the reg.
     */
    pKey = (const char*)pBuffer->GetBuffer();
    pDelete = (char*)mime_types->remove(pKey);

    /*
     * If one was already there for this extension, kill it.
     */
    if (pDelete)
    {
        delete[] pDelete;
    }
    mime_types->enter(pKey, pValue);

    pResponse->SetActiveStrDone(HXR_OK, pName, pBuffer, 0, 0);
    return HXR_OK;
}


/************************************************************************
* IHXActivePropUser::DeleteActiveProp
*
*	Async request to delete the active property.
*/
STDMETHODIMP
MimeTypesPrefController::DeleteActiveProp(const char* pName,
                            IHXActivePropUserResponse* pResponse)
{
    Dict* mime_types = m_pProc->pc->mime_type_dict;

    pResponse->DeleteActivePropDone(HXR_OK, pName, 0, 0);
    return HXR_OK;
}


