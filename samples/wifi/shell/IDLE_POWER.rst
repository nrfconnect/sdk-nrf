.. _nrf7120_wifi_shell_idle_power:

nRF7120 Wi-Fi Shell - Idle Power Optimization
##############################################

Overview
********

This document describes power optimization techniques for the ``samples/wifi/shell``
sample on nRF7120 DK, enabling low idle current consumption while maintaining
Wi-Fi availability for scan and connection operations.

The optimizations are provided as composable configuration fragments that can be
selectively combined based on measurement and testing requirements.

Power Optimization Strategy
***************************

The idle power optimizations focus on several key areas:

1. **Tickless Kernel** - Reduces unnecessary system timer interrupts during idle
2. **Selective RAM Power-Down** - Keeps only essential RAM powered (256 KiB for Wi-Fi)
3. **PM Device Runtime** - Allows peripherals to enter low-power states
4. **Low Power Mode** - Uses variable-latency low-power CPU states
5. **Wi-Fi Specific Tuning** - Enables Wi-Fi radio power management while retaining connectivity

Expected Results
================

With idle power optimization enabled on nRF7120 DK:

* **Target Idle Current**: 2-5 µA (system ON, Wi-Fi active, idle)
* **Active Current**: ~30-50 mA (Wi-Fi scan/connected operations)
* **Wake Latency**: ~5-10 ms (variable latency mode)

Exact results depend on:
- External power supply stability
- Clock source (LFXO quality)
- HVBUCK regulator efficiency
- Retained RAM size and access patterns
- Wi-Fi firmware state

Build Configurations
********************

Multiple configuration files are provided for different use cases:

Base Idle Power
===============

**File**: ``configs/idle_power_base.conf``

Enables core power management features:
- PM device support
- Tickless kernel
- Low power mode selection
- System ON idle support

Usage::

    west build -b nrf7120dk/nrf7120/cpuapp -- \
      -DCONF_FILE="prj.conf;configs/idle_power_base.conf"

RAM Power Down
==============

**File**: ``configs/ram_power_down.conf``

Configures selective RAM power-down while retaining 256 KiB for:
- Network stack (TCP/UDP, DHCP)
- Wi-Fi driver state
- Packet buffers

Usage::

    west build -b nrf7120dk/nrf7120/cpuapp -- \
      -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/ram_power_down.conf"

Wi-Fi Idle Optimization
=======================

**File**: ``configs/wifi_idle_optimization.conf``

Wi-Fi specific power optimizations:
- AUTOCGCORE workaround (WZN-9779)
- Wi-Fi radio power management
- Network event queue optimization

Device Idle Configuration
=========================

**File**: ``configs/device_idle.conf``

Peripheral and device PM settings:
- Power domain control
- Clock gating in idle
- Device runtime PM

Diagnostic Configuration
========================

**File**: ``configs/diagnostics.conf``

Optional power measurement diagnostics:
- Prints power domain state
- GPIO signal routing (P0.10)
- HVBUCK status monitoring

Build with all optimizations::

    west build -b nrf7120dk/nrf7120/cpuapp -- \
      -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/ram_power_down.conf;configs/wifi_idle_optimization.conf;configs/device_idle.conf"

Device Tree Overlay
*******************

**File**: ``boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay``

Provides DTS configuration for idle power operation:
- Low-power clock source (LFXO)
- GPIO idle phase marker (P0.00)
- Power domain and regulator configuration
- GRTC timing support during idle
- Wi-Fi radio power control

To use with the overlay::

    west build -b nrf7120dk/nrf7120/cpuapp -- \
      -DDTC_OVERLAY_FILE="boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay"

Complete Example Build
**********************

Build with all idle power optimizations::

    west build -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- \
      -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/ram_power_down.conf;configs/wifi_idle_optimization.conf;configs/device_idle.conf" \
      -DDTC_OVERLAY_FILE="boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay"

Build with diagnostics for measurement::

    west build -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- \
      -DCONF_FILE="prj.conf;configs/idle_power_base.conf;configs/ram_power_down.conf;configs/wifi_idle_optimization.conf;configs/device_idle.conf;configs/diagnostics.conf" \
      -DDTC_OVERLAY_FILE="boards/nrf7120dk_nrf7120_cpuapp_idle_power.overlay"

Hardware Connections for Measurement
************************************

**P0.00 - Idle Phase Marker**
   Connect to oscilloscope/logic analyzer channel 1
   - High when CPU is running
   - Low when CPU is in WFI (idle)
   - Directly correlates with power consumption

