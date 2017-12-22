# CorgiDS
A DS emulator that seeks to combine accuracy and speed. As of v0.1, it's a long way from either one, but with your support, CorgiDS will accomplish its goals.

## Compiling
CorgiDS requires Qt and currently only supports qmake. Your best chance of compiling the project is to use Qt Creator. Feel free to report compilation errors.

## Using the Emulator
In order to play DS games on CorgiDS, you must dump the BIOS and firmware from your DS or DS Lite. You will need three files:

* ARM9 BIOS - 4 KB
* ARM7 BIOS - 16 KB
* Firmware  - 256 KB

The names of these files do not matter as long as they match the given sizes. Load these files by going to Emulation -> Config or Preferences (on Mac OS X) on the menubar. From the same config window, you can also choose to either boot from the firmware or boot a game directly.

Currently, CorgiDS only supports hardcoded key mappings. The controls are as follows:

* X/Z: A/B
* S/A: X/Y
* Q/W: L/R
* Enter/Return: Start
* Select: Space

Future updates will add configurable key mappings and controller support.

## Contributing
If you have no coding experience but wish to contribute to CorgiDS' development, playtest as many games as possible and use the GitHub issue tracker to report problems. For v0.1, please only report egregious problems, such as ROMs not booting or major graphical glitches. Due to being relatively new, CorgiDS still has minor issues in many games.

## Licensing
CorgiDS is licensed under the GNU GPLv3. See [LICENSE](LICENSE) for details.
