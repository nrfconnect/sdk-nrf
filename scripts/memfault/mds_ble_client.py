#!/usr/bin/env python3

#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""
Memfault Diagnostic Service (MDS) client using the host Bluetooth stack (Bleak).

1. Connects to a device by BLE address or advertising name,
2. Reads MDS characteristics,
3. Subscribes to Data Export notifications.

Use ``async with MdsBleClient(...)`` from application code, or run the CLI
``python3 mds_ble_client.py`` (CLI prints reads and streams chunks as hex).
"""

from __future__ import annotations

import argparse
import asyncio
import contextlib
import logging
import os
import sys
from collections.abc import AsyncIterator, Callable

from bleak import BleakClient, BleakScanner
from bleak.exc import BleakError

logger = logging.getLogger(__name__)

__all__ = [
    "MdsBleClient",
]

# UUIDs: include/bluetooth/services/mds.h
UUID_SUPPORTED_FEATURES = "54220001-f6a5-4007-a371-722f4ebd8436"
UUID_DEVICE_IDENTIFIER = "54220002-f6a5-4007-a371-722f4ebd8436"
UUID_DATA_URI = "54220003-f6a5-4007-a371-722f4ebd8436"
UUID_AUTHORIZATION = "54220004-f6a5-4007-a371-722f4ebd8436"
UUID_DATA_EXPORT = "54220005-f6a5-4007-a371-722f4ebd8436"
DATA_STREAM_ENABLE = bytes([0x01])
DATA_STREAM_DISABLE = bytes([0x00])

# enum eMfltMessageType: components/core/src/memfault_data_packetizer.c
MEMFAULT_MESSAGE_TYPES = {
    0: "none",
    1: "coredump",
    2: "event",
    3: "log",
    4: "cdr",
}


def _decode_utf8(data: bytes | bytearray) -> str:
    return bytes(data).decode("utf-8", errors="replace")


def _decode_varint_u32(buf: bytes, start: int) -> tuple[int, int]:
    """Decode a protobuf-style varint; return (value, num_bytes_consumed)."""
    val = 0
    shift = 0
    for i, b in enumerate(buf[start : start + 5]):
        val |= (b & 0x7F) << shift
        if not b & 0x80:
            return val, i + 1
        shift += 7
    raise ValueError(f"Failed to decode varint {buf[start : start + 5].hex()}")


async def _bluez_agent_manager_call(bus: object, member: str, body: list, signature: str) -> None:
    from dbus_fast import Message
    from dbus_fast.constants import MessageType

    reply = await bus.call(
        Message(
            destination="org.bluez",
            path="/org/bluez",
            interface="org.bluez.AgentManager1",
            member=member,
            signature=signature,
            body=body,
        )
    )
    if reply.message_type == MessageType.ERROR:
        name = reply.error_name or "unknown"
        text = reply.body[0] if reply.body else ""
        raise RuntimeError(f"BlueZ AgentManager1.{member} failed: {name}: {text}")


@contextlib.asynccontextmanager
async def _linux_bluez_no_io_pairing_agent() -> AsyncIterator[None]:
    """Register a BlueZ Agent1 with capability NoInputNoOutput (Linux only).

    Auto-accepts pairing flows that only need confirmation / authorization (e.g. LE Just Works).
    Rejects PIN or passkey entry, which this capability does not support.

    This should eventually by supported by Bleak, in which case this can be removed.
    See: https://github.com/hbldh/bleak/pull/1100
    """
    if sys.platform != "linux":
        yield
        return

    from bleak.backends.bluezdbus.utils import get_dbus_authenticator
    from dbus_fast import BusType
    from dbus_fast.aio.message_bus import MessageBus
    from dbus_fast.errors import DBusError
    from dbus_fast.service import ServiceInterface, dbus_method

    # D-Bus signature strings (required here: deferred annotations + nested class
    # break dbus_fast's eval() of DBusObjectPath etc. against module globals).
    class _NoInputNoOutputAgent(ServiceInterface):
        def __init__(self) -> None:
            super().__init__("org.bluez.Agent1")

        def _raise_rejected(self, action: str) -> None:
            raise DBusError(
                "org.bluez.Error.Rejected",
                f"NoInputNoOutput agent cannot {action}",
            )

        @dbus_method()
        def Release(self):
            pass

        @dbus_method()
        def Cancel(self):
            pass

        @dbus_method()
        def RequestPinCode(self, device: "o") -> "s":  # noqa: F821, UP037
            self._raise_rejected("supply PIN")

        @dbus_method()
        def DisplayPinCode(self, device: "o", pincode: "s"):  # noqa: F821, UP037
            self._raise_rejected("display PIN")

        @dbus_method()
        def RequestPasskey(self, device: "o") -> "u":  # noqa: F821, UP037
            self._raise_rejected("supply passkey")

        @dbus_method()
        def DisplayPasskey(self, device: "o", passkey: "u", entered: "q"):  # noqa: F821, UP037
            self._raise_rejected("display passkey")

        @dbus_method()
        def RequestConfirmation(self, device: "o", passkey: "u"):  # noqa: F821, UP037
            pass

        @dbus_method()
        def RequestAuthorization(self, device: "o"):  # noqa: F821, UP037
            pass

        @dbus_method()
        def AuthorizeService(self, device: "o", uuid: "s"):  # noqa: F821, UP037
            pass

    agent_path = f"/no/nordicsemi/memfault/mds_ble_client/agent_{os.getpid()}"
    bus = MessageBus(bus_type=BusType.SYSTEM, negotiate_unix_fd=True, auth=get_dbus_authenticator())
    agent = _NoInputNoOutputAgent()
    exported = False
    registered = False
    try:
        await bus.connect()
        bus.export(agent_path, agent)
        exported = True
        await _bluez_agent_manager_call(bus, "RegisterAgent", [agent_path, "NoInputNoOutput"], "os")
        registered = True
        await _bluez_agent_manager_call(bus, "RequestDefaultAgent", [agent_path], "o")
        logger.debug("Registered BlueZ NoInputNoOutput pairing agent at %s", agent_path)
        yield
    finally:
        if registered:
            try:
                await _bluez_agent_manager_call(bus, "UnregisterAgent", [agent_path], "o")
            except Exception as err:
                logger.warning("BlueZ UnregisterAgent failed: %s", err)
        if exported:
            bus.unexport(agent_path, agent)
        if bus.connected:
            bus.disconnect()
            await bus.wait_for_disconnect()


class MdsBleClient:
    """GATT access to MDS on a connected ``BleakClient`` (use as an async context manager)."""

    def __init__(
        self,
        *,
        address: str | None = None,
        name: str | None = None,
        scan_timeout: float = 10.0,
        adapter: str | None = None,
    ) -> None:
        if (address is None) == (name is None):
            raise ValueError("Specify exactly one of address or name")
        self._address = address
        self._name = name
        self._scan_timeout = scan_timeout
        self._adapter = adapter
        self._client: BleakClient | None = None
        self._exit_stack: contextlib.AsyncExitStack | None = None

    async def _resolve_address(self) -> str:
        if (self._address is None) == (self._name is None):
            raise ValueError("Specify exactly one of address or name")
        if self._address is not None:
            return self._address.lower()
        device = await BleakScanner.find_device_by_name(self._name, timeout=self._scan_timeout)
        if device is None:
            raise RuntimeError(
                f"No advertiser with name {self._name!r} found (timeout={self._scan_timeout}s)"
            )
        logger.info(f"Resolved name {self._name!r} to address {device.address}")
        return device.address

    async def __aenter__(self) -> MdsBleClient:
        self._exit_stack = contextlib.AsyncExitStack()
        await self._exit_stack.__aenter__()
        await self._exit_stack.enter_async_context(_linux_bluez_no_io_pairing_agent())
        address = await self._resolve_address()
        kwargs = {}
        if self._adapter is not None:
            kwargs["adapter"] = self._adapter
        self._client = BleakClient(address, pair=True, **kwargs)
        print(f"Connecting to {address}")
        await self._client.__aenter__()
        print("Connected")
        return self

    async def __aexit__(self, exc_type, exc, tb):
        try:
            if self._client is not None:
                await self._client.__aexit__(exc_type, exc, tb)
                self._client = None
        finally:
            if self._exit_stack is not None:
                stack, self._exit_stack = self._exit_stack, None
                await stack.__aexit__(exc_type, exc, tb)

    def is_connected(self) -> bool:
        return self._client is not None and self._client.is_connected

    def _require_client(self) -> BleakClient:
        if self._client is None:
            raise RuntimeError("Not connected; use 'async with MdsBleClient(...) as mds'")
        return self._client

    async def read_supported_features(self) -> bytes:
        return bytes(await self._require_client().read_gatt_char(UUID_SUPPORTED_FEATURES))

    async def read_device_identifier(self) -> str:
        return _decode_utf8(await self._require_client().read_gatt_char(UUID_DEVICE_IDENTIFIER))

    async def read_data_uri(self) -> str:
        return _decode_utf8(await self._require_client().read_gatt_char(UUID_DATA_URI))

    async def read_authorization(self) -> str:
        return _decode_utf8(await self._require_client().read_gatt_char(UUID_AUTHORIZATION))

    async def subscribe_data_export(
        self,
        on_chunk: Callable[[bytes], None],
        *,
        enable_streaming: bool = True,
        write_with_response: bool = True,
    ) -> None:
        """Subscribe to Data export notifications on an existing ``BleakClient``."""

        def handler(_sender, data: bytearray) -> None:
            on_chunk(bytes(data))

        await self._require_client().start_notify(UUID_DATA_EXPORT, handler)
        if enable_streaming:
            await self._require_client().write_gatt_char(
                UUID_DATA_EXPORT, DATA_STREAM_ENABLE, response=write_with_response
            )

    async def wait_until_disconnected(self) -> None:
        """Block until the GATT connection is lost."""
        while self.is_connected():
            await asyncio.sleep(0.2)


def data_export_payload_describe(payload: bytes) -> str:
    """Decode Memfault Data Export payload (after MDS chunk-number byte)."""
    if not payload:
        return "(empty payload)"

    header = payload[0]
    i = 1
    continuation = bool(header & 0x80)
    more_data = bool(header & 0x40)

    if continuation:
        offset, n = _decode_varint_u32(payload, i)
        i += n
        payload = payload[i:]
        return f"msg continuation: offset {offset} size {len(payload)}"

    if more_data:
        total_len, n = _decode_varint_u32(payload, i)
        i += n

    # sMfltPacketizerHdr: components/core/src/memfault_data_packetizer.c
    msg_header = payload[i]
    i += 1
    uses_rle = bool(msg_header & 0x80)

    has_project_key = bool(msg_header & 0x40)
    msg_type = msg_header & 0x3F

    if has_project_key:
        end = payload.find(b"\x00", i)
        if end == -1:
            raise ValueError("Failed to decode project key: unterminated")
        project_key = payload[i:end]

    msg_type_str = MEMFAULT_MESSAGE_TYPES.get(msg_type, f"unknown({msg_type})")
    payload = payload[i:]

    if not more_data:
        total_len = len(payload)

    desc = f"msg init: type: {msg_type_str}, size {len(payload)}/{total_len}"
    if uses_rle:
        desc += ", RLE"
    if has_project_key:
        desc += f", project key: {project_key}"
    return desc


def format_data_export_chunk(data: bytes, *, decode: bool) -> str:
    payload = data[1:]
    payload_desc = payload.hex()

    if payload and decode:
        try:
            payload_desc = data_export_payload_describe(payload)
        except ValueError as err:
            logger.error(f"Error decoding data export payload: {err}")

    return f"[{data[0]:02d}] {payload_desc}"


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description="Memfault Diagnostic Service client",
        allow_abbrev=False,
    )
    g = p.add_mutually_exclusive_group(required=True)
    g.add_argument(
        "--address",
        "-a",
        type=str,
        help="Peripheral BLE address (e.g. AA:BB:CC:DD:EE:FF)",
    )
    g.add_argument("--name", "-n", type=str, help="Peripheral BLE name")
    p.add_argument(
        "--scan-timeout",
        type=float,
        default=10.0,
        help="Seconds to scan when using --name (default: 10)",
    )
    p.add_argument("--adapter", type=str, default=None, help="Host adapter name (OS-specific)")
    p.add_argument(
        "--decode",
        action="store_true",
        help="Decode Data Export chunks",
    )
    p.add_argument("-v", "--verbose", action="store_true", help="Debug logging")
    return p.parse_args(argv)


async def do_main(args: argparse.Namespace) -> int:
    def on_chunk(data: bytes) -> None:
        if not data:
            return
        print(format_data_export_chunk(data, decode=args.decode), flush=True)

    async with MdsBleClient(
        address=args.address,
        name=args.name,
        scan_timeout=args.scan_timeout,
        adapter=args.adapter,
    ) as mds:
        supported_features = await mds.read_supported_features()
        device_identifier = await mds.read_device_identifier()
        data_uri = await mds.read_data_uri()
        authorization = await mds.read_authorization()

        print(f"MDS supported features: {supported_features.hex()}")
        print(f"MDS device identifier: {device_identifier}")
        print(f"MDS data URI: {data_uri}")
        print(f"MDS authorization: {authorization}")
        print("MDS data export:", flush=True)
        await mds.subscribe_data_export(on_chunk)
        with contextlib.suppress(asyncio.CancelledError):
            await mds.wait_until_disconnected()
    return 0


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(level=log_level, format="%(message)s")
    try:
        return asyncio.run(do_main(args))
    except KeyboardInterrupt:
        return 130
    except (BleakError, RuntimeError, ValueError) as err:
        logger.error("%s", err)
        return 1


if __name__ == "__main__":
    sys.exit(main())
