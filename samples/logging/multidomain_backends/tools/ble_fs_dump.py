#!/usr/bin/env python3
"""Dump filesystem log files from multidomain_backends over custom BLE GATT."""

import argparse
import asyncio
import pathlib
import sys
import time
from dataclasses import dataclass, field
from typing import Optional

from bleak import BleakClient, BleakScanner

FS_DUMP_SERVICE_UUID = "3a0b25f0-8d4b-4cde-9f80-7d3cf6f8a100"
FS_DUMP_CMD_CHAR_UUID = "3a0b25f1-8d4b-4cde-9f80-7d3cf6f8a100"
FS_DUMP_DATA_CHAR_UUID = "3a0b25f2-8d4b-4cde-9f80-7d3cf6f8a100"


@dataclass
class SessionState:
    mode: str
    wanted_file: Optional[str] = None
    files: list[str] = field(default_factory=list)
    dump_bytes: bytearray = field(default_factory=bytearray)
    in_dump: bool = False
    list_started: bool = False
    done: asyncio.Event = field(default_factory=asyncio.Event)
    error: Optional[str] = None
    rx_buf: str = ""
    last_rx_time: float = 0.0


def handle_line(state: SessionState, line: str) -> None:
    line = line.strip()
    if not line:
        return

    state.last_rx_time = time.monotonic()
    print(f"[RX] {line}")

    if line.startswith("ERR "):
        state.error = line
        state.done.set()
        return

    if state.mode == "list":
        if line == "OK LIST BEGIN":
            state.list_started = True
        if line.startswith("FILE "):
            state.files.append(line[5:].strip())
        elif line == "OK LIST END":
            state.done.set()
    else:
        if line.startswith("OK DUMP BEGIN "):
            state.in_dump = True
        elif line.startswith("D "):
            hex_payload = line[2:].strip()
            if hex_payload:
                state.dump_bytes.extend(bytes.fromhex(hex_payload))
        elif line.startswith("OK DUMP END "):
            state.done.set()


def make_notify_cb(state: SessionState):
    def _cb(_handle: int, data: bytearray) -> None:
        text = bytes(data).decode("utf-8", errors="replace")
        state.rx_buf += text

        while "\n" in state.rx_buf:
            line, state.rx_buf = state.rx_buf.split("\n", 1)
            handle_line(state, line)

    return _cb


async def discover_target(scan_seconds: float, name_contains: str, address: Optional[str]):
    print(f"Scanning for {scan_seconds:.1f}s...")
    discoveries = await BleakScanner.discover(timeout=scan_seconds, return_adv=True)

    for _addr, (device, _adv_data) in discoveries.items():
        dev_name = (device.name or "").strip()
        if address and device.address.lower() == address.lower():
            return device
        if name_contains.lower() in dev_name.lower():
            return device

    return None


async def run_list(client: BleakClient, timeout: float) -> list[str]:
    state = SessionState(mode="list")
    await client.start_notify(FS_DUMP_DATA_CHAR_UUID, make_notify_cb(state))
    await client.write_gatt_char(FS_DUMP_CMD_CHAR_UUID, b"LIST\n", response=True)

    timeout_at = time.monotonic() + timeout

    try:
        while time.monotonic() < timeout_at:
            if state.done.is_set():
                break
            if state.list_started and state.files and state.last_rx_time > 0:
                if (time.monotonic() - state.last_rx_time) > 1.5:
                    break
            await asyncio.sleep(0.2)
        else:
            raise TimeoutError("LIST timed out")
    finally:
        await client.stop_notify(FS_DUMP_DATA_CHAR_UUID)

    if state.error:
        raise RuntimeError(state.error)

    return state.files


async def run_dump(client: BleakClient, filename: str, timeout: float) -> bytes:
    state = SessionState(mode="dump", wanted_file=filename)
    await client.start_notify(FS_DUMP_DATA_CHAR_UUID, make_notify_cb(state))
    cmd = f"DUMP {filename}\n".encode("utf-8")
    await client.write_gatt_char(FS_DUMP_CMD_CHAR_UUID, cmd, response=True)

    timeout_at = time.monotonic() + timeout

    try:
        while time.monotonic() < timeout_at:
            if state.done.is_set():
                break
            if state.in_dump and state.last_rx_time > 0:
                if (time.monotonic() - state.last_rx_time) > 1.5:
                    break
            await asyncio.sleep(0.2)
        else:
            raise TimeoutError("DUMP timed out")
    finally:
        await client.stop_notify(FS_DUMP_DATA_CHAR_UUID)

    if state.error:
        raise RuntimeError(state.error)

    return bytes(state.dump_bytes)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="BLE filesystem log dumper")
    parser.add_argument("--scan-seconds", type=float, default=12.0, help="BLE scan duration")
    parser.add_argument(
        "--name-contains",
        type=str,
        default="nRF54LM20 Multi-domain Logger",
        help="Preferred device name substring",
    )
    parser.add_argument("--address", type=str, default=None, help="Optional fixed BLE address")
    parser.add_argument("--timeout", type=float, default=20.0, help="Response timeout")

    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("list", help="List FS log files")

    dump = sub.add_parser("dump", help="Dump a specific FS log file")
    dump.add_argument("--file", required=True, help="Remote file name (e.g. err.0000)")
    dump.add_argument("--out", required=True, help="Output path for dumped bytes")

    return parser.parse_args()


async def amain(args: argparse.Namespace) -> int:
    target = await discover_target(args.scan_seconds, args.name_contains, args.address)
    if target is None:
        print("ERROR: No matching BLE target found.")
        return 2

    print(f"Connecting to {target.address} ({target.name})")

    client = BleakClient(target)
    try:
        await client.connect()
        if not client.is_connected:
            print("ERROR: Connection failed.")
            return 3

        if args.cmd == "list":
            files = await run_list(client, args.timeout)
            if not files:
                print("No files found.")
                return 0
            print("Files:")
            for name in files:
                print(f"  {name}")
            return 0

        payload = await run_dump(client, args.file, args.timeout)
        out_path = pathlib.Path(args.out)
        out_path.write_bytes(payload)
        print(f"Wrote {len(payload)} bytes to {out_path}")
        return 0
    finally:
        try:
            await client.disconnect()
        except EOFError:
            pass


def main() -> int:
    args = parse_args()
    try:
        return asyncio.run(amain(args))
    except RuntimeError as exc:
        print(f"ERROR: {exc}")
        return 4


if __name__ == "__main__":
    sys.exit(main())
