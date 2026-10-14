#!/bin/bash
# Build the firmware for the host and run the UI test plus the closed-loop
# chamber simulation over several chamber variants.  Exit code != 0 on failure.
set -e
cd "$(dirname "$0")"
mkdir -p build
CXX=${CXX:-g++}
$CXX -std=c++17 -O2 -Istubs -x c++ ui_test.cpp      -o build/ui_test
$CXX -std=c++17 -O2 -Istubs -x c++ chamber_sim.cpp  -o build/chamber_sim

echo "== UI test"
./build/ui_test

fail=0
while read -r name vars; do
  [ -z "$name" ] && continue
  echo "== $name ($vars)"
  env $vars ./build/chamber_sim || fail=1
done <<'SCENARIOS'
nominal
strong_humidifier_slow_sensor   HR=0.6 TAU=20
very_strong_humidifier          HR=1.0 TAU=30
weak_humidifier_fast_sensor     HR=0.15 TAU=5
low_thermal_mass                CAP=1000 PW=60 UA=1.5
tiny_box_big_heater             CAP=500 PW=100 HR=0.8 TAU=25 UA=2
large_thermal_mass              CAP=6000 PW=50
cold_dry_room                   HR=0.3 TAU=15 TA=20 RA=0.4
warm_humid_room                 TA=30 RA=0.75
SCENARIOS
exit $fail
