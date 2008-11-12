/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtXML module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QXMLSTREAM_P_H
#define QXMLSTREAM_P_H

class QXmlStreamReader_Table
{
public:
  enum {
    EOF_SYMBOL = 0,
    AMPERSAND = 5,
    ANY = 41,
    ATTLIST = 31,
    BANG = 25,
    CDATA = 46,
    CDATA_START = 28,
    COLON = 17,
    COMMA = 19,
    DASH = 20,
    DBLQUOTE = 8,
    DIGIT = 27,
    DOCTYPE = 29,
    DOT = 23,
    ELEMENT = 30,
    EMPTY = 40,
    ENTITIES = 50,
    ENTITY = 32,
    ENTITY_DONE = 45,
    EQ = 14,
    ERROR = 43,
    FIXED = 39,
    HASH = 6,
    ID = 47,
    IDREF = 48,
    IDREFS = 49,
    IMPLIED = 38,
    LANGLE = 3,
    LBRACK = 9,
    LETTER = 26,
    LPAREN = 11,
    NDATA = 36,
    NMTOKEN = 51,
    NMTOKENS = 52,
    NOTATION = 33,
    NOTOKEN = 1,
    PARSE_ENTITY = 44,
    PCDATA = 42,
    PERCENT = 15,
    PIPE = 13,
    PLUS = 21,
    PUBLIC = 35,
    QUESTIONMARK = 24,
    QUOTE = 7,
    RANGLE = 4,
    RBRACK = 10,
    REQUIRED = 37,
    RPAREN = 12,
    SEMICOLON = 18,
    SLASH = 16,
    SPACE = 2,
    STAR = 22,
    SYSTEM = 34,
    VERSION = 54,
    XML = 53,

    ACCEPT_STATE = 407,
    RULE_COUNT = 261,
    STATE_COUNT = 418,
    TERMINAL_COUNT = 55,
    NON_TERMINAL_COUNT = 81,

    GOTO_INDEX_OFFSET = 418,
    GOTO_INFO_OFFSET = 925,
    GOTO_CHECK_OFFSET = 925,
  };

  static const char  *const spell [];
  static const int            lhs [];
  static const int            rhs [];
  static const int   goto_default [];
  static const int action_default [];
  static const int   action_index [];
  static const int    action_info [];
  static const int   action_check [];

  inline int nt_action (int state, int nt) const
  {
    const int *const goto_index = &action_index [GOTO_INDEX_OFFSET];
    const int *const goto_check = &action_check [GOTO_CHECK_OFFSET];

    const int yyn = goto_index [state] + nt;

    if (yyn < 0 || goto_check [yyn] != nt)
      return goto_default [nt];

    const int *const goto_info = &action_info [GOTO_INFO_OFFSET];
    return goto_info [yyn];
  }

  inline int t_action (int state, int token) const
  {
    const int yyn = action_index [state] + token;

    if (yyn < 0 || action_check [yyn] != token)
      return - action_default [state];

    return action_info [yyn];
  }
};


const char *const QXmlStreamReader_Table::spell [] = {
  "end of file", 0, " ", "<", ">", "&", "#", "\'", "\"", "[",
  "]", "(", ")", "|", "=", "%", "/", ":", ";", ",",
  "-", "+", "*", ".", "?", "!", "[a-zA-Z]", "[0-9]", "[CDATA[", "DOCTYPE",
  "ELEMENT", "ATTLIST", "ENTITY", "NOTATION", "SYSTEM", "PUBLIC", "NDATA", "REQUIRED", "IMPLIED", "FIXED",
  "EMPTY", "ANY", "PCDATA", 0, 0, 0, "CDATA", "ID", "IDREF", "IDREFS",
  "ENTITIES", "NMTOKEN", "NMTOKENS", "<?xml", "version"};

const int QXmlStreamReader_Table::lhs [] = {
  55, 55, 57, 57, 57, 57, 57, 57, 57, 57,
  65, 66, 62, 70, 70, 70, 73, 64, 64, 64,
  64, 77, 76, 78, 78, 78, 78, 78, 78, 79,
  79, 79, 79, 79, 79, 79, 85, 81, 86, 86,
  86, 86, 89, 90, 91, 91, 91, 91, 92, 92,
  94, 94, 94, 95, 95, 96, 96, 97, 97, 98,
  98, 87, 87, 93, 88, 99, 99, 101, 101, 101,
  101, 101, 101, 101, 101, 101, 101, 102, 103, 103,
  103, 103, 105, 107, 108, 108, 82, 82, 109, 109,
  110, 110, 83, 83, 83, 63, 63, 74, 112, 61,
  113, 114, 84, 84, 84, 115, 115, 115, 115, 115,
  115, 115, 115, 115, 115, 115, 115, 115, 115, 115,
  115, 115, 115, 115, 115, 115, 116, 116, 116, 116,
  68, 68, 68, 68, 117, 118, 117, 118, 117, 118,
  117, 118, 120, 120, 120, 120, 120, 120, 120, 120,
  120, 120, 120, 120, 120, 120, 120, 120, 120, 120,
  120, 120, 120, 120, 119, 71, 111, 111, 111, 111,
  121, 122, 121, 122, 121, 122, 121, 122, 123, 123,
  123, 123, 123, 123, 123, 123, 123, 123, 123, 123,
  123, 123, 123, 123, 123, 123, 123, 123, 123, 123,
  123, 123, 123, 104, 104, 104, 104, 126, 127, 126,
  127, 126, 126, 127, 127, 128, 128, 128, 128, 130,
  69, 69, 69, 131, 131, 132, 60, 58, 59, 133,
  80, 125, 129, 124, 134, 134, 134, 134, 56, 56,
  56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
  72, 67, 67, 106, 75, 100, 100, 100, 100, 100,
  135};

const int QXmlStreamReader_Table:: rhs[] = {
  2, 1, 4, 2, 2, 2, 2, 2, 2, 0,
  1, 1, 9, 2, 4, 0, 4, 6, 4, 4,
  6, 1, 3, 1, 1, 1, 2, 2, 2, 1,
  1, 1, 1, 1, 1, 1, 4, 4, 1, 1,
  1, 1, 1, 2, 1, 1, 1, 0, 2, 2,
  2, 6, 6, 1, 5, 1, 5, 3, 5, 0,
  1, 6, 8, 4, 2, 1, 5, 1, 1, 1,
  1, 1, 1, 1, 1, 6, 7, 1, 2, 2,
  1, 4, 3, 3, 1, 2, 5, 6, 4, 6,
  3, 5, 5, 3, 4, 4, 5, 2, 3, 2,
  2, 4, 5, 5, 7, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 3, 3, 2, 2, 2, 2, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 2, 2, 3, 3,
  2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 2, 2, 3, 3, 2, 2, 2,
  2, 1, 1, 1, 1, 1, 1, 1, 1, 5,
  0, 1, 3, 1, 3, 2, 4, 3, 5, 3,
  3, 3, 3, 4, 1, 1, 2, 2, 2, 4,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 0,
  1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
  2};

