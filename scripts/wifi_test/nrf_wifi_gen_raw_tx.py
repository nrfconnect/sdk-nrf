#!/usr/bin/env python3

# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

"""
Generate or decode zperf raw-upload hex for nRF Wi-Fi raw TX testing.

Generate: builds raw_tx_pkt_header (12 B, big-endian multi-byte fields for UART
hex) plus WLAN + LLC/SNAP + IPv4 + UDP headers only (62 B MPDU prefix). The
802.11 part is **QoS Data** (frame control 0x88…) with a **QoS Control** field
so the MPDU is eligible for TX aggregation (A-MPDU) paths that expect QoS
subtypes. Zperf pads with 'z'; IP/UDP lengths match the full logical frame.

Zephyr zperf defaults CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE to 64; this header is
74 bytes — set the Kconfig option to at least 74 or the shell reports invalid
hex (buffer too small).

Decode: print raw_tx, 802.11, LLC/SNAP, IPv4, and UDP fields (raw_tx parsed as
big-endian; falls back to LE if magic does not match).
"""

import argparse
import re
import struct
import sys

# 802.11 QoS Data (26) + LLC/SNAP (8) + IPv4 (20) + UDP header (8) = 62 bytes.
# FC 0x88 0x00: QoS Data; QoS Control 0x0000 (TID 0) after sequence control.
_WLAN_LLC_BYTES = bytes.fromhex(
    "88000000ffffffffffff001122334455a06960e3521500000000aaaa030000000800"
)

_IP_HDR_LEN = 20
_UDP_HDR_LEN = 8
_MPDU_HDR = len(_WLAN_LLC_BYTES) + _IP_HDR_LEN + _UDP_HDR_LEN

# Minimum total buffer: vendor header + full header stack (no UDP payload in hex).
_MIN_TOTAL = 12 + _MPDU_HDR

_DEFAULT_LEN = 1024


def _ipv4_checksum(header20):
    """Return 16-bit IPv4 header checksum for a 20-byte header."""
    total = 0
    for i in range(0, 20, 2):
        total += int.from_bytes(header20[i : i + 2], "big")
    while total > 0xFFFF:
        total = (total & 0xFFFF) + (total >> 16)
    return (~total) & 0xFFFF


def _build_ip_udp_headers(udp_payload_len):
    """
    IPv4 + UDP headers only (no UDP payload). Length fields include payload
    size that zperf will pad with 'z'.
    """
    ip_total = _IP_HDR_LEN + _UDP_HDR_LEN + udp_payload_len
    udp_total = _UDP_HDR_LEN + udp_payload_len

    ip_hdr = bytearray(
        [
            0x45,
            0x00,
            (ip_total >> 8) & 0xFF,
            ip_total & 0xFF,
            0x00,
            0x00,
            0x40,
            0x00,
            0x40,
            0x11,
            0x00,
            0x00,
            0xC0,
            0xA8,
            0x01,
            0x02,
            0xC0,
            0xA8,
            0x01,
            0xFF,
        ]
    )
    csum = _ipv4_checksum(ip_hdr)
    ip_hdr[10] = (csum >> 8) & 0xFF
    ip_hdr[11] = csum & 0xFF

    udp = bytes(
        [
            0x13,
            0x89,
            0x13,
            0x89,
            (udp_total >> 8) & 0xFF,
            udp_total & 0xFF,
            0x00,
            0x00,
        ]
    )
    return bytes(ip_hdr) + udp


def build_header_hex(total_len, data_rate=7, tx_mode=0, queue=0):
    """
    Return (hex string, total_len, mpdu_len, udp_payload_len).

    total_len is the zperf packet_size (12-byte vendor + MPDU; zperf pads with
    'z' to this size). Output is 74 bytes (148 hex chars): headers only.
    """
    if total_len < _MIN_TOTAL:
        raise ValueError(f"length must be >= {_MIN_TOTAL} (12 + {_MPDU_HDR})")

    mpdu_len = total_len - 12
    if mpdu_len > 65535:
        raise ValueError("MPDU length exceeds unsigned short")

    udp_payload_len = mpdu_len - _MPDU_HDR
    if udp_payload_len < 0:
        raise ValueError("internal: MPDU smaller than header stack")

    ip_udp = _build_ip_udp_headers(udp_payload_len)
    mpdu = _WLAN_LLC_BYTES + ip_udp
    if len(mpdu) != _MPDU_HDR:
        raise RuntimeError("MPDU header block must be 60 bytes")

    # Big-endian multi-byte fields: matches shell hex "12345678" and on-air order.
    hdr12 = struct.pack(">IBHBB", 0x12345678, data_rate, mpdu_len, tx_mode, queue)
    hdr12 += b"\x00\x00\x00"
    buf = hdr12 + mpdu
    return buf.hex(), total_len, mpdu_len, udp_payload_len


