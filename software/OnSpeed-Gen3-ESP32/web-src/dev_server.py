#!/usr/bin/env python3
# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "websockets",
# ]
# ///
"""
OnSpeed Web UI Development Server

Serves the OnSpeed web interface with data from the C++ processing pipeline
or simulated sensor data for development.

Modes:
    --replay <csv>   : Replay CSV through real C++ pipeline
    --simulate       : Generate simulated sensor data (no C++)

Usage:
    uv run dev_server.py --replay ../../10_24_cockpit_data/log_4.csv
    uv run dev_server.py --simulate

Then open:
    http://localhost:8080/live      - LiveView page
    http://localhost:8080/calwiz    - Calibration Wizard
"""

import argparse
import asyncio
import http.server
import json
import math
import random
import re
import socketserver
import subprocess
import sys
import threading
from pathlib import Path

import websockets


def extract_content_from_header(header_path: Path) -> str:
    """Extract the raw string content from a C++ PROGMEM header file."""
    content = header_path.read_text()
    match = re.search(r'R"=====\((.*?)\)====="', content, re.DOTALL)
    return match.group(1) if match else ""


def build_html_page(web_dir: Path, page_type: str = "live", ws_port: int = 8081) -> str:
    """Build a complete HTML page from the header components."""
    header = extract_content_from_header(web_dir / "html_header.h")
    css_chartist = extract_content_from_header(web_dir / "css_chartist.h")

    if page_type == "live":
        body = extract_content_from_header(web_dir / "html_liveview.h")
        return header + css_chartist + body

    elif page_type == "calibration":
        body = extract_content_from_header(web_dir / "html_calibration.h")
        js_calibration = extract_content_from_header(web_dir / "javascript_calibration.h")
        js_chartist1 = extract_content_from_header(web_dir / "javascript_chartist1.h")
        js_chartist2 = extract_content_from_header(web_dir / "javascript_chartist2.h")
        js_regression = extract_content_from_header(web_dir / "javascript_regression.h")
        sg_filter = extract_content_from_header(web_dir / "sg_filter.h")
        return (
            header + css_chartist + js_chartist1 + js_chartist2 +
            js_regression + sg_filter + js_calibration + body
        )

    elif page_type == "control":
        return build_control_panel_html(ws_port)

    return header


