#!/usr/bin/env python3
"""
Plot a chamber log (SD card YYMMDD.CSV or serial_logger.py output) and
report how well temperature and humidity stayed within tolerance.

    pip install matplotlib
    python plot_log.py run1.csv
    python plot_log.py 261014.CSV --png chart.png --t-tol 1 --rh-tol 2
"""
import argparse
import csv
import datetime as dt
import sys


def parse_time(row, index):
    for key in ("timestamp", "pc_time"):
        v = row.get(key, "")
        try:
            return dt.datetime.fromisoformat(v.replace(" ", "T"))
        except ValueError:
            pass
    ts = row.get("timestamp", "")
    return float(ts) / 3600.0 if ts.replace(".", "", 1).isdigit() else index


def load(path):
    rows = []
    with open(path, newline="", encoding="utf-8") as f:
        for i, r in enumerate(csv.DictReader(f)):
            try:
                rows.append({
                    "time": parse_time(r, i),
                    "t": float(r["t_C"]), "rh": float(r["rh_pct"]),
                    "t_set": float(r["t_set_C"]), "rh_set": float(r["rh_set_pct"]),
                    "heater": float(r["heater_pct"]), "humid": float(r["humid_pct"]),
                    "exhaust": float(r["exhaust_pct"]), "stable": r.get("stable", "0") == "1",
                })
            except (KeyError, ValueError):
                continue        # skip NA rows (sensor failure) and stray lines
    return rows


def report(rows, t_tol, rh_tol):
    stable = [r for r in rows if r["stable"]] or rows
    n = len(stable)
    dts = [abs(r["t"] - r["t_set"]) for r in stable]
    dhs = [abs(r["rh"] - r["rh_set"]) for r in stable]
    inb = sum(1 for a, b in zip(dts, dhs) if a <= t_tol and b <= rh_tol)
    label = "stable period" if any(r["stable"] for r in rows) else "whole log"
    print(f"records: {len(rows)}  ({label}: {n})")
    print(f"max |dT|  = {max(dts):.2f} C   (tolerance {t_tol})")
    print(f"max |dRH| = {max(dhs):.2f} %   (tolerance {rh_tol})")
    print(f"in tolerance: {100.0 * inb / n:.1f} % of records")
    return inb == n


def plot(rows, t_tol, rh_tol, png):
    import matplotlib.pyplot as plt

    x = [r["time"] for r in rows]
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharex=True, figsize=(11, 8))

    for ax, key, sp, tol, unit in ((ax1, "t", "t_set", t_tol, "°C"), (ax2, "rh", "rh_set", rh_tol, "%RH")):
        spv = [r[sp] for r in rows]
        ax.fill_between(x, [s - tol for s in spv], [s + tol for s in spv],
                        step="post", alpha=0.18, label=f"set-point ±{tol}")
        ax.plot(x, spv, "--", lw=1, label="set-point")
        ax.plot(x, [r[key] for r in rows], lw=1.4, label="measured")
        ax.set_ylabel(unit)
        ax.grid(alpha=0.3)
        ax.legend(loc="upper right", fontsize=8)
    ax1.set_title("SCC chamber: temperature and humidity")

    ax3.plot(x, [r["heater"] for r in rows], lw=1, label="heater %")
    ax3.plot(x, [r["humid"] for r in rows], lw=1, label="humidifier %")
    ax3.plot(x, [r["exhaust"] for r in rows], lw=1, label="exhaust %")
    ax3.set_ylabel("output %")
    ax3.set_ylim(-5, 105)
    ax3.grid(alpha=0.3)
    ax3.legend(loc="upper right", fontsize=8)
    if not isinstance(x[0], dt.datetime):
        ax3.set_xlabel("hours" if isinstance(x[0], float) else "record #")

    fig.tight_layout()
    if png:
        fig.savefig(png, dpi=130)
        print(f"saved {png}")
    else:
        plt.show()


def main():
    ap = argparse.ArgumentParser(description="Plot SCC chamber log")
    ap.add_argument("csv")
    ap.add_argument("--png", help="save chart to this file instead of showing it")
    ap.add_argument("--t-tol", type=float, default=1.0)
    ap.add_argument("--rh-tol", type=float, default=2.0)
    ap.add_argument("--no-plot", action="store_true")
    args = ap.parse_args()

    rows = load(args.csv)
    if not rows:
        sys.exit("no data rows found")
    ok = report(rows, args.t_tol, args.rh_tol)
    if not args.no_plot:
        plot(rows, args.t_tol, args.rh_tol, args.png)
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
