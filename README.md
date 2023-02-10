![](header.png?raw=true)
# **SUPPORT THE OFFICIAL RELEASE OF SONIC CD**
+ Without assets from the official release, this decompilation will not run.

+ You can get an official release of Sonic CD from:
  * Windows
    * Via Steam, whether it's the original release or from [Sonic Origins](https://store.steampowered.com/app/1794960)
    * Via the Epic Games Store, from [Sonic Origins](https://store.epicgames.com/en-US/p/sonic-origins)
  * [iOS (Via the App Store)](https://apps.apple.com/us/app/sonic-cd-classic/id454316134)
  * Android
    * [Via Google Play](https://play.google.com/store/apps/details?id=com.sega.soniccd.classic)
    * [Via Amazon](https://www.amazon.com/Sega-of-America-Sonic-CD/dp/B008K9UZY4/)
    * A tutorial for finding the game assets from the Android version can be found [here](https://gamebanana.com/tuts/14942).

Even if your platform isn't supported by the official releases, you **must** buy or officially download it for the assets (you don't need to run the official release, you just need the game assets). Note that only FMV files from the original Steam release of the game are supported; mobile and Origins video files do not work.

# Nintendo 3DS Port (Again)
This is a brand new 3DS port based on the HW version of the decomp.
It uses Citro3D for the graphics backend, SDL 1.2 for the audio backend, among the usual dependencies.

## Features
* Built on the most recent decomp code base (as of Feb 2023)
* **Runs full speed on both O3DS and N3DS!**
* Stereoscopic 3D support
* No laggy/slow special stages
* (Hopefully) less crash-prone than the other existing 3DS port
* Remappable (keyboard) keys in settings.ini (Uses [bitmasks](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/services/hid.h) from libctru)
* And just like the other port, dev menu can be accessed at any time by pressing SELECT

## Setup
* You need to [dump the DSP firm](https://github.com/zoogie/DSP1/releases) before running the game, otherwise there will be no audio.
* Using Data.rsdk from the **mobile version** is highly recommended; see known issues below for reasons why.
* Just copy `SonicCD.3dsx` (or install `SonicCD.cia`) and `Data.rsdk` to `/3ds/SonicCD/` on the SD card.
* **Don't copy over an existing settings.ini**; this might result in unexpected issues (i.e. no input). Just let a new settings.ini be generated on first startup before modifying anything in the config.
* FMVs are currently unsupported, so don't bother copying them. You'll only need the Data.rsdk for now.
* Similarly, mods are temporarily disabled as there is currently a linker error when trying to build the relevant code (undefined reference to `pathconf` in a part of std::fs)

## Known Issues
* When using non-mobile Data.rsdk, the floor in special stages is graphically broken (no LOD tiles?).
* When using non-mobile Data.rsdk, Tidal Tempest is completely graphically broken (black screen/garbage).
* When using mobile Data.rsdk, the pause button is mapped to the B button, and pausing doesn't work at all in special stages.

## How to build (via Windows)
* Install and setup [msys2](https://www.msys2.org/)
* Add the [devkitpro repositories](https://devkitpro.org/wiki/devkitPro_pacman) to pacman
* Run `pacman -S git 3ds-dev 3ds-sdl 3ds-libogg 3ds-libvorbisidec 3ds-libtheora` and install all the packages
* `git clone` this repository
* `cd` to the `RSDKv3.3ds` folder
* Run `make`

## FAQ
### Q: Can the D-Pad and Circle Pad both be mapped to movement at the same time?
A: Not yet. For now, choose one you feel most comfortable with and modify the keyboard mappings in settings.ini, using the key button masks defined in libctru.
