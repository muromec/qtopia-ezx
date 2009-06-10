/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: vsrcinfo.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _VIEWINFO_H_
#define _VIEWINFO_H_

void 
QueueModificationTime(CBigByteGrowingQueue* pQ, UINT32 ulModTime);
void
QueueFileSize(CBigByteGrowingQueue* pQ, UINT32 ulFileSize);

#define z_pImage_ss		"<img src=\"%s\" align=\"RIGHT\" alt=\"%s\" border=\"0\">"

#define z_pSMILHeader		"</font>\n"\
"<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"100%\">\n"\
"<tr>\n"\
"  <td valign=\"top\">\n"\
"    <font face=\"arial, helvetica, sans-serif\">\n"

#define z_pSMILTrailer		"\n"\
"  </td>\n"\
"  <td align=\"right\">\n"\
"    <table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"\
"      <tr>\n"\
"        <td align=\"right\">\n"\
"          <font face=\"Arial, Helvetica, sans-serif\">\n"\
"            <strong>SMIL</strong>\n"\
"          </font>\n"\
"        </td>\n"\
"      </tr>\n"\
"      <tr>\n"\
"        <td align=\"right\">\n"\
"          <font face=\"arial, helvetica, sans-serif\" size=\"-1\"> \n"\
"            <a href=\"http://www.w3.org/AudioVideo/\">\n"\
"                  <strong>S</strong>ynchronized \n"\
"                  <strong>M</strong>ultimedia \n"\
"                  <strong>I</strong>ntegration \n"\
"                  <strong>L</strong>anguage</a>\n"\
"          </font>\n"\
"        </td>\n"\
"      </tr>\n"\
"      <tr>\n"\
"        <td align=\"right\">\n"\
"          <font face=\"arial, helvetica, sans-serif\" size=\"-2\">\n"\
"            A <a href=\"http://www.w3.org/\">W3C</a> Recommendation\n"\
"          </font>\n"\
"        </td>\n"\
"      </tr>\n"\
"      <tr>\n"\
"        <td align=\"right\">&nbsp;</td>\n"\
"      </tr>\n"\
"      <tr>\n"\
"        <td align=\"right\">&nbsp;</td>\n"\
"      </tr>\n"\
"    </table>\n"\
"  </td>\n"\
"</tr>\n"\
"</table>\n"\
"\n"


#define z_pSMILHREF 		"<a href=\"http://www.w3.org/AudioVideo/\">"
#define z_pSMILImageLoc		"http://www.w3.org/Icons/WWW/w3c_home"
#define z_pSMILAltText 		"Synchronized Multimedia Integration Language, a W3C Recommendation"

// Image paths
#define z_pAudioGif		"realaudio.gif"
#define z_pVideoGif		"realvideo.gif"
#define z_pJPEGGif		"realpix.gif"
#define z_pGIFGif		"realpix.gif"
#define z_pFlashGif		"reallogo.gif"
#define z_pRealTextGif		"realtext.gif"
#define z_pRealPixGif		"realpix.gif"
#define z_pSMILGif		"realsmil.gif"

#define z_pAudioName		"RealAudio"
#define z_pVideoName		"RealVideo"
#define z_pGIFName		"GIF Image"
#define z_pJPEGName		"JPEG Image"
#define z_pSMILName		"SMIL"
#define z_pRealTextName		"RealText"
#define z_pRealPixName		"RealPix"
#define z_pRAMName		"RAM File"

// defaults - for remote links
#define z_pDefaultRemoteMount	"/viewsource/template.html"
#define z_pDefaultRemotePort	"8080"

// pxjpgff & pxgifff specific
#define z_pImageType		"<strong>Image Type: </strong>"
#define z_pColorTableBits_i	"<strong>Number of Bits in Color Table: </strong> %i"
#define z_pJPEGComponents_i	"<strong>Number of JPEG Components: </strong> %i"
#define z_pJPEGProgressive	"<strong>Progressive JPEG: </strong>"
#define z_pImageDimen_ii	"<strong>Dimensions: </strong> %i x %i pixels"

