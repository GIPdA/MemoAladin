# MemoAladin
### MemoMouse replacement for Aladin dive computers

This project is a partial replacement of the Uwatec MemoMouse interface, used to download diving records off Aladin dive computers to DataTrack (Uwatec viewing software).

It only implements dive record send to DataTrack by simulating the MemoMouse protocol (no write or config to the Aladin). All stored dive records are sent (no time check), but DataTrack ignore logs that it already got.

In theory it should be compatible with all Aladin computers, but testing were only made with an Aladin Pro.

	Note that this project was done in 2017, and some of the infos below may not be up to date. The hardware should work on any Arduino-compatible board with 2 Serial ports.

## Hardware

Very cheap Arduino-compatible STM32 board, also known as "Blue Pill": [https://stm32-base.org/boards/STM32F103C8T6-Blue-Pill.html](https://stm32-base.org/boards/STM32F103C8T6-Blue-Pill.html) (! you'll need a USB-Serial interface to program the bootloader the first time to be able to use the integrated USB port !)

	You need to fix the USB pullup by replacing R10 with a 1.5k resistor or opt for the dirty fix like me and add a 1.8k resistor between PA12 and 3V3.

USB-Serial for PC comm, and original dive computer wires/connectors. Small LED for status.

Pinout:
- Aladin wire RED: GND
- Aladin wire BLACK: PA3 (Serial2 RX) through 10k resistor
- LED: PB1-GND

(some hardware prep is needed, see stm32duino wiki.)

![3D-printed casing](https://github.com/GIPdA/MemoAladin/raw/master/images/DSC_1211.JPG)
![STM32 board inside](https://github.com/GIPdA/MemoAladin/raw/master/images/DSC_1204.JPG)


Small 3D printed case, wires came from a dead MemoMouse.

#### Possible improvements:
Adjust input resistor and add a weak pullup/pulldown to reject Serial port noise when no dive computer is connected, thus detecting the presence of the dive computer (receiving zeros).


## Software

	You need to add STM32 support to Arduino: https://github.com/rogerclarkmelbourne/Arduino_STM32
	And this Arduino library: https://github.com/harrisonhjones/LEDEffect

The board must be flashed with the appropriate bootloader (see the link provided above in Hardware).

Upload code with Arduino for a STM32F103C8 board, 72MHz. No hardware-specific functions are used. Nothing else to do, it should work immediately (with the USB fix).

Serial port 2 RX is connected to the dive computer. USB-Serial is used to communicate with DataTrack.

Use it like with the MemoMouse: Connect, start DataTrack transfer (option "get newest"), start Aladin log transfer.
You may need to select the right COM port instead of the default auto mode.


LED status:
- Breathing: idle, waiting transfer
- Fast blink: data transfer in progress
- Slow blink: waiting for PC transfer or Aladin transfer (one or the other is already done)


Drivers are needed under Windows for the USB-Serial. They can be found here : <https://github.com/rogerclarkmelbourne/Arduino_STM32>

## How to Use

Plug the MemoAladin first* to your computer with a micro-usb cable, then plug the 2 wires to your Aladin computer. Then use as a MemoMouse: initiate the transfert on DataTrack, then send the logs with the Aladin.

You can also do backward and send the logs from the Aladin first, then transfert from DataTrack. The transfert will be immediate and will not wait for the Aladin (but do not wait too long or your dive logs will have wrong date&time! DataTrack syncs on the Aladin).

**If you plug the wires to the Aladin without the interface powered, the 2 wires will act as shorted and trigger button actions in your Aladin...*

## 3D Printed Casing

The STL files of my 3D printed case are provided if you want to reuse it as-is, or if you want to modify it there is a Fusion 360 archive file you can import.

Screws are 6mm head / 3mm thread diameter self-tapping screws.


## Resources
Big thanks to Wayne Heming who did all the hard work of figuring out the protocols:

<http://dive.hemnet.com.au/projects/gizmo/Aladin%20Info%203-1.htm>
<http://dive.hemnet.com.au/projects/gizmo/Aladin%20Info%201-1.htm>

The only "errata" I could make is for the MemoMouse data structure, Note 4 (page bottom): don't need to send the data twice.

(I included here copies of these pages as PDF, just in case.)


## Notes

The interface is working as-is for me, so I do not plan on improving it unless a bug fix is needed. I made it to replace my father's dead MemoMouse.

Report any issue you may find, I will at least try to help!

Source code contains a recorded dive log (straight from an Aladin) I used for tests, might be useful for someone...

If you would like to acquire a ready-made interface, contact me and we'll see what's possible.
