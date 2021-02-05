# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

from cx_Freeze import setup, Executable

setup(name="Bluetooth NUS Shell for nRF Connect SDK",
      description="BLE Console is a host-side application providing access to Shell over Bluetooth Low Energy in Nordic devices.",
      version="1.2",
      options={"build_exe": {"build_exe": "build",
                             "packages": ["gi"],
                             "include_files":
                             [("docs/nordic_semi_logo.png", "docs/nordic_semi_logo.png")],
                             }},
      executables=[Executable(script="BluetoothConsole.py",
                              targetName="BluetoothConsole",
                              )]
      )