def build_control_panel_html(ws_port: int) -> str:
    """Build the control panel HTML page with sliders."""
    return f'''<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>OnSpeed Control Panel</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            padding: 20px;
            max-width: 700px;
            margin: 0 auto;
            background: #1a1a2e;
            color: #eee;
        }}
        h1 {{ color: #00d4ff; margin-bottom: 5px; }}
        h2 {{
            border-bottom: 1px solid #444;
            padding-bottom: 8px;
            margin-top: 30px;
            color: #aaa;
            font-size: 14px;
            text-transform: uppercase;
        }}
        p {{ color: #888; margin-top: 0; }}
        a {{ color: #00d4ff; }}
        .group {{
            margin: 15px 0;
            display: flex;
            align-items: center;
        }}
        label {{
            display: inline-block;
            width: 140px;
            font-weight: 500;
            color: #ccc;
        }}
        input[type=range] {{
            flex: 1;
            margin: 0 15px;
            accent-color: #00d4ff;
        }}
        .val {{
            display: inline-block;
            width: 70px;
            text-align: right;
            font-family: "SF Mono", Monaco, monospace;
            font-size: 14px;
            color: #00d4ff;
        }}
        .derived {{
            background: #252545;
            padding: 15px;
            border-radius: 8px;
            margin: 20px 0;
        }}
        .derived-item {{
            display: flex;
            justify-content: space-between;
            margin: 8px 0;
        }}
        .derived-label {{
            color: #888;
        }}
        .derived-value {{
            font-family: "SF Mono", Monaco, monospace;
            color: #0f0;
            font-weight: bold;
        }}
        .presets {{
            margin: 20px 0;
        }}
        button {{
            padding: 10px 20px;
            margin: 5px;
            cursor: pointer;
            background: #2a2a4e;
            border: 1px solid #444;
            color: #eee;
            border-radius: 4px;
            font-size: 14px;
        }}
        button:hover {{
            background: #3a3a5e;
            border-color: #00d4ff;
        }}
        .status {{
            position: fixed;
            top: 10px;
            right: 10px;
            padding: 8px 12px;
            border-radius: 4px;
            font-size: 12px;
        }}
        .status.connected {{ background: #1a4a1a; color: #4f4; }}
        .status.disconnected {{ background: #4a1a1a; color: #f44; }}
        select {{
            background: #2a2a4e;
            border: 1px solid #444;
            color: #eee;
            padding: 5px 10px;
            border-radius: 4px;
            font-size: 14px;
        }}
    </style>
</head>
<body>
    <div class="status disconnected" id="status">Disconnected</div>
    <h1>OnSpeed Control Panel</h1>
    <p>Move sliders to control the C++ pipeline. Watch <a href="/live" target="_blank">LiveView</a> for output.</p>

    <h2>Pressure Sensors (Raw Inputs)</h2>
    <div class="group">
        <label>Pfwd (Pa):</label>
        <input type="range" id="Pfwd" min="0" max="3000" step="10" value="1200">
        <span class="val" id="Pfwd-val">1200</span>
    </div>
    <div class="group">
        <label>P45 (Pa):</label>
        <input type="range" id="P45" min="-100" max="300" step="1" value="75">
        <span class="val" id="P45-val">75</span>
    </div>

    <div class="derived">
        <div class="derived-item">
            <span class="derived-label">Coefficient P (P45/Pfwd):</span>
            <span class="derived-value" id="CP-val">0.0625</span>
        </div>
        <div class="derived-item">
            <span class="derived-label">Calculated AOA:</span>
            <span class="derived-value" id="AOA-out">--</span>
        </div>
    </div>

    <h2>Aircraft State</h2>
    <div class="group">
        <label>IAS (kts):</label>
        <input type="range" id="IAS" min="30" max="200" step="1" value="80">
        <span class="val" id="IAS-val">80</span>
    </div>
    <div class="group">
        <label>Altitude (ft):</label>
        <input type="range" id="Palt" min="0" max="15000" step="100" value="3000">
        <span class="val" id="Palt-val">3000</span>
    </div>
    <div class="group">
        <label>Flaps:</label>
        <select id="flapsPos">
            <option value="0">Up (0°)</option>
            <option value="20">20°</option>
            <option value="40">40° (Full)</option>
        </select>
    </div>

    <h2>AHRS / IMU</h2>
    <div class="group">
        <label>Pitch (deg):</label>
        <input type="range" id="Pitch" min="-30" max="30" step="0.5" value="5">
        <span class="val" id="Pitch-val">5.0</span>
    </div>
    <div class="group">
        <label>Roll (deg):</label>
        <input type="range" id="Roll" min="-60" max="60" step="1" value="0">
        <span class="val" id="Roll-val">0</span>
    </div>
    <div class="group">
        <label>Vertical G:</label>
        <input type="range" id="VerticalG" min="-1" max="4" step="0.1" value="1">
        <span class="val" id="VerticalG-val">1.0</span>
    </div>
    <div class="group">
        <label>Lateral G:</label>
        <input type="range" id="LateralG" min="-1" max="1" step="0.05" value="0">
        <span class="val" id="LateralG-val">0.00</span>
    </div>
    <div class="group">
        <label>Flight Path (deg):</label>
        <input type="range" id="FlightPath" min="-15" max="15" step="0.5" value="0">
        <span class="val" id="FlightPath-val">0.0</span>
    </div>

    <h2>Presets</h2>
    <div class="presets">
        <button onclick="setPreset('cruise')">Cruise</button>
        <button onclick="setPreset('approach')">Approach</button>
        <button onclick="setPreset('onspeed')">On Speed</button>
        <button onclick="setPreset('slow')">Slow</button>
        <button onclick="setPreset('stall')">Near Stall</button>
    </div>

    <script>
        const ws = new WebSocket('ws://localhost:{ws_port}');
        const sliderFields = ['Pfwd', 'P45', 'IAS', 'Palt', 'Pitch', 'Roll', 'VerticalG', 'LateralG', 'FlightPath'];
        const statusEl = document.getElementById('status');

        ws.onopen = () => {{
            statusEl.textContent = 'Connected';
            statusEl.className = 'status connected';
            sendUpdate();
        }};

        ws.onclose = () => {{
            statusEl.textContent = 'Disconnected';
            statusEl.className = 'status disconnected';
        }};

        ws.onerror = () => {{
            statusEl.textContent = 'Error';
            statusEl.className = 'status disconnected';
        }};

        // Update AOA display when we receive response from C++
        ws.onmessage = (event) => {{
            try {{
                const data = JSON.parse(event.data);
                if (data.AOA !== undefined) {{
                    document.getElementById('AOA-out').textContent = data.AOA.toFixed(2) + '°';
                }}
            }} catch (e) {{}}
        }};

        sliderFields.forEach(id => {{
            const slider = document.getElementById(id);
            const valEl = document.getElementById(id + '-val');
            slider.oninput = () => {{
                const decimals = slider.step.includes('.') ? (slider.step.split('.')[1] || '').length : 0;
                valEl.textContent = parseFloat(slider.value).toFixed(decimals);
                updateCP();
                sendUpdate();
            }};
        }});

        document.getElementById('flapsPos').onchange = sendUpdate;

        function updateCP() {{
            const pfwd = parseFloat(document.getElementById('Pfwd').value);
            const p45 = parseFloat(document.getElementById('P45').value);
            const cp = pfwd > 0 ? (p45 / pfwd) : 0;
            document.getElementById('CP-val').textContent = cp.toFixed(4);
        }}

        function sendUpdate() {{
            if (ws.readyState !== WebSocket.OPEN) return;
            const data = {{}};
            sliderFields.forEach(id => data[id] = parseFloat(document.getElementById(id).value));
            data.flapsPos = parseInt(document.getElementById('flapsPos').value);
            ws.send(JSON.stringify(data));
        }}

        function setPreset(name) {{
            // Presets use raw pressure values that correspond to typical flight conditions
            // CP = P45/Pfwd maps to AOA via calibration curve
            const presets = {{
                // Cruise: low AOA (~3°), high IAS
                cruise:   {{Pfwd: 2000, P45: 60,  IAS: 140, Palt: 5000, Pitch: 2,  Roll: 0, VerticalG: 1, LateralG: 0, FlightPath: 0, flapsPos: 0}},
                // Approach: moderate AOA (~7°)
                approach: {{Pfwd: 1200, P45: 75,  IAS: 85,  Palt: 1000, Pitch: 5,  Roll: 0, VerticalG: 1, LateralG: 0, FlightPath: -3, flapsPos: 20}},
                // On Speed: target AOA for landing (~8-9°)
                onspeed:  {{Pfwd: 1000, P45: 85,  IAS: 75,  Palt: 500,  Pitch: 7,  Roll: 0, VerticalG: 1, LateralG: 0, FlightPath: -3, flapsPos: 40}},
                // Slow: high AOA (~11°)
                slow:     {{Pfwd: 800,  P45: 90,  IAS: 60,  Palt: 3000, Pitch: 10, Roll: 0, VerticalG: 1, LateralG: 0, FlightPath: -5, flapsPos: 40}},
                // Near Stall: very high AOA (~14°+)
                stall:    {{Pfwd: 600,  P45: 100, IAS: 50,  Palt: 3000, Pitch: 15, Roll: 0, VerticalG: 1, LateralG: 0, FlightPath: -8, flapsPos: 40}},
            }};
            const p = presets[name];
            sliderFields.forEach(k => {{
                if (p[k] !== undefined) {{
                    const slider = document.getElementById(k);
                    slider.value = p[k];
                    const decimals = slider.step.includes('.') ? (slider.step.split('.')[1] || '').length : 0;
                    document.getElementById(k + '-val').textContent = parseFloat(p[k]).toFixed(decimals);
                }}
            }});
            document.getElementById('flapsPos').value = p.flapsPos;
            updateCP();
            sendUpdate();
        }}

        // Initialize CP display
        updateCP();
    </script>
</body>
</html>'''


