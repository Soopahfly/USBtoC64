# USB Adapter for Commodore 64, AMIGA and ATARI Joystick and Mouse Port

## This Fork

This fork is based on Emanuele Laface's original [USBtoC64](https://github.com/emanuelelaface/USBtoC64) project.

The goal of this version is to keep the original hardware and simple firmware model, while improving day-to-day compatibility with USB mice and game controllers on the Commodore 64, especially for GEOS / 1351 mouse use and mouse-as-joystick games.

### Changes From The Original Project

- Added onboard mouse/button configuration for C64 mouse speed, Amiga/Atari mouse speed, and PAL/NTSC timing. These settings are saved in EEPROM and do not require reflashing.
- Added runtime PAL/NTSC timing selection. The compile-time `PAL` define is now only the default used when EEPROM settings are first initialized.
- Added C64 and Amiga/Atari mouse speed presets from 1 to 5.
- Fixed mouse-as-joystick mode so zero mouse movement no longer asserts LEFT/UP.
- Fixed Atari mouse mode divide-by-zero on reports with no movement on one axis.
- Fixed joystick-as-C64-mouse RIGHT movement to use the X timing step instead of the Y timing step.
- Improved C64 mouse timing wrapping so larger movement reports are handled more predictably.
- Changed shared mouse timing values from 64-bit to 32-bit values, which is enough for these timer counts and avoids unnecessary ISR/task sharing risk.
- Added safer output release on USB disconnect/setup to reduce stuck direction/fire states.
- Improved custom mouse report decoding by clamping 16-bit HID deltas instead of allowing wraparound.
- Added `JM_HAT` support in custom joystick mappings for HID hat-switch D-pads and diagonals.

### Onboard Configuration

Connect a USB mouse to the adapter. Hold the adapter's **BOOT** button, then click one mouse button:

- **BOOT + left mouse button**: cycle C64 mouse speed from 1 to 5. The LED blinks blue to show the selected value.
- **BOOT + right mouse button**: cycle Amiga/Atari mouse speed from 1 to 5. The LED blinks green to show the selected value.
- **BOOT + middle mouse button**: toggle C64 PAL/NTSC timing. One red blink means NTSC; two red blinks means PAL.

Release the mouse button and BOOT button after the LED feedback. Settings are saved to EEPROM and survive power cycles.

The original target-machine selector still works: within the first 30 seconds after boot, hold a single mouse button for about 5 seconds:

- **Left button**: Commodore 64
- **Right button**: Amiga
- **Middle button**: Atari

### Compatibility Notes

For ordinary USB mice, leave `MOUSE_MAP_CUSTOM` set to `0`. This uses USB HID boot mouse protocol and should be the most compatible mode for basic wired mice such as the Raspberry Pi USB Mouse or Retro Games Ltd Tank Mouse.

Wheel support still requires `MOUSE_MAP_CUSTOM` and a report mapping that matches your mouse. That path is more device-specific.

For gamepads and arcade sticks, simple USB HID controllers should work via the learning procedure. For custom firmware mappings, `JM_HAT` can now be used for HID hat-switch D-pads so diagonals work as combined directions.

### C64 Mouse And Joystick Diagnostic PRG

The `diagnostics` folder contains `U64TEST.prg`, a small C64 BASIC diagnostic for checking the adapter from the Commodore 64 side. It is intended for quick bench testing after flashing, changing the mouse/joystick switch, trying a new USB mouse, or checking a newly learned gamepad mapping.

The C64 cannot directly read the adapter's physical mouse/joystick switch. Instead, the diagnostic watches the signals that software on the C64 can actually see:

- SID POT X/Y readings, used by C64 1351-style mouse mode.
- CIA joystick direction/fire lines, used by joystick mode and by the mouse button mappings.

Load it on the C64 with:

```text
LOAD"U64TEST.PRG",8,1
RUN
```

The program defaults to control port 2, which is what most C64 games use. Press **SPACE** to toggle between port 1 and port 2, **R** to reset the mouse/joystick/warning scores, or **Q** to quit.

Screen guide:

