#!/bin/bash
# Build the firmware for the host and run the UI test plus the closed-loop
# chamber simulation over several chamber variants.  Exit code != 0 on failure.
set -e
cd "$(dirname "$0")"
mkdir -p build
CXX=${CXX:-g++}
$CXX -std=c++17 -O2 -Istubs -x c++ ui_test.cpp      -o build/ui_test
$CXX -std=c++17 -O2 -Istubs -x c++ chamber_sim.cpp  -o build/chamber_sim
$CXX -std=c++17 -O2 -Istubs -DMIST_SWITCH_SSR=1 -x c++ chamber_sim.cpp -o build/chamber_sim_ssr

echo "== UI test"
./build/ui_test

fail=0
while read -r name vars; do
  [ -z "$name" ] && continue
  echo "== $name ($vars)"
  # "ssr" in the name: firmware built for mist heads switched by an SSR on the
  # 230 V side of their USB adapter (MIST_SWITCH_SSR 1, adapter start-up 0.5 s)
  bin=./build/chamber_sim
  if [[ $name == *ssr* ]]; then bin=./build/chamber_sim_ssr; vars="MD=0.5 $vars"; fi
  if [[ $name == info:* ]]; then
    env $vars $bin || echo "(informational - not counted)"
  else
    env $vars $bin || fail=1
  fi
done <<'SCENARIOS'
nominal_4_heads_50L              TA=25 RA=0.6
room_temperature_mode            T1=ROOM H1=75 T2=ROOM H2=40 TA=25 RA=0.3
low_limit_20C                    T1=20 H1=80 T2=20 H2=50 TA=17 RA=0.5
low_rh_20pct_dry_room            T1=30 H1=50 T2=40 H2=20 TA=25 RA=0.35
humid_thai_room                  T1=35 H1=75 T2=40 H2=60 TA=30 RA=0.7
weak_mist_big_box                HR=0.3 TAU=5
very_strong_mist_slow_sensor     HR=1.2 TAU=30
strong_mist_room_temperature     T1=ROOM H1=80 T2=ROOM H2=30 HR=1.2 TAU=15 RA=0.25
low_thermal_mass                 CAP=1000 UA=1.5
tiny_box                         CAP=500 CW=400 HR=1.2 TAU=25 UA=2
large_thermal_mass               CAP=6000 CW=1500 PW=150
small_heater                     PW=50
ssr_mist_moderate                HR=0.5
info:ssr_mist_strong             T1=ROOM H1=75 T2=ROOM H2=40 HR=1.0 TAU=15 RA=0.3
info:thick_wall_slow_heater      CW=2000 GW=2
SCENARIOS
exit $fail