const int QXmlStreamReader_Table::action_default [] = {
  10, 250, 0, 2, 1, 0, 124, 116, 118, 119,
  126, 129, 122, 11, 113, 107, 0, 108, 128, 110,
  114, 112, 120, 123, 125, 106, 109, 111, 117, 115,
  127, 121, 12, 243, 247, 242, 0, 130, 239, 246,
  16, 241, 249, 248, 0, 245, 250, 221, 244, 0,
  0, 255, 0, 236, 235, 0, 238, 237, 234, 230,
  98, 254, 0, 226, 0, 0, 251, 96, 97, 100,
  0, 0, 252, 0, 0, 166, 0, 163, 155, 157,
  158, 132, 144, 161, 152, 146, 147, 143, 149, 153,
  151, 159, 162, 142, 145, 148, 150, 156, 154, 164,
  160, 140, 165, 0, 134, 138, 136, 141, 131, 139,
  0, 137, 133, 135, 0, 15, 14, 253, 0, 22,
  19, 252, 0, 0, 18, 0, 0, 31, 36, 30,
  0, 32, 252, 0, 33, 0, 24, 0, 34, 0,
  26, 35, 25, 0, 231, 40, 39, 252, 42, 48,
  252, 41, 0, 43, 252, 48, 252, 0, 48, 252,
  0, 0, 47, 45, 46, 50, 51, 252, 252, 0,
  56, 252, 53, 252, 0, 57, 0, 54, 252, 52,
  252, 0, 55, 64, 49, 0, 252, 60, 252, 0,
  58, 61, 62, 0, 252, 0, 0, 59, 63, 44,
  65, 0, 38, 0, 0, 252, 0, 93, 94, 0,
  0, 0, 0, 252, 0, 200, 191, 193, 195, 168,
  180, 198, 189, 183, 181, 184, 179, 186, 188, 196,
  199, 178, 182, 185, 187, 192, 190, 194, 197, 201,
  203, 202, 176, 0, 0, 232, 170, 174, 172, 0,
  0, 92, 177, 167, 175, 0, 173, 169, 171, 91,
  0, 95, 0, 0, 0, 0, 0, 252, 85, 252,
  0, 253, 0, 86, 0, 88, 68, 73, 72, 69,
  70, 71, 252, 74, 75, 0, 0, 0, 260, 259,
  257, 258, 256, 66, 252, 0, 252, 0, 0, 67,
  76, 252, 0, 252, 0, 0, 77, 0, 78, 0,
  81, 84, 0, 0, 205, 215, 214, 0, 217, 219,
  218, 216, 0, 233, 207, 211, 209, 213, 204, 212,
  0, 210, 206, 208, 0, 80, 79, 0, 82, 0,
  83, 87, 99, 0, 37, 0, 0, 0, 0, 90,
  89, 0, 102, 23, 27, 29, 28, 0, 0, 252,
  253, 0, 252, 0, 105, 104, 252, 0, 103, 101,
  0, 0, 20, 252, 17, 0, 21, 0, 0, 240,
  0, 252, 0, 229, 0, 222, 228, 0, 227, 224,
  252, 252, 253, 223, 225, 0, 252, 0, 220, 252,
  0, 252, 0, 221, 0, 0, 13, 261, 9, 5,
  8, 4, 0, 7, 250, 6, 0, 3};

const int QXmlStreamReader_Table::goto_default [] = {
  2, 4, 3, 46, 379, 41, 35, 48, 45, 39,
  240, 49, 118, 75, 384, 72, 76, 117, 40, 44,
  158, 121, 122, 137, 136, 140, 129, 127, 131, 138,
  130, 150, 151, 148, 160, 159, 200, 156, 155, 157,
  178, 171, 188, 192, 294, 293, 286, 312, 311, 310,
  270, 63, 268, 269, 133, 132, 213, 36, 33, 139,
  37, 38, 110, 103, 321, 102, 255, 243, 242, 239,
  241, 330, 317, 316, 320, 389, 390, 47, 43, 55,
  0};

const int QXmlStreamReader_Table::action_index [] = {
  -28, -55, 38, 104, 870, 95, -55, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, 94, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, -55, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, 40, -55, -55, -55,
  49, -55, -55, -55, 82, -55, -55, 80, -55, -12,
  72, -55, 4, -55, -55, 90, -55, -55, -55, -55,
  -55, -55, 11, -55, 47, 22, -55, -55, -55, -55,
  46, 65, 80, 287, 340, -55, 80, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, -55, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, -55, -55, -55, -55,
  -55, -55, -55, 244, -55, -55, -55, -55, -55, -55,
  313, -55, -55, -55, 53, -55, -55, -55, 54, -55,
  -55, 80, 144, 28, -55, 41, 11, -55, -55, -55,
  122, -55, 55, 141, -55, 149, -55, 234, -55, 42,
  -55, -55, -55, 12, -55, -55, -55, 26, -55, 105,
  80, -55, 119, -55, 80, 110, 80, 23, 120, 80,
  -8, 59, -55, -55, -55, -55, 62, 80, 80, 89,
  -55, 80, 8, 27, 83, -55, 79, -55, 27, 6,
  27, 69, -55, -55, -55, 74, 27, -5, 80, -3,
  -55, -55, -55, 84, 27, -1, 5, -55, -55, -55,
  -55, 21, -55, -2, 16, 27, 14, -55, -55, 658,
  128, 499, 128, 80, 86, -55, -55, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, -55, -55, -55, -55,
  -55, -55, -55, -55, -55, -55, -55, -55, -55, -55,
  -55, -55, -55, 605, 29, -55, -55, -55, -55, 80,
  128, -55, -55, -55, -55, 711, -55, -55, -55, -55,
  32, -55, 19, 13, 17, 88, 20, 80, -55, 80,
  173, 15, 35, -55, 36, -55, -55, -55, -55, -55,
  -55, -55, 80, -55, -55, 37, 116, 171, -55, -55,
  -55, -55, -55, -55, 27, 56, 27, 27, 151, -55,
  -55, 27, 138, 27, 39, 27, -55, 446, -55, 393,
  -55, -55, 100, 87, -55, -55, -55, 764, -55, -55,
  -55, -55, -6, -55, -55, -55, -55, -55, -55, -55,
  552, -55, -55, -55, 27, -55, -55, 81, -55, 27,
  -55, -55, -55, 27, -55, 27, 27, -17, 27, -55,
  -55, 27, -55, -55, -55, -55, -55, 128, 128, 27,
  71, 7, 27, 10, -55, -55, 27, 9, -55, -55,
  -25, 181, -55, 27, -55, 3, -55, 817, 127, -55,
  -18, 27, 0, -55, 58, -26, -55, 1, -55, -55,
  27, 27, -24, -55, -55, -11, 27, 48, -55, 27,
  -4, 27, 128, 27, -9, 2, -55, -55, -55, -55,
  -55, -55, 30, -55, -55, -55, 817, -55,

  -81, -81, -81, 194, 79, -19, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, 1, -17, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, 54, -81, 58, -81, -81, -81, -81, -81,
  -81, 63, -81, -11, -27, -81, 116, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -6, -81, -81, -81, -81, -81, -81,
  -7, -81, -81, -81, 18, -81, -81, -81, -81, -81,
  -81, 24, 103, -81, -81, -81, 19, -81, -81, -81,
  22, -81, 29, -81, -81, -81, -81, 111, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, 16, -81, -81,
  20, -81, -81, -81, 21, 27, 40, -81, 41, 42,
  -81, -81, -81, -81, -81, -81, -81, 32, 37, 48,
  -81, 35, -81, 36, 33, -81, 31, -81, 30, -81,
  38, 34, -81, -81, -81, -81, 43, -81, 44, 45,
  -81, -81, -81, -81, 39, -81, 26, -81, -81, -81,
  -81, -81, -81, 14, -81, 17, -81, -81, -81, -81,
  8, -49, 25, 28, 23, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -33, -81, -81, -81, -81, -81, 55,
  74, -81, -81, -81, -81, -23, -81, -81, -81, -81,
  -81, -81, 51, -81, 62, 66, 60, 87, -81, 98,
  -81, 65, -81, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, 71, -81, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, 15, -81, 50, 64, 49, -81,
  -81, 47, 53, 57, -81, 61, -81, 80, -81, 95,
  -81, -81, -81, 59, -81, -81, -81, 102, -81, -81,
  -81, -81, -81, -81, -81, -81, -81, -81, -81, -81,
  97, -81, -81, -81, 56, -81, -81, 46, -81, -12,
  -81, -81, -81, -10, -81, 9, 13, -5, 4, -81,
  -81, -8, -81, -81, -81, -81, -81, -2, 3, 81,
  12, -81, -4, -81, -81, -81, 11, -81, -81, -81,
  -16, 75, -81, 52, -81, -81, -81, 78, -81, -81,
  -41, -1, -81, -81, -81, -38, -81, -81, -81, -81,
  89, 0, 67, -81, -81, -81, 5, -43, -81, 6,
  -81, 10, 7, 86, -81, -81, -81, -81, -81, -81,
  -81, -81, -81, -81, 2, -81, 104, -81};

