#!/usr/bin/env python3
"""Run a command file saved with 'save' in utp_shell.py, one command per line."""

import argparse

import serial
import utp_shell


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("script", help="File written by the 'save' command")
    parser.add_argument("port", help="Serial port, e.g. /dev/ttyACM1")
    parser.add_argument("--baud", type=int, default=1000000)
    args = parser.parse_args()

    with open(args.script, encoding="utf-8") as f:
        lines = [line.strip() for line in f if line.strip() and not line.startswith("#")]

    with serial.Serial(args.port, args.baud, rtscts=True, timeout=5) as port:
        utp_shell.hci_reset(port)
        shell = utp_shell.UtpShell(port)
        for line in lines:
            print(f"utp> {line}")
            shell.onecmd(line)


if __name__ == "__main__":
    main()
