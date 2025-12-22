# OnSpeed-Gen3

<div align="center">

[![Build](https://github.com/flyonspeed/OnSpeed-Gen3/actions/workflows/ci.yml/badge.svg)](https://github.com/flyonspeed/OnSpeed-Gen3/actions/workflows/ci.yml)
[![codecov](https://codecov.io/gh/flyonspeed/OnSpeed-Gen3/graph/badge.svg)](https://codecov.io/gh/flyonspeed/OnSpeed-Gen3)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

</div>

Gen3 - OnSpeed open source hardware and software

This is a device that provides simple and intuitive Onspeed tone cues that allow the pilot to
hear key performance AOA's as well outstanding progressive stall warning. ONSPEED AOA is
used for approach and landing, assists with maximum performance takeoff and provides optimum
turn performance. The aural logic simplifies energy management and helps the pilot maintain
positive aircraft control.

Gen3 builds on and extends the capabilities of Gen2 with new hardware and software. The
current Gen3 device is avionics independent. The current features include:

- Measures differential pressures from an AOA pitot tube
- Measures static pressure
- Receives, parses and logs several types of EFIS data for testing purposes
- Receives, parses and logs flight test boom data for testing purposes
- Calculates a differential pressure based calibrated Angle of Attack and uses it to generate the OnSpeed tones
- Can be wired directly into the aircraft's audio panel
- Logs all data at 50Hz to the onboard SD card and provides an experimental AOA display via its Wifi interface
- Logs can also be downloaded via Wifi

We are happy to collaborate and answer any questions. We will work with any experimenter
or manufacturer that is interested in adapting the logic and always welcome qualified
volunteers that want to join the project. Email us at team@flyonspeed.org to get in touch.

More details and videos of the OnSpeed system at http://flyOnspeed.org

## Software Development

FlyOnSpeed (FOS) Gen 3 software is targeted for the ESP32-S3-WROOM-2 system on a chip made by
Espressif. Different FOS platforms may have slightly different versions of this device
but for use in FOS all should be compatible.

The ESP32-S3-WROOM-2 comes with varying amounts of Flash RAM and external PSRAM accessed via
a dedicated SPI interface. All FOS hardware use the 32 MB Flash variant. Currently PSRAM is
not used and so the size of PSRAM available is not significant.

### Pre-built Firmware

Each commit to `master` builds firmware automatically via GitHub Actions. To flash without building:

1. Go to the [Actions tab](../../actions) and click the latest successful workflow run
2. Download the `firmware` artifact (contains `.bin` files)
3. Flash using [esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32/):
   ```bash
   pip install esptool
   esptool.py --chip esp32s3 write_flash 0x0 firmware.bin
   ```
   Or use the [ESP Web Flasher](https://esp.huhn.me/) for a browser-based option.

**Versioning:** The firmware version is derived from git tags at build time. Tag a release with `git tag v4.0.1` and the version string is automatically embedded in the firmware. Between tags, the version includes the commit hash (e.g., `v4.0.0-5-gabc1234` means 5 commits after v4.0.0).

### Building with PlatformIO

Install PlatformIO and build:

```bash
pip install platformio
pio run
```

Upload to a connected device:

```bash
pio run -t upload
```

Monitor serial output (Ctrl+C to exit):

```bash
pio device monitor
```

Or build, upload, and monitor in one command:

```bash
pio run -t upload && pio device monitor
```

All dependencies are automatically downloaded. See `platformio.ini` for configuration.

### Building with Arduino IDE

Gen 3 compiles under the Arduino 2 integrated development environment (IDE). After installing
the Arduino 2 IDE support for the ESP32-S3 is included via the Arduino Board Manager. Before
using Board Manager the Preferences must be updated to include the URL of the Espressif
ESP32 boards to be used. In the "Additional Boards Manager URLs" include the following URL:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

If you anticipate working with M5 Stack code also then also include the following URL:

```
https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
```

In the Preferences dialog also make sure your sketch location is correctly set. This is
necessary to find the necessary libraries.

From the Arduino 2 Board Manager select the ESP32S3 Dev Module Octal (WROOM2) device. Then under
the Tools menu select at least the following:

- Flash Mode: `OPI 80 MHz`
- Flash Size: `32MB (256 Mb)`
- Partition Scheme: `32M Flash (4.8MB APP/22MB LittleFS)`
- Upload Speed: `921600`

### Required Libraries

#### Vendored Libraries (in `lib/`)

These are included in the repository:

| Library | Description |
|---------|-------------|
| csv-parser | CSV file parsing |

#### External Libraries

These are automatically installed by PlatformIO, or must be installed manually for Arduino IDE:

| Library | Author | Version |
|---------|--------|---------|
| Adafruit NeoPixel | Adafruit | 1.15.2 |
| DallasTemperature | Miles Burton | 4.0.5 |
| OneButton | Matthias Hertel | 2.6.1 |
| OneWire | Jim Studt, et al | 2.3.8 |
| RunningAverage | Rob Tillaart | 0.4.8 |
| RunningMedian | Rob Tillaart | 0.3.10 |
| SavitzkyGolayFilter | James Deromedi | 1.0.1 |
| SdFat | Bill Greiman | 2.3.1 |
| tinyxml2 | Lee Thomason | 11.0.0 |
| WebSockets | Markus Sattler | 2.7.1 |