// Common
#define z_pEndLine		"<br>\n"
#define z_pListOpen		"<ul>"
#define z_pListClose		"</ul>"
#define z_pListItem		"<li><font face=\"Arial, Helvetica, sans-serif\">"
#define z_pCloseItem		"</font>"

#define z_pOpen			"<font face=\"Arial, Helvetica, sans-serif\">\n"
#define z_pClose		"</font>\n"

#define z_pStream		"<strong>Stream:</strong> "
#define z_pFileName		"<strong>File Name:</strong> "
#define z_pLastModified		"<strong>Last Modified:</strong> "
#define z_pFileSize		"<strong>File Size:</strong> "

#define z_pFileLive		"<strong>Live Feed</strong>"
#define z_pRamLink		"<strong>Stream to RealPlayer: </strong>"

#define z_pPlusURLHead_s	"<h2>ViewSource for %s</h2>\n"
#define z_pPlusURLItem_sss	"<li><a href=\"%s?%s\">%s</a>\n"
		    

// Streams: ( rminfo )
#define z_pTitle		"<strong>Title:</strong> "
#define z_pAuthor		"<strong>Author:</strong> "
#define z_pCopyright		"<strong>Copyright:</strong> "

#define z_pFileDuration_iii	"<strong>Duration:</strong> %.2i:%.2i.%i "
#define z_pFileBufferTime_f	"<strong>Buffer Time:</strong> %.1f seconds"
#define z_pFileMaxBitRate_f	"<strong>Max Bit Rate:</strong> %.1f Kbps"
#define z_pFileAvgBitRate_f	"<strong>Avg Bit Rate:</strong> %.1f Kbps"

#define z_pPerfectPlay_en	"<strong>Perfect Play:</strong> enabled"
#define z_pPerfectPlay_dis	"<strong>Perfect Play:</strong> disabled"
#define z_pRecording_off	"<strong>Allow Recording:</strong> off"
#define z_pRecording_on		"<strong>Allow Recording:</strong> on"
#define z_pDownload_off		"<strong>Allow Download:</strong> off"
#define z_pDownload_on		"<strong>Allow Download:</strong> on"
#define z_pCompatibility	"<strong>Player Compatibility:</strong> "

// stream specific stuff - ( rminfo )
#define z_pStreamNum_i		"<strong>Stream: %i</strong> "
#define z_pMimeType		"<strong>MIME type: </strong>"
#define z_pMaxBitRate_f		"<strong>Max Stream Bit Rate: </strong>%.1f Kbps"
#define z_pAvgBitRate_f		"<strong>Avg Stream Bit Rate: </strong>%.1f Kbps"
#define z_pBitRate_f		"<strong>Bit Rate: </strong>%.1f Kbps"
#define z_pDuration_iii		"<strong>Duration:</strong> %.2i:%.2i.%i "
#define z_pBufferTime_f		"<strong>Buffer Time:</strong> %.1f seconds"
#define z_pGeneric_u		"<strong>%s: </strong>%u"
#define z_pGeneric_s		"<strong>%s: </strong>%s"		

#define z_pACodec		"<strong>Audio Codec:</strong> "
#define z_pAudioCodecs		"<strong>SureStream Audio Codecs: </strong>"
#define z_pSampleRate_i		" %i Khz"
#define z_pASMPairings		"<strong>Stream Pairings</strong>"
#define z_pOldPNMCompatable	" - Backwards Compatibility Stream"
#define z_pHTTPStream	    	" - for HTTP streaming"
#define z_pVCodec		"<strong>Video Codec:</strong> "
#define z_pVideoCodecs		"<strong>SureStream Video Codecs: </strong>"
#define z_pEncFrameRate_s	"<strong>Encoded Frame Rate: </strong>%s fps"
#define z_pFrameRate_i		"<strong>Encoded Frame Rate: </strong>%i fps"
#define z_pDimensions_ii	"<strong>Dimensions: </strong>%ix%i"
#define z_pXMLSource_s		"<strong>%s source: </strong>"
#define z_pUnknownCodecs        "<strong>SureStream Bit Rates: </strong>"

