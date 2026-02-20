#!/usr/bin/env python3
"""Capture and decode dictionary logs for multidomain_backends sample.

Supported sources:
- UART stream
- BLE logger backend (NUS TX notifications)
- Filesystem backend dump over custom BLE service
"""

import argparse
import asyncio
import logging
import os
import pathlib
import sys
import time
from typing import Optional

import serial
from bleak import BleakClient, BleakScanner

import ble_fs_dump

NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
MSG_TYPE_NORMAL = 0
MSG_TYPE_DROPPED = 1


def resolve_zephyr_base(zephyr_base_arg: Optional[str]) -> pathlib.Path:
    if zephyr_base_arg:
        return pathlib.Path(zephyr_base_arg).resolve()

    env_val = os.getenv("ZEPHYR_BASE")
    if env_val:
        return pathlib.Path(env_val).resolve()

    # tools -> multidomain_backends -> logging -> samples -> nrf -> ncs-main
    candidate = pathlib.Path(__file__).resolve().parents[5] / "zephyr"
    if candidate.exists():
        return candidate

    raise RuntimeError("Cannot resolve ZEPHYR_BASE. Pass --zephyr-base.")


class DictDecoder:
    def __init__(self, dbfile: pathlib.Path, zephyr_base: pathlib.Path, debug: bool):
        parser_path = zephyr_base / "scripts" / "logging" / "dictionary"
        if not parser_path.exists():
            raise RuntimeError(f"Dictionary parser path not found: {parser_path}")

        sys.path.insert(0, str(parser_path))
        import parserlib  # pylint: disable=import-outside-toplevel

        self.parserlib = parserlib

        logging.basicConfig(format="%(message)s")
        self.logger = logging.getLogger("dict_decode")
        self.logger.setLevel(logging.DEBUG if debug else logging.INFO)

        self.log_parser = self.parserlib.get_log_parser(str(dbfile), self.logger)

    def decode(self, logdata: bytes) -> tuple[int, int, int]:
        """Decode stream with single-byte resync on malformed input."""
        buf = bytearray(logdata)
        parsed_total = 0
        skipped = 0

        while buf:
            if buf[0] not in (MSG_TYPE_NORMAL, MSG_TYPE_DROPPED):
                del buf[0]
                skipped += 1
                continue

            try:
                parsed = self.parserlib.parser(buf, self.log_parser, self.logger)
            except ValueError:
                del buf[0]
                skipped += 1
                continue

            if parsed == 0:
                break

            parsed_total += parsed
            del buf[:parsed]

        return parsed_total, skipped, len(buf)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Capture and decode Zephyr dictionary logs for multidomain_backends"
    )
    parser.add_argument("--db", required=True, help="Path to log_dictionary.json")
    parser.add_argument(
        "--zephyr-base",
        default=None,
        help="Path to Zephyr base (defaults to ZEPHYR_BASE or workspace zephyr/)",
    )
    parser.add_argument("--debug", action="store_true", help="Enable parser debug output")
    parser.add_argument("--raw-out", default=None, help="Write captured binary stream to file")

    sub = parser.add_subparsers(dest="cmd", required=True)

    uart = sub.add_parser("uart", help="Capture dictionary stream over UART and decode")
    uart.add_argument("--port", required=True, help="UART port, prefer /dev/serial/by-id/... path")
    uart.add_argument("--baudrate", type=int, default=115200, help="UART baudrate")
    uart.add_argument("--duration", type=float, default=20.0, help="Capture duration in seconds")

    ble = sub.add_parser("ble", help="Capture dictionary stream from BLE logger (NUS) and decode")
    ble.add_argument("--scan-seconds", type=float, default=10.0, help="BLE scan duration")
    ble.add_argument("--duration", type=float, default=20.0, help="Notification capture duration")
    ble.add_argument(
        "--name-contains",
        default="nRF54LM20 Multi-domain Logger",
        help="Preferred device name substring",
    )
    ble.add_argument("--address", default=None, help="Optional fixed BLE address")

    fs = sub.add_parser("fs", help="Dump FS backend file over BLE and decode dictionary data")
    fs.add_argument("--scan-seconds", type=float, default=12.0, help="BLE scan duration")
    fs.add_argument("--timeout", type=float, default=25.0, help="FS dump response timeout")
    fs.add_argument(
        "--name-contains",
        default="nRF54LM20 Multi-domain Logger",
        help="Preferred device name substring",
    )
    fs.add_argument("--address", default=None, help="Optional fixed BLE address")
    fs.add_argument("--file", required=True, help="Remote FS log file, e.g. err.0000")

    infile = sub.add_parser("file", help="Decode an existing binary dictionary log file")
    infile.add_argument("input", help="Binary log file path")

    return parser.parse_args()


