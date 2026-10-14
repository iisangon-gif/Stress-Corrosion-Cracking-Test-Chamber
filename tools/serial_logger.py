#!/usr/bin/env python3
"""
Record the chamber's CSV output from USB serial into a file on the PC.

    pip install pyserial
    python serial_logger.py --port COM3                 # Windows
    python serial_logger.py --port /dev/ttyACM0         # Linux / macOS
    python serial_logger.py --port COM3 --out run1.csv

Anything typed + Enter is sent to the Arduino (e.g. STATUS, SET T 35).
Lines starting with '#' are status messages and are shown but not saved.
"""
import argparse
import datetime as dt
import sys
import threading

import serial


def reader(ser, out_path):
    with open(out_path, "a", encoding="utf-8", newline="") as f:
        has_header = f.tell() > 0
        while True:
            raw = ser.readline()
            if not raw:
                continue
            line = raw.decode("utf-8", errors="replace").strip()
            if not line:
                continue
            print(line)
            if line.startswith("#"):
                continue
            if line.startswith("timestamp,"):
                if not has_header:
                    f.write("pc_time," + line + "\n")
                    has_header = True
                continue
            if has_header:
                f.write(dt.datetime.now().isoformat(timespec="seconds") + "," + line + "\n")
                f.flush()


def main():
    ap = argparse.ArgumentParser(description="SCC chamber serial logger")
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--out", default=None, help="CSV file to append to")
    args = ap.parse_args()

    out = args.out or dt.datetime.now().strftime("chamber_%Y%m%d_%H%M%S.csv")

    # Keep DTR low while opening so the Arduino is not reset by the connection.
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = args.port, args.baud, 1
    ser.dtr = False
    ser.open()
    ser.write(b"HEADER\n")
    print(f"# logging to {out} (Ctrl+C to quit)")

    threading.Thread(target=reader, args=(ser, out), daemon=True).start()
    try:
        for cmd in sys.stdin:
            ser.write((cmd.strip() + "\n").encode())
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