- **PORT / MODE**: shows the selected C64 control port and the current best guess: `MOUSE MODE`, `JOYSTICK MODE`, or `UNSURE`.
- **POT X/Y and DX/DY**: shows raw SID POT readings and the latest signed movement delta.
- **MOVE / JIT / SPK**: shows the dominant movement direction from the filtered POT delta, counts small idle POT jitter, and counts rejected raw POT spikes.
- **JOY**: shows the live digital joystick line state as `UP`, `DN`, `LT`, `RT`, and `FR`.
- **MOUSE / JOY / WARN**: running scores used to classify the adapter behavior and count suspicious input frames.
- **STATUS**: shows `OK` or the most recent plausibility warning.
- **Pointer box**: the on-screen `X` moves when POT X/Y movement is detected.
- **Joystick panel**: `UP`, `DOWN`, `LEFT`, `RIGHT`, and `FIRE` brighten while that input is pressed.
- **SEEN checklist**: `M U D L R F` brightens once mouse movement, up, down, left, right, or fire has been observed since the last reset.
- **BTN line**: highlights the C64 mouse-button mapping: left button as FIRE, right button as UP, and middle button as DOWN.

Expected results:

- In C64 mouse mode, moving a USB mouse should change POT X/Y and move the `X` in the pointer box. The mouse score should rise.
- In joystick mode, moving a joystick/gamepad or mouse-as-joystick should light the matching joystick words and raise the joystick score.
- In C64 mouse mode, left mouse button appears as FIRE, right button appears as UP, and middle button appears as DOWN. This matches the adapter's C64 mouse button mapping.

Plausibility warnings:

- `BAD: UP+DOWN`: both vertical directions are active at the same time.
- `BAD: LEFT+RIGHT`: both horizontal directions are active at the same time.
- `CHECK: POT NOISE`: the raw POT reading jumped by an unusually large amount in one frame. The tester still displays the raw POT value, but ignores that spike for cursor movement, mouse scoring, and mode guessing.
- `CHECK: POT+DIR`: POT movement and direction lines were seen together. This can be legitimate when pressing mouse buttons in C64 mouse mode, but it is useful for spotting noisy wiring, stuck lines, or a device behaving unexpectedly.
- `STUCK: UP/DOWN/LEFT/RIGHT/FIRE`: one input has stayed active for a long time and may be stuck, held, mis-mapped, or shorted.

The readable BASIC source is `diagnostics/USBtoC64ModeTest.bas`. The checked-in `U64TEST.prg` is generated from that source. Rebuild the PRG on Windows with:

```powershell
powershell.exe -ExecutionPolicy Bypass -File diagnostics\build-prg.ps1
```

This adapter interfaces a USB device with the CONTROL Port of the C64, AMIGA and ATARI, allowing it to be used as a mouse or joystick.

The joystick connects via pins 1, 2, 3, 4, and 6 of the CONTROL port, with the GPIOs simply set as open circuits or shorted to ground when a joystick direction is pressed.

On the C64, the mouse uses the analog part of the port (Pot X and Pot Y). The Commodore 64 has these two pins for evaluating an analog resistor that charges an internal capacitor. The charging time is decoded as a resistor value with a digital value from 0 to 255. The trick used by Commodore engineers to use it as a mouse was to send a pulse at the right moment, making the C64 believe that the capacitor is fully charged. To establish the right moment, the C64 goes through 512 cycles (almost 512 microseconds, since the clock frequencies of PAL and NTSC are not exactly 1 MHz: PAL is 0.985248 MHz, NTSC is 1.022727 MHz). During the first 256 cycles, the potentiometer is set to ground, then it charges the capacitor.

The idea is to use an ESP32 board to wait for the discharge drop and then an additional 256 cycles, finally sending the proper value to the C64 at the right moment. The ESP32 allows interrupts when an input signal is falling; however, the voltage from the C64 is a bit too low for the ESP32 interrupt because the board requires at least 3 volts, while the C64 provides around 1.2 volts. Therefore, a BJT is used to amplify the signal (and invert it, so it is HIGH when the C64 is LOW, which reduces noise in detecting the status).

The initial variable values are the timing for a PAL C64 and are obtained empirically. The NTSC version of the timings are calculated scaling for the ratio of the frequencies NTSC/PAL. It is possible that another C64 may use slightly different timing, though it should be quite stable since Commodore sold the same mouse to everyone. When I will have one NTSC Commodore, I will test it and adjust the timings if needed.

On AMIGA and ATARI the mouse is encoded in the quadrature signal using two trains of pulses with 90 degrees of shift in order to identify the steps and the directions of the motion.

There is an additional switch to make the board work in mouse mode or joystick mode. In mouse mode, any connected device will use the analog mouse, so a program like GEOS can be controlled with a USB mouse or a gamepad. In joystick mode, the board uses the joystick pins for any kind of device. This means that some games, like graphic adventure games (e.g., Maniac Mansion), can be played with a mouse even if they were originally designed for a game controller.