def _hex_to_bytes(hex_str):
    s = re.sub(r"\s+", "", hex_str.strip())
    if len(s) % 2:
        raise ValueError("hex string must have even length")
    return bytes.fromhex(s)


def _fmt_mac(b):
    return ":".join(f"{x:02x}" for x in b)


def dump_packet_hex(hex_str, out):
    """Human-readable decode of a raw TX hex blob (expects >= 12 B)."""
    data = _hex_to_bytes(hex_str)
    o = out.write

    o("=== raw_tx_pkt_header (12 bytes) ===\n")
    if len(data) < 12:
        o(f"(only {len(data)} byte(s); need 12 for full header)\n")
        o(f"hex: {data.hex()}\n")
        return

    if int.from_bytes(data[0:4], "big") == 0x12345678:
        magic, rate, plen, mode, q = struct.unpack_from(">IBHBB", data, 0)
        enc = "BE"
    elif int.from_bytes(data[0:4], "little") == 0x12345678:
        magic, rate, plen, mode, q = struct.unpack_from("<IBHBB", data, 0)
        enc = "LE (legacy)"
    else:
        o("magic:          not 0x12345678 (try valid raw_tx hex)\n")
        o(f"first 12 hex:   {data[:12].hex()}\n")
        return

    res = data[9:12]
    o(f"(raw_tx multi-byte fields: {enc})\n")
    o(f"magic_num:      0x{magic:08x}\n")
    o(f"data_rate:      {rate}\n")
    o(f"packet_length:  {plen} (MPDU bytes after this 12-byte header)\n")
    o(f"tx_mode:        {mode}\n")
    o(f"queue:          {q}\n")
    o(f"reserved[3]:    {res.hex()}\n")

    if len(data) < _MIN_TOTAL:
        o(f"\n(... truncated: {len(data)} bytes, need {_MIN_TOTAL} for WLAN+IP+UDP hdr)\n")
        return

    wlan = data[12 : 12 + 26]
    llc = data[38 : 38 + 8]
    ipb = data[46 : 46 + 20]
    udpb = data[66 : 66 + 8]

    o("\n=== 802.11 MAC header (26 bytes, QoS Data) ===\n")
    fc = int.from_bytes(wlan[0:2], "big")
    dur = int.from_bytes(wlan[2:4], "big")
    o(f"frame_control:  0x{fc:04x} (802.11 on-air byte order)\n")
    o(f"duration:       {dur} us\n")
    o(f"addr1 (RA):     {_fmt_mac(wlan[4:10])}\n")
    o(f"addr2 (TA):     {_fmt_mac(wlan[10:16])}\n")
    o(f"addr3:          {_fmt_mac(wlan[16:22])}\n")
    o(f"seq_ctrl:       0x{int.from_bytes(wlan[22:24], 'big'):04x}\n")
    o(f"qos_control:    0x{int.from_bytes(wlan[24:26], 'big'):04x}\n")

    o("\n=== LLC/SNAP (8 bytes) ===\n")
    o(f"dsap/ssap/ctl:  {llc[0:3].hex()}\n")
    o(f"oui + ethertype: {llc[3:8].hex()} (IPv4 if 08 00)\n")

    o("\n=== IPv4 (20 bytes) ===\n")
    ihl = (ipb[0] & 0x0F) * 4
    tot_len = int.from_bytes(ipb[2:4], "big")
    proto = ipb[9]
    csum = int.from_bytes(ipb[10:12], "big")
    src = ".".join(str(b) for b in ipb[12:16])
    dst = ".".join(str(b) for b in ipb[16:20])
    o(f"version_ihl:    0x{ipb[0]:02x} (IHL {ihl} bytes)\n")
    o(f"total_length:   {tot_len}\n")
    o(f"protocol:       {proto} ({'UDP' if proto == 17 else '?'})\n")
    o(f"checksum:       0x{csum:04x}\n")
    o(f"src:            {src}\n")
    o(f"dst:            {dst}\n")

    o("\n=== UDP (8 bytes) ===\n")
    sport = int.from_bytes(udpb[0:2], "big")
    dport = int.from_bytes(udpb[2:4], "big")
    ulen = int.from_bytes(udpb[4:6], "big")
    o(f"src_port:       {sport}\n")
    o(f"dst_port:       {dport}\n")
    o(f"length:         {ulen} (header + payload; payload padded by zperf)\n")
    o(f"checksum:       0x{int.from_bytes(udpb[6:8], 'big'):04x}\n")

    hdr_len = _MIN_TOTAL
    o("\n=== summary ===\n")
    o(f"hex buffer len: {len(data)} bytes (zperf hdr_len if full header only)\n")
    o(f"MPDU (packet_length): {plen} bytes\n")
    if len(data) == hdr_len:
        o(
            f"UDP payload (logical): {plen - _MPDU_HDR} bytes "
            f"(filled with 'z' by zperf when packet_size > {hdr_len})\n"
        )


