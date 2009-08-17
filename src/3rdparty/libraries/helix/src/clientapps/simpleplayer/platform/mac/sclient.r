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

data 'plst' (0) {
};

data 'carb' (0) {
};

data 'MBAR' (128, preload) {
	$"0003 0080 0081 0082"                                /* ...Ä.Å.Ç */
};

data 'MENU' (128, preload) {
	$"0080 0000 0000 0000 0000 FFFF FFFB 0114"            /* .Ä........ˇˇˇ˚.. */
	$"0C41 626F 7574 2048 656C 6C6F C900 0000"            /* .About Hello…... */
	$"0001 2D00 0000 0000"                                /* ..-..... */
};

data 'MENU' (129, preload) {
	$"0081 0000 0000 0000 0000 0000 3F01 0446"            /* .Å..........?..F */
	$"696C 6503 4E65 7700 4E00 0004 4F70 656E"            /* ile.New.N...Open */
	$"004F 0000 012D 0000 0000 0543 6C6F 7365"            /* .O...-.....Close */
	$"0057 0000 0453 6176 6500 5300 0008 5361"            /* .W...Save.S...Sa */
	$"7665 2041 73C9 0000 0000 00"                        /* ve As…..... */
};

data 'MENU' (130, preload) {
	$"0082 0000 0000 0000 0000 0000 0001 0445"            /* .Ç.............E */
	$"6469 7404 556E 646F 005A 0000 012D 0000"            /* dit.Undo.Z...-.. */
	$"0000 0343 7574 0058 0000 0443 6F70 7900"            /* ...Cut.X...Copy. */
	$"4300 0005 5061 7374 6500 5600 0005 436C"            /* C...Paste.V...Cl */
	$"6561 7200 0000 0000"                                /* ear..... */
};