The target computer selection (C64 / AMIGA / ATARI) is user-selectable and stored in non-volatile memory, so it persists across reboots. To change it you must connect a USB mouse to the adapter, turn on the computer and, within the first 30 seconds from the boot, press and hold one mouse button for about 5 seconds until the LED blinks. The button you hold selects the target machine:
- **Left button**: Commodore 64
- **Right button**: AMIGA
- **Middle button**: ATARI

After the LED blink feedback, the adapter reboots and returns to the normal LED behavior (mouse mode or joystick mode depending on the switch position).

The PCB are in two versions: THT (version 3.2) and SMD (version 4.1). The functionalities are identical.

<p align="center">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/schematic.jpeg" alt="Schematic" style="width: 50%;">
</p>

<div style="display: flex; justify-content: space-between;">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/adapter-smd.JPG" alt="SMD" style="width: 32%;">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/adapter-smd-c64.JPG" alt="SMD C64" style="width: 32%;">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/adapter-smd-amiga.JPG" alt="SMD Amiga" style="width: 32%;">
</div>

<div style="display: flex; justify-content: space-between;">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/adapter-tht.JPG" alt="THT" style="width: 32%;">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/adapter-tht-c64.jpg" alt="THT C64" style="width: 32%;">
  <img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/adapter-tht-c64-mouse.jpg" alt="THT C64 Mouse" style="width: 32%;">
</div>

## Pre-assembled and Tested Board

<div style="display: flex; justify-content: space-between;">
  <a href="https://www.tindie.com/products/burglar_ot/usbtoc64/"><img src="https://github.com/emanuelelaface/USBtoC64/blob/main/images/tindie-logo.png" alt="Tindie Logo Link" width="150" height="78"></a>
</div>

