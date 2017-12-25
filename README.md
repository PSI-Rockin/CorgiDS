# CorgiDS
A DS emulator that seeks to combine accuracy and speed. As of v0.1, it's a long way from either one, but with your support, CorgiDS will accomplish its goals.

## Compiling
CorgiDS requires Qt 5.9 and currently supports qmake and meson. Please report any compilation errors or other problems that happen during this process. Pull requests are welcome for CMake support.

### Known issues
On Windows, builds compiled with Visual Studio fail to boot any games and display a gray screen during execution. For now, use mingw instead.

### Building with qmake
```
cd CorgiDS/CorgiDS
qmake CorgiDS.pro
make
```

### Building with meson
```
meson build --buildtype=release
cd build
ninja
```

## Using the Emulator
### Setup
In order to play DS games on CorgiDS, you must dump the BIOS and firmware from your DS or DS Lite. You will need three files:

* ARM9 BIOS - 4 KB
* ARM7 BIOS - 16 KB
* Firmware  - 256 KB

The names of these files do not matter as long as they match the given sizes. Load these files by going to Emulation -> Config or Preferences (on Mac OS X) on the menubar. From the same config window, you can also choose to either boot from the firmware or boot a game directly.

High-level BIOS emulation is in the works but currently remains in an unusable state for games. Once this is implemented, having the above files will be optional.

### Saving
BACK UP YOUR SAVEFILES BEFORE USING CORGIDS

CorgiDS is incapable of automatically detecting what kind of save an arbitrary game uses. It is recommended to have save files corresponding with the game you want to test.

Alternatively, you may use a save database. CorgiDS supports the AKAIO (AceKard All-In-One) savelist.bin. This is configured the same way the BIOS/firmware are. [Download savelist.bin here.](http://akaio.net/data/savelist.bin) Note that the database is outdated and may not work for newer DS games.

If neither of the above checks pass, CorgiDS defaults to a 1 MB FLASH save, under the assumption that newer games will rely on FLASH rather than EEPROM due to increased storage space.

### Controls
Currently, CorgiDS only supports hardcoded key mappings. The controls are as follows:

* X/Z: A/B
* S/A: X/Y
* Q/W: L/R
* Enter/Return: Start
* Select: Space

Other features are accessible through these keys:

* Tab: enable/disable framelimiter
* O: enable/disable frameskip (boosts gameplay speed at the expense of graphical smoothness)
* P: pause/unpause emulation

## Contributing
If you have no coding experience but wish to contribute to CorgiDS' development, playtest as many games as possible and use the GitHub issue tracker to report problems. For v0.1, please only report egregious problems, such as ROMs not booting or major graphical glitches. Due to being relatively new, CorgiDS still has minor issues in many games, such as shadows not appearing or bad transitions in the intros of many games.

## Licensing
CorgiDS is licensed under the GNU GPLv3. See [LICENSE](LICENSE) for details.
