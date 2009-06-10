/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: pixres.h,v 1.3 2007/07/06 21:58:27 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _PIXRES_H
#define _PIXRES_H

#include "resid.h"

#define IDS_ERR_PIX_NOTLICENSED         HX_RP_RES_INIT_ID +   0
#define IDS_ERR_PIX_BADEXTENSION        HX_RP_RES_INIT_ID +   1
#define IDS_ERR_PIX_NOCODEC             HX_RP_RES_INIT_ID +   2
#define IDS_ERR_PIX_MISSINGFILE         HX_RP_RES_INIT_ID +   3
#define IDS_ERR_PIX_NOSTART             HX_RP_RES_INIT_ID +   4
#define IDS_ERR_PIX_NOEND               HX_RP_RES_INIT_ID +   5
#define IDS_ERR_PIX_NOXMLEND            HX_RP_RES_INIT_ID +   6
#define IDS_ERR_PIX_NULLTITLE           HX_RP_RES_INIT_ID +   7
#define IDS_ERR_PIX_NULLAUTHOR          HX_RP_RES_INIT_ID +   8
#define IDS_ERR_PIX_NULLCOPYRIGHT       HX_RP_RES_INIT_ID +   9
#define IDS_ERR_PIX_NULLVERSION         HX_RP_RES_INIT_ID +  10
#define IDS_ERR_PIX_BADTIMEFORMAT       HX_RP_RES_INIT_ID +  11
#define IDS_ERR_PIX_BADSTARTTIME        HX_RP_RES_INIT_ID +  12
#define IDS_ERR_PIX_BADPREROLL          HX_RP_RES_INIT_ID +  13
#define IDS_ERR_PIX_NULLURL             HX_RP_RES_INIT_ID +  14
#define IDS_ERR_PIX_URLALLWHITE         HX_RP_RES_INIT_ID +  15
#define IDS_ERR_PIX_BADDURATION         HX_RP_RES_INIT_ID +  16
#define IDS_ERR_PIX_ZERODURATION        HX_RP_RES_INIT_ID +  17
#define IDS_ERR_PIX_NOBITRATE           HX_RP_RES_INIT_ID +  18
#define IDS_ERR_PIX_BADBITRATE          HX_RP_RES_INIT_ID +  19
#define IDS_ERR_PIX_NOWIDTH             HX_RP_RES_INIT_ID +  20
#define IDS_ERR_PIX_NOHEIGHT            HX_RP_RES_INIT_ID +  21
#define IDS_ERR_PIX_UNKNOWNTAG          HX_RP_RES_INIT_ID +  22
#define IDS_ERR_PIX_INVALIDHEAD         HX_RP_RES_INIT_ID +  23
#define IDS_ERR_PIX_NOEFFECTS           HX_RP_RES_INIT_ID +  24
#define IDS_ERR_PIX_NODURNOEFFECT       HX_RP_RES_INIT_ID +  25
#define IDS_ERR_PIX_INVALIDEFFECTS      HX_RP_RES_INIT_ID +  26
#define IDS_ERR_PIX_NOHANDLE            HX_RP_RES_INIT_ID +  27
#define IDS_ERR_PIX_BADHANDLE           HX_RP_RES_INIT_ID +  28
#define IDS_ERR_PIX_NONAME              HX_RP_RES_INIT_ID +  29
#define IDS_ERR_PIX_NULLNAME            HX_RP_RES_INIT_ID +  30
#define IDS_ERR_PIX_BADEFFECT           HX_RP_RES_INIT_ID +  31
#define IDS_ERR_PIX_GENERALERROR        HX_RP_RES_INIT_ID +  32
#define IDS_ERR_PIX_BADASPECTFLAG       HX_RP_RES_INIT_ID +  33
#define IDS_ERR_PIX_UNKHEADATTR         HX_RP_RES_INIT_ID +  34
#define IDS_ERR_PIX_BADATTRIBUTE        HX_RP_RES_INIT_ID +  35
#define IDS_ERR_PIX_MISSINGSTART        HX_RP_RES_INIT_ID +  36
#define IDS_ERR_PIX_MISSINGDURATION     HX_RP_RES_INIT_ID +  37
#define IDS_ERR_PIX_MISSINGCOLOR        HX_RP_RES_INIT_ID +  38
#define IDS_ERR_PIX_BADCOLOR            HX_RP_RES_INIT_ID +  39
#define IDS_ERR_PIX_MISSINGTARGET       HX_RP_RES_INIT_ID +  40
#define IDS_ERR_PIX_MISSINGNAME         HX_RP_RES_INIT_ID +  41
#define IDS_ERR_PIX_MISSINGPACKAGE      HX_RP_RES_INIT_ID +  42
#define IDS_ERR_PIX_BADBOOL             HX_RP_RES_INIT_ID +  43
#define IDS_ERR_PIX_BADWIPEDIR          HX_RP_RES_INIT_ID +  44
#define IDS_ERR_PIX_BADWIPETYPE         HX_RP_RES_INIT_ID +  45
#define IDS_ERR_PIX_BADBGCOLOR          HX_RP_RES_INIT_ID +  46
#define IDS_ERR_PIX_ILLEGALATTR         HX_RP_RES_INIT_ID +  47
#define IDS_ERR_PIX_MISSREQATTR         HX_RP_RES_INIT_ID +  48
#define IDS_ERR_PIX_ROOTNOTFIRST        HX_RP_RES_INIT_ID +  49
#define IDS_ERR_PIX_HEADNOTFIRST        HX_RP_RES_INIT_ID +  50
#define IDS_ERR_PIX_BADATTRVALUE        HX_RP_RES_INIT_ID +  51
#define IDS_ERR_PIX_BADDSTRECT          HX_RP_RES_INIT_ID +  52
#define IDS_ERR_PIX_BADCENTERFLAG       HX_RP_RES_INIT_ID +  53
#define IDS_ERR_PIX_FUTUREVERSION       HX_RP_RES_INIT_ID +  54
#define IDS_ERR_PIX_INCOMPATVERSION     HX_RP_RES_INIT_ID +  55
#define IDS_ERR_PIX_DUPHANDLE           HX_RP_RES_INIT_ID +  56
#define IDS_ERR_PIX_ZEROSIZE            HX_RP_RES_INIT_ID +  57

