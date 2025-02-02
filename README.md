![](header.png?raw=true)

A complete decompilation of Retro Engine v3.

# **SUPPORT THE OFFICIAL RELEASE OF SONIC CD**
+ Without assets from the official release, this decompilation will not run.

+ You can get an official release of Sonic CD from:
  * Windows
    * Via Steam, whether it's the original release or from [Sonic Origins](https://store.steampowered.com/app/1794960)
    * Via the Epic Games Store, from [Sonic Origins](https://store.epicgames.com/en-US/p/sonic-origins)
  * [iOS (Via the App Store)](https://apps.apple.com/us/app/sonic-cd-classic/id454316134)
    * A tutorial for finding the game assets from the iOS version can be found [here](https://gamebanana.com/tuts/14491).
  * Android
    * [Via Google Play](https://play.google.com/store/apps/details?id=com.sega.soniccd.classic)
    * [Via Amazon](https://www.amazon.com/Sega-of-America-Sonic-CD/dp/B008K9UZY4/)
    * A tutorial for finding the game assets from the Android version can be found [here](https://gamebanana.com/tuts/14942).

Even if your platform isn't supported by the official releases, you **must** buy or officially download it for the assets (you don't need to run the official release, you just need the game assets). Note that only FMV files from the original Steam release of the game are supported; mobile and Origins video files do not work.

# 3DS Port
## Features
- Built on recent decomp source, as of Feb 2025
- Both Old and New 3DS supported
- Fast hardware (GPU) rendering by default
- (NEW) Software renderer supported, but slow special stages on N3DS, and slow in general on O3DS
- (NEW) FMV playback (Currently slow on O3DS and choppy on N3DS, use ffmpeg to scale down OGVs)
- (NEW) Mod support, set up and install them as you normally would
- Stereoscopic 3D support (hardware renderer only)
- Remappable keys in settings.ini (uses [bitmasks](https://github.com/devkitPro/libctru/blob/master/libctru/include/3ds/services/hid.h) from libctru)
- Access dev menu at any time by pressing SELECT

## Setup
- [Dump dspfirm.cdc](https://github.com/zoogie/DSP1/releases) from your 3DS, make sure it's at `sdmc:/3ds/`. This is necessary for audio to work.
- For the best experience, use Data.rsdk from the official mobile version and place it at `sdmc:/3ds/SonicCD/`.
- For the best experience, copy the decompiled scripts from [here](https://github.com/RSDKModding/RSDKv3-Script-Decompilation) to `sdmc:/3DS/SonicCD/Scripts/`.
  - Make sure to set `TxtScripts` in settings.ini to `true` after doing this.
- (Optional) To slightly improve loading times, [extract Data.rsdk contents](https://forums.sonicretro.org/index.php?threads/rsdk-unpacker.30338/) to `sdmc:/3ds/SonicCD/Data/`.
  - Make sure Data.rsdk itself is removed from `sdmc:/3ds/SonicCD/` or set `DataFile` in settings.ini to blank.
- (Optional) For FMV playback, copy the `videos/` folder from the original Steam release to `sdmc:/3ds/SonicCD/`.
  - To ensure the best playback performance, scale down the OGV files to 400x240 using ffmpeg:
  - `ffmpeg -i input.ogv -s 400x240 -c:v libtheora -q:v 7 -c:a libvorbis -q:a 4 output.ogv`
- (Optional) To set up mods, place any mod folders into `sdmc:/3ds/SonicCD/mods/`.
  - Also copy the decompiled scripts as instructed above if you haven't already, as most mods require them.

## Building
- [Install devkitARM and 3ds-dev](https://devkitpro.org/wiki/Getting_Started)
- Install the following packages: `3ds-sdl 3ds-libogg 3ds-libvorbisidec 3ds-libtheora`
- Clone/download this repository
- `cd` to the `RSDKv3.3DS` folder
- Run `make`

## Acknowledgements
- oreo639 - 3ds-theoraplayer code
- Rubberduckycooly, st×tic, and RSDKv3-Decompilation contributors
- Christian "Taxman" Whitehead - Original RSDKv3 author