const int QXmlStreamReader_Table::action_info [] = {
  61, 51, 61, 396, 383, 388, 406, 376, 61, 51,
  401, 365, 323, 368, 364, 405, 1, 191, 207, 180,
  208, 198, 59, 51, 51, 202, 68, 173, 66, 66,
  144, 51, 124, 342, 199, 183, 261, 51, 407, 341,
  275, 61, 399, 51, 69, 61, 51, 245, 301, 66,
  263, 305, 296, 74, 73, 309, 307, 66, 120, 370,
  74, 73, 386, 119, 0, 62, 60, 0, 297, 296,
  154, 65, 74, 73, 387, 168, 358, 357, 74, 73,
  154, 167, 66, 71, 70, 51, 187, 186, 309, 307,
  154, 203, 50, 50, 154, 51, 195, 194, 54, 53,
  154, 50, 263, 345, 0, 51, 408, 16, 58, 51,
  369, 370, 51, 51, 51, 51, 57, 56, 62, 60,
  61, 51, 308, 309, 307, 153, 162, 164, 0, 163,
  154, 162, 164, 147, 163, 74, 73, 336, 335, 334,
  0, 162, 164, 380, 163, 51, 66, 125, 211, 209,
  0, 62, 60, 61, 0, 288, 0, 32, 289, 126,
  0, 291, 146, 145, 292, 290, 0, 0, 288, 263,
  0, 289, 0, 0, 291, 212, 210, 292, 290, 264,
  262, 265, 266, 66, 282, 372, 0, 0, 288, 13,
  119, 289, 0, 0, 291, 0, 0, 292, 290, 0,
  0, 0, 0, 0, 0, 278, 285, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 276,
  279, 280, 281, 277, 283, 284, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 66, 125, 0, 0,
  0, 0, 0, 0, 353, 0, 99, 0, 94, 126,
  85, 105, 104, 86, 95, 88, 96, 90, 84, 89,
  98, 78, 97, 79, 80, 91, 100, 83, 92, 77,
  87, 82, 0, 0, 0, 0, 0, 0, 0, 13,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 99,
  0, 94, 0, 85, 93, 81, 86, 95, 88, 96,
  90, 84, 89, 98, 78, 97, 79, 80, 91, 100,
  83, 92, 77, 87, 82, 99, 0, 94, 0, 85,
  112, 111, 86, 95, 88, 96, 90, 84, 89, 98,
  78, 97, 79, 80, 91, 100, 83, 92, 77, 87,
  82, 0, 99, 0, 94, 0, 85, 108, 107, 86,
  95, 88, 96, 90, 84, 89, 98, 78, 97, 79,
  80, 91, 100, 83, 92, 77, 87, 82, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 99, 0, 94, 313, 85,
  328, 327, 86, 95, 88, 96, 90, 84, 89, 98,
  78, 97, 79, 80, 91, 100, 83, 92, 77, 87,
  82, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 13, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 99, 0,
  94, 313, 85, 315, 314, 86, 95, 88, 96, 90,
  84, 89, 98, 78, 97, 79, 80, 91, 100, 83,
  92, 77, 87, 82, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 13, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 237, 224, 232, 214, 223, 253, 252, 225, 233,
  227, 234, 228, 222, 0, 236, 216, 235, 217, 218,
  229, 238, 221, 230, 215, 226, 220, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 13, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 99, 0, 94, 313, 85, 332,
  331, 86, 95, 88, 96, 90, 84, 89, 98, 78,
  97, 79, 80, 91, 100, 83, 92, 77, 87, 82,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 13, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 237, 224, 232,
  214, 223, 247, 246, 225, 233, 227, 234, 228, 222,
  0, 236, 216, 235, 217, 218, 229, 238, 221, 230,
  215, 226, 220, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  13, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  237, 224, 232, 214, 223, 231, 219, 225, 233, 227,
  234, 228, 222, 0, 236, 216, 235, 217, 218, 229,
  238, 221, 230, 215, 226, 220, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 13, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 237, 224, 232, 214, 223, 257, 256,
  225, 233, 227, 234, 228, 222, 0, 236, 216, 235,
  217, 218, 229, 238, 221, 230, 215, 226, 220, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 13, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 99, 0, 94, 313,
  85, 325, 324, 86, 95, 88, 96, 90, 84, 89,
  98, 78, 97, 79, 80, 91, 100, 83, 92, 77,
  87, 82, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 13,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 30,
  378, 25, 5, 15, 24, 10, 17, 26, 19, 27,
  21, 14, 20, 29, 7, 28, 8, 9, 22, 31,
  12, 23, 6, 18, 11, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 13, 0, 0, 0, 0, 0, 0, 0,
  32, 0, 30, 16, 25, 5, 15, 24, 10, 17,
  26, 19, 27, 21, 14, 20, 29, 7, 28, 8,
  9, 22, 31, 12, 23, 6, 18, 11, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 13, 0, 0, 0, 0,
  0, 0, 0, 32, 0,

  385, 52, 377, 416, 371, 340, 398, 344, 363, 352,
  381, 382, 395, 391, 359, 348, 366, 397, 400, 254,
  403, 349, 402, 367, 249, 362, 347, 295, 152, 206,
  350, 115, 201, 161, 205, 248, 123, 109, 259, 143,
  260, 204, 179, 244, 169, 258, 197, 172, 174, 176,
  181, 196, 166, 101, 185, 189, 193, 113, 106, 302,
  149, 0, 298, 165, 375, 190, 0, 0, 177, 304,
  175, 182, 250, 337, 64, 67, 116, 184, 306, 322,
  351, 300, 343, 287, 0, 170, 346, 251, 42, 42,
  319, 0, 374, 361, 299, 338, 373, 303, 360, 272,
  404, 393, 267, 385, 271, 319, 392, 319, 417, 128,
  274, 141, 319, 134, 42, 271, 339, 128, 391, 141,
  142, 134, 135, 0, 0, 0, 0, 0, 356, 0,
  135, 0, 0, 114, 0, 354, 355, 0, 0, 0,
  0, 0, 394, 0, 0, 0, 0, 34, 34, 318,
  273, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 318, 0, 318, 0, 329, 0,
  333, 318, 0, 34, 0, 326, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 414, 0, 411,
  409, 415, 413, 410, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 412, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0};

