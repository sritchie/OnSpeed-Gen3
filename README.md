# OnSpeed-Gen3
Gen3 - OnSpeed open source hardware and software

This is a device that provides simple and intuitive Onspeed tone cues that allow the pilot to 
hear key performance AOA's as well outstanding progressive stall warning. ONSPEED AOA is 
used for approach and landing, assists with maximum performance takeoff and provides optimum 
turn performance.  The aural logic simplifies energy management and helps the pilot maintain 
positive aircraft control.

Gen3 builds on and extends the capabilities of Gen2 with new hardware and software. The 
current Gen3 device is avionics independent. The current features include:

    Measures differential pressures from an AOA pitot tube
  
    Measures static pressure
  
    Receives, parses and logs several types of EFIS data for testing purposes
  
    Receives, parses and logs flight test boom data for testing purposes
  
    Calculates a differential pressure based calibrated Angle of Attack and uses it to generate the OnSpeed tones.
  
    Can be wired directly into the aircraft's audio panel.
  
    It logs all data at 50Hz to the onboard SD card and provides an experimental AOA display via its Wifi interface.
  
    Logs can also be downloaded via Wifi.

We are happy to collaborate and answer any questions.  We will work with any experimenter 
or manufacturer that is interested in adapting the logic and always welcome qualified 
volunteers that want to join the project. Email us at team@flyonspeed.org to get in touch.

More details and videos of the OnSpeed system at http://flyOnspeed.org

---

## Development

### Building the Firmware

See [CLAUDE.md](../../CLAUDE.md) for detailed build instructions using PlatformIO or Arduino IDE.

```bash
cd software/OnSpeed-Gen3-ESP32

# Build with PlatformIO
pio run

# Build and upload
pio run -t upload
```

### Web UI Development

The OnSpeed device includes a web-based configuration interface served over WiFi. The web content (HTML/CSS/JavaScript) is embedded in C++ header files in `software/OnSpeed-Gen3-ESP32/Web/`.

#### Local Development Server

To develop the web UI without hardware, use the included development server:

```bash
cd software/OnSpeed-Gen3-ESP32/web-src

# Run the development server (uv handles dependencies automatically)
uv run dev_server.py
```

Then open in your browser:
- **LiveView**: http://localhost:8080/live - Real-time sensor display
- **Calibration Wizard**: http://localhost:8080/calwiz - AOA calibration interface

The dev server:
- Extracts HTML/JS/CSS from the existing `.h` header files
- Patches WebSocket URLs to use localhost
- Simulates sensor data via WebSocket at 50Hz

#### Editing Web Content

The web content is stored in `Web/*.h` files as C++ raw string literals:

| File | Content |
|------|---------|
| `html_header.h` | Common HTML header, navigation, styles |
| `html_liveview.h` | LiveView page (AOA indicator, attitude display) |
| `html_calibration.h` | Calibration wizard UI |
| `javascript_calibration.h` | Calibration logic, curve fitting |
| `javascript_chartist*.h` | Chartist.js charting library |
| `javascript_regression.h` | Polynomial regression library |
| `css_chartist.h` | Chartist.js styles |

To extract these to editable source files:

```bash
cd software/OnSpeed-Gen3-ESP32/web-src
python build.py extract
```

This creates separate files in `web-src/css/`, `web-src/js/`, and `web-src/pages/`.

After editing, regenerate the headers:

```bash
python build.py build
```

#### Connecting to Real Hardware

When connected to the OnSpeed device's WiFi:
- **SSID**: `OnSpeed`
- **Password**: `angleofattack`
- **Web Interface**: http://192.168.0.1

### Running Tests

```bash
cd software/OnSpeed-Gen3-ESP32

# Run unit tests (no hardware required)
pio test -e native
```
