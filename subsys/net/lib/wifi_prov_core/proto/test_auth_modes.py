#!/usr/bin/env python3
"""
Test script to verify auth mode validation in generate_eap_tls_config.py

Copyright (c) 2025 Nordic Semiconductor ASA

SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
"""

import sys
import os

# Add current directory to path to import the script
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

try:
    from generate_wifi_prov_config import generate_wifi_config
except ImportError as e:
    print(f"Error importing generate_wifi_prov_config: {e}")
    print("Make sure to run 'make generate' first to create protobuf files")
    sys.exit(1)

def test_auth_mode_validation():
    """Test auth mode validation with various values."""

    print("Testing auth mode validation...")

    # Test valid auth modes
    valid_modes = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23]

    for mode in valid_modes:
        try:
            result = generate_wifi_config(
                ssid="TestWiFi",
                bssid="00:11:22:33:44:55",
                auth_mode=mode,
                passphrase="testpass"
            )
            if result is not None:
                print(f"✅ Auth mode {mode} is valid")
            else:
                print(f"❌ Auth mode {mode} returned None")
        except ValueError as e:
            print(f"❌ Auth mode {mode} failed: {e}")
        except Exception as e:
            print(f"❌ Auth mode {mode} unexpected error: {e}")

    # Test invalid auth modes
    invalid_modes = [-1, 24, 100, 999]

    for mode in invalid_modes:
        try:
            result = generate_wifi_config(
                ssid="TestWiFi",
                bssid="00:11:22:33:44:55",
                auth_mode=mode,
                passphrase="testpass"
            )
            print(f"❌ Auth mode {mode} should have failed but didn't")
        except ValueError as e:
            if "invalid enumerator" in str(e):
                print(f"✅ Auth mode {mode} correctly rejected: {e}")
            else:
                print(f"❌ Auth mode {mode} unexpected ValueError: {e}")
        except Exception as e:
            if "invalid enumerator" in str(e):
                print(f"✅ Auth mode {mode} correctly rejected: {e}")
            else:
                print(f"❌ Auth mode {mode} unexpected error: {e}")

if __name__ == "__main__":
    test_auth_mode_validation()