If you like this project and want a fully assembled and tested board, you can purchase it on [Tindie](https://www.tindie.com/products/burglar_ot/usbtoc64/). By doing so, you can also benefit from a customized configuration and support the future development of the project.

## Components

- **ESP32 S3 Mini**: I use the [Waveshare](https://www.waveshare.com/esp32-s3-zero.htm) version. There are other boards with a similar form factor, but the pinout may be different, which would require redesigning the board.
- **2N3904 NPN transistor**.
- **PCB Slide Switch, 3-pin**.
- **Two 1 kOhm resistors, 1% tolerance**.
- **Two 150 Ohm resistors, 1% tolerance**.
- **One 5.1 kOhm resistor, 1% tolerance**.
- **Two BAT 43 Schottky Diodes**.
- **DE-9 (also known as D-SUB 9 or DB 9) female connector**: It's good practice to remove the metallic enclosure because it can easily short the +5V line of the C64 when inserted, potentially damaging your computer.

---

## Installation From Arduino IDE

To install the code from the source file **USBtoC64.ino**, you will need the Arduino IDE. Ensure that the ESP32 board is installed, specifically the ESP32S3 Dev Module.  
The flag `PAL` sets the default C64 PAL / NTSC timing used when EEPROM settings are first initialized. After flashing, PAL/NTSC timing can be changed from the adapter using **BOOT + middle mouse button**.

Additionally, the ESP32 USB HID HOST library is required. This library is not available in the official repository. You can download the ZIP file of the repository from [ESP32_USB_Host_HID](https://github.com/esp32beans/ESP32_USB_Host_HID). To install it, go to `Sketch -> Include Library -> Add .ZIP Library` in the Arduino IDE.

To set the board in upload mode, hold the **BOOT** button while the board is disconnected from the USB port. Then, connect the board to the USB port and after one second, the USB port should appear in the list of ports in the Arduino IDE. You can then upload the code.

To choose the default PAL or NTSC timing before first boot, set the `#define PAL` line to true or false. After the adapter has initialized EEPROM, use the onboard configuration gesture instead.

## Installation From the Binary File

Alternatively, you can load the binary file **USBtoC64-AMIGA-ATARI-PAL.bin** or **USBtoC64-AMIGA-ATARI-NTSC.bin**, which are located in the `BIN` folder.
The tool to upload the binary is `esptool`. This is available as a web page or as python. The web page should be compatible with Chrome browser or similar, probably not with Firefox, but on some operating system (like Mac OS) there can be a problem of binding the port to the web page. Anyway my suggestion is to try the web page first because it is very fast, and if it does not work try with the python installation.

### From the web page

1. Disconnect the adapter from the Commodore 64 / AMIGA / ATARI.
2. Press and hold the **BOOT** button before connecting the board to the USB cable on the computer. Then, connect the board, wait a second, and release the button.
3. Go to the [esptool](https://espressif.github.io/esptool-js/) website, click on **Connect**, select the port for your adapter, change the Flash Address into `0x0000` and upload the firmware.

### From Python

1. Install the esptool with `pip install esptool`.
2. Disconnect the adapter from the Commodore 64.
3. Press and hold the **BOOT** button before connecting the board to the USB cable on the computer. Then, connect the board, wait a second, and release the button.
4. On the computer, run:

   `esptool.py -b 921600 -c esp32s3 -p <PORT> write_flash --flash_freq 80m 0x00000 USBtoC64-<PAL|NTSC>.bin`

   where `<PORT>` is the USB port created once the board is connected. On Windows, it is probably COM3 or something similar. On Linux and Mac, it will be `/dev/tty.USBsomething` or `/dev/cu.usbsomething`. `<PAL|NTSC>` is the version with the timings for PAL or for NTSC version of the Commodore 64.

---

## Joystick Configuration

Each joystick or gamepad presents data to the USB in a different way. The library used for ESP32 receives an array of `uint8_t` where each element of the array is connected to a button or an axis, and it is not possible to predict in advance how the joystick will associate this data to the buttons. Therefore, the user has to configure the USBtoC64 manually. To do this, follow the procedure below:

- Identify the **BOOT** button on the board. It is the left button when the USB port is oriented towards the top.
- Disconnect the joystick from the controller.
- When the controller is connected to the Commodore 64, it will boot with a `RED` LED for one second, then it will change to green (for Joystick) or blue (for mouse) depending on the position of the switch mode.
- To set the controller in configuration mode, the **BOOT** button must be pressed during the red light. This can be done by rebooting the board (pressing the other button on the board, which is the reset button) and then holding the **BOOT** button during the red LED.
- If pressed correctly, the red light will flash 10 times quickly, and then a `GREEN` LED will indicate that the board is in configuration mode.
- Insert the USB controller to configure; the green light should turn off, and the controller is ready to be programmed.
- The procedure now requires the insertion of 7 controls: the 4 directions in the exact sequence **UP, DOWN, LEFT, RIGHT**, and then 3 buttons for **FIRE**. (3 because most controllers have many buttons and it can be useful to map more than one to fire. Additionally, when the joystick is used in mouse mode, the first 2 fire buttons are assigned as left and right buttons.)
- To associate the controls, press the **BOOT** button each time and then the controller direction.
- Press the **BOOT** button; a `BLUE` LED should appear, and the controller will wait for the UP direction. Once the UP direction is pressed on the controller, the blue LED will turn off.
- Repeat the previous step for DOWN, LEFT, RIGHT, FIRE1, FIRE2, FIRE3.
- After the last button, the controller will flash a `GREEN` LED and will be set to work with that controller.
- It is now possible to reboot the controller and use it with the configured joystick.

If your controller is advanced, with analog joystick and you want to map specifically as mouse or you want some more advanced customization you can discover how your values are mapped following this [Configurator](https://raw.githack.com/emanuelelaface/USBtoC64/main/configurator/config.html) (it works on Chrome and similar browsers, not on Firefox) and once you download the JoystickMapping file you can contact me for a specific configuration, or if you know your business you can code your controls directly in the firmware source replacing the example JoystickMapping.h file and upload to your controller.

---

## Mouse Wheel Support (C64OS + AmigaOS)

An additional **MouseMapping.h** is available to support the **mouse wheel (scroll)** on both **C64OS** and **AmigaOS**.
In order to activate it in the code the variable **MOUSE_MAP_CUSTOM** must be set to **1** and the proper mouse mapping has to be added in the **MM_REPORT_MAPS**. This requires to know how the bytes of the mouse are mapped, I am working on a procedure to get this authomatically in the future.

### C64OS (Wheel Support)
- Requires **C64OS version 1.03 or newer**.
- In the **Mouse** settings/menu, select the **Micromys** driver.

### AmigaOS (Wheel Support)
- Install [FreeWheel](https://aminet.net/package/util/mouse/FreeWheel) on your Amiga.
- Open **FreeWheel** settings, go to **Set Buttons**, and set the **Middle button** to enable scrolling.

---

WARNING: Some controllers may use the USB port to charge a battery (especially if they are also Bluetooth), and this could draw more than 100 mA from the C64, potentially shutting down the Commodore (and possibly damaging it). If you use a controller with a battery, you should remove the battery before connecting it or disable the charging functionality if possible.

---

WARNING: DON'T CONNECT THE COMMODORE 64 AND THE USB PORT TO A SOURCE OF POWER AT THE SAME TIME.  
THE POWER WILL ARRIVE DIRECTLY TO THE SID OF THE COMMODORE AND MAY DESTROY IT.

---

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

