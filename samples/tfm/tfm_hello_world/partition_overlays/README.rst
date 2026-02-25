.. _tfm_partition_overlays:

TF-M Partition Layout Examples
##############################

This directory contains example DTS overlay files that demonstrate different
TF-M partition layouts for various nRF SoC families and use cases.

These overlays can be used as a reference when creating custom partition layouts
for your own applications. They can also be applied to this sample using::

    west build -b <board>/ns -- -DDTC_OVERLAY_FILE=partition_overlays/<overlay>.overlay

.. important::

   Each SoC family has different TrustZone alignment requirements. All
   secure/non-secure partition boundaries must be aligned to the hardware
   granularity:

   .. list-table:: Alignment requirements per SoC family
      :header-rows: 1

      * - SoC Family
        - Flash Alignment
        - RAM Alignment
        - Enforcement
      * - nRF91 Series (nRF9160, nRF9161, nRF9151)
        - 32 kB
        - 8 kB
        - SPU
      * - nRF53 Series (nRF5340)
        - 16 kB
        - 8 kB
        - SPU
      * - nRF54L Series (nRF54L15)
        - 4 kB
        - 4 kB
        - MPC

Available Overlays
******************

nRF54L15 (4 kB alignment)
==========================

``nrf54l15_minimal.overlay``
   Minimal TF-M profile without MCUboot. No Protected Storage. Only ITS and
   OTP partitions. Maximizes non-secure application space (~1416 kB).

``nrf54l15_mcuboot.overlay``
   MCUboot + TF-M with DFU support. Includes primary and secondary slots,
   all TF-M storage partitions, and non-secure storage.

``nrf54l15_full.overlay``
   Full TF-M profile with MCUboot. All TF-M services enabled with generous
   partition sizes. Intended for development and prototyping.

``nrf54l15_bluetooth.overlay``
   Optimized for Bluetooth LE applications. No Protected Storage (bonds stored
   in ITS/NVS). Larger NS partition for BLE host stack. MCUboot for BLE DFU.

``nrf54l15_matter.overlay``
   Optimized for Matter (Connected Home over IP). Enlarged PS for DAC/PAI
   certificates. Enlarged ITS for fabric credentials. Includes factory data
   partition. MCUboot for Matter OTA.

nRF5340 (16 kB alignment)
===========================

``nrf5340_minimal.overlay``
   Minimal TF-M profile without MCUboot. ITS and OTP sized to 16 kB SPU
   regions. Demonstrates the coarser alignment impact on partition planning.

``nrf5340_full.overlay``
   Full TF-M with MCUboot following the upstream TF-M reference layout.
   Includes primary/secondary slots with secure and non-secure sub-regions.

``nrf5340_bluetooth.overlay``
   Optimized for BLE applications on the nRF5340 application core.
   Network core runs the BLE controller separately.

``nrf5340_matter.overlay``
   Optimized for Matter. Includes factory data partition. Notes on using
   external QSPI flash for the secondary DFU slot.

nRF91 Series (32 kB alignment)
================================

``nrf91_minimal.overlay``
   Minimal TF-M without MCUboot. Demonstrates the 32 kB alignment impact
   where ITS and OTP each consume a full 32 kB SPU region.

``nrf91_full.overlay``
   Full TF-M with MCUboot. All TF-M services with equal-sized primary
   and secondary slots for FOTA.

``nrf91_lte.overlay``
   Optimized for cellular IoT / LTE applications. Enlarged PS for TLS
   certificates. Larger NS storage for LwM2M/nRF Cloud state. Notes on
   modem library SRAM requirements.

Design Guidelines
*****************

When creating your own partition layouts:

1. **Alignment first**: Start by determining your S/NS boundaries based on the
   hardware alignment requirements. Round up partition sizes to the alignment
   granularity to avoid wasting flash.

2. **MCUboot secondary slot**: If using MCUboot, the secondary slot must be at
   least as large as the combined primary secure + non-secure partitions.

3. **TF-M storage placement**: On nRF54L Series, TF-M storage partitions (PS,
   ITS, OTP) are placed between the secure image and non-secure image. On
   nRF91/nRF53 Series, they are typically placed at the end of flash before
   non-secure storage.

4. **SRAM split**: The secure SRAM allocation depends on the TF-M profile. The
   minimal profile needs ~32 kB, while the full profile may need 88-128 kB.

5. **Erase block alignment**: All partition addresses should be aligned to the
   flash erase-block-size (4 kB for all nRF SoCs) regardless of the TrustZone
   alignment.
