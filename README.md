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
* This is a brand new 3DS port based on the HW version of the decomp.
* It uses Citro3D for the graphics backend, SDL 1.2 for the audio backend, among the usual dependencies.
* It also **runs full speed on both O3DS and N3DS!**
* Using Data.rsdk from the mobile version is highly recommended, as the special stages are graphically broken with the non-mobile Data.rsdk (no LOD tiles?).
* Stereo 3D support isn't currently implemented yet, but is planned. If you'd like to play with some form of S3D support right now, SaturnSH2x2's 3DS port (HW version) supports it quite well (apart from special stages, which are broken in that version).

This port is still very work-in-progress, so if you're looking to just play the game right now with no issues, go and grab SaturnSH2x2's 3DS port over on their repository:
https://github.com/SaturnSH2x2/Sonic-CD-11-3DS

# How to build
* Coming soon... (unless you're already savvy, of course)

# FAQ
### Q: Can the D-Pad and Circle Pad both be mapped to movement at the same time?
A: Not yet. For now, choose one you feel most comfortable with and modify the keyboard mappings in settings.ini, using the key button masks defined in libctru.
