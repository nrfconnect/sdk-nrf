#!/usr/bin/env python3
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

"""Check the real G.722 selection dependencies with isolated prerequisite symbols."""

import tempfile
import unittest
from pathlib import Path

import kconfiglib


class G722KconfigTest(unittest.TestCase):
    """@brief Evaluate the actual application codec dependencies in isolation."""

    def test_converter_width_and_rate(self):
        """@brief Check codec visibility and selection for each rate/SRC-width case."""
        root = Path(__file__).resolve().parents[3]
        source = (root / "applications/nrf_audio/src/audio/Kconfig").read_text()
        stanza = (
            "config SW_CODEC_G722\n"
            + source.split("config SW_CODEC_G722\n", 1)[1].split("\nconfig ", 1)[0]
        )

        prerequisites = (
            "TRANSPORT_CIS",
            "AUDIO_FRAME_DURATION_10_MS",
            "AUDIO_BIT_DEPTH_16",
            "AUDIO_SAMPLE_RATE_16000_HZ",
            "AUDIO_SAMPLE_RATE_48000_HZ",
            "SAMPLE_RATE_CONVERTER",
            "SAMPLE_RATE_CONVERTER_BIT_DEPTH_16",
        )

        cases = (
            (48000, True, 16, True),
            (48000, True, 32, False),
            (48000, False, 16, False),
            (16000, False, 16, True),
            (16000, True, 16, True),
            (16000, True, 32, True),
        )

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "Kconfig"
            path.write_text(
                "\n".join(f'config {name}\n\tbool "{name}"\n' for name in prerequisites) + stanza
            )

            for rate, converter, width, allowed in cases:
                with self.subTest(rate=rate, converter=converter, width=width):
                    config = kconfiglib.Kconfig(str(path), warn=False)

                    for name in prerequisites[:3]:
                        config.syms[name].set_value("y")

                    config.syms[f"AUDIO_SAMPLE_RATE_{rate}_HZ"].set_value("y")
                    config.syms["SAMPLE_RATE_CONVERTER"].set_value("y" if converter else "n")
                    config.syms["SAMPLE_RATE_CONVERTER_BIT_DEPTH_16"].set_value(
                        "y" if width == 16 else "n"
                    )

                    codec = config.syms["SW_CODEC_G722"]
                    self.assertEqual(codec.visibility, 2 if allowed else 0)
                    codec.set_value("y")
                    self.assertEqual(codec.str_value, "y" if allowed else "n")


if __name__ == "__main__":
    unittest.main()
