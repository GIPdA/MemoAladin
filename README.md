# MemoAladin
### MemoMouse replacement for Aladin dive computers

This project is a partial replacement of the Uwatec MemoMouse interface, used to download diving records off Aladin dive computers to DataTrack (Uwatec viewing software).

It only implements dive record send to DataTrack by simulating the MemoMouse protocol (no write or config to the Aladin). All stored dive records are sent (no time check), but DataTrack ignore logs that it already got.

In theory it should be compatible with all Aladin computers, but testing were only made with an Aladin Pro.


## Hardware

Very cheap Arduino-compatible STM32 board, also known as "Blue Pill": <http://wiki.stm32duino.com/index.php?title=Blue_Pill>

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

Upload code with Arduino for a STM32F103C8 board, 72MHz. No hardware-specific functions are used.

Serial port 2 RX is connected to the dive computer. USB-Serial is used to communicate with DataTrack.

Use is the same as with the MemoMouse: Connect, start DataTrack transfer (option "get newest"), start Aladin log transfer.


LED status:
- Breathing: idle, waiting transfer
- Fast blink: data transfer in progress
- Slow blink: waiting for PC transfer or Aladin transfer (one or the other is already done)


Drivers are needed under Windows for the USB-Serial. They can be found here : <https://github.com/rogerclarkmelbourne/Arduino_STM32>


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

If you are interested by acquiring a ready-made interface, contact me.
