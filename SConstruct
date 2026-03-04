#!/usr/bin/env python
import os
import sys

# ──────────────────────────────────────────────
# Import the environment from godot-cpp (standard & reliable way)
# ──────────────────────────────────────────────
env = SConscript("godot-cpp/SConstruct")

# Optional: Cache builds to speed up recompiles
CacheDir(".scons_cache/")

# ──────────────────────────────────────────────
# Project-specific configuration
# ──────────────────────────────────────────────

# Add your source include path
env.Append(CPPPATH=["src/"])

# Enable C++ exceptions (recommended)
env.Append(CXXFLAGS=["-fexceptions"])

# ── Bluetooth / sdbus-c++ dependencies ───────────────
# On Pop!_OS/Ubuntu/Debian, try pkg-config first (cleaner)
try:
    env.ParseConfig("pkg-config --cflags --libs sdbus-c++")
except OSError:
    # Fallback if pkg-config not available or fails
    print("Warning: pkg-config for sdbus-c++ failed → using manual paths")
    env.Append(
        CPPPATH=["/usr/include/sdbus-c++"],
        LIBPATH=["/usr/lib", "/usr/lib/x86_64-linux-gnu", "/usr/lib64", "/usr/local/lib"],
        LIBS=["sdbus-c++"]
    )

# Optional dev flags
# env.Append(CPPDEFINES=["DEBUG_BLE=1"])
# env.Append(CXXFLAGS=["-Wall", "-Wextra"])

# ──────────────────────────────────────────────
# Source files
# ──────────────────────────────────────────────
sources = Glob("src/*.cpp")

# ──────────────────────────────────────────────
# Output library
# ──────────────────────────────────────────────
lib_name_base = "ble_gdextension"

if env["platform"] == "macos":
    library = env.SharedLibrary(
        f"bin/lib{lib_name_base}.{env['platform']}.{env['target']}.framework/lib{lib_name_base}.{env['platform']}.{env['target']}",
        source=sources,
    )
elif env["platform"] == "ios":
    if env.get("ios_simulator", False):
        library = env.StaticLibrary(
            f"bin/lib{lib_name_base}.{env['platform']}.{env['target']}.simulator.a",
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            f"bin/lib{lib_name_base}.{env['platform']}.{env['target']}.a",
            source=sources,
        )
else:
    # Linux/Windows/Android – suffix is .editor / .template_debug / .template_release
    suffix = env["suffix"]
    library = env.SharedLibrary(
        f"bin/lib{lib_name_base}{suffix}{env['SHLIBSUFFIX']}",
        source=sources,
    )

Default(library)