// Flash
#define z_pFileVersion		"<strong>File Version:</strong> "

#define z_pNotRealDataType	"<h1>Unrecognized file Format.</h1>"

// View Index Files
#define z_pMountPointName	"<strong>Browsing: </strong>"
#define z_pIndexHeaderRamGen	"        Info  File Name                      Play\n"
#define z_pIndexHeader		"        Info  File Name"

// Tags used in xmlesc.cpp
#define tag_BEGIN_TAG		"<strong>"
#define tag_END_TAG		"</strong>"
#define tag_BEGIN_TAG_NAME	"<font face=\"arial, helvetica, sans-serif\" color=\"#551A8B\">"
#define tag_END_TAG_NAME	"</font>"
#define tag_BEGIN_ATTRIBUTE	"<font face=\"arial, helvetica, sans-serif\" color=\"#003E98\">"
#define tag_END_ATTRIBUTE	"</font>"
#define tag_BEGIN_BROKEN_ATT	"<font face=\"arial, helvetica, sans-serif\" color=\"#0000FF\"><blink>"
#define tag_END_BROKEN_ATT	"</blink></font>"
#define tag_BEGIN_COMMENT	"<em><font face=\"arial, helvetica, sans-serif\" color=\"black\">"
#define tag_END_COMMENT		"</font></em>"
#define tag_BEGIN_AMPERSAND	"<font face=\"arial, helvetica, sans-serif\" color=\"#2F4F2F\">"
#define tag_END_AMPERSAND	"</font>"
#define tag_BEGIN_HREF		"<A href=\""
#define tag_END_HREF		"</A>"
#define tag_PROCESSING_INSTRUCTIONS "<strong><font face=\"arial, helvetica, sans-serif\" color=\"green\">"
#define tag_END_PI		"</font></B>"
#define tag_BEGIN_CDATA		"<font face=\"Arial, Helvetica, sans-serif\" color=\"Fuchsia\">"
#define tag_END_CDATA		"</font>"
#define tag_BEGIN_DTD		"<font face=\"Arial, Helvetica, sans-serif\" color=\"red\">"
#define tag_END_DTD		"</font>"

#define style_BEGIN_TAG		"<span class=\"TagMarkup\">"
#define style_END_TAG		"</span>"
#define style_BEGIN_TAG_NAME	"<span class=\"TagNameMarkup\">"
#define style_END_TAG_NAME	"</span>"
#define style_BEGIN_ATTRIBUTE	"<span class=\"AttributeValueMarkup\">"
#define style_END_ATTRIBUTE	"</span>"
#define style_BEGIN_BROKEN_ATT	"<span class=\"BrokenAttributeMarkup\">"
#define style_END_BROKEN_ATT	"</span>"
#define style_BEGIN_COMMENT	"<span class=\"CommentMarkup\">"
#define style_END_COMMENT	"</span>"
#define style_BEGIN_AMPERSAND	"<span class=\"AmpersandThingyMarkup\">"
#define style_END_AMPERSAND	"</span>"
#define style_BEGIN_HREF	"<a href=\""
#define style_END_HREF		"</a>"
#define style_PROCESSING_INSTRUCTIONS "<span class=\"ProcessingInstructionsMarkup\">"
#define style_END_PI		"</span>"
#define style_BEGIN_CDATA	"<span class=\"CDATAMarkup\">"
#define style_END_CDATA		"</span>"
#define style_BEGIN_DTD		"<span class=\"DocumentTypeDeffinitionMarkup\">"
#define style_END_DTD		"</span>"



#endif // _VIEWINFO_H_
