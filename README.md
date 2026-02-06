# PicoNandwich
PicoNandwich is a dual nand solution for the Xbox 360 that uses a Raspberry Pi Pico for nand switching (and other features).

### Pinout/Wiring
|Pico|Console|
|--|--|
|GPIO 0|BUZZER +|
|GPIO 1|ROL LED POINT|
|GND|BUZZER -|
|GPIO 2|SYNC POINT|
|GPIO 3|NAND 1 CE|
|GPIO 4|NAND 2 CE|
|VSYS|5V or 3.3V Standby|
|GND|Console GND|

### How To Use

 - Hold the sync button briefly, LED/Buzzer will indicate which NAND is selected by lighting up or beeping one, two, three or four times depending on the setup. 
 - Hold the sync button for around 8 seconds to put the Pico into BOOTSEL/bootloader mode. Pico will flash 3 times with a long beep. (If you don't have access to the Pico's physical button, do NOT turn off the console or unplug the cable since this will wipe the firmware and you will have no way to write the firmware back to the Pico without opening your console.)
 


### Quality of Life Features
 - NAND Save States - Remembers which nand was selected after power loss.
 - Serial Console - For those technical nerds.
 - LED - While standard on dual nand consoles, ill mention it anyways. Wire an ROL LED to the pico.
 - Buzzer/Speaker - Wire up a buzzer/speaker for audio feedback. (cause why not?)
 - BOOTSEL Mode - Reflash the Pico's firmware while installed in the console (USB must be plugged in)


### Serial Features
This firmware contains some Serial features as well. You can check the status of the pico/console and send commands back. It uses a 115200 Baud Rate.
![Console](https://github.com/solinym/PicoNandwich/blob/main/Arduino_IDE_ldOfcprlcx.png?raw=true)

### ***Some Disclaimers***
Power sensing is NOT yet implemented, so do this at your own risk. I am not responsible for corrupted NANDs OR banned consoles. **DO NOT HOTSWAP YOUR NAND UNLESS YOU KNOW WHAT YOU'RE DOING.**

I **DO NOT** recommend powering the console while connected to a PC under normal circumstances. This risks backfeeding your console from the PC's USB port. Typically the Pico has protection against this, but not when voltages over USB and the VSYS pin are drastically different. 
If you do this, make sure to have the proper protections (like a diode) in place. (Do this if you're using serial features too.)
