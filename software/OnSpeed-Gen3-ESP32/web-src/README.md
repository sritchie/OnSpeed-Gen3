# OnSpeed Web UI Source Files

This directory contains the source HTML/CSS/JavaScript files for the OnSpeed web interface,
plus a development server for local testing without ESP32 hardware.

## Directory Structure

```
web-src/
├── css/
│   └── chartist.css       # Chartist.js styles
├── js/
│   ├── calibration.js     # Calibration wizard logic
│   ├── chartist.js        # Chartist.js library (combined)
│   ├── regression.js      # Polynomial regression library
│   └── sg_filter.js       # Savitzky-Golay filter
├── pages/
│   ├── header.html        # Common header (nav, styles)
│   ├── liveview.html      # LiveView page body
│   └── calibration.html   # Calibration wizard body
├── dev_server.py          # Local development server
└── build.py               # Converts to C++ headers
```

## Local Development Server

The development server allows you to test the web UI locally with either:
- **Replay mode**: Real C++ processing pipeline fed by CSV flight logs
- **Simulate mode**: Generated sensor data (no C++ build required)

### Prerequisites

```bash
# uv is recommended (https://docs.astral.sh/uv/)
# The script uses inline dependencies, so no manual install needed
```

### Replay Mode (Real C++ Pipeline)

This runs your actual OnSpeed C++ code locally, processing CSV flight logs:

```bash
# First, build the native replay program (from parent directory)
cd ..
pio run -e native-replay
cd web-src

# Run with a flight log CSV (using aircraft-specific calibration)
uv run dev_server.py --config /path/to/onspeed.cfg --replay /path/to/flight.csv

# Or without config (uses default calibration)
uv run dev_server.py --replay /path/to/flight.csv

# Open browser
open http://localhost:8080/live
```

The `--config` option loads aircraft-specific calibration curves and AOA setpoints from
a `.cfg` file (the same format used by the OnSpeed hardware).

The CSV file should have these columns:
- `timeStamp`, `PfwdSmoothed`, `P45Smoothed`, `flapsPos`, `Palt`, `IAS`
- `DataMark`, `VSI`, `VerticalG`, `LateralG`, `ForwardG`
- `RollRate`, `PitchRate`, `YawRate`, `Pitch`, `Roll`, `FlightPath`

Real flight logs from the OnSpeed hardware work directly.

### Simulate Mode (Quick Testing)

For UI development without building C++:

```bash
uv run dev_server.py --simulate
open http://localhost:8080/live
```

### Interactive Mode (Control Panel)

For testing AOA thresholds and audio tones with slider controls:

```bash
# Build native replay first
cd ..
pio run -e native-replay
cd web-src

# Run with interactive control panel (with calibration config)
uv run dev_server.py --config /path/to/onspeed.cfg --interactive

# Open two browser tabs:
open http://localhost:8080/control   # Sliders to control flight state
open http://localhost:8080/live      # LiveView shows processed output
```

The control panel lets you adjust raw sensor inputs:
- **Pfwd / P45**: Pressure values (Pa) - the system calculates AOA from these using the calibration curve
- **IAS / Altitude**: Basic flight state
- **Pitch / Roll / G-loads**: AHRS data
- **Flaps position**: Selects which calibration curve to use

Move sliders on the control panel → values are sent to C++ → processed output updates LiveView.

### Command Line Options

```
uv run dev_server.py [OPTIONS]

Options:
  -p, --port PORT      HTTP port (default: 8080)
  -w, --ws-port PORT   WebSocket port (default: 8081)
  -c, --config CFG     Config file (.cfg) with calibration data
  -r, --replay CSV     Replay CSV file through C++ pipeline
  -s, --simulate       Use simulated sensor data (default)
  -i, --interactive    Interactive mode with control panel sliders
  -h, --help           Show help message
```

### How It Works

```
┌─────────────────────────────────────────────────────────────┐
│                    Replay Mode                              │
│                                                             │
│   CSV File → C++ native-replay → JSON stdout → WebSocket   │
│                    (real processing)                        │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                   Simulate Mode                             │
│                                                             │
│   Python generates fake sensor data → WebSocket             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

In replay mode, the dev server:
1. Launches `.pio/build/native-replay/program` as a subprocess
2. Passes the CSV file path as an argument
3. Reads JSON lines from stdout
4. Broadcasts each line to connected WebSocket clients

## Building for ESP32

After making changes to source files:

```bash
python build.py
```

This generates the `Web/*.h` files that get compiled into the firmware.

## File Format

The `.h` files use C++ raw string literals:

```cpp
const char htmlLiveView[] PROGMEM = R"=====(
<style>
  /* CSS here */
</style>
<script>
  // JavaScript here
</script>
<div>
  <!-- HTML here -->
</div>
)=====";
```

The `PROGMEM` directive stores the strings in flash memory rather than RAM.

## Troubleshooting

**"Address already in use" error**: The server sets `SO_REUSEADDR`, but if you still
get this error, wait a few seconds or use different ports with `-p` and `-w`.

**"Native replay program not found"**: Run `pio run -e native-replay` from the
parent directory first.

**CSV file not found**: Use an absolute path or path relative to where you run the command.