def capture_uart(port: str, baudrate: int, duration_s: float) -> bytes:
    deadline = time.monotonic() + duration_s
    raw = bytearray()

    while time.monotonic() < deadline:
        if not os.path.exists(port):
            time.sleep(0.1)
            continue

        try:
            with serial.Serial(port, baudrate, timeout=0.2) as ser:
                while time.monotonic() < deadline:
                    chunk = ser.read(1024)
                    if chunk:
                        raw.extend(chunk)
        except (serial.SerialException, OSError):
            time.sleep(0.1)

    return bytes(raw)


async def discover_nus_target(scan_seconds: float, name_contains: str, address: Optional[str]):
    discoveries = await BleakScanner.discover(timeout=scan_seconds, return_adv=True)

    for _addr, (device, adv_data) in discoveries.items():
        dev_name = (device.name or "").strip()
        service_uuids = [u.lower() for u in (adv_data.service_uuids or [])]

        if address and (device.address.lower() == address.lower()):
            return device
        if name_contains.lower() in dev_name.lower():
            return device
        if NUS_SERVICE_UUID in service_uuids:
            return device

    return None


async def capture_ble_nus(
    scan_seconds: float, duration_s: float, name_contains: str, address: Optional[str]
) -> bytes:
    target = await discover_nus_target(scan_seconds, name_contains, address)
    if target is None:
        raise RuntimeError("No matching BLE logger target found")

    print(f"Connecting to {target.address} ({target.name})")
    raw = bytearray()

    def on_notify(_handle: int, data: bytearray):
        raw.extend(bytes(data))

    async with BleakClient(target) as client:
        if not client.is_connected:
            raise RuntimeError("BLE connection failed")

        await client.start_notify(NUS_TX_CHAR_UUID, on_notify)
        await asyncio.sleep(duration_s)
        await client.stop_notify(NUS_TX_CHAR_UUID)

    return bytes(raw)


async def dump_fs_over_ble(
    scan_seconds: float,
    timeout_s: float,
    name_contains: str,
    address: Optional[str],
    filename: str,
) -> bytes:
    target = await ble_fs_dump.discover_target(scan_seconds, name_contains, address)
    if target is None:
        raise RuntimeError("No matching BLE target found for FS dump")

    print(f"Connecting to {target.address} ({target.name})")
    client = BleakClient(target)
    try:
        await client.connect()
        if not client.is_connected:
            raise RuntimeError("BLE connection failed")
        return await ble_fs_dump.run_dump(client, filename, timeout_s)
    finally:
        try:
            await client.disconnect()
        except EOFError:
            pass


def write_raw(raw: bytes, path: Optional[str]) -> None:
    if not path:
        return

    out = pathlib.Path(path)
    out.write_bytes(raw)
    print(f"Wrote raw capture to {out}")


def main() -> int:
    args = parse_args()

    dbfile = pathlib.Path(args.db).resolve()
    if not dbfile.exists():
        print(f"ERROR: Dictionary DB not found: {dbfile}")
        return 2

    try:
        zephyr_base = resolve_zephyr_base(args.zephyr_base)
        decoder = DictDecoder(dbfile, zephyr_base, args.debug)
    except Exception as exc:  # noqa: BLE001
        print(f"ERROR: Failed to initialize decoder: {exc}")
        return 3

    try:
        if args.cmd == "uart":
            raw = capture_uart(args.port, args.baudrate, args.duration)
        elif args.cmd == "ble":
            raw = asyncio.run(
                capture_ble_nus(
                    args.scan_seconds,
                    args.duration,
                    args.name_contains,
                    args.address,
                )
            )
        elif args.cmd == "fs":
            raw = asyncio.run(
                dump_fs_over_ble(
                    args.scan_seconds,
                    args.timeout,
                    args.name_contains,
                    args.address,
                    args.file,
                )
            )
        else:
            raw = pathlib.Path(args.input).read_bytes()
    except Exception as exc:  # noqa: BLE001
        print(f"ERROR: Capture failed: {exc}")
        return 4

    print(f"Captured {len(raw)} bytes")
    write_raw(raw, args.raw_out)

    if not raw:
        print("ERROR: No data captured")
        return 5

    parsed_total, skipped, remaining = decoder.decode(raw)
    print(
        "Decode summary: "
        f"parsed={parsed_total} bytes, skipped={skipped} byte(s), remaining={remaining} byte(s)"
    )

    if parsed_total == 0:
        print("ERROR: Parser could not decode any dictionary message")
        return 6

    return 0


if __name__ == "__main__":
    sys.exit(main())