#define IDS_ERR_GIF_BADBITRATE          HX_RP_RES_INIT_ID + 200
#define IDS_ERR_GIF_BADDURATION         HX_RP_RES_INIT_ID + 201
#define IDS_ERR_GIF_BADPREROLL          HX_RP_RES_INIT_ID + 202
#define IDS_ERR_GIF_BADURL              HX_RP_RES_INIT_ID + 203
#define IDS_ERR_GIF_BADTARGET           HX_RP_RES_INIT_ID + 204
#define IDS_ERR_GIF_BADBGCOLOR          HX_RP_RES_INIT_ID + 205
#define IDS_ERR_GIF_BADRELFLAG          HX_RP_RES_INIT_ID + 206
#define IDS_ERR_GIF_BITRATEZERO         HX_RP_RES_INIT_ID + 207
#define IDS_ERR_GIF_ILLEGALTARGET       HX_RP_RES_INIT_ID + 208
#define IDS_ERR_GIF_BADTIMEFORMAT       HX_RP_RES_INIT_ID + 209
#define IDS_ERR_GIF_UNKPLAYERCOMMAND    HX_RP_RES_INIT_ID + 210
#define IDS_ERR_GIF_NOTARGETBROWSER     HX_RP_RES_INIT_ID + 211
#define IDS_ERR_GIF_GENERALERROR        HX_RP_RES_INIT_ID + 212
#define IDS_ERR_GIF_CORRUPTFILE         HX_RP_RES_INIT_ID + 213

#define IDS_ERR_JPG_BADBITRATE          HX_RP_RES_INIT_ID + 400
#define IDS_ERR_JPG_BADPREROLL          HX_RP_RES_INIT_ID + 401
#define IDS_ERR_JPG_BADDURATION         HX_RP_RES_INIT_ID + 402
#define IDS_ERR_JPG_BADDISPLAYTIME      HX_RP_RES_INIT_ID + 403
#define IDS_ERR_JPG_BADURL              HX_RP_RES_INIT_ID + 404
#define IDS_ERR_JPG_BADTARGET           HX_RP_RES_INIT_ID + 405
#define IDS_ERR_JPG_BADRELFLAG          HX_RP_RES_INIT_ID + 406
#define IDS_ERR_JPG_BITRATEZERO         HX_RP_RES_INIT_ID + 407
#define IDS_ERR_JPG_DURATIONZERO        HX_RP_RES_INIT_ID + 408
#define IDS_ERR_JPG_DISPTIMETOOBIG      HX_RP_RES_INIT_ID + 409
#define IDS_ERR_JPG_ILLEGALTARGET       HX_RP_RES_INIT_ID + 410
#define IDS_ERR_JPG_BADSEEKTIME         HX_RP_RES_INIT_ID + 411
#define IDS_ERR_JPG_UNKPLAYERCOMMAND    HX_RP_RES_INIT_ID + 412
#define IDS_ERR_JPG_NOTARGETBROWSER     HX_RP_RES_INIT_ID + 413
#define IDS_ERR_JPG_NOPROGRESSIVE       HX_RP_RES_INIT_ID + 414
#define IDS_ERR_JPG_GENERALERROR        HX_RP_RES_INIT_ID + 415

