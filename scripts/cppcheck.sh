#!/bin/bash
#
# Run cppcheck static analysis on project source files
#
# Usage: ./scripts/cppcheck.sh [--strict]
#   --strict: Exit with error code on errors (not warnings/style)
#
# Install cppcheck:
#   macOS:  brew install cppcheck
#   Ubuntu: sudo apt-get install cppcheck
#

set -e

# Find repo root (directory containing this script's parent)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
SRC_DIR="$REPO_ROOT/software/OnSpeed-Gen3-ESP32"

# Check if cppcheck is installed
if ! command -v cppcheck &> /dev/null; then
    echo "Error: cppcheck not found. Install with:"
    echo "  macOS:  brew install cppcheck"
    echo "  Ubuntu: sudo apt-get install cppcheck"
    exit 1
fi

# Parse arguments
STRICT_MODE=0
if [[ "$1" == "--strict" ]]; then
    STRICT_MODE=1
fi

echo "Running cppcheck on $SRC_DIR..."
echo ""

# Run cppcheck
# --enable=warning,style,performance,portability: Enable useful checks
# --suppress=missingIncludeSystem: Don't warn about missing system headers
# --suppress=unusedFunction: Don't warn about unused functions (common in embedded)
# --suppress=uninitMemberVar: Constructor initialization is often done elsewhere in embedded
# --suppress=noExplicitConstructor: Single-arg constructors without explicit are common
# --inline-suppr: Allow inline suppression comments
# -I: Include path for project headers

CPPCHECK_ARGS=(
    --language=c++
    --std=c++20
    --enable=warning,style,performance,portability
    --suppress=missingIncludeSystem
    --suppress=unusedFunction
    --suppress=uninitMemberVar
    --suppress=noExplicitConstructor
    --suppress=unusedStructMember
    --inline-suppr
    -I "$SRC_DIR"
)

if [[ $STRICT_MODE -eq 1 ]]; then
    # Only fail on actual errors, not warnings/style
    CPPCHECK_ARGS+=(--error-exitcode=1)
fi

cppcheck "${CPPCHECK_ARGS[@]}" \
    "$SRC_DIR"/*.cpp \
    "$SRC_DIR"/*.h

echo ""
echo "cppcheck complete."
