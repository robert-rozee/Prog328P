/*
 * Arduino/STK500 mini uploader for ATmega328P
 *
 * Copyright (C) 2015 Robert Rozee
 * Version 1.0 27-July-2015
 *
 * This file is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>                   // string handling functions
#include <ctype.h>                    // character handling functions
// include <unistd.h>                 // POSIX operating system API
// include <signal.h>                 // signal processing
// include <getopt.h>                 // parse command-line options
// include <sys/time.h>               // time and date functions
// include <sys/stat.h>               // file attribute constructs
// include <libgen.h>                 // defns for pattern matching functions

#include "serial.h"

#define NIBBLE(x)       (isdigit(x) ? (x)-'0' : tolower(x)+10-'a')

#define STK_GET_SYNC            0x30            // synchronize
#define STK_ENTER_PROGMODE      0x50            // enter program mode 
#define STK_READ_SIGN           0x75            // read signature
#define STK_LOAD_ADDRESS        0x55            // load address
#define STK_PROG_PAGE           0x64            // program page
#define STK_LEAVE_PROGMODE      0x51            // exit program mode

#define CRC_EOP                 0x20            // end of packet

#define STK_INSYNC              0x14            // response - insync
#define STK_OK                  0x10            // response - OK



int main (int argc, char **argv)
{
    printf("\n");

    if ((argc < 3) || ( argc > 4)) {            // less than 2 or more than 3
        printf ("*******************************************\n");
        printf ("Arduino/STK500 mini uploader for ATmega328P\n");
        printf ("Copyright (C) 2015 Robert Rozee\n");
        printf ("Version 1.0 27-July-2015\n");
        printf ("*******************************************\n\n");
        printf ("Usage:\n");
        printf ("       %s filename.hex port [baud]\n\n\n", argv[0]);
        printf ("The default baud rate is 57600. Baud rate\n");
        printf ("values of 1-4 are aliases for 9600, 19200,\n");
        printf ("57600, and 115200 respectively.\n");
        exit (0);
    }


//////////////////////////////////////////////////////////////////////////////
// this block of code reads in the .hex file content to a buffer (memory[]) //
//////////////////////////////////////////////////////////////////////////////

    FILE *fd;                                   // source (.HEX file)
    unsigned char memory [0x20000];             // memory image, 128k max
    unsigned char buffer [256];                 // input line buffer
    int count, start, i, n;
    int line = 0;
    int memtop = 0;

    memset(memory ,0xff, sizeof(memory));

    fd = fopen (argv[1], "r");
    if (! fd) {
        printf ("Unable to open file: %s\n", argv[1]);
        exit (-1);
    }

    while (fgets ((char*) buffer, sizeof(buffer), fd)) {    // while not EOF
        line++;
//      printf("%05i %s\n", line, buffer);

        if (buffer[0] == '\n')                              // skip empty lines
            continue;
        if (buffer[0] != ':') {                             // bail on an invalid record
            printf ("Invalid record in line %i\n", line);
            fclose (fd);
            exit (-1);
        }

        if ((buffer[7] == '0') && (buffer[8] == '0')) {     // type 0 is data

            count = (NIBBLE(buffer[1]) << 4) +
                     NIBBLE(buffer[2]);

            start = NIBBLE(buffer[3]);
            for (i = 4; i < 7; i++) start = (start << 4) + NIBBLE(buffer[i]);

            if ((start+count) > sizeof (memory)) {
                printf("Memory buffer overrun (beyond 128k)\n");
                fclose (fd);
                exit (-1);
            }

//          printf("address = %04x, count = %i(d)\n", start, count);

            for (i = 9; count > 0; count--) {
                memory[start++] = (NIBBLE(buffer[i + 0]) << 4) + 
                                   NIBBLE(buffer[i + 1]);
                i += 2;
            }
            if (start > memtop) memtop = start;
        }                                                   // end of if type 0 record
    }                                                       // end of while not EOF

    fclose (fd);
    memtop = (memtop + 0x7F) & ~0x7F;                       // round up part page to next page
//  printf("Memory size = %i\n", memtop);


//////////////////////////////////////////////////////////////////////////////
// this block uses the Arduino/STK500v1 protocol to send memory[] to target //
//////////////////////////////////////////////////////////////////////////////

    int bps[] = {0, 9600, 19200, 57600, 115200 };   // known arduino bootloader baud rates
 
    if (argc == 4) i = strtol (argv[3], NULL, 10);
        else       i = 57600;                       // default to 57600 baud if not specified
    if ((i > 0) && (i < 5)) i = bps[i];             // map 1-4 as aliases for bps[] values
    if (i == 0) i = 57600;                          // strtoi failed, fall back to 57600

    if (serial_open (argv[2], i, 100) < 0) {
        printf ("Unable to configure serial port %s\n", argv[2]);
        serial_close();
        exit (-1);
    }
    printf("%i baud ", i);

    for (i = 0; i < 40; i++) {

        buffer[0] = STK_GET_SYNC;                   // get synchronization
        buffer[1] = CRC_EOP;

        serial_write (buffer, 2);
        printf (".");
        n = serial_read (buffer, 2);
        if ((n == 2) && (buffer[0] == STK_INSYNC) && (buffer[1] == STK_OK)) i=100;
    }

    if (i < 100) { 
        printf ("\nFailed to find arduino/STK500 bootloader\n");
        serial_close();
        exit (-1);
    }
    printf (" synchronized\n"); 

    buffer[0] = STK_ENTER_PROGMODE;                 // enter program mode (not needed)
    buffer[1] = CRC_EOP;
    serial_write (buffer, 2);
    serial_read (buffer, 2);

    if ((n != 2) || (buffer[0] != STK_INSYNC) || (buffer[1] != STK_OK)) { 
        printf ("Failed to enter program mode\n");
        serial_close();
        exit (-1);
    }
        
    buffer[0] = STK_READ_SIGN;                      // read signature bytes (3)
    buffer[1] = CRC_EOP;
    serial_write (buffer, 2);
    n = serial_read (buffer, 5);

    if ((n != 5) || (buffer[0] != STK_INSYNC) || (buffer[4] != STK_OK)) { 
        printf ("Failed to get signature\n");
        serial_close();
        exit (-1);
    }

    unsigned ID = (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
    printf ("Signature = %06x   Device = %s\n", ID, ID == 0x1e950f ? "ATmega328P" : "(wrong uP)"); 

    count = memtop / (0x80 * 30);
    if (count == 0) count = 1;
    count *= 0x80;
    for (i = 0; i < memtop; i += 0x80) if ((i % count) == 0) printf (".");
    for (i = 0; i < memtop; i += 0x80) if ((i % count) == 0) printf ("\b");

    for (i = 0; i < memtop; i += 0x80) {
        if ((i % count) == 0) printf ("#");

        buffer[0] = STK_LOAD_ADDRESS;               // load address
        buffer[1] = (i >> 1) % 0x100;               // address low (word boundary)
        buffer[2] = (i >> 1) / 0x100;               // address high
        buffer[3] = CRC_EOP;
        serial_write (buffer, 4);
        n = serial_read (buffer, 2);

        if ((n != 2) || (buffer[0] != STK_INSYNC) || (buffer[1] != STK_OK)) { 
            printf ("\nFailed to load address %04x\n", i);
            serial_close();
            exit (-1);
        }

        buffer[0] = STK_PROG_PAGE;                  // program page
        buffer[1] = 0x00;                           // length high (in bytes, NOT words)
        buffer[2] = 0x80;                           // length low (order reverse to address)
        buffer[3] = 'F';                            // memory type: 'E' = eeprom, 'F' = flash
        memcpy (&buffer[4], &memory[i], 0x80);      // data (128 bytes)
        buffer[4 + 0x80] = CRC_EOP;
        serial_write (buffer, 4 + 0x80 + 1);
        n = serial_read (buffer, 2);

        if ((n != 2) || (buffer[0] != STK_INSYNC) || (buffer[1] != STK_OK)) { 
            printf ("\nFailed to program page at %04x\n", i);
            serial_close();
            exit (-1);
        }
    }
    printf ("\n");

    buffer[0] = STK_LEAVE_PROGMODE;                 // leave program mode
    buffer[1] = CRC_EOP;
    serial_write (buffer, 2);
    n = serial_read (buffer, 2);

    if ((n != 2) || (buffer[0] != STK_INSYNC) || (buffer[1] != STK_OK)) { 
        printf ("Failed to exit program mode\n");
        serial_close();
        exit (-1);
    }
    printf ("Firmware uploaded OK\n"); 
    serial_close();
    exit (0);
}                    // finished performing function, exit program