def patch_websocket_url(html: str, ws_port: int) -> str:
    """Replace ESP32 WebSocket URLs with localhost."""
    return re.sub(
        r'ws://192\.168\.[0-9]+\.[0-9]+:81',
        f'ws://localhost:{ws_port}',
        html
    )


class SimulatedSensorData:
    """Generates simulated flight data for testing the web UI."""

    def __init__(self):
        self.time = 0
        self.ias = 100.0
        self.aoa = 5.0
        self.pitch = 3.0
        self.roll = 0.0
        self.altitude = 3000.0
        self.vsi = 0.0

    def update(self, dt: float = 0.02):
        """Update sensor values with simulated variation."""
        self.time += dt
        self.pitch = 3.0 + 2.0 * math.sin(self.time * 0.3)
        self.roll = 5.0 * math.sin(self.time * 0.2)
        self.ias = 100.0 + 10.0 * math.sin(self.time * 0.1) + random.gauss(0, 0.5)
        self.aoa = 5.0 + 3.0 * math.sin(self.time * 0.15) + random.gauss(0, 0.1)
        self.altitude = 3000.0 + 100.0 * math.sin(self.time * 0.05)
        self.vsi = 500.0 * math.cos(self.time * 0.05)

    def to_json(self) -> str:
        """Generate JSON matching the ESP32 WebSocket format."""
        flight_path = self.pitch - self.aoa
        cp = 0.5 + 0.3 * (self.aoa / 15.0)

        return json.dumps({
            "AOA": round(self.aoa, 2),
            "IAS": round(self.ias, 1),
            "PAlt": round(self.altitude, 0),
            "verticalGLoad": round(1.0 + 0.1 * math.sin(self.time), 2),
            "lateralGLoad": round(0.05 * math.sin(self.time * 2), 2),
            "Pitch": round(self.pitch, 2),
            "Roll": round(self.roll, 2),
            "kalmanVSI": round(self.vsi, 0),
            "flightPath": round(flight_path, 2),
            "PitchRate": round(2.0 * math.cos(self.time * 0.3), 2),
            "dataMark": 0,
            "flapsPos": 0,
            "CP": round(cp, 4),
            "LDmax": 3.0,
            "OnspeedFast": 6.0,
            "OnspeedSlow": 8.0,
            "OnspeedWarn": 12.0,
        })


