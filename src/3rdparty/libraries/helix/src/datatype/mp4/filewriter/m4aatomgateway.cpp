/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */
#include "mp4wrtr.h"
#include "m4asm.h"
#include "m4aatomgateway.h"

const char* CMP4FileWriter::zm_pFileMimeTypes[] = {"audio/X-RN-MP4-RAWAU", NULL};
const char* CMP4FileWriter::zm_pFileExtensions[] = {"m4a", "mp4", NULL};
const char* CMP4FileWriter::zm_pFileOpenNames[] = {"MP4 Files (.mp4;.m4a)", NULL};

 
// Build out an stsd entry for a specific stream
HX_RESULT CM4AAtomsGateway::BuildSpecificStsdEntry(CM4AStreamMixer* pM4AStreamMixer,
                                                        CMP4Atom_stsd* pStsd, UINT16 usStreamNum )
{
    if (pM4AStreamMixer)
    {
        return pM4AStreamMixer->StartBuildStsdEntry(pStsd, usStreamNum);
    }
    else
    {
       return HXR_FAIL;
    }
    
}
