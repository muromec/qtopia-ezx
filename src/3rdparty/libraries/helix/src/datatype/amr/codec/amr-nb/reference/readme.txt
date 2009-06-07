To build the AMR codec you'll need to follow the following steps.

1. Download the AMR reference source from 
   ftp://ftp.3gpp.org/specs/latest/Rel-5/26_series/26104-500.zip

2. Unzip the 26104-500.zip file

3. Unzip the 26104-500_ANSI-C_source_code.zip file that is contained
  in 26104-500.zip

4. Edit your .buildrc file to contain a line similar to the following
   
   SetSDKPath("amr_nb_src", "d:/26104-500/26104-500_ANSI-C_source_code")

   The path should point to the directory the AMR source is in

5. Run umake to generate the Makefile

6. Use the generated makefile to build the code
