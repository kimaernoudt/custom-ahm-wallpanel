#!/bin/sh
# Builds the panel firmware for this computer (Arduino APIs replaced by stand-ins in this folder)
# and runs it against the fake AHM: turns knobs, presses buttons, saves settings.
set -e
cd "$(dirname "$0")"
g++ -std=gnu++17 -Wall -I. -x c++ test_main.cpp -o /tmp/panel_test -lpthread
python3 ../ahm_emulator.py --port 51390 & EMU=$!
sleep 1
/tmp/panel_test 51390
kill $EMU
# same again, built for the panel PCB (PANEL_PCB=1)
g++ -std=gnu++17 -Wall -DPANEL_PCB=1 -I. -x c++ test_main.cpp -o /tmp/panel_test_pcb -lpthread
python3 ../ahm_emulator.py --port 51391 & EMU=$!
sleep 1
/tmp/panel_test_pcb 51391
kill $EMU
