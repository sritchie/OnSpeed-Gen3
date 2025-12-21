/*
 * Native build main entry point
 *
 * This is the main() function for running OnSpeed locally (no ESP32).
 * It can replay CSV flight logs or accept interactive input.
 *
 * Usage:
 *   ./program path/to/log.csv     # Replay CSV file
 *   ./program --interactive       # Read from stdin (for dev server control panel)
 */

#ifdef NATIVE_BUILD

#include <iostream>
#include <sstream>
#include <string>
#include "native_compat.h"
#include "LogReplay.h"

// Simple JSON value parser (avoids adding library dependency)
static float getJsonFloat(const std::string& json, const char* key, float defaultVal = 0.0f) {
    std::string search = std::string("\"") + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return defaultVal;
    pos += search.length();
    // Skip whitespace
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    try {
        return std::stof(json.substr(pos));
    } catch (...) {
        return defaultVal;
    }
}

// Interactive mode: reads JSON from stdin, processes, outputs JSON to stdout
// Pattern follows TestPotTask from LogReplay.cpp
//
// Accepts two input modes:
// 1. Raw pressures: {"Pfwd":1200,"P45":60,...} - AOA calculated from calibration curve
// 2. Direct AOA: {"AOA":8.0,...} - AOA used directly (fallback)
void StdinInputTask() {
    std::string line;

    std::cerr << "Interactive mode: reading JSON from stdin" << std::endl;
    std::cerr << "Send JSON like: {\"Pfwd\":1200,\"P45\":60,\"IAS\":80,\"Pitch\":5}" << std::endl;
    std::cerr << "  or direct AOA: {\"AOA\":8.0,\"IAS\":80,\"Pitch\":5}" << std::endl;

    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        if (line[0] != '{') continue;  // Skip non-JSON lines

        // Parse JSON input from control panel
        // Pattern matches TestPotTask: set globals, call UpdateTones(), output JSON

        // Set flap position first (affects which calibration curve to use)
        int flapsPos = static_cast<int>(getJsonFloat(line, "flapsPos", 0));
        g_Flaps.iPosition = flapsPos;
        g_Flaps.iIndex = 0;  // Default to flaps up config

        // Find matching flap config
        for (size_t i = 0; i < g_Config.aFlaps.size(); i++) {
            if (g_Flaps.iPosition == g_Config.aFlaps[i].iDegrees) {
                g_Flaps.iIndex = static_cast<int>(i);
                break;
            }
        }

        // Check if we have raw pressure values (preferred) or direct AOA (fallback)
        float pfwd = getJsonFloat(line, "Pfwd", -1.0f);
        float p45 = getJsonFloat(line, "P45", -1.0f);

        if (pfwd >= 0 && p45 >= -100) {
            // Raw pressure mode: calculate AOA from pressures using calibration curve
            g_Sensors.PfwdSmoothed = pfwd;
            g_Sensors.P45Smoothed = p45;
            g_Sensors.AOA = CalcAOA(pfwd, p45, g_Flaps.iIndex, g_Config.iAoaSmoothing);
        } else {
            // Direct AOA mode (fallback for simple testing)
            g_Sensors.AOA = getJsonFloat(line, "AOA", g_Sensors.AOA);
        }

        // Other sensor values
        g_Sensors.IAS         = getJsonFloat(line, "IAS", g_Sensors.IAS);
        g_Sensors.Palt        = getJsonFloat(line, "Palt", g_Sensors.Palt);
        g_AHRS.SmoothedPitch  = getJsonFloat(line, "Pitch", g_AHRS.SmoothedPitch);
        g_AHRS.SmoothedRoll   = getJsonFloat(line, "Roll", g_AHRS.SmoothedRoll);
        g_pIMU->Az            = getJsonFloat(line, "VerticalG", g_pIMU->Az);
        g_pIMU->Ay            = getJsonFloat(line, "LateralG", g_pIMU->Ay);
        g_pIMU->Ax            = getJsonFloat(line, "ForwardG", g_pIMU->Ax);
        g_AHRS.gPitch         = getJsonFloat(line, "PitchRate", g_AHRS.gPitch);
        g_AHRS.FlightPath     = getJsonFloat(line, "FlightPath", g_AHRS.FlightPath);

        // Run the audio processing (same as TestPotTask)
        g_AudioPlay.UpdateTones();

        // Update derived values
        g_AHRS.AccelLatCorr = g_pIMU->Ay;
        g_AHRS.AccelVertCorr = g_pIMU->Az;

        // Output processed data as JSON
        std::cout << UpdateLiveDataJson() << std::endl;
        std::cout.flush();
    }
}

void printUsage(const char* prog) {
    std::cerr << "OnSpeed Native Build" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Usage:" << std::endl;
    std::cerr << "  " << prog << " [--config FILE] <csv_file>      # Replay CSV file" << std::endl;
    std::cerr << "  " << prog << " [--config FILE] --interactive   # Read JSON from stdin" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --config, -c FILE   Load calibration from .cfg file" << std::endl;
    std::cerr << "  --interactive, -i   Read sensor JSON from stdin" << std::endl;
    std::cerr << std::endl;
    std::cerr << "Examples:" << std::endl;
    std::cerr << "  " << prog << " path/to/log.csv" << std::endl;
    std::cerr << "  " << prog << " --config onspeed.cfg path/to/log.csv" << std::endl;
    std::cerr << "  echo '{\"Pfwd\":1200,\"P45\":60}' | " << prog << " -c onspeed.cfg -i" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string configPath;
    std::string csvPath;
    bool interactive = false;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--config" || arg == "-c") {
            if (i + 1 < argc) {
                configPath = argv[++i];
            } else {
                std::cerr << "Error: --config requires a file path" << std::endl;
                return 1;
            }
        } else if (arg == "--interactive" || arg == "-i") {
            interactive = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] != '-') {
            csvPath = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Load config file if specified
    if (!configPath.empty()) {
        if (!g_Config.loadFromFile(configPath)) {
            std::cerr << "Warning: Could not load config, using defaults" << std::endl;
        }
    } else {
        std::cerr << "  [Config] Using default calibration (no --config specified)" << std::endl;
    }

    if (interactive) {
        // Interactive mode: read JSON from stdin
        StdinInputTask();
    } else if (!csvPath.empty()) {
        // Replay mode: treat arg as CSV file path
        std::cerr << "Replaying: " << csvPath << std::endl;

        SuParamsReplay params;
        params.sReplayLogFile = csvPath;

        LogReplayTask(&params);
    } else {
        std::cerr << "Error: No CSV file or --interactive mode specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}

#endif // NATIVE_BUILD
