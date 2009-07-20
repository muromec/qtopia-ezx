/*
 * ===================================================================
 *  TS 26.104
 *  R99   V3.4.0 2002-02
 *  REL-4 V4.3.0 2002-02
 *  3GPP AMR Floating-point Speech Codec
 * ===================================================================
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include "hlxclib/memory.h"
#include "interf_dec.h"
#include "typedef.h"

void Copyright(void){
fprintf (stderr,
"===================================================================\n"
" TS 26.104                                                         \n"
" R99   V3.4.0 2002-02                                              \n"
" REL-4 V4.3.0 2002-02                                              \n"
" 3GPP AMR Floating-point Speech Decoder                            \n"
"===================================================================\n"
);
}
/*
 * main
 *
 *
 * Function:
 *    Speech decoder main program
 *
 *    Usage: decoder bitstream_file synthesis_file
 *
 *    Format for ETSI bitstream file:
 *       1 word (2-byte) for the TX frame type
 *       244 words (2-byte) containing 244 bits.
 *          Bit 0 = 0x0000 and Bit 1 = 0x0001
 *       1 word (2-byte) for the mode indication
 *       4 words for future use, currently written as zero
 *
 *    Format for 3GPP bitstream file:
 *       Holds mode information and bits packed to octets.
 *       Size is from 1 byte to 31 bytes.
 *
 *    Format for synthesis_file:
 *       Speech is written to a 16 bit 8kHz file.
 *
 *    ETSI bitstream file format is defined using ETSI as preprocessor
 *    definition
 * Returns:
 *    0
 */
int main (int argc, char * argv[]){

   FILE * file_speech, *file_analysis;

   short synth[160];
   int frames = 0;
   int * destate;
   int read_size;
#ifndef ETSI
   unsigned char analysis[31];
   enum Mode dec_mode;
#else
   short analysis[250];
#endif

   /* Process command line options */
   if (argc == 3){

      file_speech = fopen(argv[2], "wb");
      if (file_speech == NULL){
         fprintf ( stderr, "%s%s%s\n","Use: ",argv[0], " input.file output.file " );
         return 1;
      }

      file_analysis = fopen(argv[1], "rb");
      if (file_analysis == NULL){
         fprintf ( stderr, "%s%s%s\n","Use: ",argv[0], " input.file output.file " );
         fclose(file_speech);
         return 1;
      }

   }
   else {
      fprintf ( stderr, "%s%s%s\n","Use: ",argv[0], " input.file output.file " );
      return 1;
   }
   Copyright();
   /* init decoder */
   destate = Decoder_Interface_init();

#ifndef ETSI

   /* find mode, read file */
   while (fread(analysis, sizeof (unsigned char), 1, file_analysis ) > 0)
   {
      dec_mode = analysis[0] & 0x000F;
      switch (dec_mode){
      case 0:
         read_size = 12;
         break;
      case 1:
         read_size = 13;
         break;
      case 2:
         read_size = 15;
         break;
      case 3:
         read_size = 17;
         break;
      case 4:
         read_size = 18;
         break;
      case 5:
         read_size = 20;
         break;
      case 6:
         read_size = 25;
         break;
      case 7:
         read_size = 30;
         break;
      case 8:
         read_size = 5;
         break;
      case 15:
         read_size = 0;
      default:
         read_size = 0;
         break;
      };
      fread(&analysis[1], sizeof (char), read_size, file_analysis );
#else

   read_size = 250;
   /* read file */
   while (fread(analysis, sizeof (short), read_size, file_analysis ) > 0)
   {
#endif

      frames ++;

      /* call decoder */
      Decoder_Interface_Decode(destate, analysis, synth, 0);

      fwrite( synth, sizeof (short), 160, file_speech );
   }

   Decoder_Interface_exit(destate);

   fclose(file_speech);
   fclose(file_analysis);
   fprintf ( stderr, "\n%s%i%s\n","Decoded ", frames, " frames.");

   return 0;
}
