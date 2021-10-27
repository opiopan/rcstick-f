rcstick-f
====
<img alt="board image" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/board02.jpg" width=400 align="right">

RCSTICK-f is a USB HID device which has a RF reciever complient with the Futaba S-FHSS protocol.
RCSTICK-f is a device designed to control RC Flight Simulators and other programs using your Futaba S-FHSS transmitter to act as  HID device. 
<br>

One of the most simple and common ways to use a RC transmitter as a controller of a RC simulator is to handle the PPM/PWM signal from the trainer port on a transmitter.<br>
Some simulator programs can recognise this PPM/PWM signal through audio input or require an external usb dongle to connect to the simulator program.
<br>
There are some USB dongles that convert a general PPM/PWM signal to a USB HID device like [these products](https://www.ebay.com/sch/i.html?_nkw=RC+simulator+USB+dongle+cable&_sacat=0&LH_TitleDesc=0&_osacat=0&_odkw=rc+simulator+usb+dongle).
<br>
The advantages of these systems are that they are inexpensive and compatible with many RC transmitters. The downside lies in the fact that the cable and the reliability of such products differ from time to time. 

In recent years, RC transmitter manufacturers and brands have taken a mix of both concepts and taken the RF signal and turned it into a USB HID device. For example, [Spektrum WS1000](https://www.spektrumrc.com/Products/Default.aspx?ProdID=SPMWS1000) for the DSM2/DSMX protocols or [FrSky XSR-SIM](https://www.frsky-rc.com/product/xsr-sim/) for the FrSky protocol
<br>
These products are designed for their specific protocols. Thus, the compatibility among brands is not maintained. However, users with compatible RC transmitters can enjoy convience due to the wireless and effective connection with support from the original manufacturer. 
<br>
In addition, the dongle is more response and has higher precision than the PPM signal based solution with not unnecessary data conversion using an ADC.

The reason I decided to embark on this project was becuase I couldn't find such a device that supported any of the Futaba RC protocols.
<br>

<p align="center">
<img alt="demo" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/demo.gif">
</p>

## Futaba S-FHSS protocol
S-FHSS is a 2.4 GHz band RC transceiver protocol designed and used by Futaba. It is a older protocol but is still widely used among Futaba products. This rotocol is widely used ana analysed. This is the reason wy I chose S-FHSS for my reciever implementation.


[This commentary article](https://rfengfpv.wordpress.com/2017/01/10/futaba-s-fhss-protocol-overview/) and
[this C source code](https://github.com/DeviationTX/deviation/blob/2ce0f46fe94d80198ae94fd5a6f6a008863ec420/src/protocol/sfhss_cc2500.c)
which implements and uses the S-FHSS transmitter are very helpful to understand and analyse the protocol. I sincerely appreiate their effort. 
which impremens S-FHSS transmitter are very helpful to understand the protocol.


## PCB Design
I designed two types of PCB's for rcstick-f.<br>
One is small for the QFP32 package of the STM32 MCU while the other one is for the QFN32 package for the STM32. 
Both designs are optimised to enable and allow for hand-soldering with a soldering iron. You don't require a reflow oven ora hot air gun.
<br>
All pads, spaces and parts are designed to allow for contact soldering irons. The large ground pad in the center of the QFN package is solderable from a via on the back side of the pcb.

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
J2         | MOLEX 53047-0410       | **OPTIONAL for debugging**<br> Firmware uploading and downloading can be done using DFU mode
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

### Case Design
You can download the [STL file](https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/data/case.zip) for the case for the tiny version of PCB. THis is designed assumeing that is is printed using a resin printer.<br>
*Note that PCB thickness should be 1.2mm if you apply this case design.*

<p align="center">
<img alt="case structure" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/case-structure.jpg" width=700>
</p>

## Building Firmware
1. **Requirements**<br>
[GNU Arm Embedded Toolchain](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)
must be installed in a directory indicated by `PATH` environment variable.

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
There are two ways to upload firmware to rcstick-f.
<br>
One is downloading firmware via SWD by using debugger. rcstick-f exports a SWD I/F at the J2 connector.<br>
The other one is downloading it via USB port in DFU mode. You can switch rcstick-f in DFU mode by inserting it the to USB port with the button pressed.

### Downloading via Debugger
1. **Preparing Debugger**<br>
    Make sure that a debugger is connected with rcstick-f J2 port and debugger controlling software which works as a GDB server with OpenOCD.

2. **Downloading**<br>
    You can download the firmware by making ```flash``` target with ```DEBUGSERVER``` parameter.
    If debugger controlling software is running on the same PC, specify ```localhost``` for ```DEBUGSERVER``` parameter.

    ```shell
    $ make DEBUGSERVER=localhost flash
    ```
### Downloading via USB
1. **Install required software**<br>
    Make sure that following software is installed.
    - [dfu-util](http://dfu-util.sourceforge.net)

2. **Switching rcstick-f in DFU mode**<br>
    Insert rcstick-f to USB port with the button (SW1) is pressed.

3. **Downloading**<br>
    You can download  by making ```dfuflash``` target.

    ```shell
    $ make dfuflash
    ```

4. **Exsiting from DFU mode**<br>
    Pull out rcstick-f from USB port, then re-insert it to the USB port.


## How to use rcstick-f
0. Make sure that the settings of transmitter are appropriate as [next section](#transmitter-settings).

1. Insert rcstick-f to USB port of your PC, and make sure that 
    LED on the rcstick-f flashes approximately twice every two seconds.

2. Turn on the power of the RC transmitter, and make sure that the transmitter protocol is S-FHSS.<br>
    When rcstick-f detect the RF signal, the LED flashing pattern changes to flash once every second.

3. Wait until the LED holds. This means that the rcstick-f is binded with the RC transmitter.


4. Now you can use your RC transmitter as a joystick on your PC.<br>
    Each channel of S-FHSS transmitter is mapped to each USB HID axis as below.

    Channel  | USB HID usage
    ---------|---------------
    CH 1     | X
    CH 2     | Y
    CH 3     | Z
    CH 4     | X rotation
    CH 5     | Y rotation
    CH 6     | Z rotation
    CH 7     | Slider
    CH 8     | Slider

## Transmitter Settings
I recommend you to register a dedicated model for rcstick-f in your transmitter. That model should be satisfy the following conditions.

- **Communication Protocol (REQUIRED)**<br>
    Choose ```S-FHSS``` for communication protocol since rcstick-f can recognize only the S-FHSS protocol.
    <p align="center">
    <img alt="protocol setting screen" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/tx_model.jpg" width=300>
    </p>

- **End Point (RECOMMENDED)**<br>
    Set end point for each channel as that RX PWM output pulse width will be in range between 1009us and 2031us.<br> 
    In S-FHSS packet, each channel position is expressed as a 12 bit number. Those values are equivalent of PWM pulse width output from the Reciever in microseconds. So value of a channel which is on center position should be approximately 1520.<br>
    rcstick-f generates values for each axis of USB HID by subtracting 1520 from value in S-FHSS packet, and clips values in range of 10 bit 2's complement number, that is  between -511 to 511.<br>
    Therefore, the most efficient setting of end point is that each channel value will be in range between 1520 - 511 and 1520 + 511.<br>

    In case of the [Futaba 10J](https://futabausa.com/product/10j/), the end point settings satisfied above condition are 124% or 125% for channels 1 to 4 and 92% or 93% for channels 5 to 8.

    rcstick-f provides a helper function to determine the proper endpoint ratio. Refer to the [next section](#function-to-detect-value-clipping) for the details.

    <p align="center">
    <img alt="end point setting screen" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/tx_endpoint.jpg" width=300>
    </p>

- **Servo Reverse (RECOMMENDED)**<br>
    In order to prevent runaway when the simulator is started without a connection between rcstick-f and transmitter, rcstick-f automatically sets the throttle value to -511 on chanel 3.<br>
    For Futaba transmitters, the more throttle, the PWM duty cycle reduces. To allow for correct throttle and stick position relationship, the reversal of channel 3 may e required. 
    <p align="center">
    <img alt="reverse setting screen" src="https://raw.githubusercontent.com/wiki/opiopan/rcstick-f/images/tx_reverse.jpg" width=300>
    </p>

## Function to detect value clipping
As mentioned in the previous section, setting the end point ratio as largest value within the range possible is important. To determine that appropriate end point ratio, rcstick-f has a mode to detect clipping value for channel. <br>
Find the appropriate end point ratio as following steps.

1. **Preparing**<br>
    To ensure that the movable area of servo exactly maches end point setting, make sure following settings for each channel are followed.

    - Trim and Sub trim: should be reset
    - Dual Rate: should be 100%
    - No mixing with other channels
    - Throttle curve and Pitch curve: should start from 0% and  end to 100%

    Then, make position of all sticks, slide levers, and switches associated with channels 1 to 8 centered.<br>
    If that is impossible/impractical due to associations with 2 position switchs, reduce the end point ratio significantly for that channel.

2. **Binding rcstick-f and transmitter**<br>
    Bind transmitter with rcstick-f according to steps described in the section ["How to use rcstick-f"](#how-to-use-rcstick-f).<br>

3. **Changing mode of rcstick-f**<br>
    Press a button on the rcstick-f in order to enable a function to detect value clipping. The LED on the rcstick-f will flash three times in one second.<br>
    If LED still holds continuously, a channel value is larger than 1520 + 511 or less than 1520 - 511 at least. Make sure that all channel settings are satisfy conditions mentioned in step 1.

4. **Determining enb point ratio**<br>
    Choose any one channel, then move the stick, the slide bar, or the switch assosiated with that channel to the maximum or minimum position.<br>
    If the LED is still flashing, the value of the channel is in range and is not clipped. In this case, increase end point ratio by 1% until LED is flashing.<br>
    If LED flashing pattern changes, the value of channel is out of range and is clipped. In this case, decrease end point ratio by 1% until the LED starts to flash.<br>

    Usually, you can apply same ratio determined above for all other channels. However you should be careful if your transmitter is an entry or intermediate model such as [the 10J](https://www.rc.futaba.co.jp/english/propo/air/10j.html).<br>
    It is well established that the servo angle at 100% for end points in channels 5 and above is wider than channels 1 to 4 in some Futaba Transmitters. If you notive such effect on your transmitters, you need to determine at least two channels.