**P0.10 - Power Domain Signal**
   Connect to oscilloscope/logic analyzer channel 2
   Reflects selected power domain status (configured via CONFIG_NRF7120_PDSELECT_SIGNAL):
   - 1: PD_MAIN
   - 3: PD_PERIPH
   - 4: PD_MCU
   - 5: PD_RADIO
   - 6: PD_CRACEN/PD_MCU_AUX
   - 7: PD_WIFI
   - 8: PwrAboveElv
   - 11: InLpMode

Measurement Procedure
*********************

1. **Connect PPK2 or equivalent power measurement instrument**
   - Measure between VUSB and GND or battery supply

2. **Connect logic analyzer (optional but recommended)**
   - P0.00: Idle phase marker
   - P0.10: Selected power domain signal
   - Reference clock output

3. **Flash the sample with idle power configs**::

       west flash

4. **Allow system to reach steady idle state**
   - Device may take 1-2 seconds to reach minimum idle current
   - Observe on oscilloscope/PPK2

5. **Perform Wi-Fi operations from shell**
   - Scan: ``wifi scan``
   - Connect: ``wifi connect <ssid> <psk>``
   - Disconnect: ``wifi disconnect``
   - Return to idle after operations complete

6. **Measure current consumption**
   - Idle: observe minimum current during WFI
   - Active: observe current during scan/connect
   - Calculate power: P = V × I (typical V = 3.3V)

Troubleshooting
***************

Wi-Fi Operations Fail
======================

If Wi-Fi scan/connect fails after enabling idle power configs:

1. Increase retained RAM size in ``configs/ram_power_down.conf``::

      CONFIG_NRF7120_RAM_384K_ONLY=y

2. Disable RAM power down temporarily::

      # Comment out: -include ../configs/ram_power_down.conf

3. Verify Wi-Fi firmware is properly initialized::

      wifi status
      wifi version

Idle Current Not Improving
============================

1. Check CONFIG_NRF7120_FORCE_LOWPWR is enabled
2. Verify LFXO is running (check LFXO->STATUS in diagnostics)
3. Ensure tickless kernel is enabled
4. Check PM device runtime is working::

      device info

Crashes During Idle
====================

1. Disable RAM power down (CONFIG_NRF7120_RAM_256K_ONLY)
2. Verify retained RAM is sufficient for application
3. Check stack sizes in ``prj.conf``
4. Review system crash dump if available

GPIO Signals Not Appearing
===========================

1. Verify P0.00 and P0.10 are available on your DK
2. Check logic analyzer connections
3. Enable diagnostics config to ensure driver initialization
4. Verify GPIO peripheral is powered in idle

Configuration Tips
*******************

For Minimum Idle Current
=========================

1. Use ``configs/quiet.conf`` - disables UART/console output
2. Increase retained RAM to minimum required (adjust ``CONFIG_NRF7120_RAM_*K_ONLY``)
3. Enable all optimizations including device_idle.conf
4. Use LFXO instead of RC oscillator for timing

For Development/Debugging
==========================

1. Keep UART enabled (don't use quiet.conf)
2. Enable diagnostics.conf for power state monitoring
3. Use PPK2 or logic analyzer to observe transitions
4. Keep initial idle power configs minimal, add incrementally

For Production
==============

1. Combine idle_power_base + ram_power_down + wifi_idle_optimization
2. Use device tree overlay for proper clock/regulator configuration
3. Disable diagnostics (diagnostics.conf has overhead)
4. Verify Wi-Fi operations work at target temperature/voltage ranges
5. Measure current at representative operating conditions

References
**********

- `System ON Idle Sample <../zephyr/boards/nordic/system_on_idle/README.rst>`__
- nRF7120 Product Brief
- nRF7120 Hardware Design Guide
- `PM Device Documentation <https://docs.zephyrproject.org/latest/reference/power_management/device.html>`__

Wi-Fi Specific Power Considerations
***********************************

Radio Power States
===================

- **Idle**: Wi-Fi radio off, device responds to wake events
- **Active Scan**: Radio transmitting probe requests (~100 mA peak)
- **Connected**: Radio in power save mode (~50 mA average)
- **Deep Sleep**: Radio in extended power save (few mA)

Recommended Wi-Fi Power Settings::

    # Enable Wi-Fi power management
    CONFIG_WIFI_NM_WPA_SUPPLICANT_PM_ENABLED=y

    # Set power save mode after connection
    wifi power_save on

    # Set power save timeout (default ~100ms)
    wifi ps_timeout 100

Measuring Wi-Fi Operations Power Impact
========================================

Baseline (system idle)::

    Current: ~3 µA
    State: WFI, all domains powered down except AO/MAIN

Scan operation::

    Current during scan: ~20-50 mA peak
    Duration: 100-500 ms per scan
    Recovery to idle: ~100-200 ms

Connected (power save enabled)::

    Current average: ~10-20 mA
    Periodic wake-up: every ~100 ms for beacon monitoring
    Idle within wake interval: ~3-5 µA

