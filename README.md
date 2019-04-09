rcstick-f
====
<img alt="board image" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/board01.jpg" width=450 align="right">

rcstick-f is a USB HID device which has 8 analog axis.
And this device is also a RF receiver which conplient with Futaba S-FHSS protocol.<br>
This device is designed to be used to controll RC flight simulator by Futaba RC transmitter.

[This product](https://www.google.com/search?q=WS1000+Spektrum&rlz=1C5CHFA_enJP742JP742&oq=WS1000+Spektrum&aqs=chrome..69i57j0l5.14185j0j8&sourceid=chrome&ie=UTF-8) is almost same with rcstick-f except supported protocol. I could'nt find a such kind of device supported Futaba RC protocol.<br>
That's why I determined to start this project.

## Project Status
The PoC has been done in [this project](https://github.com/opiopan/sfhss-study).
And PCB production finished.<br>
Now I'm procuring components.

## PCB design
Eagle dsign files are [here](pcb/small/). You can also use [this gerber data](https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/data/rcstic-f-gerber.zip) for production.

<p align="center">
<img alt="PCB schema" src="images/schema.svg" width=800>
</p>

<p align="center">
<img alt="description" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/front.png" width=750>
</p>

<p align="center">
<img alt="description" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/back.png" width=750>
</p>

Components | Value / Product Number | Remarks
-----------|------------------------|------------------------------
U1         | STM32F042K6T6          |
U2         | TI CC2500              | 
U3         | MCP1703AT-3302E/CB     | SOT-23 LDO
J1         | MOLEX 48037-0001       | USB type A male connector
J2         | MOLEX 53047-0410       | **OPTIONAL for debugging**<br>firmware downloading can be proceeded by using DFU mode
X1         | 16MHz Crystal (3225)   | load capacitance: 10pF
X2         | 26MHz Crystal (3225)   | load capacitance: 9pF
LED1       | OSBL1608C1A            |
SW1        | ALPS SKRPACE010        |
C1, C2, C9 | 1uF (1005)             |
C3, C4, C10, C11, C12, C15, C16| 0.1uF (1005)           |
C5         | 4.7uF (1005)           |
C6, C7     | 15pF (1005)            | need to change value if X1's load capacitance is not 10pF
C8         | 10nF (1005)
C13, C14, C17| 220pF (1005)
C18, C19   | 12pF (1005)            | need to change value if X2's load capacitance is not 10pF
C20, C21   | 100pF (1005)           |
C22, C23   | 1pF (1005)             |
C24        | 1.8pF (1005)           |
C25        | 1.5pF (1005)           |
R1         | 10k ohm (1005)         | 
R2         | 56k ohm (1005)         |
R3         | 100 ohm (1005)         |
L1,L2,L3   | 1.2nH (1005)           |