class CppReplayDataSource:
    """Runs the C++ native-replay program and provides JSON output."""

    def __init__(self, csv_path: Path, program_path: Path, config_path: Path = None):
        self.csv_path = csv_path
        self.program_path = program_path
        self.config_path = config_path
        self.process = None
        self.current_json = None
        self._lock = threading.Lock()
        self._finished = False

    def start(self):
        """Start the C++ process."""
        if not self.program_path.exists():
            raise FileNotFoundError(f"Native replay program not found: {self.program_path}\n"
                                   "Run: pio run -e native-replay")
        if not self.csv_path.exists():
            raise FileNotFoundError(f"CSV file not found: {self.csv_path}")

        print(f"Starting C++ replay: {self.program_path}")
        print(f"  CSV file: {self.csv_path}")
        if self.config_path:
            print(f"  Config: {self.config_path}")

        # Build command with optional config
        cmd = [str(self.program_path)]
        if self.config_path:
            cmd.extend(["--config", str(self.config_path)])
        cmd.append(str(self.csv_path))

        self.process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1  # Line buffered
        )

        # Start a thread to read stderr (status messages)
        threading.Thread(target=self._read_stderr, daemon=True).start()

    def _read_stderr(self):
        """Read stderr from C++ process for status messages."""
        for line in self.process.stderr:
            print(f"  [C++] {line.rstrip()}")

    def read_next(self) -> str | None:
        """Read the next JSON line from C++ stdout."""
        if self.process is None or self._finished:
            return None

        line = self.process.stdout.readline()
        if not line:
            self._finished = True
            print("  [C++] Replay finished")
            return None

        line = line.strip()
        if line.startswith("{"):
            with self._lock:
                self.current_json = line
            return line
        return None

    def is_finished(self) -> bool:
        return self._finished

    def stop(self):
        """Stop the C++ process."""
        if self.process:
            self.process.terminate()
            self.process.wait()


