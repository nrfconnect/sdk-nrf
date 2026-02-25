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

``nrf54l15_thread.overlay``
   Optimized for OpenThread (802.15.4). Sized for Full Thread Devices (FTD)
   with DTLS commissioning crypto. Enlarged NVS for Thread operational
   dataset and network settings.

``nrf54l15_crypto_tls.overlay``
   Optimized for TLS/DTLS-heavy applications (cloud connectivity). Enlarged
   PS (32 kB) for X.509 certificate chains. Enlarged ITS (32 kB) for TLS
   session keys and PSKs. More secure SRAM for concurrent crypto operations.

``nrf54l15_isolation_lvl2.overlay``
   TF-M Isolation Level 2 layout. Larger secure image (384 kB) for IPC model
   overhead. More secure SRAM for per-partition stacks. Requires
   ``CONFIG_TFM_ISOLATION_LEVEL=2`` and ``CONFIG_TFM_IPC=y``.

``nrf54l15_max_app_space.overlay``
   Absolute minimum TF-M to maximize non-secure application space (~1460 kB
   = 95.7% of RRAM). TF-M minimal profile in SFN mode, no MCUboot, no PS,
   smallest viable ITS. Answers: "How do I get the most flash for my app?"

``nrf54l15_debug.overlay``
   Debug-friendly layout with extra secure flash (+64 kB) and SRAM (+24 kB)
   for TF-M debug builds. Prevents overflow when ``CONFIG_DEBUG_OPTIMIZATIONS``
   is enabled. Switch to a release overlay when debugging is done.

``nrf54l15_split_image_update.overlay``
   Separate primary/secondary slot pairs for TF-M and NS app. Update each
   independently — NS app updates are 37% smaller since TF-M is excluded.
   Four slots total (S primary/secondary + NS primary/secondary).

``nrf54l15_mcuboot_overwrite.overlay``
   MCUboot overwrite-only mode (``SB_CONFIG_MCUBOOT_MODE_OVERWRITE_ONLY=y``).
   The secondary slot can be *smaller* than the primary since no swap buffer
   is needed. Gains ~192 kB of NS application space compared to swap mode.
   Trade-off: no rollback on power loss during update.

``nrf54l15_nsib_mcuboot.overlay``
   Full secure boot chain: NSIB (b0) → MCUboot → TF-M → NS app.
   Includes immutable bootloader partition, provisioning data for public
   keys and monotonic counters, and hardware downgrade prevention.
   Required for PSA Certified products.

``nrf54l15_direct_xip.overlay``
   Direct-XIP (A/B) update mode. Both Slot A and Slot B are executable
   in place — MCUboot selects the slot with the highest version. No image
   copy at boot means faster boot time. Both slots must be the same size.
   Supports revert with ``SB_CONFIG_MCUBOOT_MODE_DIRECT_XIP_WITH_REVERT=y``.

nRF54LV10A (4 kB alignment, 1012 kB RRAM, 192 kB SRAM)
========================================================

``nrf54lv10a_minimal.overlay``
   Minimal TF-M profile without MCUboot. Demonstrates partition planning
   with tighter memory constraints compared to nRF54L15.

``nrf54lv10a_mcuboot.overlay``
   MCUboot + TF-M. Shows how to fit primary and secondary DFU slots within
   the smaller 1012 kB RRAM. Symmetric slot layout for MCUboot compatibility.

``nrf54lv10a_full.overlay``
   Full TF-M profile with MCUboot and all TF-M storage partitions. Tighter
   trade-offs compared to nRF54L15 due to less RRAM.

``nrf54lv10a_bluetooth.overlay``
   Optimized for BLE. No Protected Storage, enlarged NS partition for the
   BLE stack within the constrained 1012 kB RRAM.

``nrf54lv10a_matter.overlay``
   Matter on the nRF54LV10A is tight. Includes a warning that 200 kB NS
   may be insufficient for full Matter and suggests external flash for the
   secondary slot.

``nrf54lv10a_thread.overlay``
   Optimized for OpenThread. Sized for Full Thread Devices (FTD) with
   networking and NVS storage within 1012 kB.

``nrf54lv10a_crypto_tls.overlay``
   TLS/DTLS-heavy with enlarged PS and ITS for certificate and key storage.

nRF54LM20A (4 kB alignment, 2036 kB RRAM, 508 kB SRAM)
=========================================================

``nrf54lm20a_minimal.overlay``
   Minimal TF-M without MCUboot. The largest nRF54L-family SoC — leaves
   ~1916 kB for the non-secure application.

``nrf54lm20a_mcuboot.overlay``
   MCUboot + TF-M with generous DFU slots. 576 kB NS application + 896 kB
   symmetric primary/secondary slots.

``nrf54lm20a_full.overlay``
   Full TF-M profile with all services, MCUboot, and 512 kB NS application.
   The 2 MB RRAM comfortably fits everything internally.

``nrf54lm20a_bluetooth.overlay``
   Optimized for BLE with 768 kB NS partition — room for complex BLE
   applications including audio and mesh.

``nrf54lm20a_matter.overlay``
   Optimized for Matter with 512 kB NS, enlarged PS/ITS, factory data, and
   832 kB DFU slot — all fitting internally without external flash.

``nrf54lm20a_thread.overlay``
   Optimized for OpenThread FTD with 640 kB NS for the full Thread stack
   and 64 kB NVS for Thread settings persistence.

``nrf54lm20a_crypto_tls.overlay``
   TLS/DTLS-heavy with 32 kB each for PS and ITS, 448 kB NS application,
   and 768 kB secondary slot for secure firmware updates.

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

``nrf5340_extflash_dfu.overlay``
   Secondary DFU slot on external QSPI flash (MX25R64). Frees all internal
   flash for the primary image, giving a much larger NS application partition
   (592 kB vs 192 kB). Recommended for production devices with FOTA.

``nrf5340_netcore_dfu.overlay``
   Application core layout with a network core update staging area. MCUboot
   on the app core manages updates for both cores. The ``image-2`` partition
   holds the cpunet firmware update image. Required for products that need
   to update the BLE controller or 802.15.4 radio stack in the field.

``nrf5340_shared_sram.overlay``
   Explicit 3-way SRAM split: secure (TF-M) + non-secure (app) + shared
   (IPC to network core). Includes an ASCII diagram and explains the sizing
   trade-offs. Answers: "Where did my SRAM go?"

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

``nrf91_lwm2m_carrier.overlay``
   Optimized for the LwM2M carrier library. Includes a dedicated carrier
   storage partition (16 kB). Sized for carrier-initiated FOTA. Notes on
   modem library and carrier library SRAM requirements.

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

Migration from pm_static.yml
*****************************

If you are migrating from Partition Manager (``pm_static.yml``) to DTS-based
partition definitions, see :ref:`tfm_pm_to_dts_migration` in
``MIGRATION_GUIDE.rst`` for a step-by-step translation guide with examples.
