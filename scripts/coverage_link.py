"""
PlatformIO extra script to add coverage linking flags.

This script adds -lgcov to the linker flags for the native-coverage environment,
enabling code coverage analysis with gcov/lcov.
"""
Import("env")

# Add coverage library to linker
env.Append(LINKFLAGS=["--coverage"])