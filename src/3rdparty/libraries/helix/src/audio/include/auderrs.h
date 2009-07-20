/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: auderrs.h,v 1.2 2004/07/09 18:38:45 hubbe Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
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

#ifndef _AUDERRS_H
#define _AUDERRS_H


//
//  Error codes
#define HX_RESAMPLER_ERROR 		10
#define HX_MIXER_ERROR 			11
#define HX_NO_AUDIO_CLOCK		12  		
#define HX_AUDIO_BUFFER_OVERFLOW 	13  		
#define HX_AUDIO_LATE_BUFFER    	14  		

#define AUDIONOBUFF			10000

/*
 *      Used to identify audio errors.  It is the responsibility of the platform
 *      specific audio device to translate a platform specific error into one of 
 *      these.
 */
typedef enum audio_error
{
        RA_AOE_NOERR = 0,                                       //      No error
        RA_AOE_BADDEVICEID,                             //      device ID out of range (
        RA_AOE_NOTENABLED,                              //      driver failed enable (li
        RA_AOE_ALLOCATED,                               //      device already allocated
        RA_AOE_DEVBUSY,                                 //      device is not closed, pr
        RA_AOE_NOMEM,                                   //      insufficient memory erro
        RA_AOE_NOTSUPPORTED,                    //      function isn't supported (like a
        RA_AOE_BADERRNUM,                               //      error value out of range
        RA_AOE_INVALPARAM,                              //      invalid parameter passed
        RA_AOE_BADFORMAT,                               //      unsupported audio format
        RA_AOE_BADWRITE,                                //      unable to write the requ
        RA_AOE_BADOPEN,                                 //      unable to open audio dev
        RA_AOE_DEVNOTOPEN,                              //      a request has been made 
        RA_AOE_STILLPLAYING,                    //      still something playing (can't c
        RA_AOE_NOBUFFS = AUDIONOBUFF,   //      This indicates the device has no room fo
        RA_AOE_NOMIXDEV,                                //  For Linux, no voxware /dev/m
        RA_AOE_UNDETERMINED,                    //      results of a request are inconcl
        RA_AOE_GENERAL                                  //      General error return (No
}       AUDIOERROR;


#endif  // _AUDERRS_H