const int QXmlStreamReader_Table::action_check [] = {
  26, 26, 26, 14, 4, 4, 4, 4, 26, 26,
  14, 4, 18, 4, 4, 24, 44, 22, 4, 13,
  4, 22, 18, 26, 26, 4, 4, 19, 2, 2,
  18, 26, 4, 20, 42, 12, 4, 26, 0, 4,
  4, 26, 54, 26, 4, 26, 26, 18, 11, 2,
  20, 12, 13, 7, 8, 7, 8, 2, 4, 29,
  7, 8, 4, 9, -1, 24, 25, -1, 12, 13,
  11, 24, 7, 8, 16, 13, 34, 35, 7, 8,
  11, 19, 2, 34, 35, 26, 12, 13, 7, 8,
  11, 36, 6, 6, 11, 26, 12, 13, 26, 27,
  11, 6, 20, 15, -1, 26, 2, 3, 18, 26,
  28, 29, 26, 26, 26, 26, 26, 27, 24, 25,
  26, 26, 6, 7, 8, 6, 21, 22, -1, 24,
  11, 21, 22, 11, 24, 7, 8, 37, 38, 39,
  -1, 21, 22, 16, 24, 26, 2, 3, 7, 8,
  -1, 24, 25, 26, -1, 17, -1, 53, 20, 15,
  -1, 23, 40, 41, 26, 27, -1, -1, 17, 20,
  -1, 20, -1, -1, 23, 34, 35, 26, 27, 30,
  31, 32, 33, 2, 11, 4, -1, -1, 17, 45,
  9, 20, -1, -1, 23, -1, -1, 26, 27, -1,
  -1, -1, -1, -1, -1, 32, 33, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 46,
  47, 48, 49, 50, 51, 52, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, 2, 3, -1, -1,
  -1, -1, -1, -1, 10, -1, 2, -1, 4, 15,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, -1, -1, -1, -1, -1, -1, -1, 45,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,
  -1, 4, -1, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, 2, -1, 4, -1, 6,
  7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, -1, 2, -1, 4, -1, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 2, -1, 4, 5, 6,
  7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
  27, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, 45, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, 2, -1,
  4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
  14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
  24, 25, 26, 27, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 45, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
  11, 12, 13, 14, -1, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, 45, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, 2, -1, 4, 5, 6, 7,
  8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
  18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 45, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 2, 3, 4,
  5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
  -1, 16, 17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  45, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, -1, 16, 17, 18, 19, 20, 21,
  22, 23, 24, 25, 26, 27, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 45, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 2, 3, 4, 5, 6, 7, 8,
  9, 10, 11, 12, 13, 14, -1, 16, 17, 18,
  19, 20, 21, 22, 23, 24, 25, 26, 27, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, 45, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, 2, -1, 4, 5,
  6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
  26, 27, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 45,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 2,
  3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
  13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
  23, 24, 25, 26, 27, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, 45, -1, -1, -1, -1, -1, -1, -1,
  53, -1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
  20, 21, 22, 23, 24, 25, 26, 27, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, 45, -1, -1, -1, -1,
  -1, -1, -1, 53, -1,

  17, 20, 1, 1, 20, 17, 49, 17, 12, 17,
  51, 12, 12, 51, 16, 20, 13, 12, 12, 68,
  13, 17, 12, 12, 16, 13, 17, 12, 12, 12,
  17, 13, 12, 12, 20, 68, 12, 64, 13, 20,
  12, 12, 12, 20, 12, 68, 20, 12, 12, 12,
  12, 12, 12, 64, 12, 12, 12, 64, 64, 12,
  38, -1, 12, 36, 12, 20, -1, -1, 37, 12,
  37, 37, 17, 17, 20, 17, 13, 36, 17, 20,
  20, 17, 20, 12, -1, 37, 20, 13, 10, 10,
  10, -1, 17, 12, 45, 49, 21, 44, 17, 12,
  14, 12, 51, 17, 17, 10, 17, 10, 4, 6,
  12, 8, 10, 10, 10, 17, 51, 6, 51, 8,
  17, 10, 19, -1, -1, -1, -1, -1, 17, -1,
  19, -1, -1, 17, -1, 24, 25, -1, -1, -1,
  -1, -1, 75, -1, -1, -1, -1, 69, 69, 69,
  52, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, 69, -1, 69, -1, 73, -1,
  73, 69, -1, 69, -1, 73, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, 3, -1, 5,
  6, 7, 8, 9, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, 19, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1};


#include <QCoreApplication>
template <typename T> class QXmlStreamSimpleStack {
    T *data;
    int tos, cap;
public:
    inline QXmlStreamSimpleStack():data(0), tos(-1), cap(0){}
    inline ~QXmlStreamSimpleStack(){ if (data) qFree(data); }

    inline void reserve(int extraCapacity) {
        if (tos + extraCapacity + 1 > cap) {
            cap = qMax(tos + extraCapacity + 1, cap << 1 );
            data = reinterpret_cast<T *>(qRealloc(data, cap * sizeof(T)));
        }
    }

    inline T &push() { reserve(1); return data[++tos]; }
    inline T &rawPush() { return data[++tos]; }
    inline const T &top() const { return data[tos]; }
    inline T &top() { return data[tos]; }
    inline T &pop() { return data[tos--]; }
    inline T &operator[](int index) { return data[index]; }
    inline const T &at(int index) const { return data[index]; }
    inline int size() const { return tos + 1; }
    inline void resize(int s) { tos = s - 1; }
    inline bool isEmpty() const { return tos < 0; }
    inline void clear() { tos = -1; }
};


class QXmlStream
{
    Q_DECLARE_TR_FUNCTIONS(QXmlStream)
};

class QXmlStreamPrivateTagStack {
public:
    struct NamespaceDeclaration
    {
        QStringRef prefix;
        QStringRef namespaceUri;
    };

    struct Tag
    {
        QStringRef name;
        QStringRef qualifiedName;
        NamespaceDeclaration namespaceDeclaration;
        int tagStackStringStorageSize;
        int namespaceDeclarationsSize;
    };


    QXmlStreamPrivateTagStack();
    QXmlStreamSimpleStack<NamespaceDeclaration> namespaceDeclarations;
    QString tagStackStringStorage;
    int tagStackStringStorageSize;
    int tagStackDefaultStringStorageSize;
    bool tagsDone;

    inline QStringRef addToStringStorage(const QStringRef &s) {
        int pos = tagStackStringStorageSize;
	int sz = s.size();
	if (pos != tagStackStringStorage.size())
	    tagStackStringStorage.resize(pos);
        tagStackStringStorage.insert(pos, s.unicode(), sz);
        tagStackStringStorageSize += sz;
        return QStringRef(&tagStackStringStorage, pos, sz);
    }
    inline QStringRef addToStringStorage(const QString &s) {
        int pos = tagStackStringStorageSize;
	int sz = s.size();
	if (pos != tagStackStringStorage.size())
	    tagStackStringStorage.resize(pos);
        tagStackStringStorage.insert(pos, s.unicode(), sz);
        tagStackStringStorageSize += sz;
        return QStringRef(&tagStackStringStorage, pos, sz);
    }

    QXmlStreamSimpleStack<Tag> tagStack;

    inline void initTagStack() {
        tagStackStringStorageSize = tagStackDefaultStringStorageSize;
        namespaceDeclarations.resize(1);
    }

    inline Tag &tagStack_pop() {
        Tag& tag = tagStack.pop();
        tagStackStringStorageSize = tag.tagStackStringStorageSize;
        namespaceDeclarations.resize(tag.namespaceDeclarationsSize);
        tagsDone = tagStack.isEmpty();
        return tag;
    }
    inline Tag &tagStack_push() {
        Tag &tag = tagStack.push();
        tag.tagStackStringStorageSize = tagStackStringStorageSize;
        tag.namespaceDeclarationsSize = namespaceDeclarations.size();
        return tag;
    }
};

class QXmlStreamReaderPrivate : public QXmlStreamReader_Table, public QXmlStreamPrivateTagStack{
    QXmlStreamReader *q_ptr;
    Q_DECLARE_PUBLIC(QXmlStreamReader)
public:
    QXmlStreamReaderPrivate(QXmlStreamReader *q);
    ~QXmlStreamReaderPrivate();
    void init();