def parse_args(argv=None):
    p = argparse.ArgumentParser(
        description=__doc__.strip(),
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    p.add_argument(
        "-D",
        "--decode",
        metavar="HEX",
        default=None,
        help="Decode HEX (whitespace ok) and print headers; no zperf line",
    )

    p.add_argument(
        "-l",
        "--length",
        type=int,
        default=_DEFAULT_LEN,
        metavar="N",
        help=(
            "Total zperf packet size in bytes (vendor 12 + MPDU). "
            "Default %(default)s. Ignored with --decode."
        ),
    )
    p.add_argument(
        "-r",
        "--data-rate",
        type=int,
        default=7,
        metavar="N",
        help="raw_tx data_rate (default: %(default)s)",
    )
    p.add_argument(
        "-m",
        "--tx-mode",
        type=int,
        default=0,
        metavar="N",
        help="raw_tx tx_mode (default: %(default)s)",
    )
    p.add_argument(
        "-Q",
        "--queue",
        type=int,
        default=0,
        metavar="N",
        help="raw_tx queue (default: %(default)s)",
    )
    p.add_argument(
        "-i",
        "--if-index",
        type=int,
        default=1,
        metavar="N",
        help="zperf interface index (default: %(default)s)",
    )
    p.add_argument(
        "-d",
        "--duration",
        type=int,
        default=10,
        metavar="SEC",
        help="zperf duration seconds (default: %(default)s)",
    )
    p.add_argument(
        "-R",
        "--rate",
        default="50M",
        metavar="KBPS",
        help="zperf rate (default: %(default)s)",
    )
    p.add_argument(
        "-x",
        "--hex-only",
        action="store_true",
        help="Print only the hex blob (no zperf command)",
    )
    p.add_argument(
        "-q",
        "--quiet",
        action="store_true",
        help="No stderr summary on generate",
    )

    p.epilog = (
        "Generate mode: all options are optional (defaults match typical tests). "
        "Decode mode: pass a hex string with -D/--decode (required for that mode)."
    )

    return p.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)

    if args.decode is not None:
        try:
            dump_packet_hex(args.decode, sys.stdout)
        except ValueError as e:
            print(f"error: {e}", file=sys.stderr)
            return 1
        return 0

    try:
        full_hex, total_len, mpdu_len, udp_pl = build_header_hex(
            args.length,
            data_rate=args.data_rate,
            tx_mode=args.tx_mode,
            queue=args.queue,
        )
    except ValueError as e:
        print(f"error: {e}", file=sys.stderr)
        return 1

    if args.hex_only:
        print(full_hex)
        return 0

    if not args.quiet:
        hb = len(full_hex) // 2
        sys.stderr.write(
            f"Header hex: {hb} bytes (zperf fills to length={total_len}; "
            f"MPDU={mpdu_len}, UDP payload logical={udp_pl})\n"
        )
        sys.stderr.write(
            "Set CONFIG_NET_ZPERF_RAW_TX_MAX_HDR_SIZE>=74 "
            "(Zephyr default 64 is too small for this 74-byte header).\n"
        )
        sys.stderr.write("-" * 40 + "\n")

    print(f"zperf raw upload {args.if_index} {full_hex} {args.duration} {total_len} {args.rate}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
