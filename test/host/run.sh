#!/bin/sh
# Build and run the host-side tests (UI, settings screens, grind by weight)
# with a normal C++ compiler. No board or PlatformIO needed.
set -e
cd "$(dirname "$0")"
ROOT=../..
${CXX:-g++} -std=gnu++11 -Wall -Wextra -O1 -g -Istubs -I$ROOT/include \
  test.cpp \
  $ROOT/src/ui.cpp $ROOT/src/grind.cpp $ROOT/src/grinderservo.cpp \
  $ROOT/src/panelcal.cpp $ROOT/src/servosetup.cpp $ROOT/src/valueedit.cpp \
  -o host_test
./host_test
