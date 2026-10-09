#!/bin/sh
# Build and run the host-side tests (UI, settings screens, grind by weight)
# with a normal C++ compiler. No board or PlatformIO needed.
set -e
cd "$(dirname "$0")"
ROOT=../..
# Fixed test values for the setup-specific defaults (config.h), and no
# config_local.h, so the checks don't depend on anyone's own settings
DEFS="-DGRINDERBOT_NO_LOCAL_CONFIG -DDOSE1_DEFAULT_DG=90 -DDOSE2_DEFAULT_DG=180 -DGRIND_MAX_DEFAULT_S=60"
${CXX:-g++} -std=gnu++11 -Wall -Wextra -O1 -g $DEFS -Istubs -I$ROOT/include \
  test.cpp \
  $ROOT/src/ui.cpp $ROOT/src/grind.cpp $ROOT/src/grinderservo.cpp \
  $ROOT/src/panelcal.cpp $ROOT/src/servosetup.cpp $ROOT/src/valueedit.cpp \
  -o host_test
./host_test
