# DreamDexed ![Github Build Status](https://github.com/DreamDexed/DreamDexed/actions/workflows/build.yml/badge.svg)

DreamDexed is a [MiniDexed](https://github.com/probonopd/MiniDexed) fork with the following additional features:

- [x] Configurable default screen
- [x] Multiple font sizes for ST7789
- [x] Support for MIDI controller encoders with relative mode
- [x] Noiseless performance and volume change
- [x] Configurable TG compressors
- [x] 3-band EQ (per TG and Master)
- [x] TG-Link
- [x] Two Send FX + Master FX with ZynDistortion, YKChrous, ZynChorus, ZynAPhaser, ZynPhaser, DreamDelay, PlateReverb, CloudSeed2, Compressor, EQ
- [ ] 8 channel mixer (Rpi4+)
- [ ] Multiple parts (RPi4+)
- [ ] Overlay Menu for easier parameter changes
- [ ] MIDI Controller DAW integration

# DreamDexed Arturia fork

This is a fork of DreamDexed that integrates better with Arturia keyboards.

Maybe someday it will be part of the DreamDexed

## Enabling

This feature can be enabled in the minidexed.ini with:
```
DAWControllerEnabled=1
```

## Features

### Controller Display, Main Encoder
The controller display in DAW mode shows the DreamDexed menu.

With the main encoder you can navigate in the menu.

The Home is the shift+Main click

Supported:
- MiniLab 3 (tested)
- KeyLab mkII (tested)
- Keylab Essential
- Keylab Essential 3

### MiniLab 3 features:

#### Encoders

By holding down shift will bring up an overlay menu, where you can see and adjust what the encoders do.

You can use the main encoder to select the current encoding page.

Short pressing the shift will show the actual values.

Holding down the shift will goes back to the normal menu.

This overlay menu is context-aware, it is different if you are in the effect menu, or in a TG menu.

If you are in a TG menu, it only affects the selected TG.

If you are in the main menu, it affects the TGs on the first TG's channel.

- Main Overlay 1 (default)
  - Cutoff, Resonance, FX1Send, FX2Send
  - PortamentoTime, Program, Volume, MasterVolume
  - Faders: ChG 1 Vol, ChG 2 Vol, ChG 3 Vol, ChG 4 Vol

- Effect Overlay 1
  - MasterEQLow, MasterEQMid, MasterEQHigh, MasterEQGain
  - MasterEQLowMidFreq, MasterEQMidHighFreq, None, None
  - Faders: MasterEQLow, MasterEQMid, MasterEQHigh, MasterEQGain

- Effect Overlay 2
  - MasterCREnable, MasterCRPreGain, MasterCRAttack, MasterCRRelease
  - MasterCRThresh, MasterCRRatio, MasterCRHPFilter, None
  - Faders: MasterCRPreGain, MasterCRAttack, MasterCRRelease, MasterCRThresh

- Main Overlay 2
  - Pan1, Pan2, Pan3, Pan4
  - Det1, Det2, Det3, Det4
  - Faders: TG1 Vol, TG2 Vol, TG3 Vol, TG4 Vol

- Main Overlay 3
  - Pan5, Pan6, Pan7, Pan8
  - Det5, Det6, Det7, Det8
  - Faders: TG5 Vol, TG6 Vol, TG7 Vol, TG8 Vol

- TG Overlay 1
  - Cutoff, Resonance, FX1Send, FX2Send
  - MasterTune, PortamentoTime, Volume, Pan
  - Faders: Cutoff, FX1Send, FX2Send, Volume

- TG Overlay 2
  - MIDIChannel, Program, None, PitchBendRange
  - PortamentoGlissando, MonoMode, None, PitchBendStep

- TG Overlay 3
  - TGEQLow, TGEQMid, TGEQHigh, TGEQGain
  - TGEQLowMidFreq, TGEQMidHighFreq, None, None
  - Faders: TGEQLow, TGEQMid, TGEQHigh, TGEQGain

- TG Overlay 4
  - CompressorEnable, CompressorPreGain, CompressorMakeupGain, None
  - CompressorAttack, CompressorRelease, CompressorThresh, CompressorRatio
  - Faders: CompressorPreGain, CompressorAttack, CompressorRelease, CompressorThresh

- TG Overlay 5
  - MWRange, MWPitch, FCRange, FCPitch
  - MWEGBias, MWAmplitude, FCEGBias, FCAmplitude

- TG Overlay 6
  - BCRange, BCPitch, ATRange, ATPitch
  - BCEGBias, BCAmplitude, ATEGBias, ATAmplitude

- Voice Overlay 1
  - ALGORITHM, FEEDBACK, TRANSPOSE, None,
  - None, None, None, None

- Voice Overlay 2
  - PITCH_EG_R1, PITCH_EG_R2, PITCH_EG_R3, PITCH_EG_R4
  - PITCH_EG_L1, PITCH_EG_L2, PITCH_EG_L3, PITCH_EG_L4
  - Faders: PITCH_EG_L1, PITCH_EG_L2, PITCH_EG_L3, PITCH_EG_L4

- Voice Overlay 3
  - OSC_KEY_SYNC, LFO_SPEED, LFO_DELAY, LFO_PITCH_MOD_DEP
  - LFO_SYNC, LFO_WAVE, LFO_PITCH_MOD_SENS, LFO_AMP_MOD_DEP

- OP Overlay 1
  - OP_OUTPUT_LEV, OP_FREQ_COARSE, OP_FREQ_FINE, OP_OSC_DETUNE,
  - OP_OSC_MODE, OP_ENABLE, None, None

- OP Overlay 2
  - OP_EG_R1, OP_EG_R2, P_EG_R3, OP_EG_R4
  - OP_EG_L1, OP_EG_L2, OP_EG_L3, OP_EG_L4,
  - Faders: OP_EG_L1, OP_EG_L2, OP_EG_L3, OP_EG_L4,

- OP Overlay 3
  - OP_LEV_SCL_BRK_PT, OP_SCL_LEFT_DEPTH, OP_SCL_RGHT_DEPTH, OP_AMP_MOD_SENS
  - OP_OSC_RATE_SCALE, OP_SCL_LEFT_CURVE, OP_SCL_RGHT_CURVE, OP_KEY_VEL_SENS

In the Main overlay you can reach the TG and voice overlays also.

In the voice overlay there is a page for every OP control also, with the 6 operator and with an encoder which sets all.

#### Pads on Bank A
- Mono/Poly mode
- Portamento enable/disable, hard press to enable/disable Portamento Glissando
- Sostenuto
- Sustain
- All Sound Off
- Hold2
- None
- Channel AfterTouch Pad

#### Pads on Bank B
- Short press a TG will enable/disable that TG
- Long press will enable/disable all the TG on the TG's channel
- Hard press will enable only that TG (it uses the pad's AT messsages)


### KeyLab mkII features

#### Faders
- The faders 1-8 adjust the volume of the MIDI channels of the current performance
- The fader 9 adjusts the master volume.

#### Encoders
- Cutoff, Resonance, Portamento Time
- None, None, Dry Level, FX1 Send, FX2 Send, None

#### DAW buttons
- Mono/Poly mode
- Portamento enable/disable, hard press to enable/disable Portamento Glissando
- Sostenuto
- Sustain
- Hold2

#### Track select buttons
- Short press a TG will enable/disable that TG
- Long press will enable only that TG



![minidexed](https://user-images.githubusercontent.com/2480569/161813414-bb156a1c-efec-44c0-802a-8926412a08e0.jpg)

DreamDexed is a FM synthesizer closely modeled on the famous DX7 by a well-known Japanese manufacturer running on a bare metal Raspberry Pi (without a Linux kernel or operating system). On Raspberry Pi 2 and larger, it can run 8 tone generators, not unlike the TX816/TX802 (8 DX7 instances without the keyboard in one box). [Featured by HACKADAY](https://hackaday.com/2022/04/19/bare-metal-gives-this-pi-some-classic-synths/), [Adafruit](https://blog.adafruit.com/2022/04/25/free-yamaha-dx7-synth-emulator-on-a-raspberry-pi/), [The MagPi magazine](https://magpi.raspberrypi.com/articles/mini-dexed) (Issue 142 June 2024, [PDF](https://magpi.raspberrypi.com/issues/142)) and [Synth Geekery](https://www.youtube.com/watch?v=TDSy5nnm0jA).

## Demo songs

Listen to some examples made with MiniDexed by Banana71 [here](https://soundcloud.com/soundplantage/sets/minidexed2).

## Features

- [x] Uses [Synth_Dexed](https://codeberg.org/dcoredump/Synth_Dexed) with [circle-stdlib](https://github.com/smuehlst/circle-stdlib)
- [x] SD card contents can be downloaded from [GitHub Releases](../../releases)
- [x] Runs on all Raspberry Pi models (except Pico); see below for details
- [x] Produces sound on the headphone jack, HDMI display or [audio extractor](https://github.com/DreamDexed/DreamDexed/wiki/Hardware#hdmi-to-audio) (better), or a [dedicated DAC](https://github.com/DreamDexed/DreamDexed/wiki/Hardware#i2s-dac) (best)
- [x] Supports multiple voices through Program Change and Bank Change LSB/MSB MIDI messages
- [x] Loads voices from `.syx` files from SD card (e.g., using `getsysex.sh` or from [Dexed_cart_1.0.zip](http://hsjp.eu/downloads/Dexed/Dexed_cart_1.0.zip))
- [x] Menu structure on optional [HD44780 display](https://www.berrybase.de/sensoren-module/displays/alphanumerische-displays/alphanumerisches-lcd-16x2-gr-252-n/gelb) and rotary encoder
- [x] Runs up to 8 Dexed instances simultaneously (like in a TX816) and mixes their output together
- [x] Allows for each Dexed instance to be detuned and stereo shifted
- [x] Allows to configure multiple Dexed instances through `performance.ini` files (e.g., [converted](https://github.com/BobanSpasic/MDX_Vault) from DX1, DX5, TX816, DX7II, TX802)
- [x] Compressor effect
- [x] Reverb effect
- [x] Voices can be edited over MIDI, e.g., using the [synthmata](https://synthmata.github.io/volca-fm/) online editor (requires [additional hardware](https://github.com/DreamDexed/DreamDexed/wiki/Hardware#usb-midi-devices))

## Introduction

Video about this project by [Floyd Steinberg](https://www.youtube.com/watch?v=Z3t94ceMHJo):

[![YouTube Video about MiniDexed (Floyd Steinberg)](https://i.ytimg.com/vi/Z3t94ceMHJo/sddefault.jpg)](https://www.youtube.com/watch?v=Z3t94ceMHJo)

## System Requirements

- Raspberry Pi 1, 2, 3, 4, or 400. Raspberry Pi Zero and Zero 2 can be used but need HDMI or a supported i2s DAC for audio out. On Raspberry Pi 1 and on Raspberry Pi Zero there will be severely limited functionality (only one tone generator instead of 8)
- Raspberry Pi 5 can be used but currently support is experimental: HDMI sound and USB Gadget mode are not available yet, and it is not clear if there are implications for cooling from running DreamDexed. Also, DreamDexed is currently not taking advantage of the higher processing power of the Raspberry Pi 5 yet. *Hence, you may consider using one of the less expensive, older Raspberry Pi boards for your first build.*
- A [PCM5102A or PCM5122 based DAC](https://github.com/DreamDexed/DreamDexed/wiki/Hardware#i2s-dac), HDMI display or [audio extractor](https://github.com/DreamDexed/DreamDexed/wiki/Hardware#hdmi-to-audio) for good sound quality. If you don't have this, you can use the headphone jack on the Raspberry Pi but on anything but the Raspberry 4 the sound quality will be seriously limited
- Optionally (but highly recommended), an [LCDC1602 Display](https://www.berrybase.de/en/sensors-modules/displays/alphanumeric-displays/alphanumerisches-lcd-16x2-gr-252-n/gelb) (with or without i2c "backpack" board) and a [KY-040 rotary encoder](https://www.berrybase.de/en/components/passive-components/potentiometer/rotary-encoder/drehregler/rotary-encoder-mit-breakoutboard-ohne-gewinde-und-mutter)

## Usage

- In the case of Raspberry Pi 4, Update the firmware and bootloader to the latest version (not doing this may cause USB reliability issues)
- Download from [GitHub Releases](../../releases)
- Unzip
- Put the files into the root directory of a FAT32 formatted partition on SD/microSD card (Note for small SD cards which are no longer sold: If less than 65525 clusters, you may need to format as FAT16.)
- Put SD/microSD card into Raspberry Pi 1, 2, 3 or 4, or 400 (Zero and Zero 2 can be used but need HDMI or a supported i2c DAC for audio out)
- Attach headphones to the headphone jack using `SoundDevice=pwm` in `minidexed.ini` (default) (poor audio quality)
- Alternatively, attach a  PCM5102A or PCM5122 based DAC and select i2c sound output using `SoundDevice=i2s` in `minidexed.ini` (best audio quality)
- Alternatively, attach a HDMI display with sound and select HDMI sound output using `SoundDevice=hdmi` in `minidexed.ini` (this may introduce slight latency)
- Attach a MIDI keyboard via USB (alternatively you can build a circuit that allows you to attach a "traditional" MIDI keyboard using a DIN connector, or use a DIN-MIDI-to-USB adapter)
- If you are using a LCDC1602 with an i2c "backpack" board, then you need to set `LCDI2CAddress=0x27` (or another address your i2c "backpack" board is set to) in `minidexed.ini`
- Boot
- Start playing
- If the system seems to become unresponsive after a few seconds, remove `usbspeed=full` from `cmdline.txt` and repeat ([details](https://github.com/probonopd/MiniDexed/issues/39))
- Optionally, put voices in `.syx` files onto the SD card (e.g., using `getsysex.sh`)
- See the Wiki for [Menu](https://github.com/DreamDexed/DreamDexed/wiki/Menu) operation
- For voice programming, use any DX series editor (using MIDI sysex), including Dexed
- For library management, use the dedicated [MiniDexedLibrarian](https://github.com/BobanSpasic/MiniDexedLibrarian) software
- If something is unclear or does not work, don't hesitate to [ask](https://github.com/DreamDexed/DreamDexed/discussions/)!

## Pinout

All devices on Raspberry Pi GPIOs are **optional**.

![Raspberry Pi Pinout/GPIO Diagram](https://user-images.githubusercontent.com/2480569/166105580-da11481c-8fc7-4375-8ab1-3031ab5c6ad0.png)

Please see the [wiki](https://github.com/DreamDexed/DreamDexed/wiki) for more information.

## Downloading

Compiled versions are available on [GitHub Releases](../../releases). Just download and put on a FAT32 formatted SD card.

## Building

Please see the [wiki](https://github.com/DreamDexed/DreamDexed/wiki/Development#building-locally) on how to compile the code yourself.

## Contributing

This project lives from the contributions of skilled C++ developers, testers, writers, etc. Please see <https://github.com/DreamDexed/DreamDexed/issues>.

## Discussions

We are happy to hear from you. Please join the discussions on <https://github.com/DreamDexed/DreamDexed/discussions>.

## Documentation

Project documentation is at <https://github.com/DreamDexed/DreamDexed/wiki>.

## Acknowledgements

This project stands on the shoulders of giants. Special thanks to:

- [raphlinus](https://github.com/raphlinus) for the [MSFA](https://github.com/google/music-synthesizer-for-android) sound engine
- [asb2m10](https://github.com/asb2m10/dexed) for the [Dexed](https://github.com/asb2m10/dexed) software
- [dcoredump](https://github.com/dcoredump) for [Synth Dexed](https://codeberg.org/dcoredump/Synth_Dexed), a port of Dexed for embedded systems
- [rsta2](https://github.com/rsta2) for [Circle](https://github.com/rsta2/circle), the library to run code on bare metal Raspberry Pi (without a Linux kernel or operating system) and for the bulk of the DreamDexed code
- [smuehlst](https://github.com/smuehlst) for [circle-stdlib](https://github.com/smuehlst/circle-stdlib), a version with Standard C and C++ library support
- [Banana71](https://github.com/Banana71) for the sound design of the [Soundplantage](https://github.com/Banana71/Soundplantage) performances shipped with DreamDexed
- [BobanSpasic](https://github.com/BobanSpasic) for the [MiniDexedLibrarian](https://github.com/BobanSpasic/MiniDexedLibrarian) software, [MiniDexed performance converter](https://github.com/BobanSpasic/MDX_PerfConv) and [collection of performances for MiniDexed](https://github.com/BobanSpasic/MDX_Vault)
- [diyelectromusic](https://github.com/diyelectromusic/) for many [contributions](https://github.com/DreamDexed/DreamDexed/commits?author=diyelectromusic)
- [dwhinham/mt32-pi](https://github.com/dwhinham/mt32-pi) for creating networking support for Circle
- [omersiar](https://github.com/omersiar) for porting networking support to MiniDexed
- [soyersoyer](https://github.com/soyersoyer) for sound and other improvements, and for debugging
  
## About the Reverb (Cloud Seed)

DreamDexed includes the **Cloud Seed** reverb algorithm, originally created by [Ghost Note Audio](https://github.com/ValdemarOrn/CloudSeed).

- The algorithm is MIT-licensed and may be used in both free and commercial projects.  
- **Cloud Seed** is the work of Ghost Note Audio and not part of DreamDexed itself.  
- When mentioning *Cloud Seed* in documentation or marketing, it must be clear that DreamDexed is an independent project that simply makes use of this reverb algorithm.  
- Ghost Note Audio reserves the right to withdraw the use of the *Cloud Seed* name if these conditions are not respected.  

### Credits 
A huge thanks to **Ghost Note Audio** for releasing this gem under MIT.  
Without Cloud Seed, our DX tones would be stuck in the dry desert of 80s presets —  
now they can float through infinite digital cathedrals. 🌌🎹✨
