#!/usr/bin/env python3

#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

import argparse
import signal
import struct
import sys

PIPE_DATA = "/tmp/esb_sniffer_data"
PIPE_COMM = "/tmp/esb_sniffer_comm"

COMM_START_CAPTURE = "1"
COMM_STOP_CAPTURE  = "2"

def list_interfaces():
    print("extcap {version=0.1}{display=ESB sniffer extcap}")
    print("interface {value=ESBextcap}{display=ESB sniffer interface}")

def list_dlts():
    print("dlt {number=147}{name=ESB}{display=ESB}")

def list_config():
    return None

def _pcap_header():
    return struct.pack('<IHHiIII', 0xa1b2c3d4, 2, 4, 0, 0, 65535, 147)

def capture(ws_fifo, sn_pipe):
    try:
        ws = open(ws_fifo, 'wb')
    except Exception as err:
        print(f"Failed to open wireshark pipe: {err}", file=sys.stderr)
        return

    try:
        ws.write(_pcap_header())
        ws.flush()
        while True:
            pkt = sn_pipe.read(64)
            if pkt:
                ws.write(pkt)
            else:
                raise OSError

            ws.flush()
    except OSError:
        print(f"Failed to read packets from the device: {err}", file=sys.stderr)
        ws.close()

def main():
    parser = argparse.ArgumentParser(description="ESB Sniffer Extcap", allow_abbrev=False)
    parser.add_argument("--extcap-interfaces", action="store_true")
    parser.add_argument("--extcap-interface")
    parser.add_argument("--extcap-dlts", action="store_true")
    parser.add_argument("--extcap-config", action="store_true")
    parser.add_argument("--capture", action="store_true")
    parser.add_argument("--fifo")
    com_pipe = None
    sn_pipe = None

    def _sig_stop(sig, frame):
        try:
            com_pipe.write(COMM_STOP_CAPTURE)
            com_pipe.flush()
            com_pipe.close()
            sn_pipe.close()
        # Ignoring all exceptions, because we are shutting down anyway.
        except Exception:
            pass

        sys.exit(0)

    args, _ = parser.parse_known_args()

    if args.extcap_interfaces:
        list_interfaces()
    elif args.extcap_dlts:
        list_dlts()
    elif args.extcap_config:
        list_config()
    elif args.capture:
        if not args.fifo:
            print("Missing --fifo argument", file=sys.stderr)
            sys.exit(1)

        signal.signal(signal.SIGTERM, _sig_stop)

        try:
            com_pipe = open(PIPE_COMM, 'w')
            sn_pipe = open(PIPE_DATA, 'rb')
            com_pipe.write(COMM_START_CAPTURE)
            com_pipe.flush()
        except FileNotFoundError as err:
            print(f"Sniffer pipes not found: {err}. Is sniffer script running?", file=sys.stderr)
            sys.exit(1)
        except BrokenPipeError as err:
            print(f"Failed to open sniffer pipes: {err}", file=sys.stderr)
            sys.exit(1)
        except Exception as err:
            print(f"Failed to start capture: {err}", file=sys.stderr)
            sys.exit(1)

        capture(args.fifo, sn_pipe)
        _sig_stop(None, None)
    else:
        print("Invalid extcap call", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