class CppInteractiveDataSource:
    """Runs C++ in interactive mode, accepting JSON input and producing JSON output."""

    def __init__(self, program_path: Path, config_path: Path = None):
        self.program_path = program_path
        self.config_path = config_path
        self.process = None
        self._lock = threading.Lock()

    def start(self):
        """Start the C++ process in interactive mode."""
        if not self.program_path.exists():
            raise FileNotFoundError(f"Native replay program not found: {self.program_path}\n"
                                   "Run: pio run -e native-replay")

        print(f"Starting C++ interactive mode: {self.program_path}")
        if self.config_path:
            print(f"  Config: {self.config_path}")

        # Build command with optional config
        cmd = [str(self.program_path)]
        if self.config_path:
            cmd.extend(["--config", str(self.config_path)])
        cmd.append("--interactive")

        self.process = subprocess.Popen(
            cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1  # Line buffered
        )

        # Start a thread to read stderr (status messages)
        threading.Thread(target=self._read_stderr, daemon=True).start()

    def _read_stderr(self):
        """Read stderr from C++ process for status messages."""
        for line in self.process.stderr:
            print(f"  [C++] {line.rstrip()}")

    def send_and_receive(self, json_input: str) -> str | None:
        """Send JSON to C++ and receive processed JSON output."""
        if self.process is None:
            return None

        with self._lock:
            try:
                self.process.stdin.write(json_input + "\n")
                self.process.stdin.flush()
                output = self.process.stdout.readline()
                return output.strip() if output else None
            except (BrokenPipeError, OSError):
                return None

    def stop(self):
        """Stop the C++ process."""
        if self.process:
            self.process.terminate()
            self.process.wait()


async def websocket_handler_simulated(websocket, sensor_data: SimulatedSensorData):
    """Handle WebSocket connections with simulated sensor data."""
    print(f"WebSocket client connected: {websocket.remote_address}")
    try:
        while True:
            sensor_data.update()
            await websocket.send(sensor_data.to_json())
            await asyncio.sleep(0.02)  # 50Hz update rate
    except websockets.exceptions.ConnectionClosed:
        print(f"WebSocket client disconnected")


async def websocket_handler_replay(websocket, cpp_source: CppReplayDataSource):
    """Handle WebSocket connections with C++ replay data."""
    print(f"WebSocket client connected: {websocket.remote_address}")
    try:
        while not cpp_source.is_finished():
            json_line = cpp_source.read_next()
            if json_line:
                await websocket.send(json_line)
            else:
                # No data yet or finished, wait a bit
                await asyncio.sleep(0.001)
    except websockets.exceptions.ConnectionClosed:
        print(f"WebSocket client disconnected")


