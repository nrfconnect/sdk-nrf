"""
BICR Generation Tool for nRF92
------------------------------

Generate or decode a BICR (Board Information Configuration Register) image
from JSON or Intel HEX, using the nRF9220 application SVD.

The JSON configuration covers the board-specific settings: the P1, P2 and P5
port supplies and the P5 drive impedance. The LFOSC and HFXO64M registers are
always written with the fixed nRF9251 SiP configuration. All other registers
are left erased.

Copyright (c) 2026 Nordic Semiconductor ASA
SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import argparse
import json
import struct
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass, field
from enum import Enum
from pathlib import Path
from pprint import pprint

from intelhex import IntelHex


class Register:
    def __init__(self, regs: ET.Element, name: str, data: bytes | bytearray | None = None) -> None:
        cluster_name, reg_name = name.split(".")

        cluster = regs.find(f".//registers/cluster[name='{cluster_name}']")
        self._offset = int(cluster.find("addressOffset").text, 0)

        self._reg = cluster.find(f".//register[name='{reg_name}']")
        self._offset += int(self._reg.find("addressOffset").text, 0)
        self._size = int(self._reg.find("size").text, 0) // 8

        self._data = data

    @property
    def offset(self) -> int:
        return self._offset

    def _msk_pos(self, name: str) -> tuple[int, int]:
        field = self._reg.find(f".//fields/field[name='{name}']")
        field_lsb = int(field.find("lsb").text, 0)
        field_msb = int(field.find("msb").text, 0)

        mask = (0xFFFFFFFF - (1 << field_lsb) + 1) & (0xFFFFFFFF >> (31 - field_msb))

        return mask, field_lsb

    def _enums(self, field: str) -> list[ET.Element]:
        return self._reg.findall(
            f".//fields/field[name='{field}']/enumeratedValues/enumeratedValue"
        )

    def __getitem__(self, field: str) -> int:
        if not self._data:
            raise TypeError("Empty register")

        msk, pos = self._msk_pos(field)
        raw = struct.unpack("<I", self._data[self._offset : self._offset + 4])[0]
        return (raw & msk) >> pos

    def __setitem__(self, field: str, value: int) -> None:
        if not isinstance(self._data, bytearray):
            raise TypeError("Register is read-only")

        msk, pos = self._msk_pos(field)
        raw = struct.unpack("<I", self._data[self._offset : self._offset + 4])[0]
        raw &= ~msk
        raw |= (value << pos) & msk
        self._data[self._offset : self._offset + 4] = struct.pack("<I", raw)

    def enum_get(self, field: str) -> str:
        value = self[field]
        for enum in self._enums(field):
            if value == int(enum.find("value").text, 0):
                return enum.find("name").text

        raise ValueError(f"Invalid enum value for {field}: {value}")

    def enum_set(self, field: str, value: str) -> None:
        for enum in self._enums(field):
            if value == enum.find("name").text:
                self[field] = int(enum.find("value").text, 0)
                return

        raise ValueError(f"Invalid enum value for {field}: {value}")


class IoPortPower(Enum):
    UNCONFIGURED = "Unconfigured"
    DISCONNECTED = "Disconnected"
    EXTERNAL = "External"
    SW_EXT_HIGH = "SwExtHigh"
    SW_EXT_LOW = "SwExtLow"


@dataclass
class IoPortPowerConfig:
    p1_supply: IoPortPower
    p2_supply: IoPortPower
    p5_supply: IoPortPower

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "IoPortPowerConfig":
        ioport_power0 = Register(bicr_spec, "IOPORT.POWER0", data)

        return cls(
            p1_supply=IoPortPower(ioport_power0.enum_get("P1")),
            p2_supply=IoPortPower(ioport_power0.enum_get("P2")),
            p5_supply=IoPortPower(ioport_power0.enum_get("P5")),
        )

    @classmethod
    def from_json(cls, data: dict) -> "IoPortPowerConfig":
        ioport_power = data["ioPortPower"]

        return cls(
            p1_supply=IoPortPower[ioport_power["p1Supply"]],
            p2_supply=IoPortPower[ioport_power["p2Supply"]],
            p5_supply=IoPortPower[ioport_power["p5Supply"]],
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        ioport_power0 = Register(bicr_spec, "IOPORT.POWER0", buf)

        ioport_power0.enum_set("P1", self.p1_supply.value)
        ioport_power0.enum_set("P2", self.p2_supply.value)
        ioport_power0.enum_set("P5", self.p5_supply.value)

    def to_json(self, buf: dict) -> None:
        buf["ioPortPower"] = {
            "p1Supply": self.p1_supply.name,
            "p2Supply": self.p2_supply.name,
            "p5Supply": self.p5_supply.name,
        }


@dataclass
class IoPortImpedanceConfig:
    p5_impedance_ohms: int

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "IoPortImpedanceConfig":
        drivectl0 = Register(bicr_spec, "IOPORT.DRIVECTRL0", data)

        return cls(p5_impedance_ohms=int(drivectl0.enum_get("P5")[4:]))

    @classmethod
    def from_json(cls, data: dict) -> "IoPortImpedanceConfig":
        return cls(p5_impedance_ohms=data["ioPortImpedance"]["p5ImpedanceOhms"])

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        drivectl0 = Register(bicr_spec, "IOPORT.DRIVECTRL0", buf)

        drivectl0.enum_set("P5", f"Ohms{self.p5_impedance_ohms}")

    def to_json(self, buf: dict) -> None:
        buf["ioPortImpedance"] = {"p5ImpedanceOhms": self.p5_impedance_ohms}


class LFXOMode(Enum):
    PIERCE = "Pierce"
    PIXO = "PIXO"
    EXT_SINE = "ExtSine"
    EXT_SQUARE = "ExtSquare"


@dataclass
class LFXOConfig:
    accuracy_ppm: int
    mode: LFXOMode
    builtin_load_capacitors: bool
    builtin_load_capacitance_pf: int | None
    startup_time_ms: int

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "LFXOConfig":
        lfosc_lfxoconfig = Register(bicr_spec, "LFOSC.LFXOCONFIG", data)

        try:
            loadcap = lfosc_lfxoconfig.enum_get("LOADCAP")
        except ValueError:
            builtin_load_capacitors = True
            builtin_load_capacitance_pf = lfosc_lfxoconfig["LOADCAP"]
        else:
            if loadcap == "Unconfigured":
                raise ValueError("Invalid LFXO load capacitors configuration")

            builtin_load_capacitors = False
            builtin_load_capacitance_pf = None

        startup_time_ms = lfosc_lfxoconfig["TIME"]
        try:
            lfosc_lfxoconfig.enum_get("TIME")
        except ValueError:
            pass
        else:
            raise ValueError("Invalid LFXO startup time (not configured)")

        return cls(
            accuracy_ppm=int(lfosc_lfxoconfig.enum_get("ACCURACY")[:-3]),
            mode=LFXOMode(lfosc_lfxoconfig.enum_get("MODE")),
            builtin_load_capacitors=builtin_load_capacitors,
            builtin_load_capacitance_pf=builtin_load_capacitance_pf,
            startup_time_ms=startup_time_ms,
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        lfosc_lfxoconfig = Register(bicr_spec, "LFOSC.LFXOCONFIG", buf)

        lfosc_lfxoconfig.enum_set("ACCURACY", f"{self.accuracy_ppm}ppm")
        lfosc_lfxoconfig.enum_set("MODE", self.mode.value)
        lfosc_lfxoconfig["TIME"] = self.startup_time_ms

        if self.builtin_load_capacitors:
            lfosc_lfxoconfig["LOADCAP"] = self.builtin_load_capacitance_pf
        else:
            lfosc_lfxoconfig.enum_set("LOADCAP", "External")


@dataclass
class LFRCCalibrationConfig:
    calibration_enabled: bool
    temp_meas_interval_seconds: float | None = None
    temp_delta_calibration_trigger_celsius: float | None = None
    max_meas_interval_between_calibrations: int | None = None

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "LFRCCalibrationConfig":
        lfosc_lfrcautocalconfig = Register(bicr_spec, "LFOSC.LFRCAUTOCALCONFIG", data)

        if lfosc_lfrcautocalconfig.enum_get("ENABLE") != "Enabled":
            return cls(calibration_enabled=False)

        return cls(
            calibration_enabled=True,
            temp_meas_interval_seconds=lfosc_lfrcautocalconfig["TEMPINTERVAL"] * 0.25,
            temp_delta_calibration_trigger_celsius=lfosc_lfrcautocalconfig["TEMPDELTA"] * 0.25,
            max_meas_interval_between_calibrations=lfosc_lfrcautocalconfig["INTERVALMAXNO"],
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        lfosc_lfrcautocalconfig = Register(bicr_spec, "LFOSC.LFRCAUTOCALCONFIG", buf)

        lfosc_lfrcautocalconfig.enum_set(
            "ENABLE", "Enabled" if self.calibration_enabled else "Disabled"
        )
        if self.calibration_enabled:
            lfosc_lfrcautocalconfig["TEMPINTERVAL"] = int(self.temp_meas_interval_seconds / 0.25)
            lfosc_lfrcautocalconfig["TEMPDELTA"] = int(
                self.temp_delta_calibration_trigger_celsius / 0.25
            )
            lfosc_lfrcautocalconfig["INTERVALMAXNO"] = self.max_meas_interval_between_calibrations


class LFOSCSource(Enum):
    LFXO = "LFXO"
    LFRC = "LFRC"
    LFLPRC = "LFLPRC"
    SYNTH = "Synth"


@dataclass
class LFOSCConfig:
    source: LFOSCSource
    lfxo: LFXOConfig
    lfrccal: LFRCCalibrationConfig
    lfxo_calibrate: bool = field(compare=False)

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "LFOSCConfig":
        lfosc_config = Register(bicr_spec, "LFOSC.CONFIG", data)
        lfosc_lfxocal = Register(bicr_spec, "LFOSC.LFXOCAL", data)

        return cls(
            source=LFOSCSource(lfosc_config.enum_get("SRC")),
            lfxo=LFXOConfig.from_raw(bicr_spec, data),
            lfrccal=LFRCCalibrationConfig.from_raw(bicr_spec, data),
            lfxo_calibrate=lfosc_lfxocal["LFXOCAL"] == 0xFFFFFFFF,
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        lfosc_config = Register(bicr_spec, "LFOSC.CONFIG", buf)
        lfosc_lfxocal = Register(bicr_spec, "LFOSC.LFXOCAL", buf)

        lfosc_config.enum_set("SRC", self.source.value)
        self.lfxo.to_raw(bicr_spec, buf)
        self.lfrccal.to_raw(bicr_spec, buf)

        if self.lfxo_calibrate:
            lfosc_lfxocal.enum_set("LFXOCAL", "Calibrate")


class HFXOMode(Enum):
    NORMAL = "Normal"
    TCXO = "TCXO"


@dataclass
class HFXOConfig:
    mode: HFXOMode
    startup_time_us: int

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "HFXOConfig":
        hfxo_config = Register(bicr_spec, "HFXO64M.CONFIG", data)
        hfxo_startuptime = Register(bicr_spec, "HFXO64M.STARTUPTIME", data)

        startup_time_us = hfxo_startuptime["TIME"]
        try:
            hfxo_startuptime.enum_get("TIME")
        except ValueError:
            pass
        else:
            raise ValueError("Invalid HFXO startup time (not configured)")

        return cls(mode=HFXOMode(hfxo_config.enum_get("MODE")), startup_time_us=startup_time_us)

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        hfxo_config = Register(bicr_spec, "HFXO64M.CONFIG", buf)
        hfxo_startuptime = Register(bicr_spec, "HFXO64M.STARTUPTIME", buf)

        hfxo_config.enum_set("MODE", self.mode.value)
        hfxo_startuptime["TIME"] = self.startup_time_us


@dataclass
class SiPConfig:
    lfosc: LFOSCConfig
    hfxo: HFXOConfig

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "SiPConfig":
        return cls(
            lfosc=LFOSCConfig.from_raw(bicr_spec, data),
            hfxo=HFXOConfig.from_raw(bicr_spec, data),
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        self.lfosc.to_raw(bicr_spec, buf)
        self.hfxo.to_raw(bicr_spec, buf)


NRF9251_SIP_CONFIG = SiPConfig(
    lfosc=LFOSCConfig(
        source=LFOSCSource.LFXO,
        lfxo=LFXOConfig(
            accuracy_ppm=20,
            mode=LFXOMode.PIERCE,
            builtin_load_capacitors=False,
            builtin_load_capacitance_pf=None,
            startup_time_ms=500,
        ),
        lfrccal=LFRCCalibrationConfig(calibration_enabled=False),
        lfxo_calibrate=True,
    ),
    hfxo=HFXOConfig(mode=HFXOMode.NORMAL, startup_time_us=1000),
)


@dataclass
class BICR:
    ioport_power: IoPortPowerConfig
    ioport_impedance: IoPortImpedanceConfig

    @classmethod
    def from_raw(cls, bicr_spec: ET.Element, data: bytes) -> "BICR":
        return cls(
            ioport_power=IoPortPowerConfig.from_raw(bicr_spec, data),
            ioport_impedance=IoPortImpedanceConfig.from_raw(bicr_spec, data),
        )

    @classmethod
    def from_json(cls, data: dict) -> "BICR":
        return cls(
            ioport_power=IoPortPowerConfig.from_json(data),
            ioport_impedance=IoPortImpedanceConfig.from_json(data),
        )

    def to_raw(self, bicr_spec: ET.Element, buf: bytearray) -> None:
        self.ioport_power.to_raw(bicr_spec, buf)
        self.ioport_impedance.to_raw(bicr_spec, buf)
        NRF9251_SIP_CONFIG.to_raw(bicr_spec, buf)

    def to_json(self, buf: dict) -> None:
        self.ioport_power.to_json(buf)
        self.ioport_impedance.to_json(buf)


BICR_SIZE = 0x3C


def check_sip_config(bicr_spec: ET.Element, data: bytes) -> None:
    try:
        sip_config = SiPConfig.from_raw(bicr_spec, data)
    except ValueError as e:
        print(f"WARNING: invalid SiP oscillator configuration: {e}")
        return

    pprint(sip_config)

    if sip_config != NRF9251_SIP_CONFIG:
        print("WARNING: SiP oscillator configuration differs from the fixed nRF9251 values")


def load_bicr_spec(svd_file: Path) -> ET.Element:
    return ET.parse(svd_file).getroot().find(".//peripheral[name='BICR_NS']")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("-i", "--input", type=Path, required=True, help="Input file")
    parser.add_argument("-s", "--svd", type=Path, help="SVD file")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("-o", "--output", type=Path, help="Output file")
    group.add_argument("-l", "--list", action="store_true", help="List BICR options")
    args = parser.parse_args()

    def is_hex_path(path: Path) -> bool:
        return path.suffix in (".hex", ".ihex")

    if not args.svd and (is_hex_path(args.input) or (args.output and is_hex_path(args.output))):
        sys.exit("SVD file is required for hex files")

    bicr_spec = load_bicr_spec(args.svd) if args.svd else None

    raw = None
    if is_hex_path(args.input):
        ih = IntelHex()
        ih.loadhex(args.input)
        raw = ih.tobinstr()[:BICR_SIZE]

        bicr = BICR.from_raw(bicr_spec, raw)
    elif args.input.suffix == ".json":
        with open(args.input) as f:
            data = json.load(f)
        bicr = BICR.from_json(data)
    else:
        sys.exit("Unsupported input file format")

    if args.output:
        if is_hex_path(args.output):
            bicr_address = int(bicr_spec.find("baseAddress").text, 0)

            buf = bytearray([0xFF] * BICR_SIZE)
            bicr.to_raw(bicr_spec, buf)

            ih = IntelHex()
            ih.frombytes(buf, offset=bicr_address)
            ih.tofile(args.output, format="hex")
        elif args.output.suffix == ".json":
            out = {}
            bicr.to_json(out)

            with open(args.output, "w") as f:
                json.dump(out, f, indent=4)
        else:
            sys.exit("Unsupported output file format")
    elif args.list:
        pprint(bicr)
        if raw is not None:
            check_sip_config(bicr_spec, raw)