#define ERRSTR_PIX_NOTLICENSED          "RealPix: This server is NOT licensed to deliver RealPix streams."
#define ERRSTR_PIX_BADEXTENSION         "RealPix: Image file has unrecognized extension: %s"
#define ERRSTR_PIX_NOCODEC              "RealPix: No codec available to process %s"
#define ERRSTR_PIX_MISSINGFILE          "RealPix: Image file %s does not exist."
#define ERRSTR_PIX_NOSTART              "RealPix: No <imfl> tag present - this might not be a RealPix file."
#define ERRSTR_PIX_NOEND                "RealPix: No </imfl> tag present."
#define ERRSTR_PIX_NOXMLEND             "RealPix: Missing XML tag end in %s"
#define ERRSTR_PIX_NULLTITLE            "RealPix: Title attribute present in head tag but NULL value."
#define ERRSTR_PIX_NULLAUTHOR           "RealPix: Author attribute present in head tag but NULL value."
#define ERRSTR_PIX_NULLCOPYRIGHT        "RealPix: Copyright attribute present in head tag but NULL value."
#define ERRSTR_PIX_NULLVERSION          "RealPix: Version attribute present in head tag but NULL value."
#define ERRSTR_PIX_BADTIMEFORMAT        "RealPix: Unrecognized time format specifier: %s"
#define ERRSTR_PIX_BADSTARTTIME         "RealPix: Invalid start time formatting in <head> tag."
#define ERRSTR_PIX_BADPREROLL           "RealPix: Invalid preroll time formatting in <head> tag."
#define ERRSTR_PIX_NULLURL              "RealPix: URL attribute present in head tag but NULL value."
#define ERRSTR_PIX_URLALLWHITE          "RealPix: URL attribute in <head> contains only whitespace."
#define ERRSTR_PIX_BADDURATION          "RealPix: Invalid duration time formatting in <head> tag."
#define ERRSTR_PIX_ZERODURATION         "RealPix: Zero duration in head tag."
#define ERRSTR_PIX_NOBITRATE            "RealPix: Missing bitrate attribute in head tag."
#define ERRSTR_PIX_BADBITRATE           "RealPix: Bitrate cannot be less than or equal to zero."
#define ERRSTR_PIX_NOWIDTH              "RealPix: Missing width attribute in head tag."
#define ERRSTR_PIX_NOHEIGHT             "RealPix: Missing height attribute in head tag."
#define ERRSTR_PIX_UNKNOWNTAG           "RealPix: Unknown tag: %s"
#define ERRSTR_PIX_INVALIDHEAD          "RealPix: Invalid or missing head tag."
#define ERRSTR_PIX_NOEFFECTS            "RealPix: No effects found."
#define ERRSTR_PIX_NODURNOEFFECT        "RealPix: Duration attribute missing and no valid effects."
#define ERRSTR_PIX_INVALIDEFFECTS       "RealPix: Unexpected error."
#define ERRSTR_PIX_NOHANDLE             "RealPix: Missing handle attribute in %s"
#define ERRSTR_PIX_BADHANDLE            "RealPix: Handle must be greater than zero in %s"
#define ERRSTR_PIX_NONAME               "RealPix: Missing name attribute in %s"
#define ERRSTR_PIX_NULLNAME             "RealPix: Name attribute present, but NULL value in %s"
#define ERRSTR_PIX_BADEFFECT            "RealPix: Missing attribute or formatting error in %s"
#define ERRSTR_PIX_GENERALERROR         "RealPix: General Error."
#define ERRSTR_PIX_BADASPECTFLAG        "RealPix: aspect attribute in <head> must be either true or false"
#define ERRSTR_PIX_UNKHEADATTR          "RealPix: Unknown attribute in <head>: %s"
#define ERRSTR_PIX_BADATTRIBUTE         "RealPix: Unknown attribute in effect: %s"
#define ERRSTR_PIX_MISSINGSTART         "RealPix: Missing start attribute in %s effect"
#define ERRSTR_PIX_MISSINGDURATION      "RealPix: Missing duration attribute in %s effect"
#define ERRSTR_PIX_MISSINGCOLOR         "RealPix: Missing color attribute in %s effect"
#define ERRSTR_PIX_BADCOLOR             "RealPix: Bad value for color attribute in %s effect"
#define ERRSTR_PIX_MISSINGTARGET        "RealPix: Missing target attribute in %s effect"
#define ERRSTR_PIX_MISSINGNAME          "RealPix: Missing name attribute in external effect"
#define ERRSTR_PIX_MISSINGPACKAGE       "RealPix: Missing package attribute in external effect"
#define ERRSTR_PIX_BADBOOL              "RealPix: %s attribute in %s effect must be either true or false"
#define ERRSTR_PIX_BADWIPEDIR           "RealPix: Unknown wipe direction"
#define ERRSTR_PIX_BADWIPETYPE          "RealPix: Unknown wipe type"
#define ERRSTR_PIX_BADBGCOLOR           "RealPix: Invalid background color in <head>"
#define ERRSTR_PIX_ILLEGALATTR          "RealPix: Illegal attribute %s in tag %s"
#define ERRSTR_PIX_MISSREQATTR          "RealPix: Missing required attribute %s in tag %s"
#define ERRSTR_PIX_ROOTNOTFIRST         "RealPix: Root element <imfl> must be first element in file"
#define ERRSTR_PIX_HEADNOTFIRST         "RealPix: <head> element must be the first element inside root element"
#define ERRSTR_PIX_BADATTRVALUE         "RealPix: Bad value for attribute %s in tag %s"
#define ERRSTR_PIX_BADDSTRECT           "RealPix: Destination rect for effect is not contained in display window"
#define ERRSTR_PIX_BADCENTERFLAG        "RealPix: center attribute in <head> must be either true or false"
#define ERRSTR_PIX_FUTUREVERSION        "RealPix: Version %s is a future version not supported by this parser"
#define ERRSTR_PIX_INCOMPATVERSION      "RealPix: %s attribute is not compatible with this language version"
#define ERRSTR_PIX_DUPHANDLE            "RealPix: Duplicate image handle"
#define ERRSTR_PIX_ZEROSIZE             "RealPix: Zero or non-numeric size in image tag"