async def run_websocket_server_simulated(port: int, sensor_data: SimulatedSensorData):
    """Run the WebSocket server with simulated data."""
    async with websockets.serve(
        lambda ws: websocket_handler_simulated(ws, sensor_data),
        "localhost",
        port,
        reuse_address=True
    ):
        print(f"WebSocket server running on ws://localhost:{port}")
        await asyncio.Future()


async def run_websocket_server_replay(port: int, cpp_source: CppReplayDataSource):
    """Run the WebSocket server with C++ replay data."""
    # Start the C++ process
    cpp_source.start()

    async with websockets.serve(
        lambda ws: websocket_handler_replay(ws, cpp_source),
        "localhost",
        port,
        reuse_address=True
    ):
        print(f"WebSocket server running on ws://localhost:{port}")
        await asyncio.Future()


# Store connected clients for broadcasting in interactive mode
interactive_clients: set = set()


async def websocket_handler_interactive(websocket, cpp_source: CppInteractiveDataSource):
    """Handle WebSocket connections for interactive mode (bidirectional)."""
    print(f"WebSocket client connected: {websocket.remote_address}")
    interactive_clients.add(websocket)

    try:
        async for message in websocket:
            # Receive JSON from control panel, send to C++, broadcast response
            if message.startswith("{"):
                response = cpp_source.send_and_receive(message)
                if response:
                    # Broadcast to all connected clients (LiveView + Control Panel)
                    disconnected = set()
                    for client in interactive_clients:
                        try:
                            await client.send(response)
                        except websockets.exceptions.ConnectionClosed:
                            disconnected.add(client)
                    interactive_clients.difference_update(disconnected)
    except websockets.exceptions.ConnectionClosed:
        pass
    finally:
        interactive_clients.discard(websocket)
        print(f"WebSocket client disconnected")


async def run_websocket_server_interactive(port: int, cpp_source: CppInteractiveDataSource):
    """Run the WebSocket server in interactive mode."""
    # Start the C++ process
    cpp_source.start()

    async with websockets.serve(
        lambda ws: websocket_handler_interactive(ws, cpp_source),
        "localhost",
        port,
        reuse_address=True
    ):
        print(f"WebSocket server running on ws://localhost:{port}")
        await asyncio.Future()


class OnSpeedHTTPHandler(http.server.BaseHTTPRequestHandler):
    """HTTP handler for serving OnSpeed web pages."""

    web_dir = None
    ws_port = None

    def log_message(self, format, *args):
        """Suppress default logging."""
        pass

    def do_GET(self):
        if self.path == "/" or self.path == "/live":
            self.send_page("live")
        elif self.path == "/calwiz" or self.path.startswith("/calwiz?"):
            self.send_page("calibration")
        elif self.path == "/control":
            self.send_page("control")
        else:
            self.send_error(404, "Page not found")

    def send_page(self, page_type: str):
        html = build_html_page(self.web_dir, page_type, self.ws_port)
        html = patch_websocket_url(html, self.ws_port)

        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.send_header("Content-Length", len(html.encode()))
        self.end_headers()
        self.wfile.write(html.encode())
        print(f"  GET {self.path} -> {page_type}")


class ReusableTCPServer(socketserver.TCPServer):
    """TCP server that allows address reuse to avoid 'Address already in use' errors."""
    allow_reuse_address = True


def run_http_server(port: int, web_dir: Path, ws_port: int, show_control: bool = False):
    """Run the HTTP server."""
    OnSpeedHTTPHandler.web_dir = web_dir
    OnSpeedHTTPHandler.ws_port = ws_port

    with ReusableTCPServer(("", port), OnSpeedHTTPHandler) as httpd:
        print(f"HTTP server running on http://localhost:{port}")
        print(f"  - LiveView:    http://localhost:{port}/live")
        print(f"  - Calibration: http://localhost:{port}/calwiz")
        if show_control:
            print(f"  - Control:     http://localhost:{port}/control")
        httpd.serve_forever()


