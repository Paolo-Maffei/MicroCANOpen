
ORIGINAL MICROCANOPEN FROM 2003 AS DESCRIBED IN THE BOOK
"EMBEDDED NETWORKING WITH CAN AND CANOPEN."

NOTE: CANOPEN AND MICROCANOPEN HAVE GREATLY EVOLVED OVER THE LAST
YEARS. THIS IMPLEMENTATION IS FOR REFERENCE, EDUCATIONAL AND STRICTLY
NON-COMMERCIAL PURPOSES ONLY.

SEE HTTP://WWW.CANOPENSTORE.COM/PIP/MICROCANOPEN-PLUS.HTML FOR THE
LATEST, RECOMMENDED VERSION.


MicroCANopen Example Implementation
===================================

CONTAINS:  Example application using MicroCANopen
           Tested with Phytec's phyCORE P87C591 development board
           Using the Keil Compiler - www.keil.com or
           Using the Raisonance Compiler - www.amrai.com
COPYRIGHT: Embedded Systems Academy, Inc. 2002 - 2003
           All rights reserved. www.microcanopen.com
           This software was written in accordance to the guidelines at
           www.esacademy.com/software/softwarestyleguide.pdf
DISCLAIM:  Read and understand our disclaimer before using this code!
           www.esacademy.com/disclaim.htm
LICENSE:   Users that have purchased a license for PCANopenMagic
           (www.esacademy.com/software/pcanopenmagic)
           may use this code in commercial projects.
           Otherwise ONLY EDUCATIONAL USE is acceptable.
VERSION:   2.00, Pf/Aa/Ck 21-OCT-03


To compile, create a working directory with all the files from the 
directories MCO_Example591 and MCO. From within the compiler's
user interface create a new project and add all .c source files to
the project.


Implementation Description
==========================

Node-ID: 7
Baudrate: 125kbit

Messages generated:
0x707 - Heartbeat
  Send every 2 seconds
0x187 - TPDO1 - Four bytes of digital inputs. First byte is the dip
                switches. Second byte is button S1. Third byte is button S2.
                The remaining byte is copied from a digital output value
                received in RPDO1.
0x287 - TPDO2 - Two 16-bit analog inputs. The first value is the potentiometer.
                The second value is copied from the analog output value
                received in RPDO2.

Messages received:
0x000 - NMT Master command message
0x207 - RPDO1 - Four bytes of digital outputs. First byte is the LEDs.
                The second and third bytes are ignored. The remaining byte
                is copied to TPDO1.
0x307 - RPDO2 - Two 16-bit analog outputs. The first value is ignored. The
                second value is copied to TPDO2.


Note on CANopen Conformance
The provided example program passes the CANopen conformance test with 
some limitations:

a) The available version of the conformance test (2.0.02) can not 
correctly deal with nodes that only support heartbeat and have no 
node guarding.

b) The conformance test offers limited flexibility in regards to
pre-configured nodes. In order for an EDS to pass the check, many
defaults are expected to be zero. However, pre-configured nodes
typically do not use zeros for event and other timers.


