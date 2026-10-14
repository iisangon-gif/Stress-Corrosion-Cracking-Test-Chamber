#!/bin/bash
# Build the firmware for the host and run the UI test plus the closed-loop
# chamber simulation over several chamber variants.  Exit code != 0 on failure.
set -e
cd "$(dirname "$0")"
mkdir -p build
CXX=${CXX:-g++}
$CXX -std=c++17 -O2 -Istubs -x c++ ui_test.cpp      -o build/ui_test
$CXX -std=c++17 -O2 -Istubs -x c++ chamber_sim.cpp  -o build/chamber_sim
$CXX -std=c++17 -O2 -Istubs -DMIST_AC_SIDE_SWITCH=1 -x c++ chamber_sim.cpp -o build/chamber_sim_ac

echo "== UI test"
./build/ui_test

fail=0
while read -r name vars; do
  [ -z "$name" ] && continue
  echo "== $name ($vars)"
  # "ac_side" in the name: firmware built for mist heads switched on the 230 V
  # side of their USB adapter (MIST_AC_SIDE_SWITCH 1, adapter start-up 0.5 s)
  bin=./build/chamber_sim
  if [[ $name == *ac_side* ]]; then bin=./build/chamber_sim_ac; vars="MD=0.5 $vars"; fi
  if [[ $name == info:* ]]; then
    env $vars $bin || echo "(informational - not counted)"
  else
    env $vars $bin || fail=1
  fi
done <<'SCENARIOS'
nominal_200L_insulated           TA=25 RA=0.6
uninsulated_300W                 UA=6.6 PW=300
uninsulated_200W                 UA=6.6
heavy_contents                   CAP=15000
light_contents                   CAP=3000
weak_heater_fan                  GW=4
poor_mixing_slow_sensor          TAU=30
strong_mist                      HR=0.35
weak_mist                        HR=0.1
room_temperature_mode            T1=ROOM H1=75 T2=ROOM H2=40 TA=25 RA=0.3
low_limit_20C                    T1=20 H1=80 T2=20 H2=50 TA=17 RA=0.5
low_rh_20pct_dry_room            T1=30 H1=50 T2=40 H2=20 TA=25 RA=0.35
humid_thai_room_80pct            T1=35 H1=75 T2=40 H2=80 TA=30 RA=0.7
small_heater_100W                PW=100
ac_side_mist                     HR=0.19
SCENARIOS
exit $fail
