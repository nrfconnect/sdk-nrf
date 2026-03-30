#!/usr/bin/env python3

# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
nrf_wifi_zperf_raw_tx — tshark-based throughput for zperf raw TX tests.

Read packets from a live interface or from a PCAP file and print estimated
throughput per second from the sum of frame.len for frames matching TCP/UDP
port (default iperf2: 5001). Rates use SI megabits per second (Mbps, divide by
10^6), consistent with iperf2-style Mbits/sec reporting. For live capture, lines
print as each second completes (flush). For PCAP, output appears as the file is
processed. Uses L2 frame sizes; Wireshark may label traffic iPerf2 or malformed
for zperf/raw tests — this tool only aggregates lengths. Use -d/--duration or
-T/--timeout with live capture to stop after SEC s (also limits how much of a
PCAP is read).
"""

import argparse
import os
import signal
import subprocess
import sys

# SI megabits per second (same scaling as typical iperf2 Mbits/sec output).
_BITS_PER_MBPS = 1_000_000.0


def parse_args(argv=None):
    p = argparse.ArgumentParser(
        description=__doc__.strip(),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    p.add_argument(
        "-i",
        "--interface",
        default="mon0",
        metavar="IF",
        help="Live capture interface (default: %(default)s; not used with -r)",
    )
    p.add_argument(
        "-r",
        "--read",
        "--pcap",
        dest="pcap",
        metavar="FILE",
        default=None,
        help="Read packets from PCAP/PCAPNG instead of a live interface (-i ignored)",
    )

    p.add_argument(
        "-p",
        "--port",
        type=int,
        default=5001,
        metavar="N",
        help="TCP and UDP port to match (iperf2 default %(default)s; iperf3 often 5201)",
    )
    p.add_argument(
        "-d",
        "-T",
        "--duration",
        "--timeout",
        type=int,
        default=None,
        metavar="SEC",
        help=(
            "Live: stop capture after SEC s. PCAP: stop after SEC s of packet time "
            "from file start (tshark -a duration)"
        ),
    )
    p.add_argument(
        "-t",
        "--tshark",
        default="tshark",
        metavar="PATH",
        help="tshark executable (default: %(default)s)",
    )
    p.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="No startup line (only per-second lines)",
    )

    p.epilog = (
        "See --help for all options. Live capture usually needs sudo. "
        "Examples: sudo %(prog)s -i mon0 -p 5001 -d 30; "
        "%(prog)s -r capture.pcap -p 5001"
    )

    return p.parse_args(argv)


def _emit_second(sec, buckets, total_bytes, total_secs):
    b = buckets.get(sec, 0)
    mbits = (b * 8.0) / _BITS_PER_MBPS
    print(f"Second {sec}: {mbits:.2f} Mbps", flush=True)
    total_bytes[0] += b
    total_secs[0] += 1


def main(argv=None):
    args = parse_args(argv)

    if args.pcap is not None and not os.path.isfile(args.pcap):
        print(f"error: PCAP not found: {args.pcap}", file=sys.stderr)
        return 1

    cmd = [
        args.tshark,
        "-Y",
        f"(tcp.port=={args.port} || udp.port=={args.port})",
        "-T",
        "fields",
        "-e",
        "frame.time_relative",
        "-e",
        "frame.len",
    ]
    if args.pcap is not None:
        cmd.extend(["-r", args.pcap])
    else:
        cmd.extend(["-i", args.interface, "-l"])

    if args.duration is not None:
        cmd.extend(["-a", f"duration:{args.duration}"])

    if not args.quiet:
        sys.stderr.write(" ".join(cmd) + "\n")

    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=None,
            text=True,
            bufsize=1,
        )
    except OSError as e:
        print(f"error: cannot start tshark: {e}", file=sys.stderr)
        return 1

    buckets = {}
    last_sec = None
    last_emitted = -1
    total_bytes = [0]
    total_secs = [0]

    def handle_sigint(_sig, _frame):
        if proc.poll() is None:
            proc.terminate()

    signal.signal(signal.SIGINT, handle_sigint)

    try:
        assert proc.stdout is not None
        for line in proc.stdout:
            line = line.strip()
            if not line:
                continue
            parts = line.split("\t")
            if len(parts) != 2:
                continue
            try:
                t = float(parts[0])
                ln = int(parts[1])
            except ValueError:
                continue
            sec = int(t)
            buckets[sec] = buckets.get(sec, 0) + ln
            if last_sec is not None and sec > last_sec:
                for k in range(last_sec, sec):
                    _emit_second(k, buckets, total_bytes, total_secs)
                    last_emitted = k
            last_sec = sec
    except KeyboardInterrupt:
        if proc.poll() is None:
            proc.terminate()
    finally:
        if proc.poll() is None:
            proc.wait(timeout=5)
        elif proc.returncode not in (0, -signal.SIGTERM, -signal.SIGINT):
            pass

    if last_sec is None:
        print("No frames matched.", file=sys.stderr)
        return 1

    if last_sec > last_emitted and buckets.get(last_sec, 0) > 0:
        _emit_second(last_sec, buckets, total_bytes, total_secs)

    if total_secs[0] > 0:
        avg = (total_bytes[0] * 8.0) / (_BITS_PER_MBPS * total_secs[0])
        print(
            f"Average over {total_secs[0]} second(s): {avg:.2f} Mbps",
            file=sys.stderr,
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
