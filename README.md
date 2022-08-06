# TinyNightLight - Night Light with NeoPixels
TinyNightLight is a rotary encoder controlled and battery powered nightlight with NeoPixels based on the ATtiny13A.

- Design Files (EasyEDA): https://easyeda.com/wagiminator/attiny13-tinynightlights

![TinyNightLight_pic3.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-TinyNightLight/main/documentation/TinyNightLight_pic3.jpg)

# Hardware
## Schematic
![TinyNightLight_wiring.png](https://raw.githubusercontent.com/wagiminator/ATtiny13-TinyNightLight/main/documentation/TinyNightLight_wiring.png)

## WS2812 Addressable LEDs
WS2812-2020 is an intelligent control LED light source, which integrates the control circuit and RGB chip in a small 2020 package. It internally contains an intelligent digital port data latch, a signal conditioning gain driver circuit, a precision oscillator and a voltage-programmable constant current control part, which effectively ensures that the pixel point light color level is consistent. Make sure you use the 5mA variant.

## MCP73831 Li-Ion Battery Charger IC
For battery charging the MCP73831 is used. The MCP73831 is a highly advanced linear charge management controller for use in space-limited, cost-sensitive applications. It employs a constant-current/constant-voltage charge algorithm with selectable preconditioning and charge termination. The constant current value is set with one external resistor (I = 1000V / R2). Charging is done via the built-in USB-C connector. The MCP73831 can be replaced by the much cheaper TP4054 without further modifications.

## Power Path Control
The device can be operated both by battery and external power. The load sharing power path control system interrupts the connection between the battery and the load when an external power supply is connected. In this case, the device is powered by the external power supply while the battery is being charged. This protects the battery and increases its lifespan. Without an external power supply, the device is powered directly by the battery.

# Software
## NeoPixel Implementation
The NeoPixel implementation is based on [NeoController](https://github.com/wagiminator/ATtiny13-NeoController).

## Compiling and Uploading the Firmware
Since there is no ICSP header on the board, you have to program the ATtiny either before soldering using an [SOP adapter](https://aliexpress.com/wholesale?SearchText=sop-8+150mil+adapter), or after soldering using an [EEPROM clip](https://aliexpress.com/wholesale?SearchText=sop8+eeprom+programming+clip). The [AVR Programmer Adapter](https://github.com/wagiminator/AVR-Programmer/tree/master/AVR_Programmer_Adapter) can help with this. Remove the battery before connecting the device to the programmer.

### If using the Arduino IDE
- Make sure you have installed [MicroCore](https://github.com/MCUdude/MicroCore).
- Go to **Tools -> Board -> MicroCore** and select **ATtiny13**.
- Go to **Tools** and choose the following board options:
  - **Clock:**  9.6 MHz internal osc.
  - **BOD:**    BOD 2.7V
  - **Timing:** Micros disabled
- Connect your programmer to your PC and to the ATtiny.
- Go to **Tools -> Programmer** and select your ISP programmer (e.g. [USBasp](https://aliexpress.com/wholesale?SearchText=usbasp)).
- Go to **Tools -> Burn Bootloader** to burn the fuses.
- Open the TinyNightLight sketch and click **Upload**.

### If using the precompiled hex-file
- Make sure you have installed [avrdude](https://learn.adafruit.com/usbtinyisp/avrdude).
- Connect your programmer to your PC and to the ATtiny.
- Open a terminal.
- Navigate to the folder with the hex-file.
- Execute the following command (if necessary replace "usbasp" with the programmer you use):
  ```
  avrdude -c usbasp -p t13 -U lfuse:w:0x3a:m -U hfuse:w:0xff:m -U flash:w:tinynightlight.hex
  ```

### If using the makefile (Linux/Mac)
- Make sure you have installed [avr-gcc toolchain and avrdude](http://maxembedded.com/2015/06/setting-up-avr-gcc-toolchain-on-linux-and-mac-os-x/).
- Connect your programmer to your PC and to the ATtiny.
- Open the makefile and change the programmer if you are not using usbasp.
- Open a terminal.
- Navigate to the folder with the makefile and the sketch.
- Run "make install" to compile, burn the fuses and upload the firmware.

# Operating Instructions
Place a 3.7V protected 16340 (LR123A) Li-Ion battery in the holder. The battery can be charged via the USB-C port. A red LED lights up during charging. It goes out when charging is complete.

The device has four different states, which can be switched between by pressing the rotary encoder button.

|State|Description|
|:-|:-|
|0|All LEDs are off and the ATtiny is in sleep mode.|
|1|All LEDs light up white. The brightness can be changed by turning the rotary encoder. The brightness set by this is also retained for the following states.|
|2|The LEDs show a color animation with the previously set brightness. The speed of the animation can be adjusted using the rotary encoder.|
|3|All LEDs show the same color with the previously set brightness. The color can be changed by turning the rotary encoder.|

![TinyNightLight_pic4.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-TinyNightLight/main/documentation/TinyNightLight_pic4.jpg)

# References, Links and Notes
1. [ATtiny13A Datasheet](http://ww1.microchip.com/downloads/en/DeviceDoc/doc8126.pdf)
2. [WS2812-2020 Datasheet](https://www.led-stuebchen.de/download/WS2812-2020_V1.1_EN.pdf)
3. [MCP73831 Datasheet](https://datasheet.lcsc.com/lcsc/1809191822_Microchip-Tech-MCP73831T-2ATI-OT_C14879.pdf)
4. [NeoPixel Implementation](https://github.com/wagiminator/ATtiny13-NeoController)

![TinyNightLight_pic1.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-TinyNightLight/main/documentation/TinyNightLight_pic1.jpg)
![TinyNightLight_pic2.jpg](https://raw.githubusercontent.com/wagiminator/ATtiny13-TinyNightLight/main/documentation/TinyNightLight_pic2.jpg)

# License
![license.png](https://i.creativecommons.org/l/by-sa/3.0/88x31.png)

This work is licensed under Creative Commons Attribution-ShareAlike 3.0 Unported License. 
(http://creativecommons.org/licenses/by-sa/3.0/)
