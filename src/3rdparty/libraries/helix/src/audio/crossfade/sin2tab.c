/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sin2tab.c,v 1.2 2004/07/09 18:36:41 hubbe Exp $
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

#include "xfade.h"

const struct COEFF XFader_sin2tab[2*257] = {
 /* fade out table */
 0x40000000, -0x0000009e, 0x3fff6217, -0x000001da,
 0x3ffd8861, -0x00000315, 0x3ffa72f0, -0x00000451,
 0x3ff621e3, -0x0000058c, 0x3ff09566, -0x000006c8,
 0x3fe9cdad, -0x00000803, 0x3fe1cafd, -0x0000093d,
 0x3fd88da4, -0x00000a78, 0x3fce15fd, -0x00000bb2,
 0x3fc26471, -0x00000ceb, 0x3fb57972, -0x00000e24,
 0x3fa7557f, -0x00000f5c, 0x3f97f925, -0x00001094,
 0x3f8764fa, -0x000011cb, 0x3f7599a4, -0x00001302,
 0x3f6297d0, -0x00001438, 0x3f4e603b, -0x0000156d,
 0x3f38f3ac, -0x000016a1, 0x3f2252f7, -0x000017d4,
 0x3f0a7efc, -0x00001906, 0x3ef178a4, -0x00001a38,
 0x3ed740e7, -0x00001b68, 0x3ebbd8c9, -0x00001c97,
 0x3e9f4157, -0x00001dc6, 0x3e817bab, -0x00001ef3,
 0x3e6288ec, -0x0000201f, 0x3e426a4b, -0x00002149,
 0x3e212105, -0x00002273, 0x3dfeae62, -0x0000239b,
 0x3ddb13b7, -0x000024c1, 0x3db65262, -0x000025e7,
 0x3d906bcf, -0x0000270a, 0x3d696174, -0x0000282d,
 0x3d4134d1, -0x0000294d, 0x3d17e774, -0x00002a6c,
 0x3ced7af4, -0x00002b8a, 0x3cc1f0f4, -0x00002ca6,
 0x3c954b21, -0x00002dc0, 0x3c678b35, -0x00002ed8,
 0x3c38b2f2, -0x00002fef, 0x3c08c426, -0x00003103,
 0x3bd7c0ac, -0x00003216, 0x3ba5aa67, -0x00003327,
 0x3b728345, -0x00003436, 0x3b3e4d3f, -0x00003543,
 0x3b090a58, -0x0000364e, 0x3ad2bc9e, -0x00003756,
 0x3a9b6629, -0x0000385d, 0x3a63091b, -0x00003961,
 0x3a29a7a0, -0x00003a64, 0x39ef43ef, -0x00003b64,
 0x39b3e048, -0x00003c61, 0x39777ef5, -0x00003d5d,
 0x393a224a, -0x00003e56, 0x38fbcca4, -0x00003f4c,
 0x38bc806b, -0x00004040, 0x387c4010, -0x00004132,
 0x383b0e0c, -0x00004221, 0x37f8ece3, -0x0000430e,
 0x37b5df22, -0x000043f8, 0x3771e75f, -0x000044df,
 0x372d0838, -0x000045c4, 0x36e74455, -0x000046a6,
 0x36a09e66, -0x00004785, 0x36591926, -0x00004862,
 0x3610b755, -0x0000493c, 0x35c77bbe, -0x00004a13,
 0x357d6935, -0x00004ae7, 0x35328293, -0x00004bb8,
 0x34e6cabc, -0x00004c86, 0x349a449c, -0x00004d51,
 0x344cf325, -0x00004e1a, 0x33fed953, -0x00004edf,
 0x33affa29, -0x00004fa1, 0x336058b1, -0x00005061,
 0x330ff7fd, -0x0000511d, 0x32bedb26, -0x000051d6,
 0x326d054d, -0x0000528c, 0x321a7999, -0x0000533e,
 0x31c73b3a, -0x000053ee, 0x31734d64, -0x0000549a,
 0x311eb354, -0x00005543, 0x30c9704d, -0x000055e9,
 0x30738799, -0x0000568b, 0x301cfc87, -0x0000572a,
 0x2fc5d26e, -0x000057c6, 0x2f6e0ca9, -0x0000585e,
 0x2f15ae9c, -0x000058f3, 0x2ebcbbae, -0x00005984,
 0x2e63374d, -0x00005a12, 0x2e0924ec, -0x00005a9d,
 0x2dae8805, -0x00005b24, 0x2d536416, -0x00005ba7,
 0x2cf7bca2, -0x00005c27, 0x2c9b9532, -0x00005ca4,
 0x2c3ef153, -0x00005d1d, 0x2be1d499, -0x00005d92,
 0x2b844298, -0x00005e04, 0x2b263eef, -0x00005e72,
 0x2ac7cd3b, -0x00005edc, 0x2a68f121, -0x00005f43,
 0x2a09ae4a, -0x00005fa6, 0x29aa0861, -0x00006005,
 0x294a0317, -0x00006061, 0x28e9a220, -0x000060b9,
 0x2888e931, -0x0000610d, 0x2827dc07, -0x0000615e,
 0x27c67e5f, -0x000061aa, 0x2764d3f9, -0x000061f3,
 0x2702e09b, -0x00006239, 0x26a0a809, -0x0000627a,
 0x263e2e0f, -0x000062b8, 0x25db7678, -0x000062f1,
 0x25788511, -0x00006327, 0x25155dac, -0x0000635a,
 0x24b2041c, -0x00006388, 0x244e7c34, -0x000063b2,
 0x23eac9cb, -0x000063d9, 0x2386f0b9, -0x000063fc,
 0x2322f4d8, -0x0000641b, 0x22beda01, -0x00006436,
 0x225aa412, -0x0000644d, 0x21f656e8, -0x00006461,
 0x2191f65f, -0x00006470, 0x212d8657, -0x0000647c,
 0x20c90ab0, -0x00006483, 0x20648748, -0x00006487,
 0x20000000, -0x00006487, 0x1f9b78b8, -0x00006483,
 0x1f36f550, -0x0000647c, 0x1ed279a9, -0x00006470,
 0x1e6e09a1, -0x00006461, 0x1e09a918, -0x0000644d,
 0x1da55bee, -0x00006436, 0x1d4125ff, -0x0000641b,
 0x1cdd0b28, -0x000063fc, 0x1c790f47, -0x000063d9,
 0x1c153635, -0x000063b2, 0x1bb183cc, -0x00006388,
 0x1b4dfbe4, -0x0000635a, 0x1aeaa254, -0x00006327,
 0x1a877aef, -0x000062f1, 0x1a248988, -0x000062b8,
 0x19c1d1f1, -0x0000627a, 0x195f57f7, -0x00006239,
 0x18fd1f65, -0x000061f3, 0x189b2c07, -0x000061aa,
 0x183981a1, -0x0000615e, 0x17d823f9, -0x0000610d,
 0x177716cf, -0x000060b9, 0x17165de0, -0x00006061,
 0x16b5fce9, -0x00006005, 0x1655f79f, -0x00005fa6,
 0x15f651b6, -0x00005f43, 0x15970edf, -0x00005edc,
 0x153832c5, -0x00005e72, 0x14d9c111, -0x00005e04,
 0x147bbd68, -0x00005d92, 0x141e2b67, -0x00005d1d,
 0x13c10ead, -0x00005ca4, 0x13646ace, -0x00005c27,
 0x1308435e, -0x00005ba7, 0x12ac9bea, -0x00005b24,
 0x125177fb, -0x00005a9d, 0x11f6db14, -0x00005a12,
 0x119cc8b3, -0x00005984, 0x11434452, -0x000058f3,
 0x10ea5164, -0x0000585e, 0x1091f357, -0x000057c6,
 0x103a2d92, -0x0000572a, 0x0fe30379, -0x0000568b,
 0x0f8c7867, -0x000055e9, 0x0f368fb3, -0x00005543,
 0x0ee14cac, -0x0000549a, 0x0e8cb29c, -0x000053ee,
 0x0e38c4c6, -0x0000533e, 0x0de58667, -0x0000528c,
 0x0d92fab3, -0x000051d6, 0x0d4124da, -0x0000511d,
 0x0cf00803, -0x00005061, 0x0c9fa74f, -0x00004fa1,
 0x0c5005d7, -0x00004edf, 0x0c0126ad, -0x00004e1a,
 0x0bb30cdb, -0x00004d51, 0x0b65bb64, -0x00004c86,
 0x0b193544, -0x00004bb8, 0x0acd7d6d, -0x00004ae7,
 0x0a8296cb, -0x00004a13, 0x0a388442, -0x0000493c,
 0x09ef48ab, -0x00004862, 0x09a6e6da, -0x00004785,
 0x095f619a, -0x000046a6, 0x0918bbab, -0x000045c4,
 0x08d2f7c8, -0x000044df, 0x088e18a1, -0x000043f8,
 0x084a20de, -0x0000430e, 0x0807131d, -0x00004221,
 0x07c4f1f4, -0x00004132, 0x0783bff0, -0x00004040,
 0x07437f95, -0x00003f4c, 0x0704335c, -0x00003e56,
 0x06c5ddb6, -0x00003d5d, 0x0688810b, -0x00003c61,
 0x064c1fb8, -0x00003b64, 0x0610bc11, -0x00003a64,
 0x05d65860, -0x00003961, 0x059cf6e5, -0x0000385d,
 0x056499d7, -0x00003756, 0x052d4362, -0x0000364e,
 0x04f6f5a8, -0x00003543, 0x04c1b2c1, -0x00003436,
 0x048d7cbb, -0x00003327, 0x045a5599, -0x00003216,
 0x04283f54, -0x00003103, 0x03f73bda, -0x00002fef,
 0x03c74d0e, -0x00002ed8, 0x039874cb, -0x00002dc0,
 0x036ab4df, -0x00002ca6, 0x033e0f0c, -0x00002b8a,
 0x0312850c, -0x00002a6c, 0x02e8188c, -0x0000294d,
 0x02becb2f, -0x0000282d, 0x02969e8c, -0x0000270a,
 0x026f9431, -0x000025e7, 0x0249ad9e, -0x000024c1,
 0x0224ec49, -0x0000239b, 0x0201519e, -0x00002273,
 0x01dedefb, -0x00002149, 0x01bd95b5, -0x0000201f,
 0x019d7714, -0x00001ef3, 0x017e8455, -0x00001dc6,
 0x0160bea9, -0x00001c97, 0x01442737, -0x00001b68,
 0x0128bf19, -0x00001a38, 0x010e875c, -0x00001906,
 0x00f58104, -0x000017d4, 0x00ddad09, -0x000016a1,
 0x00c70c54, -0x0000156d, 0x00b19fc5, -0x00001438,
 0x009d6830, -0x00001302, 0x008a665c, -0x000011cb,
 0x00789b06, -0x00001094, 0x006806db, -0x00000f5c,
 0x0058aa81, -0x00000e24, 0x004a868e, -0x00000ceb,
 0x003d9b8f, -0x00000bb2, 0x0031ea03, -0x00000a78,
 0x0027725c, -0x0000093d, 0x001e3503, -0x00000803,
 0x00163253, -0x000006c8, 0x000f6a9a, -0x0000058c,
 0x0009de1d, -0x00000451, 0x00058d10, -0x00000315,
 0x0002779f, -0x000001da, 0x00009de9, -0x0000009e,
 0x00000000, -0x00000000,
 /* fade in table */
 0x00000000, 0x0000009e, 0x00009de9, 0x000001da,
 0x0002779f, 0x00000315, 0x00058d10, 0x00000451,
 0x0009de1d, 0x0000058c, 0x000f6a9a, 0x000006c8,
 0x00163253, 0x00000803, 0x001e3503, 0x0000093d,
 0x0027725c, 0x00000a78, 0x0031ea03, 0x00000bb2,
 0x003d9b8f, 0x00000ceb, 0x004a868e, 0x00000e24,
 0x0058aa81, 0x00000f5c, 0x006806db, 0x00001094,
 0x00789b06, 0x000011cb, 0x008a665c, 0x00001302,
 0x009d6830, 0x00001438, 0x00b19fc5, 0x0000156d,
 0x00c70c54, 0x000016a1, 0x00ddad09, 0x000017d4,
 0x00f58104, 0x00001906, 0x010e875c, 0x00001a38,
 0x0128bf19, 0x00001b68, 0x01442737, 0x00001c97,
 0x0160bea9, 0x00001dc6, 0x017e8455, 0x00001ef3,
 0x019d7714, 0x0000201f, 0x01bd95b5, 0x00002149,
 0x01dedefb, 0x00002273, 0x0201519e, 0x0000239b,
 0x0224ec49, 0x000024c1, 0x0249ad9e, 0x000025e7,
 0x026f9431, 0x0000270a, 0x02969e8c, 0x0000282d,
 0x02becb2f, 0x0000294d, 0x02e8188c, 0x00002a6d,
 0x0312850c, 0x00002b8a, 0x033e0f0c, 0x00002ca6,
 0x036ab4df, 0x00002dc0, 0x039874cb, 0x00002ed8,
 0x03c74d0e, 0x00002fef, 0x03f73bda, 0x00003103,
 0x04283f54, 0x00003216, 0x045a5599, 0x00003327,
 0x048d7cbb, 0x00003436, 0x04c1b2c1, 0x00003543,
 0x04f6f5a8, 0x0000364e, 0x052d4362, 0x00003756,
 0x056499d7, 0x0000385d, 0x059cf6e5, 0x00003961,
 0x05d65860, 0x00003a64, 0x0610bc11, 0x00003b64,
 0x064c1fb8, 0x00003c61, 0x0688810b, 0x00003d5d,
 0x06c5ddb6, 0x00003e56, 0x0704335c, 0x00003f4c,
 0x07437f95, 0x00004040, 0x0783bff0, 0x00004132,
 0x07c4f1f4, 0x00004221, 0x0807131d, 0x0000430e,
 0x084a20de, 0x000043f8, 0x088e18a1, 0x000044df,
 0x08d2f7c8, 0x000045c4, 0x0918bbab, 0x000046a6,
 0x095f619a, 0x00004785, 0x09a6e6da, 0x00004862,
 0x09ef48ab, 0x0000493c, 0x0a388442, 0x00004a13,
 0x0a8296cb, 0x00004ae7, 0x0acd7d6d, 0x00004bb8,
 0x0b193544, 0x00004c86, 0x0b65bb64, 0x00004d51,
 0x0bb30cdb, 0x00004e1a, 0x0c0126ad, 0x00004edf,
 0x0c5005d7, 0x00004fa1, 0x0c9fa74f, 0x00005061,
 0x0cf00803, 0x0000511d, 0x0d4124da, 0x000051d6,
 0x0d92fab3, 0x0000528c, 0x0de58667, 0x0000533e,
 0x0e38c4c6, 0x000053ee, 0x0e8cb29c, 0x0000549a,
 0x0ee14cac, 0x00005543, 0x0f368fb3, 0x000055e9,
 0x0f8c7867, 0x0000568b, 0x0fe30379, 0x0000572a,
 0x103a2d92, 0x000057c6, 0x1091f357, 0x0000585e,
 0x10ea5164, 0x000058f3, 0x11434452, 0x00005984,
 0x119cc8b3, 0x00005a12, 0x11f6db14, 0x00005a9d,
 0x125177fb, 0x00005b24, 0x12ac9bea, 0x00005ba7,
 0x1308435e, 0x00005c27, 0x13646ace, 0x00005ca4,
 0x13c10ead, 0x00005d1d, 0x141e2b67, 0x00005d92,
 0x147bbd68, 0x00005e04, 0x14d9c111, 0x00005e72,
 0x153832c5, 0x00005edc, 0x15970edf, 0x00005f43,
 0x15f651b6, 0x00005fa6, 0x1655f79f, 0x00006005,
 0x16b5fce9, 0x00006061, 0x17165de0, 0x000060b9,
 0x177716cf, 0x0000610d, 0x17d823f9, 0x0000615e,
 0x183981a1, 0x000061aa, 0x189b2c07, 0x000061f3,
 0x18fd1f65, 0x00006239, 0x195f57f7, 0x0000627a,
 0x19c1d1f1, 0x000062b8, 0x1a248988, 0x000062f1,
 0x1a877aef, 0x00006327, 0x1aeaa254, 0x0000635a,
 0x1b4dfbe4, 0x00006388, 0x1bb183cc, 0x000063b2,
 0x1c153635, 0x000063d9, 0x1c790f47, 0x000063fc,
 0x1cdd0b28, 0x0000641b, 0x1d4125ff, 0x00006436,
 0x1da55bee, 0x0000644d, 0x1e09a918, 0x00006461,
 0x1e6e09a1, 0x00006470, 0x1ed279a9, 0x0000647c,
 0x1f36f550, 0x00006483, 0x1f9b78b8, 0x00006487,
 0x20000000, 0x00006487, 0x20648748, 0x00006483,
 0x20c90ab0, 0x0000647c, 0x212d8657, 0x00006470,
 0x2191f65f, 0x00006461, 0x21f656e8, 0x0000644d,
 0x225aa412, 0x00006436, 0x22beda01, 0x0000641b,
 0x2322f4d8, 0x000063fc, 0x2386f0b9, 0x000063d9,
 0x23eac9cb, 0x000063b2, 0x244e7c34, 0x00006388,
 0x24b2041c, 0x0000635a, 0x25155dac, 0x00006327,
 0x25788511, 0x000062f1, 0x25db7678, 0x000062b8,
 0x263e2e0f, 0x0000627a, 0x26a0a809, 0x00006239,
 0x2702e09b, 0x000061f3, 0x2764d3f9, 0x000061aa,
 0x27c67e5f, 0x0000615e, 0x2827dc07, 0x0000610d,
 0x2888e931, 0x000060b9, 0x28e9a220, 0x00006061,
 0x294a0317, 0x00006005, 0x29aa0861, 0x00005fa6,
 0x2a09ae4a, 0x00005f43, 0x2a68f121, 0x00005edc,
 0x2ac7cd3b, 0x00005e72, 0x2b263eef, 0x00005e04,
 0x2b844298, 0x00005d92, 0x2be1d499, 0x00005d1d,
 0x2c3ef153, 0x00005ca4, 0x2c9b9532, 0x00005c27,
 0x2cf7bca2, 0x00005ba7, 0x2d536416, 0x00005b24,
 0x2dae8805, 0x00005a9d, 0x2e0924ec, 0x00005a12,
 0x2e63374d, 0x00005984, 0x2ebcbbae, 0x000058f3,
 0x2f15ae9c, 0x0000585e, 0x2f6e0ca9, 0x000057c6,
 0x2fc5d26e, 0x0000572a, 0x301cfc87, 0x0000568b,
 0x30738799, 0x000055e9, 0x30c9704d, 0x00005543,
 0x311eb354, 0x0000549a, 0x31734d64, 0x000053ee,
 0x31c73b3a, 0x0000533e, 0x321a7999, 0x0000528c,
 0x326d054d, 0x000051d6, 0x32bedb26, 0x0000511d,
 0x330ff7fd, 0x00005061, 0x336058b1, 0x00004fa1,
 0x33affa29, 0x00004edf, 0x33fed953, 0x00004e1a,
 0x344cf325, 0x00004d51, 0x349a449c, 0x00004c86,
 0x34e6cabc, 0x00004bb8, 0x35328293, 0x00004ae7,
 0x357d6935, 0x00004a13, 0x35c77bbe, 0x0000493c,
 0x3610b755, 0x00004862, 0x36591926, 0x00004785,
 0x36a09e66, 0x000046a6, 0x36e74455, 0x000045c4,
 0x372d0838, 0x000044df, 0x3771e75f, 0x000043f8,
 0x37b5df22, 0x0000430e, 0x37f8ece3, 0x00004221,
 0x383b0e0c, 0x00004132, 0x387c4010, 0x00004040,
 0x38bc806b, 0x00003f4c, 0x38fbcca4, 0x00003e56,
 0x393a224a, 0x00003d5d, 0x39777ef5, 0x00003c61,
 0x39b3e048, 0x00003b64, 0x39ef43ef, 0x00003a64,
 0x3a29a7a0, 0x00003961, 0x3a63091b, 0x0000385d,
 0x3a9b6629, 0x00003756, 0x3ad2bc9e, 0x0000364e,
 0x3b090a58, 0x00003543, 0x3b3e4d3f, 0x00003436,
 0x3b728345, 0x00003327, 0x3ba5aa67, 0x00003216,
 0x3bd7c0ac, 0x00003103, 0x3c08c426, 0x00002fef,
 0x3c38b2f2, 0x00002ed8, 0x3c678b35, 0x00002dc0,
 0x3c954b21, 0x00002ca6, 0x3cc1f0f4, 0x00002b8a,
 0x3ced7af4, 0x00002a6d, 0x3d17e774, 0x0000294d,
 0x3d4134d1, 0x0000282d, 0x3d696174, 0x0000270a,
 0x3d906bcf, 0x000025e7, 0x3db65262, 0x000024c1,
 0x3ddb13b7, 0x0000239b, 0x3dfeae62, 0x00002273,
 0x3e212105, 0x00002149, 0x3e426a4b, 0x0000201f,
 0x3e6288ec, 0x00001ef3, 0x3e817bab, 0x00001dc6,
 0x3e9f4157, 0x00001c97, 0x3ebbd8c9, 0x00001b68,
 0x3ed740e7, 0x00001a38, 0x3ef178a4, 0x00001906,
 0x3f0a7efc, 0x000017d4, 0x3f2252f7, 0x000016a1,
 0x3f38f3ac, 0x0000156d, 0x3f4e603b, 0x00001438,
 0x3f6297d0, 0x00001302, 0x3f7599a4, 0x000011cb,
 0x3f8764fa, 0x00001094, 0x3f97f925, 0x00000f5c,
 0x3fa7557f, 0x00000e24, 0x3fb57972, 0x00000ceb,
 0x3fc26471, 0x00000bb2, 0x3fce15fd, 0x00000a78,
 0x3fd88da4, 0x0000093d, 0x3fe1cafd, 0x00000803,
 0x3fe9cdad, 0x000006c8, 0x3ff09566, 0x0000058c,
 0x3ff621e3, 0x00000451, 0x3ffa72f0, 0x00000315,
 0x3ffd8861, 0x000001da, 0x3fff6217, 0x0000009e,
 0x40000000, 0x00000000,
};
