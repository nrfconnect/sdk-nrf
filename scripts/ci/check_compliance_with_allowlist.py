#!/usr/bin/env python3
"""
Wrapper script to add custom Kconfig symbols to allowlist before running compliance check.
This avoids modifying the Zephyr check_compliance.py script directly.
"""

import sys
import os

# Add Zephyr scripts to path
ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
if not ZEPHYR_BASE:
    print("ERROR: ZEPHYR_BASE environment variable not set")
    sys.exit(1)

sys.path.insert(0, os.path.join(ZEPHYR_BASE, 'scripts', 'ci'))

# Import the check_compliance module
import check_compliance

# Add custom Kconfig symbols to the allowlist
# These symbols are defined in dragoon (proprietary) modules
DRAGOON_KCONFIG_SYMBOLS = {
    "BT_LL_SOFTDEVICE_BUILD_TYPE_SRC_DEBUG",  # Defined in dragoon modules (build-from-source)
    "MPSL_BUILD_TYPE_SRC_DEBUG",              # Defined in dragoon modules (build-from-source)
    "NRF_802154_SOURCE_INTERNAL",             # Defined in dragoon modules (build-from-source)
}

# Patch the KconfigCheck class to include our custom symbols
original_kconfig_allowlist = check_compliance.KconfigCheck.UNDEF_KCONFIG_ALLOWLIST
check_compliance.KconfigCheck.UNDEF_KCONFIG_ALLOWLIST = original_kconfig_allowlist | DRAGOON_KCONFIG_SYMBOLS

# Also patch SysbuildKconfigCheck if it exists
if hasattr(check_compliance, 'SysbuildKconfigCheck'):
    original_sysbuild_allowlist = check_compliance.SysbuildKconfigCheck.UNDEF_KCONFIG_ALLOWLIST
    check_compliance.SysbuildKconfigCheck.UNDEF_KCONFIG_ALLOWLIST = original_sysbuild_allowlist | DRAGOON_KCONFIG_SYMBOLS

print(f"✓ Added {len(DRAGOON_KCONFIG_SYMBOLS)} custom Kconfig symbols to allowlist")
print(f"  Symbols: {', '.join(sorted(DRAGOON_KCONFIG_SYMBOLS))}")
print()

# Run the main compliance check with all original arguments
check_compliance.main(sys.argv[1:])
