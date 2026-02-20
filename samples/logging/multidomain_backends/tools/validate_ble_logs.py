#!/usr/bin/env python3
"""Validate BLE logger backend output from the multidomain_backends sample.

The script scans for the NUS-compatible logger service, connects, subscribes to
its TX characteristic, and verifies that application log lines are received.
"""

import argparse
import asyncio
import sys
import time
from typing import List, Optional

from bleak import BleakClient, BleakScanner

NUS_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
NUS_TX_CHAR_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"


async def discover_target(scan_seconds: float, name_contains: Optional[str],
                          address: Optional[str]):
    print(f"Scanning for BLE logger target for {scan_seconds:.1f}s...")
    discoveries = await BleakScanner.discover(timeout=scan_seconds, return_adv=True)

    def has_nus_service(service_uuids: List[str]) -> bool:
        uuids = [u.lower() for u in service_uuids]
        return NUS_SERVICE_UUID in uuids

    for _addr, (device, adv_data) in discoveries.items():
        dev_name = (device.name or "").strip()
        service_uuids = adv_data.service_uuids or []
        if address and device.address.lower() == address.lower():
            return device
        if name_contains and name_contains.lower() in dev_name.lower():
            return device
        if has_nus_service(service_uuids):
            return device

    return None


async def run_validation(args) -> int:
    target = await discover_target(args.scan_seconds, args.name_contains, args.address)
    if target is None:
        print("ERROR: No matching BLE logger device found.")
        return 2

    print(f"Connecting to {target.address} ({target.name})")

    received: List[bytes] = []
    received_lines: List[str] = []

    def on_notify(_handle: int, data: bytearray):
        payload = bytes(data)
        received.append(payload)
        text = payload.decode("utf-8", errors="replace").strip()
        stamp = time.strftime("%H:%M:%S")
        if text:
            received_lines.append(text)
            print(f"[{stamp}] {text}")
        else:
            print(f"[{stamp}] <{payload.hex()}>")

    try:
        async with BleakClient(target) as client:
            if not client.is_connected:
                print("ERROR: BLE connection failed.")
                return 3

            await client.start_notify(NUS_TX_CHAR_UUID, on_notify)
            print(f"Subscribed to {NUS_TX_CHAR_UUID}. Collecting for {args.duration}s...")
            await asyncio.sleep(args.duration)
            await client.stop_notify(NUS_TX_CHAR_UUID)
    except Exception as exc:  # noqa: BLE001
        print(f"ERROR: BLE operation failed: {exc}")
        return 4

    print(f"Received {len(received)} notification(s).")

    if len(received) < args.min_messages:
        print(f"ERROR: Expected at least {args.min_messages} notifications.")
        return 5

    app_hits = [line for line in received_lines if ("cpuapp_" in line or "cpuflpr_" in line)]
    if len(app_hits) < args.min_app_lines:
        print(
            "ERROR: Not enough application log lines received "
            f"(got {len(app_hits)}, expected >= {args.min_app_lines})."
        )
        return 6

    print("BLE validation PASSED.")
    return 0


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate BLE logs from multidomain sample")
    parser.add_argument("--scan-seconds", type=float, default=10.0,
                        help="BLE scan duration before connecting")
    parser.add_argument("--duration", type=int, default=20,
                        help="Notification collection duration after connect")
    parser.add_argument("--name-contains", type=str, default="nRF54LM20 Multi-domain Logger",
                        help="Preferred device name substring")
    parser.add_argument("--address", type=str, default=None,
                        help="Optional fixed BLE address")
    parser.add_argument("--min-messages", type=int, default=5,
                        help="Minimum BLE notifications expected")
    parser.add_argument("--min-app-lines", type=int, default=2,
                        help="Minimum app log lines containing cpuapp_/cpuflpr_")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    return asyncio.run(run_validation(args))


if __name__ == "__main__":
    sys.exit(main())