    QByteArray rawReadBuffer;
    QByteArray dataBuffer;
    uchar firstByte;
    qint64 nbytesread;
    QString readBuffer;
    int readBufferPos;
    QXmlStreamSimpleStack<uint> putStack;
    struct Entity {
        Entity(const QString& str = QString())
            :value(str), external(false), unparsed(false), literal(false),
             hasBeenParsed(false), isCurrentlyReferenced(false){}
        static inline Entity createLiteral(const QString &entity)
            { Entity result(entity); result.literal = result.hasBeenParsed = true; return result; }
        QString value;
        uint external : 1;
        uint unparsed : 1;
        uint literal : 1;
        uint hasBeenParsed : 1;
        uint isCurrentlyReferenced : 1;
    };
    QHash<QString, Entity> entityHash;
    QHash<QString, Entity> parameterEntityHash;
    QXmlStreamSimpleStack<Entity *>entityReferenceStack;
    inline bool referenceEntity(Entity &entity) {
        if (entity.isCurrentlyReferenced) {
            raiseWellFormedError(QXmlStream::tr("Recursive entity detected."));
            return false;
        }
        entity.isCurrentlyReferenced = true;
        entityReferenceStack.push() = &entity;
        injectToken(ENTITY_DONE);
        return true;
    }


    QIODevice *device;
    bool deleteDevice;
#ifndef QT_NO_TEXTCODEC
    QTextCodec *codec;
    QTextDecoder *decoder;
#endif
    bool atEnd;

    /*!
      \sa setType()
     */
    QXmlStreamReader::TokenType type;
    QXmlStreamReader::Error error;
    QString errorString;

    qint64 lineNumber, lastLineStart, characterOffset;


    void write(const QString &);
    void write(const char *);


    QXmlStreamAttributes attributes;
    QStringRef namespaceForPrefix(const QStringRef &prefix);
    void resolveTag();
    void resolvePublicNamespaces();
    void resolveDtd();
    uint resolveCharRef(int symbolIndex);
    bool checkStartDocument();
    void startDocument(const QStringRef &version);
    void parseError();
    void checkPublicLiteral(const QStringRef &publicId);

    bool scanDtd;
    QStringRef lastAttributeValue;
    bool lastAttributeIsCData;
    struct DtdAttribute {
        QStringRef tagName;
        QStringRef attributeQualifiedName;
        QStringRef attributePrefix;
        QStringRef attributeName;
        QStringRef defaultValue;
        bool isCDATA;
    };
    QXmlStreamSimpleStack<DtdAttribute> dtdAttributes;
    struct NotationDeclaration {
        QStringRef name;
        QStringRef publicId;
        QStringRef systemId;
    };
    QXmlStreamSimpleStack<NotationDeclaration> notationDeclarations;
    QXmlStreamNotationDeclarations publicNotationDeclarations;
    QXmlStreamNamespaceDeclarations publicNamespaceDeclarations;

    struct EntityDeclaration {
        QStringRef name;
        QStringRef notationName;
        QStringRef publicId;
        QStringRef systemId;
        QStringRef value;
        bool parameter;
        bool external;
        inline void clear() {
            name.clear();
            notationName.clear();
            publicId.clear();
            systemId.clear();
            value.clear();
            parameter = external = false;
        }
    };
    QXmlStreamSimpleStack<EntityDeclaration> entityDeclarations;
    QXmlStreamEntityDeclarations publicEntityDeclarations;

    QStringRef text;

    QStringRef prefix, namespaceUri, qualifiedName, name;
    QStringRef processingInstructionTarget, processingInstructionData;
    uint isEmptyElement : 1;
    uint isWhitespace : 1;
    uint isCDATA : 1;
    uint standalone : 1;
    uint hasCheckedStartDocument : 1;
    uint normalizeLiterals : 1;
    uint hasSeenTag : 1;
    uint inParseEntity : 1;
    uint referenceToUnparsedEntityDetected : 1;
    uint referenceToParameterEntityDetected : 1;
    uint hasExternalDtdSubset : 1;
    uint lockEncoding : 1;
    uint namespaceProcessing : 1;

    int resumeReduction;
    void resume(int rule);

    inline bool entitiesMustBeDeclared() const {
        return (!inParseEntity
                && (standalone
                    || (!referenceToUnparsedEntityDetected
                        && !referenceToParameterEntityDetected // Errata 13 as of 2006-04-25
                        && !hasExternalDtdSubset)));
    }

    // qlalr parser
    int tos;
    int stack_size;
    struct Value {
        int pos;
        int len;
        int prefix;
        ushort c;
    };

    Value *sym_stack;
    int *state_stack;
    inline void reallocateStack();
    inline Value &sym(int index) const
    { return sym_stack[tos + index - 1]; }
    QString textBuffer;
    inline void clearTextBuffer() {
        if (!scanDtd) {
            textBuffer.resize(0);
            textBuffer.reserve(256);
        }
    }
    struct Attribute {
        Value key;
        Value value;
    };
    QXmlStreamSimpleStack<Attribute> attributeStack;

    inline QStringRef symString(int index) {
        const Value &symbol = sym(index);
        return QStringRef(&textBuffer, symbol.pos + symbol.prefix, symbol.len - symbol.prefix);
    }
    inline QStringRef symName(int index) {
        const Value &symbol = sym(index);
        return QStringRef(&textBuffer, symbol.pos, symbol.len);
    }
    inline QStringRef symString(int index, int offset) {
        const Value &symbol = sym(index);
        return QStringRef(&textBuffer, symbol.pos + symbol.prefix + offset, symbol.len - symbol.prefix -  offset);
    }
    inline QStringRef symPrefix(int index) {
        const Value &symbol = sym(index);
        if (symbol.prefix)
            return QStringRef(&textBuffer, symbol.pos, symbol.prefix - 1);
        return QStringRef();
    }
    inline QStringRef symString(const Value &symbol) {
        return QStringRef(&textBuffer, symbol.pos + symbol.prefix, symbol.len - symbol.prefix);
    }
    inline QStringRef symName(const Value &symbol) {
        return QStringRef(&textBuffer, symbol.pos, symbol.len);
    }
    inline QStringRef symPrefix(const Value &symbol) {
        if (symbol.prefix)
            return QStringRef(&textBuffer, symbol.pos, symbol.prefix - 1);
        return QStringRef();
    }

    inline void clearSym() { Value &val = sym(1); val.pos = textBuffer.size(); val.len = 0; }


    short token;
    ushort token_char;

    uint filterCarriageReturn();
    inline uint getChar();
    inline uint peekChar();
    inline void putChar(uint c) { putStack.push() = c; }
    inline void putChar(QChar c) { putStack.push() =  c.unicode(); }
    void putString(const QString &s, int from = 0);
    void putStringLiteral(const QString &s);
    void putReplacement(const QString &s);
    void putReplacementInAttributeValue(const QString &s);
    ushort getChar_helper();

    bool scanUntil(const char *str, short tokenToInject = -1);
    bool scanString(const char *str, short tokenToInject, bool requireSpace = true);
    inline void injectToken(ushort tokenToInject) {
        putChar(int(tokenToInject) << 16);
    }

    static bool validateName(const QStringRef &name);

    void parseEntity(const QString &value);
    QXmlStreamReaderPrivate *entityParser;

    bool scanAfterLangleBang();
    bool scanPublicOrSystem();
    bool scanNData();
    bool scanAfterDefaultDecl();
    bool scanAttType();


    // scan optimization functions. Not strictly necessary but LALR is
    // not very well suited for scanning fast
    int fastScanLiteralContent();
    int fastScanSpace();
    int fastScanContentCharList();
    int fastScanName(int *prefix = 0);
    inline int fastScanNMTOKEN();


    bool parse();
    inline void consumeRule(int);

    void raiseError(QXmlStreamReader::Error error, const QString& message = QString());
    void raiseWellFormedError(const QString &message);

private:
    /*! \internal
       Never assign to variable type directly. Instead use this function.

       This prevents errors from being ignored.
     */
    inline void setType(const QXmlStreamReader::TokenType t)
    {
        if(type != QXmlStreamReader::Invalid)
            type = t;
    }
};

