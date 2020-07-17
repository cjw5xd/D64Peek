/*
Copyright 2020 Cody Williams

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.

  3. Neither the name of the copyright holder nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "help.h"

#define TRUE 1
#define FALSE 0

typedef uint8_t BYTE;


// PETSCII to ASCII translation
const char * PETSCII = "................................ !\"#$%&'()*+,-./01234"
  "56789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_.ABCDEFGHIJKLMNOPQRSTUVWXYZ...."
  "................................. ................................ABCDEFGHI"
  "JKLMNOPQRSTUVWXYZ.....................................";


// Command line options
int opt_hex_ts = FALSE;
int opt_hide_petscii = FALSE;
int opt_show_help = FALSE;


/*
Print the message and exit with execution failure status.
*/
void die(const char * const message)
{
    printf("\033[1md64peek:\033[0m ");
    puts(message);
    exit(EXIT_FAILURE);
}


/*
Copy the next sector from the D64 file into the byte array.
*/
BYTE * read(BYTE * const data,
            FILE * const file)
{
    for(size_t i = 0; i < 256; i++) {
        data[i] = getc(file);
    }

    return data;
}


/*
Given the sector index, assign the track and sector numbers with the
corresponding values for the T/S pair.
*/
void tsp(const size_t index,
         size_t * const track,
         size_t * const sector)
{
    if(index < 357) {
        *track = index / 21 + 1;
        *sector = index % 21;
    }
    else if(index < 490) {
        *track = (index - 357) / 19 + 18;
        *sector = (index - 357) % 19;
    }
    else if(index < 598) {
        *track = (index - 490) / 18 + 25;
        *sector = (index - 490) % 18;
    }
    else {
        *track = (index - 598) / 17 + 31;
        *sector = (index - 598) % 17;
    }
}


/*
Print the sector data to the standard output.
*/
void dump(const BYTE * const data,
          const size_t track,
          const size_t sector,
          int * const first)
{
    const char * SECTOR_FMT = opt_hex_ts ? "S=%02X   " : "S=%02d   ";
    const char * TRACK_FMT = opt_hex_ts ? "T=%02X   " : "T=%02d   ";

    if(*first) {
      *first = FALSE;
    }
    else {
        putchar('\n');
    }

    for(size_t i = 0; i < 16; i++) {
        if(i == 0) {
            printf(TRACK_FMT, (int) track);
        }
        else if(i == 1) {
            printf(SECTOR_FMT, (int) sector);
        }
        else {
            printf("       ");
        }

        for(size_t k = 0; k < 16; k++) {
            if(k == 8) {
                putchar(' ');
            }

            printf(" %02X", data[i * 16 + k]);
        }

        if(!opt_hide_petscii) {
            printf("    ");

            for(size_t k = 0; k < 16; k++) {
                putchar(PETSCII[data[i * 16 + k]]);
            }
        }

        putchar('\n');
    }
}


/*
Set options based on command line arguments.
*/
void setopts(const int argc,
             char ** const argv)
{
    for(size_t i = 0; i < argc; i++) {
        if(strcmp(argv[i], "-h") == 0) {
            opt_show_help = TRUE;
        }
        else if(strcmp(argv[i], "-p") == 0) {
            opt_hide_petscii = TRUE;
        }
        else if(strcmp(argv[i], "-x") == 0) {
            opt_hex_ts = TRUE;
        }
    }
}


/*
Main function.
*/
int main(int argc,
         char ** argv,
         char ** envp)
{
    FILE * file = NULL;
    int first = TRUE;

    BYTE data[256];
    size_t file_size;
    size_t sector;
    size_t sector_count;
    size_t track;

    setopts(argc, argv);

    if(opt_show_help) {
        puts(HELP);
        exit(EXIT_SUCCESS);
    }

    // Open D64 file
    if(argc == 1) {
        die("no input file");
        exit(EXIT_FAILURE);
    }

    file = fopen(argv[argc - 1], "r");

    if(file == NULL) {
        die("file unreadable or not found");
    }

    // Get D64 file size
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Get sector count
    if(file_size == 174848 || file_size == 175531) {
        sector_count = 683;
    }
    else if(file_size == 196608 || file_size == 197376) {
        sector_count = 768;
    }
    else {
        fclose(file);
        die("invalid file size");
    }

    // Print sector data
    for(size_t index = 0; index < sector_count; index++) {
        tsp(index, & track, & sector);
        read(data, file);
        dump(data, track, sector, & first);
    }

    fclose(file);
    return EXIT_SUCCESS;
}
