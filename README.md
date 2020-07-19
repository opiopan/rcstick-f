rcstick-f
====
<img alt="board image" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/board01.jpg" width=450 align="right">

rcstick-f is a USB HID device which has 8 analog axis.
And this device is also a RF receiver which conplient with Futaba S-FHSS protocol.<br>
This device is designed to be used to controll RC flight simulator by Futaba RC transmitter.

[This product](https://www.google.com/search?q=WS1000+Spektrum&rlz=1C5CHFA_enJP742JP742&oq=WS1000+Spektrum&aqs=chrome..69i57j0l5.14185j0j8&sourceid=chrome&ie=UTF-8) is almost same with rcstick-f except supported protocol. <br>
The reason why I determined to start this project is that I could'nt find a such kind of device supported Futaba RC protocol.<br>


## Futaba S-FHSS protocol
S-FHSS is a 2.4 GHz band based RC transceiver protocol designed by Futaba.
It is not brand new protocol, However many Futaba transmitter suport this protolol. And this protocol is well analyzed. That's why I choose S-FHSS for my own receiver imprementation.

[This commentary article](https://rfengfpv.wordpress.com/2017/01/10/futaba-s-fhss-protocol-overview/) and
[this C source code](https://github.com/DeviationTX/deviation/blob/2ce0f46fe94d80198ae94fd5a6f6a008863ec420/src/protocol/sfhss_cc2500.c)
which impremens S-FHSS transmitter are very helpful to understand the protocol.
I sincerely appreciate there effort.

## PCB Design
I designed two types of PCB artworks for rcstick-f.<br>
One is sufficiently small that QFP32 package of STM32 MCU is installed, the other one is very small that QFN32 package of MCU is installed.<br>

<p align="center">
<img alt="description" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/formfactors.jpg" width=750>
</p>

### Design FIles
Board Type        | Eagle design files | Gerber files 
------------------|--------------------|--------------
QFP32 MCU version | [pcb/small](pcb/small)| [rcstick-f-gerber.zip](https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/data/rcstick-f-gerber.zip)
QFN32 MCU version | [pcb/tiny](pcb/tiny)| [rcstick-f-tiny-gerber.zip](https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/data/rcstick-f-tiny-gerber.zip)

### Schematic Diagram
<p align="center">
<img alt="PCB schema" src="images/schema.svg" width=800>
</p>

### BOM List

Components | Value / Product Number | Remarks
-----------|------------------------|------------------------------
U1         | STM32F042K6T6 or STM32F042K6U6 |
U2         | TI CC2500              | 
U3         | MCP1703AT-3302E/CB     | SOT-23 LDO
J1         | MOLEX 48037-0001       | USB type A male connector
J2         | MOLEX 53047-0410       | **OPTIONAL for debugging**<br>Firmware downloading can be proceeded by using DFU mode
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
C18, C19   | 12pF (1005)            | need to change value if X2's load capacitance is not 9pF
C20, C21   | 100pF (1005)           |
C22, C23   | 1pF (1005)             |
C24        | 1.8pF (1005)           |
C25        | 1.5pF (1005)           |
R1         | 10k ohm (1005)         | 
R2         | 56k ohm (1005)         |
R3         | 100 ohm (1005)         |
L1,L2,L3   | 1.2nH (1005)           |

## Building Firmware
1. **Requirements**<br>
[GNU Arm Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)
must be installed in a directory indecateed by `PATH` environment variable.

2. **Downloading Source Codes**<br>

    ```shell
    $ git clone https://github.com/opiopan/rcstick-f.git
    ```

3. **Compiling**
    ```shell
    $ cd rcstick-f/firmware
    $ make all
    ```

## Downloading Firmware
You can choose two way to download firmware to rcstick-f board.<br>
One is downloading firmware via SWD by using debugger. rcstick-f exports a SWD I/F at J2 connector.<br>
The other one is downloading it via USB port in DFU mode. You can switch rcstick-f in DFU mode by inserting that to USB port with the button pressed.

If openocd is running to controll debugger, you can download by making ```flash``` target with ```DEBUGSERVER``` parameter.

```shell
$ make DEBUGSERVER=localhost flash
```

If rcstick-f is connected to USB port and that is in DFU mode, you can download by making ```dfuflash``` target. In this case, [dfu-util](http://dfu-util.sourceforge.net) has to be installed in advance.

```shell
$ make dfuflash
```
## How to use rcstick-f
1. Insert rcstick-f to USB port of your PC, and make sure that 
    LED on rcstick-f flashs twice every two seconds.

2. Turn on the power of RC transmitter.<br>
    When rcstick-f detect that radio waves, LED flashing pattern is changed that flash once every one second.

3. Wait until LED will light continuously that means rcstick-f is binded with RC transmitter.

4. Now you can use your RC transmitter as a joystick on your PC.<br>
    First time to use rcstick-f on a PC, you may need to calibrate a joystick as next section.

## Calibration
Since a range of values for each channel of a transmitter is biased in a range that packet structure can be expressed, rcstick-f must be calibrated by OS function or application fucntion at first.<br>
On Windows, you can [carribrate a controller and save that result as system wide configuration](https://www.tenforums.com/tutorials/103910-calibrate-game-controller-windows-10-a.html). 
On Mac, calibration should be done for each application.

This inconvenience results from inefficiency of S-FHSS protocol.<br>
S-FHSS data packet provides 12 bit payload for each channel. That means channel that the range of values which each channel can take is 0 to 4095.<br>
However, actual transmitter uses range of approximately only 10 bit values. In my transmitter [Futaba 10J](https://www.rc.futaba.co.jp/propo/air/10j.html) case, minimum value is 946 and maximum value is 2094. 1149 level that 2049 - 946 + 1 is lazar-thin larger than the range of 10 bit values. Moreever, maximum value 2049 is only 2 larger than the range of 11 bits value.<br>
I don't know why Futaba design the payload in pakcet as 10 bits value. As mentioned in [this article](https://rfengfpv.wordpress.com/2017/01/10/futaba-s-fhss-protocol-overview/),
There are many inefficiency other than channel value payload case.
