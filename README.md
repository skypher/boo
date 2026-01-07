# Boo 👻

A cute ghostly Tamagotchi for M5Stack Cardputer.

![Platform](https://img.shields.io/badge/platform-M5Stack%20Cardputer-blue)
![Framework](https://img.shields.io/badge/framework-Arduino-green)

## Features

- Adorable ghost pet with pink/purple theme
- Bouncing animation with sparkles, hearts, and stars
- Feed your ghost to keep it happy
- Play "Catch the Stars" mini-game
- Stats tracking (age, happiness, hunger)
- Persistent save data via ESP32 NVS

## Controls

| Key | Action |
|-----|--------|
| 1 | Open menu (Feed / Play / Return) |
| 2 | Show stats |
| ! | Enter flash mode for development (Shift+1) |

## Building

Requires [PlatformIO](https://platformio.org/).

```bash
# Install dependencies and build
pio run

# Upload via USB (device must be in flash mode)
pio run -t upload
```

## Flash Mode

Boo includes a built-in flash mode for easy development. Press `!` (Shift+1) while Boo is running to enter download mode, then flash via esptool:

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM1 --baud 921600 \
    write-flash 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 firmware.bin
```

The firmware files are in `.pio/build/m5stack-cardputer/` after building.

## Hardware

- **Device**: M5Stack Cardputer
- **MCU**: ESP32-S3 (8MB Flash)
- **Display**: 240x135 TFT
- **Input**: 56-key keyboard

## Credits

Inspired by the original [Boo for MicroHydra](https://github.com/BeepBeep84/cardputer) by BeepBeep84.

Ported to Arduino/C++ for better debugging and the M5 Launcher ecosystem.

## License

MIT
