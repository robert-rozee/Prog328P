*******************************************
Arduino/STK500 mini uploader for ATmega328P
Copyright (C) 2015 Robert Rozee
Version 1.0 30-July-2015
*******************************************

Prog328P is a small utility (7k when compressed) designed to upload .hex files created by the Arduino IDE to an Arduino board containing an ATmega328P or similar processor. In order to keep down the size and reduce complexity, Prog328P only supports a subset of the STK500 version 1 protocol, as used by OptiBoot and similar Arduino compatible bootloaders. This limitation precludes using the utility to upload .hex files to the Arduino Mega 2560 or any Arduino that has more than 128k of flash memory. Furthermore, Prog328P only processes type 0 records from the .hex file - in theory this should not be an issue with .hex files generated for the small-memory processors (<128k flash) used on Arduino boards.

For incompatible Arduino boards, use AVRDUDE shipped with the Arduino IDE.

The intention in writing Prog328P was to provide a utility small enough that it can be emailed along with compiled code (.hex file) to an inexperienced user who wishes to upload or update code on an Arduino board without needing to have AVRDUDE or the Arduino IDE installed. AVRDUDE, the required configuration file, and libusb0.dll can be as large as 2mb in total - this is pushing the limits of what can sensibly (and politely) be transferred through email.


=== Usage ====

    Prog328P filename.hex port [baud]


The default baud rate is 57600. Baud rate values of 1-4 are aliases for 9600, 19200, 57600, and 115200 respectively. Higher numbers are taken as actual baud rates.


=== Sources ===

Sources are distributed under the terms of GPL. You can download sources from GitHub. Use is made of Serge Vakulenko's serial.c unit to provide cross-platform serial port support. The serial.c unit is part of the Pic32prog package, also available from GitHub under the same GPL. For convenience, serial.c is included with Prog328P.

While written and compiled on a Win32 computer using MinGW, there is no reason the sources of Prog328P should not be compiled for Linux or Mac OS X.

___
Regards,
Robert Rozee