bool QXmlStreamReaderPrivate::parse()
{
    // cleanup currently reported token

    switch (type) {
    case QXmlStreamReader::StartElement:
        name.clear();
        prefix.clear();
	qualifiedName.clear();
        namespaceUri.clear();
        if (publicNamespaceDeclarations.size())
            publicNamespaceDeclarations.clear();
        if (attributes.size())
            attributes.resize(0);
        if (isEmptyElement) {
            setType(QXmlStreamReader::EndElement);
            Tag &tag = tagStack_pop();
            namespaceUri = tag.namespaceDeclaration.namespaceUri;
            name = tag.name;
	    qualifiedName = tag.qualifiedName;
            isEmptyElement = false;
            return true;
        }
        clearTextBuffer();
        break;
    case QXmlStreamReader::EndElement:
        name.clear();
        prefix.clear();
	qualifiedName.clear();
        namespaceUri.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::DTD:
        publicNotationDeclarations.clear();
        publicEntityDeclarations.clear();
        // fall through
    case QXmlStreamReader::Comment:
    case QXmlStreamReader::Characters:
        isCDATA = isWhitespace = false;
        text.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::EntityReference:
        text.clear();
        name.clear();
        clearTextBuffer();
        break;
    case QXmlStreamReader::ProcessingInstruction:
        processingInstructionTarget.clear();
        processingInstructionData.clear();
	clearTextBuffer();
        break;
    case QXmlStreamReader::NoToken:
    case QXmlStreamReader::Invalid:
        break;
    case QXmlStreamReader::StartDocument:
	lockEncoding = true;
#ifndef QT_NO_TEXTCODEC
	if(decoder->hasFailure()) {
	    raiseWellFormedError(QXmlStream::tr("Encountered incorrectly encoded content."));
	    readBuffer.clear();
	    return false;
	}
#endif
        // fall through
    default:
        clearTextBuffer();
        ;
    }

    setType(QXmlStreamReader::NoToken);


    // the main parse loop
    int act, r;

    if (resumeReduction) {
        act = state_stack[tos-1];
        r = resumeReduction;
        resumeReduction = 0;
        goto ResumeReduction;
    }

    act = state_stack[tos];

    forever {
        if (token == -1 && - TERMINAL_COUNT != action_index[act]) {
            uint cu = getChar();
            token = NOTOKEN;
            token_char = cu;
            if (cu & 0xff0000) {
                token = cu >> 16;
            } else switch (token_char) {
            case 0xfffe:
            case 0xffff:
                token = ERROR;
                break;
            case '\r':
                token = SPACE;
                if (cu == '\r') {
                    if ((token_char = filterCarriageReturn())) {
                        ++lineNumber;
                        lastLineStart = characterOffset + readBufferPos;
                        break;
                    }
                } else {
                    break;
                }
                // fall through
            case '\0': {
                token = EOF_SYMBOL;
                if (!tagsDone && !inParseEntity) {
                    int a = t_action(act, token);
                    if (a < 0) {
                        raiseError(QXmlStreamReader::PrematureEndOfDocumentError);
                        return false;
                    }
                }

            } break;
            case '\n':
                ++lineNumber;
                lastLineStart = characterOffset + readBufferPos;
            case ' ':
            case '\t':
                token = SPACE;
                break;
            case '&':
                token = AMPERSAND;
                break;
            case '#':
                token = HASH;
                break;
            case '\'':
                token = QUOTE;
                break;
            case '\"':
                token = DBLQUOTE;
                break;
            case '<':
                token = LANGLE;
                break;
            case '>':
                token = RANGLE;
                break;
            case '[':
                token = LBRACK;
                break;
            case ']':
                token = RBRACK;
                break;
            case '(':
                token = LPAREN;
                break;
            case ')':
                token = RPAREN;
                break;
            case '|':
                token = PIPE;
                break;
            case '=':
                token = EQ;
                break;
            case '%':
                token = PERCENT;
                break;
            case '/':
                token = SLASH;
                break;
            case ':':
                token = COLON;
                break;
            case ';':
                token = SEMICOLON;
                break;
            case ',':
                token = COMMA;
                break;
            case '-':
                token = DASH;
                break;
            case '+':
                token = PLUS;
                break;
            case '*':
                token = STAR;
                break;
            case '.':
                token = DOT;
                break;
            case '?':
                token = QUESTIONMARK;
                break;
            case '!':
                token = BANG;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                token = DIGIT;
                break;
            default:
                if (cu < 0x20)
                    token = NOTOKEN;
                else
                    token = LETTER;
                break;
            }
        }

        act = t_action (act, token);
        if (act == ACCEPT_STATE) {
            // reset the parser in case someone resumes (process instructions can follow a valid document)
            tos = 0;
            state_stack[tos++] = 0;
            state_stack[tos] = 0;
            return true;
        } else if (act > 0) {
            if (++tos == stack_size)
                reallocateStack();

            Value &val = sym_stack[tos];
            val.c = token_char;
            val.pos = textBuffer.size();
            val.prefix = 0;
            val.len = 1;
            if (token_char)
                textBuffer.inline_append(token_char);

            state_stack[tos] = act;
            token = -1;


        } else if (act < 0) {
            r = - act - 1;

#if defined (QLALR_DEBUG)
            int ridx = rule_index[r];
            printf ("%3d) %s ::=", r + 1, spell[rule_info[ridx]]);
            ++ridx;
            for (int i = ridx; i < ridx + rhs[r]; ++i) {
                int symbol = rule_info[i];
                if (const char *name = spell[symbol])
                    printf (" %s", name);
                else
                    printf (" #%d", symbol);
            }
            printf ("\n");
#endif

            tos -= rhs[r];
            act = state_stack[tos++];
        ResumeReduction:
            switch (r) {

        case 0:
            setType(QXmlStreamReader::EndDocument);
        break;

        case 1:
            if (type != QXmlStreamReader::Invalid) {
                if (hasSeenTag || inParseEntity) {
                    setType(QXmlStreamReader::EndDocument);
                } else {
                    raiseError(QXmlStreamReader::PrematureEndOfDocumentError, QXmlStream::tr("Start tag expected."));
                    // reset the parser
                    tos = 0;
                    state_stack[tos++] = 0;
                    state_stack[tos] = 0;
                    return false;
                }
            }
        break;

        case 10:
            entityReferenceStack.pop()->isCurrentlyReferenced = false;
            clearSym();
        break;

        case 11:
            if (!scanString(spell[VERSION], VERSION, false) && atEnd) {
                resume(11);
                return false;
            }
        break;

        case 12:
            setType(QXmlStreamReader::StartDocument);
            startDocument(symString(6));
        break;

        case 13:
            hasExternalDtdSubset = true;
        break;

        case 14:
            checkPublicLiteral(symString(2));
            hasExternalDtdSubset = true;
        break;

        case 16:
            if (!scanPublicOrSystem() && atEnd) {
                resume(16);
                return false;
            }
        break;

        case 17:
        case 18:
        case 19:
        case 20:
            setType(QXmlStreamReader::DTD);
            text = &textBuffer;
        break;

        case 21:
            scanDtd = true;
        break;

        case 22:
            scanDtd = false;
        break;

        case 36:
            if (!scanString(spell[EMPTY], EMPTY, false)
                && !scanString(spell[ANY], ANY, false)
                && atEnd) {
                resume(36);
                return false;
            }
        break;

        case 42:
            if (!scanString(spell[PCDATA], PCDATA, false) && atEnd) {
                resume(42);
                return false;
            }
        break;

        case 67: {
            lastAttributeIsCData = true;
        } break;

        case 77:
            if (!scanAfterDefaultDecl() && atEnd) {
                resume(77);
                return false;
            }
        break;

        case 82:
                sym(1) = sym(2);
                lastAttributeValue.clear();
                lastAttributeIsCData = false;
                if (!scanAttType() && atEnd) {
                    resume(82);
                    return false;
                }
        break;

        case 83: {
            DtdAttribute &dtdAttribute = dtdAttributes.push();
            dtdAttribute.tagName.clear();
            dtdAttribute.isCDATA = lastAttributeIsCData;
            dtdAttribute.attributePrefix = addToStringStorage(symPrefix(1));
            dtdAttribute.attributeName = addToStringStorage(symString(1));
            dtdAttribute.attributeQualifiedName = addToStringStorage(symName(1));
            if (lastAttributeValue.isNull()) {
                dtdAttribute.defaultValue.clear();
            } else {
                if (dtdAttribute.isCDATA)
                    dtdAttribute.defaultValue = addToStringStorage(lastAttributeValue);
                else
                    dtdAttribute.defaultValue = addToStringStorage(lastAttributeValue.toString().simplified());

            }
        } break;

        case 87: {
            if (referenceToUnparsedEntityDetected && !standalone)
                break;
            int n = dtdAttributes.size();
            QStringRef tagName = addToStringStorage(symString(3));
            while (n--) {
                DtdAttribute &dtdAttribute = dtdAttributes[n];
                if (!dtdAttribute.tagName.isNull())
                    break;
                dtdAttribute.tagName = tagName;
                for (int i = 0; i < n; ++i) {
                    if ((dtdAttributes[i].tagName.isNull() || dtdAttributes[i].tagName == tagName)
                        && dtdAttributes[i].attributeQualifiedName == dtdAttribute.attributeQualifiedName) {
                        dtdAttribute.attributeQualifiedName.clear(); // redefined, delete it
                        break;
                    }
                }
            }
        } break;

        case 88: {
            if (!scanPublicOrSystem() && atEnd) {
                resume(88);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.push();
            entityDeclaration.clear();
            entityDeclaration.name = symString(3);
        } break;

        case 89: {
            if (!scanPublicOrSystem() && atEnd) {
                resume(89);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.push();
            entityDeclaration.clear();
            entityDeclaration.name = symString(5);
            entityDeclaration.parameter = true;
        } break;

        case 90: {
            if (!scanNData() && atEnd) {
                resume(90);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            entityDeclaration.systemId = symString(3);
            entityDeclaration.external = true;
        } break;

        case 91: {
            if (!scanNData() && atEnd) {
                resume(91);
                return false;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            checkPublicLiteral((entityDeclaration.publicId = symString(3)));
            entityDeclaration.systemId = symString(5);
            entityDeclaration.external = true;
        } break;

        case 92: {
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            entityDeclaration.notationName = symString(3);
            if (entityDeclaration.parameter)
                raiseWellFormedError(QXmlStream::tr("NDATA in parameter entity declaration."));
        }
        //fall through

        case 93:
        case 94: {
            if (referenceToUnparsedEntityDetected && !standalone) {
                entityDeclarations.pop();
                break;
            }
            EntityDeclaration &entityDeclaration = entityDeclarations.top();
            if (!entityDeclaration.external)
                entityDeclaration.value = symString(2);
            QString entityName = entityDeclaration.name.toString();
            QHash<QString, Entity> &hash = entityDeclaration.parameter ? parameterEntityHash : entityHash;
            if (!hash.contains(entityName)) {
                Entity entity(entityDeclaration.value.toString());
                entity.unparsed = (!entityDeclaration.notationName.isNull());
                entity.external = entityDeclaration.external;
                hash.insert(entityName, entity);
            }
        } break;

        case 95: {
            setType(QXmlStreamReader::ProcessingInstruction);
            int pos = sym(4).pos + sym(4).len;
            processingInstructionTarget = symString(3);
            if (scanUntil("?>")) {
                processingInstructionData = QStringRef(&textBuffer, pos, textBuffer.size() - pos - 2);
                const QString piTarget(processingInstructionTarget.toString());
                if (!piTarget.compare(QLatin1String("xml"), Qt::CaseInsensitive)) {
                    raiseWellFormedError(QXmlStream::tr("XML declaration not at start of document."));
                }
                else if(!QXmlUtils::isNCName(piTarget))
                    raiseWellFormedError(QXmlStream::tr("%1 is an invalid processing instruction name.").arg(piTarget));
            } else if (type != QXmlStreamReader::Invalid){
                resume(95);
                return false;
            }
        } break;

        case 96:
            setType(QXmlStreamReader::ProcessingInstruction);
            processingInstructionTarget = symString(3);
            if (!processingInstructionTarget.toString().compare(QLatin1String("xml"), Qt::CaseInsensitive))
                raiseWellFormedError(QXmlStream::tr("Invalid processing instruction name."));
        break;

        case 97:
            if (!scanAfterLangleBang() && atEnd) {
                resume(97);
                return false;
            }
        break;

        case 98:
            if (!scanUntil("--")) {
                resume(98);
                return false;
            }
        break;

        case 99: {
            setType(QXmlStreamReader::Comment);
            int pos = sym(1).pos + 4;
            text = QStringRef(&textBuffer, pos, textBuffer.size() - pos - 3);
        } break;

        case 100: {
            setType(QXmlStreamReader::Characters);
            isCDATA = true;
            int pos = sym(2).pos;
            if (scanUntil("]]>", -1)) {
                text = QStringRef(&textBuffer, pos, textBuffer.size() - pos - 3);
            } else {
                resume(100);
                return false;
            }
        } break;

        case 101: {
            if (!scanPublicOrSystem() && atEnd) {
                resume(101);
                return false;
            }
            NotationDeclaration &notationDeclaration = notationDeclarations.push();
            notationDeclaration.name = symString(3);
        } break;

        case 102: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            notationDeclaration.systemId = symString(3);
            notationDeclaration.publicId.clear();
        } break;

        case 103: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            notationDeclaration.systemId.clear();
            checkPublicLiteral((notationDeclaration.publicId = symString(3)));
        } break;

        case 104: {
            NotationDeclaration &notationDeclaration = notationDeclarations.top();
            checkPublicLiteral((notationDeclaration.publicId = symString(3)));
            notationDeclaration.systemId = symString(5);
        } break;

        case 126:
            isWhitespace = true;
            // fall through

        case 127:
        case 128:
        case 129:
            setType(QXmlStreamReader::Characters);
            sym(1).len += fastScanContentCharList();
            if (atEnd && !inParseEntity) {
                resume(129);
                return false;
            }
            text = &textBuffer;
        break;

        case 130:
        case 131:
            clearSym();
        break;

        case 132:
        case 133:
            sym(1) = sym(2);
        break;

        case 134:
        case 135:
        case 136:
        case 137:
            sym(1).len += sym(2).len;
        break;

        case 163:
	    if (normalizeLiterals)
                textBuffer.data()[textBuffer.size()-1] = QLatin1Char(' ');
        break;

        case 164:
            sym(1).len += fastScanLiteralContent();
            if (atEnd) {
                resume(164);
                return false;
            }
        break;

        case 165: {
            if (!QXmlUtils::isPublicID(symString(1).toString())) {
                raiseWellFormedError(QXmlStream::tr("%1 is an invalid PUBLIC identifier.").arg(symString(1).toString()));
                resume(165);
                return false;
            }
        } break;

        case 166:
        case 167:
            clearSym();
        break;

        case 168:
        case 169:
	    sym(1) = sym(2);
        break;

        case 170:
        case 171:
        case 172:
        case 173:
            sym(1).len += sym(2).len;
        break;

        case 203:
        case 204:
            clearSym();
        break;

        case 205:
        case 206:
            sym(1) = sym(2);
            lastAttributeValue = symString(1);
        break;

        case 207:
        case 208:
        case 209:
        case 210:
            sym(1).len += sym(2).len;
        break;

        case 219: {
            QStringRef prefix = symPrefix(1);
            if (prefix.isEmpty() && symString(1) == QLatin1String("xmlns") && namespaceProcessing) {
                NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                namespaceDeclaration.prefix.clear();

                const QStringRef ns(symString(5));
                if(ns == QLatin1String("http://www.w3.org/2000/xmlns/") ||
                   ns == QLatin1String("http://www.w3.org/XML/1998/namespace"))
                    raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));
                else
                    namespaceDeclaration.namespaceUri = addToStringStorage(ns);
            } else {
                Attribute &attribute = attributeStack.push();
                attribute.key = sym(1);
                attribute.value = sym(5);

                QStringRef attributeQualifiedName = symName(1);
                bool normalize = false;
                for (int a = 0; a < dtdAttributes.size(); ++a) {
                    DtdAttribute &dtdAttribute = dtdAttributes[a];
                    if (!dtdAttribute.isCDATA
                        && dtdAttribute.tagName == qualifiedName
                        && dtdAttribute.attributeQualifiedName == attributeQualifiedName
                        ) {
                        normalize = true;
                        break;
                    }
                }
                if (normalize) {
                    // normalize attribute value (simplify and trim)
                    int pos = textBuffer.size();
                    int n = 0;
                    bool wasSpace = true;
                    for (int i = 0; i < attribute.value.len; ++i) {
                        QChar c = textBuffer.at(attribute.value.pos + i);
                        if (c.unicode() == ' ') {
                            if (wasSpace)
                                continue;
                            wasSpace = true;
                        } else {
                            wasSpace = false;
                        }
                        textBuffer.inline_append(textBuffer.at(attribute.value.pos + i));
                        ++n;
                    }
                    if (wasSpace)
                        while (n && textBuffer.at(pos + n - 1).unicode() == ' ')
                            --n;
                    attribute.value.pos = pos;
                    attribute.value.len = n;
                }
                if (prefix == QLatin1String("xmlns") && namespaceProcessing) {
                    NamespaceDeclaration &namespaceDeclaration = namespaceDeclarations.push();
                    QStringRef namespacePrefix = symString(attribute.key);
                    QStringRef namespaceUri = symString(attribute.value);
                    attributeStack.pop();
                    if ((namespacePrefix == QLatin1String("xml")
                         ^ namespaceUri == QLatin1String("http://www.w3.org/XML/1998/namespace"))
                        || namespaceUri == QLatin1String("http://www.w3.org/2000/xmlns/")
                        || namespaceUri.isEmpty()
                        || namespacePrefix == QLatin1String("xmlns"))
                        raiseWellFormedError(QXmlStream::tr("Illegal namespace declaration."));

                    namespaceDeclaration.prefix = addToStringStorage(namespacePrefix);
                    namespaceDeclaration.namespaceUri = addToStringStorage(namespaceUri);
                }
            }
        } break;

        case 225: {
            normalizeLiterals = true;
            Tag &tag = tagStack_push();
            prefix = tag.namespaceDeclaration.prefix  = addToStringStorage(symPrefix(2));
            name = tag.name = addToStringStorage(symString(2));
            qualifiedName = tag.qualifiedName = addToStringStorage(symName(2));
            if (!validateName(qualifiedName))
                raiseWellFormedError(QXmlStream::tr("Invalid XML name."));
        } break;

        case 226:
            isEmptyElement = true;
        // fall through

        case 227:
            setType(QXmlStreamReader::StartElement);
            resolveTag();
            if (tagStack.size() == 1 && hasSeenTag && !inParseEntity)
                raiseWellFormedError(QXmlStream::tr("Extra content at end of document."));
            hasSeenTag = true;
        break;

        case 228: {
            setType(QXmlStreamReader::EndElement);
            Tag &tag = tagStack_pop();

            namespaceUri = tag.namespaceDeclaration.namespaceUri;
            name = tag.name;
            qualifiedName = tag.qualifiedName;
            if (qualifiedName != symName(3))
                raiseWellFormedError(QXmlStream::tr("Opening and ending tag mismatch."));
        } break;

        case 229: {
            sym(1).len += sym(2).len + 1;
            QString reference = symString(2).toString();
            if (entityHash.contains(reference)) {
                Entity &entity = entityHash[reference];
                if (entity.unparsed) {
                    raiseWellFormedError(QXmlStream::tr("Reference to unparsed entity '%1'.").arg(reference));
                } else {
                    if (!entity.hasBeenParsed) {
                        parseEntity(entity.value);
                        entity.hasBeenParsed = true;
                    }
                    if (entity.literal)
                        putStringLiteral(entity.value);
                    else if (referenceEntity(entity))
                        putReplacement(entity.value);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                }
                break;
            } else if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(reference));
                break;
            }
            setType(QXmlStreamReader::EntityReference);
            name = symString(2);

        } break;

        case 230: {
            sym(1).len += sym(2).len + 1;
            QString reference = symString(2).toString();
            if (parameterEntityHash.contains(reference)) {
                referenceToParameterEntityDetected = true;
                Entity &entity = parameterEntityHash[reference];
                if (entity.unparsed || entity.external) {
                    referenceToUnparsedEntityDetected = true;
                } else {
                    if (referenceEntity(entity))
                        putString(entity.value);
                    textBuffer.chop(2 + sym(2).len);
                    clearSym();
                }
            } else if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(symString(2).toString()));
            }
        } break;

        case 231:
            sym(1).len += sym(2).len + 1;
        break;

        case 232: {
            sym(1).len += sym(2).len + 1;
            QString reference = symString(2).toString();
            if (entityHash.contains(reference)) {
                Entity &entity = entityHash[reference];
                if (entity.unparsed || entity.value.isNull()) {
                    raiseWellFormedError(QXmlStream::tr("Reference to external entity '%1' in attribute value.").arg(reference));
                    break;
                }
                if (!entity.hasBeenParsed) {
                    parseEntity(entity.value);
                    entity.hasBeenParsed = true;
                }
                if (entity.literal)
                    putStringLiteral(entity.value);
                else if (referenceEntity(entity))
                    putReplacementInAttributeValue(entity.value);
                textBuffer.chop(2 + sym(2).len);
                clearSym();
            } else if (entitiesMustBeDeclared()) {
                raiseWellFormedError(QXmlStream::tr("Entity '%1' not declared.").arg(reference));
            }
        } break;

        case 233: {
            if (uint s = resolveCharRef(3)) {
                if (s >= 0xffff)
                    putStringLiteral(QString::fromUcs4(&s, 1));
                else
                    putChar((LETTER << 16) | s);

                textBuffer.chop(3 + sym(3).len);
                clearSym();
            } else {
                raiseWellFormedError(QXmlStream::tr("Invalid character reference."));
            }
        } break;

        case 236:
        case 237:
            sym(1).len += sym(2).len;
        break;

        case 250:
            sym(1).len += fastScanSpace();
            if (atEnd) {
                resume(250);
                return false;
            }
        break;

        case 253: {
            sym(1).len += fastScanName(&sym(1).prefix);
            if (atEnd) {
                resume(253);
                return false;
            }
        } break;

        case 254:
            sym(1).len += fastScanName();
            if (atEnd) {
                resume(254);
                return false;
            }
        break;

        case 255:
        case 256:
        case 257:
        case 258:
        case 259:
            sym(1).len += fastScanNMTOKEN();
            if (atEnd) {
                resume(259);
                return false;
            }

        break;

    default:
        ;
    } // switch
            act = state_stack[tos] = nt_action (act, lhs[r] - TERMINAL_COUNT);
            if (type != QXmlStreamReader::NoToken)
                return true;
        } else {
            parseError();
            break;
        }
    }
    return false;
}

#endif // QXMLSTREAM_P_H