#define ERRSTR_GIF_BADBITRATE           "GIF: Bad URL-encoded bitrate."
#define ERRSTR_GIF_BADDURATION          "GIF: Bad URL-encoded duration."
#define ERRSTR_GIF_BADPREROLL           "GIF: Bad URL-encoded preroll."
#define ERRSTR_GIF_BADURL               "GIF: Bad URL-encoded url."
#define ERRSTR_GIF_BADTARGET            "GIF: Bad URL-encoded target."
#define ERRSTR_GIF_BADBGCOLOR           "GIF: Bad URL-encoded background color."
#define ERRSTR_GIF_BADRELFLAG           "GIF: Bad URL-encoded reliable flag."
#define ERRSTR_GIF_BITRATEZERO          "GIF: URL-encoded bitrate is zero."
#define ERRSTR_GIF_ILLEGALTARGET        "GIF: URL-encoded target must either be _player or _browser"
#define ERRSTR_GIF_BADTIMEFORMAT        "GIF: Illegal time formatting in URL-encoded seek time."
#define ERRSTR_GIF_UNKPLAYERCOMMAND     "GIF: Unknown player command in URL-encoded url attribute."
#define ERRSTR_GIF_NOTARGETBROWSER      "GIF: Cannot target browser with a player command."
#define ERRSTR_GIF_GENERALERROR         "GIF: General Error."
#define ERRSTR_GIF_CORRUPTFILE          "GIF: %s is unparseable and may be corrupt."

#define ERRSTR_JPG_BADBITRATE           "JPEG: Bad URL-encoded bitrate."
#define ERRSTR_JPG_BADPREROLL           "JPEG: Illegal time formatting in URL-encoded preroll."
#define ERRSTR_JPG_BADDURATION          "JPEG: Illegal time formatting in URL-encoded duration."
#define ERRSTR_JPG_BADDISPLAYTIME       "JPEG: Illegal time formatting in URL-encoded display time."
#define ERRSTR_JPG_BADURL               "JPEG: Bad URL-encoded url."
#define ERRSTR_JPG_BADTARGET            "JPEG: Bad URL-encoded target."
#define ERRSTR_JPG_BADRELFLAG           "JPEG: Bad URL-encoded reliable flag."
#define ERRSTR_JPG_BITRATEZERO          "JPEG: URL-encoded bitrate is zero."
#define ERRSTR_JPG_DURATIONZERO         "JPEG: URL-encoded duration is zero."
#define ERRSTR_JPG_DISPTIMETOOBIG       "JPEG: URL-encoded display time is greater than duration."
#define ERRSTR_JPG_ILLEGALTARGET        "JPEG: URL-encoded target must either be _player or _browser."
#define ERRSTR_JPG_BADSEEKTIME          "JPEG: Illegal time formatting in URL-encoded seek time."
#define ERRSTR_JPG_UNKPLAYERCOMMAND     "JPEG: Unknown player command in url URL encoding."
#define ERRSTR_JPG_NOTARGETBROWSER      "JPEG: Cannot target browser with a player command."
#define ERRSTR_JPG_NOPROGRESSIVE        "JPEG: Progressive JPEGs are not supported."
#define ERRSTR_JPG_GENERALERROR         "JPEG: General Error."

#endif