def main():
    parser = argparse.ArgumentParser(
        description="OnSpeed Web UI Development Server",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --simulate                     # Simulated flight data
  %(prog)s --replay path/to/flight.csv    # Replay through C++ pipeline
  %(prog)s --interactive                  # Control panel with sliders
"""
    )
    parser.add_argument("--port", "-p", type=int, default=8080,
                        help="HTTP port (default: 8080)")
    parser.add_argument("--ws-port", "-w", type=int, default=8081,
                        help="WebSocket port (default: 8081)")
    parser.add_argument("--config", "-c", type=Path, metavar="CFG",
                        help="Config file (.cfg) for C++ calibration data")

    # Data source modes (mutually exclusive)
    mode_group = parser.add_mutually_exclusive_group()
    mode_group.add_argument("--replay", "-r", type=Path, metavar="CSV",
                            help="Replay CSV file through C++ pipeline")
    mode_group.add_argument("--simulate", "-s", action="store_true",
                            help="Use simulated sensor data (default)")
    mode_group.add_argument("--interactive", "-i", action="store_true",
                            help="Interactive mode with control panel sliders")

    args = parser.parse_args()

    # Find the Web directory (contains the .h files)
    script_dir = Path(__file__).parent
    esp32_dir = script_dir.parent
    web_dir = esp32_dir / "src" / "Web"  # After PlatformIO restructure

    if not web_dir.exists():
        print(f"Error: Web directory not found at {web_dir}")
        sys.exit(1)

    # Find the native-replay program
    program_path = esp32_dir / ".pio" / "build" / "native-replay" / "program"

    print("=" * 60)
    print("OnSpeed Development Server")
    print("=" * 60)
    print(f"Web content: {web_dir}")

    # Resolve config path if provided
    config_path = None
    if args.config:
        config_path = args.config.resolve()
        if not config_path.exists():
            print(f"Error: Config file not found: {config_path}")
            sys.exit(1)

    # Determine mode and create data source
    mode = "simulated"
    cpp_source = None
    show_control = False

    if args.replay:
        # Resolve CSV path relative to current directory or absolute
        csv_path = args.replay.resolve()
        if not csv_path.exists():
            print(f"Error: CSV file not found: {csv_path}")
            sys.exit(1)
        if not program_path.exists():
            print(f"Error: Native replay program not found: {program_path}")
            print("Run: pio run -e native-replay")
            sys.exit(1)

        print(f"Mode: C++ Replay")
        print(f"  CSV: {csv_path}")
        print(f"  Program: {program_path}")
        if config_path:
            print(f"  Config: {config_path}")
        cpp_source = CppReplayDataSource(csv_path, program_path, config_path)
        mode = "replay"

    elif args.interactive:
        if not program_path.exists():
            print(f"Error: Native replay program not found: {program_path}")
            print("Run: pio run -e native-replay")
            sys.exit(1)

        print(f"Mode: Interactive (control panel)")
        print(f"  Program: {program_path}")
        if config_path:
            print(f"  Config: {config_path}")
        cpp_source = CppInteractiveDataSource(program_path, config_path)
        mode = "interactive"
        show_control = True

    else:
        # Default to simulated mode
        print(f"Mode: Simulated sensor data")

    print()

    # Start HTTP server in a thread
    http_thread = threading.Thread(
        target=run_http_server,
        args=(args.port, web_dir, args.ws_port, show_control),
        daemon=True
    )
    http_thread.start()

    # Run WebSocket server
    try:
        if mode == "replay":
            asyncio.run(run_websocket_server_replay(args.ws_port, cpp_source))
        elif mode == "interactive":
            asyncio.run(run_websocket_server_interactive(args.ws_port, cpp_source))
        else:
            sensor_data = SimulatedSensorData()
            asyncio.run(run_websocket_server_simulated(args.ws_port, sensor_data))
    except KeyboardInterrupt:
        print("\nShutting down...")
        if cpp_source:
            cpp_source.stop()



if __name__ == "__main__":
    main()
