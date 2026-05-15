.. _ug_bootloader_nrf54l_memory_protection:

nRF54L Series bootloader RRAM protection
#########################################

.. contents::
   :local:
   :depth: 2

On the nRF54L Series, the immutable bootloader and (when used) MCUboot execute from RRAM.
The RRAM controller (RRAMC) can enforce access rules per region so that bootloader memory stays intact and, after the boot chain hands off to the application, is not usable for code-reuse attacks.

This page describes how those protections fit together in |NCS|, in the order they affect the running system: from chip reset through the bootloaders to the application.
For build commands, flashing, and signature provisioning on nRF54L devices, see also :ref:`ug_nrf54l_dfu_config` and :ref:`ug_bootloader_adding_sysbuild`.

Hardware background
*******************

The nRF54L15 Product Specification documents how RRAMC maps **boot configuration** to locked regions and how the **UICR BOOTCONF** register supplies part of that configuration at reset.
See `nRF54L15 RRAMC boot configuration regions`_ and `nRF54L15 UICR BOOTCONF register`_ for the authoritative hardware description.
Other nRF54L Series SoCs use the same overall mechanism with different maximum region sizes; the build-generated :file:`bootconf.hex` file accounts for the selected SoC.

.. _ug_bootloader_nrf54l_memory_protection_reset:

1. At reset, before bootloader software runs
=============================================

When you use |NSIB| as the first-stage bootloader, the build can produce a small Intel HEX file named :file:`bootconf.hex`.
This image programs the **UICR BOOTCONF** value so that RRAMC applies an **immutable boot region** as soon as the device leaves reset, before any of your bootloader code executes.
You typically flash :file:`bootconf.hex` together with the rest of the programmed images (for example using ``west flash``), so the protection configuration is deployed with the project.

Sysbuild exposes this behavior as :kconfig:option:`SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES`.
On SoCs where the feature is supported, it defaults to enabled: the UICR-based lock blocks writes to the immutable bootloader region except through a full chip erase, which matches the strongest form of immutability for the NSIB partition.

The maximum contiguous region size that can be covered by this mechanism depends on the SoC (for example, 31 kB on nRF54L15, nRF54L10, and nRF54L05, and a larger limit on selected derivatives).
If the first-stage image does not fit in that span, you must combine mechanisms as described in :ref:`ug_bootloader_nrf54l_memory_protection_fprotect`.

.. _ug_bootloader_nrf54l_memory_protection_nsib:

2. While |NSIB| runs
====================

If your boot chain includes |NSIB|, it executes inside the immutable region configured at reset and verifies the next image before chaining to MCUboot or to the application.

Write protection for the **next** stage (MCUboot) can be enabled from NSIB using :kconfig:option:`CONFIG_SB_DISABLE_NEXT_W`.
When enabled, NSIB programs an RRAMC region so that the upgradable bootloader partition is not writable from later software stages, within the size limits described in the Kconfig help text (for example, up to 31 kB on nRF54L15, nRF54L10, and nRF54L05, and a larger limit on nRF54LV10A and nRF54LM20 variants).

This option is only available when :kconfig:option:`CONFIG_FPROTECT` is disabled for the NSIB image, because both features compete for overlapping protection resources.

.. _ug_bootloader_nrf54l_memory_protection_nsib_handoff:

3. When |NSIB| hands off to the next image
===========================================

Before jumping to MCUboot or directly to the application, NSIB can remove its own memory from the attack surface of later stages by enabling :kconfig:option:`CONFIG_SB_DISABLE_SELF_RWX`.
That configures RRAMC so the NSIB image region is no longer readable, writable, or executable after handoff, which mitigates attacks that attempt to execute or read back first-stage code once the application is running.

:kconfig:option:`CONFIG_SB_DISABLE_SELF_RWX` cannot be combined with :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` on the NSIB image, because the combined FPROTECT path is intended for different sizing and locking trade-offs on RRAM.

.. _ug_bootloader_nrf54l_memory_protection_mcuboot:

4. While MCUboot runs (two-stage chains)
========================================

If MCUboot is part of the chain, it performs slot management and signature checks, then starts the application (or another loadable image).

On nRF54L Series devices you can drop the :ref:`fprotect_readme` library for MCUboot in favor of :kconfig:option:`CONFIG_NCS_MCUBOOT_DISABLE_SELF_RWX`, which programs an RRAMC region (region 4 in the default description in Kconfig) so that the MCUboot partition loses read, write, and execute access for later stages.
This option requires :kconfig:option:`CONFIG_FPROTECT` to be disabled on the MCUboot image, because the two approaches are mutually exclusive in Kconfig.

If MCUboot is used **without** NSIB and fits within a single RRAMC region, the default layout can rely on FPROTECT alone for overwrite protection, as described in :ref:`ug_nrf54l_dfu_config`.

.. _ug_bootloader_nrf54l_memory_protection_application:

5. From the application's point of view
=========================================

After the boot chain completes, the application runs with the RRAMC configuration left by the last bootloader stage.

In a typical secure configuration that uses the features above, the application cannot modify immutable bootloader memory, cannot execute instructions from those addresses, and—where self-RWX disabling is enabled—cannot even read bootloader contents.
That is the intended mitigation for code-reuse and tampering attacks that target bootloader memory once application code is executing.

Operational changes to bootloader images still require a full image update flow through the supported DFU path (for example, upgrading MCUboot when a second stage is present), not a direct write to RRAM from the application.

.. _ug_bootloader_nrf54l_memory_protection_fprotect:

FPROTECT and combined regions (when they are still appropriate)
****************************************************************

The :ref:`fprotect_readme` library reflects an older nRF-family pattern based on BPROT-like hardware locking.
On nRF54L15, it remains relevant when **MCUboot alone** must be protected as an immutable first stage and its image size **exceeds one RRAMC region** (above roughly 31 kB).
In that situation you can enable :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` on the MCUboot image so that two RRAMC regions are merged for a larger locked span (up to the combined limit described in :ref:`ug_nrf54l_dfu_config`).

Using combined FPROTECT for large MCUboot images trades away the Kconfig combinations that require ``!FPROTECT`` or ``!FPROTECT_ALLOW_COMBINED_REGIONS`` for the RRAMC “disable self RWX” style options on NSIB or MCUboot.
Choose one consistent strategy per stage: either the RRAMC boot-configuration and RWX-disable path (preferred when it fits your image sizes), or FPROTECT with optional combined regions when image size or dependencies require it.

Summary
*******

.. list-table::
   :header-rows: 1
   :widths: 22 28 50

   * - When it applies
     - |NCS| / Kconfig lever
     - Role
   * - UICR programmed at flash time
     - :kconfig:option:`SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES` and :file:`bootconf.hex`
     - Immutable boot region from reset; true write-once style locking of NSIB span in RRAMC.
   * - During NSIB, before chaining
     - :kconfig:option:`CONFIG_SB_DISABLE_NEXT_W`
     - Write-disables the next bootloader partition (typically MCUboot), within region size limits.
   * - End of NSIB
     - :kconfig:option:`CONFIG_SB_DISABLE_SELF_RWX`
     - Removes read, write, and execute access to NSIB program memory for later stages.
   * - End of MCUboot
     - :kconfig:option:`CONFIG_NCS_MCUBOOT_DISABLE_SELF_RWX`
     - Removes read, write, and execute access to MCUboot program memory for later stages.
   * - Large standalone MCUboot on nRF54L15
     - :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS`
     - Extends hardware locking across two RRAMC regions when RWX-disable cannot cover the image alone.
