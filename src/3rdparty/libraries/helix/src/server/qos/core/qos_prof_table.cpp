/* C++ code produced by gperf version 2.7.2 */
/* Command-line: gperf -t -c -C -I -S 1 -L C++ -N SelectProfileId -Z ProfileLookupTable ./qos_prof_table.gperf  */
/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: qos_prof_table.cpp,v 1.2 2003/04/08 19:26:47 damonlan Exp $ 
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
/* This code is automatically generated, do not edit the .cpp file! */
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxqossig.h"
#include "hxqos.h"
struct qos_profile_id { char *name; HX_QOS_PROFILE_TYPE type; };
#include <string.h>

#define TOTAL_KEYWORDS 4
#define MIN_WORD_LENGTH 42
#define MAX_WORD_LENGTH 63
#define MIN_HASH_VALUE 42
#define MAX_HASH_VALUE 63
/* maximum key range = 22, duplicates = 0 */

class ProfileLookupTable
{
private:
  static inline unsigned int hash (const char *str, unsigned int len);
public:
  static const struct qos_profile_id *SelectProfileId (const char *str, unsigned int len);
};

inline unsigned int
ProfileLookupTable::hash (register const char *str, register unsigned int len)
{
  static const unsigned char asso_values[] =
    {
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64,  0, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64,  0, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
      64, 64, 64, 64, 64, 64
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

const struct qos_profile_id *
ProfileLookupTable::SelectProfileId (register const char *str, register unsigned int len)
{
  static const struct qos_profile_id wordlist[] =
    {
      {"example-gen-useragent;example-gen-datatype",  HX_QOS_PROFILE_RTP_GENERIC },
      {"example-rma-useragent;example-rma-datatype;example-rma-xport", HX_QOS_PROFILE_RMA },
      {"example-annexg-useragent;example-ag-datatype;example-ag-xport", HX_QOS_PROFILE_3GPP_ANNEXG },
      {"example-3gpp-useragent;example-3gpp-datatype;example-3gpp-xport", HX_QOS_PROFILE_3GPP_REL5}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
        {
          register const struct qos_profile_id *resword;

          switch (key - 42)
            {
              case 0:
                resword = &wordlist[0];
                goto compare;
              case 18:
                resword = &wordlist[1];
                goto compare;
              case 19:
                resword = &wordlist[2];
                goto compare;
              case 21:
                resword = &wordlist[3];
                goto compare;
            }
          return 0;
        compare:
          {
            register const char *s = resword->name;

            if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
              return resword;
          }
        }
    }
  return 0;
}
