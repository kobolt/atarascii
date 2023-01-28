# atarascii
ASCII Atari 2600 Emulator

This is an emulator to play Atari 2600 games in a Linux terminal.

Features:
* Curses based UI with full 256-color support if available.
* Terminal window can be resized to see all video scanlines.
* SDL2 graphical output also available and can run in parallel.
* Use a joystick/gamepad (as detected by SDL2) or SDL2 keyboard play.
* Keyboard input on the terminal is possible but sketchy and very hard to use.
* Audio is supported but not entirely accurate.
* 2K, 4K and 8K (bank switched) cartridge ROMs supported.
* Timings are currently hardcoded around NTSC.
* Save/Load state supported but only one slot and only in memory.
* Ctrl+C in the terminal breaks into a debugger for dumping data.
* Accepts TAS input in a custom CSV format.

Known issues and missing features:
* PAL and SECAM video modes or timings are not supported.
* Paddles and other custom input devices are not supported.
* Larger cartridge sizes and weird bank switching are not supported.
* TIA audio control modes #2 and #3 faked.
* TIA RSYNC register not implemented.
* PIA INSTAT register not implemented.

YouTube video:
* [Dragster 5.57 TAS](https://www.youtube.com/shorts/jezs96ZDexw)

