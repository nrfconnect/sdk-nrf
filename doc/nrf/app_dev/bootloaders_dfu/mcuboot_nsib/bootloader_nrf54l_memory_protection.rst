.. _ug_bootloader_nrf54l_memory_protection:

nRF54L Series bootloader RRAM protection
#########################################

.. contents::
   :local:
   :depth: 2

On the nRF54L Series, the immutable bootloader and, when present, MCUboot execute from RRAM.
The RRAM controller (RRAMC) can enforce access rules per region.
This keeps bootloader memory intact and, after the boot chain hands off to the application, prevents its use in code-reuse attacks.

This page describes how those protection mechanisms fit together in |NCS|, in the order they affect the running system: from chip reset through the bootloaders to the application.
For build commands, flashing, and signature provisioning on nRF54L devices, see also :ref:`ug_nrf54l_dfu_config` and :ref:`ug_bootloader_adding_sysbuild`.

.. _ug_bootloader_nrf54l_memory_protection_hw_background:

Hardware background
*******************

The nRF54L15 Product Specification documents how RRAMC maps boot configuration to locked regions and how the UICR BOOTCONF register supplies part of that configuration at reset.
See `nRF54L15 RRAMC regions protection`_ and `nRF54L15 UICR BOOTCONF register`_ for the authoritative hardware description.
Other nRF54L Series SoCs use the same overall mechanism with different maximum region sizes; the build-generated :file:`bootconf.hex` file accounts for the selected SoC.
The region size limits are roughly 31 kB per region on the nRF54L15, nRF54L10, and nRF54L05 SoCs, and larger on the nRF54LV10A and nRF54LM20 SoCs.
See the Kconfig help text for exact per-option limits.

There are two protection families on RRAM: the RRAMC boot-configuration/RWX-disable path (described in boot order) and the older FPROTECT locking (see :ref:`ug_bootloader_nrf54l_memory_protection_fprotect`).
You cannot combine them per stage.

.. _ug_bootloader_nrf54l_memory_protection_reset:

.. rst-class:: numbered-step

At reset (before bootloader software runs)
==========================================

When you use |NSIB| or MCUboot as the immutable first-stage bootloader, the build can produce a small Intel HEX file named :file:`bootconf.hex` (as described in :ref:`ug_bootloader_nrf54l_memory_protection_hw_background`).
This image programs the UICR BOOTCONF value so that RRAMC applies an immutable boot region as soon as the device leaves reset, before any of your bootloader code executes.
You typically flash :file:`bootconf.hex` together with the rest of the programmed images (for example using ``west flash``), so the protection configuration is deployed with the project.

Sysbuild exposes this behavior through the following Kconfig options:
* :kconfig:option:`SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES` for |NSIB| 
* :kconfig:option:`SB_CONFIG_MCUBOOT_BOOTCONF_LOCK_WRITES` for MCUboot
 
If you enable either option and the immutable bootloader partition is larger than the maximum BOOTCONF region size of the selected SoC, the build fails.

On the nRF54L05, nRF54L10, and nRF54L15 SoCs, both options are disabled by default because the default bootloader partitions are larger than the maximum region size of approximately 31 kB.
To enable BOOTCONF protection on these SoCs, first reduce the immutable bootloader partition to fit within the limit stated in the Kconfig help text, then enable the option for your immutable bootloader.

.. note::
   When BOOTCONF protection is active, it blocks all writes to the immutable bootloader region.
   To restore write access, you must perform full chip erase, which erases all RRAM and UICR contents.

If the immutable bootloader cannot fit within the BOOTCONF region size, use FPROTECT with combined regions instead, as described in :ref:`ug_bootloader_nrf54l_memory_protection_fprotect`.

.. _ug_bootloader_nrf54l_memory_protection_nsib:

.. rst-class:: numbered-step

During |NSIB| execution
=======================

If your boot chain includes |NSIB|, it executes inside the immutable region configured at reset and verifies the next image before chaining to MCUboot or to the application.

Write protection for the next stage (MCUboot) can be enabled from NSIB using the :kconfig:option:`CONFIG_SB_DISABLE_NEXT_W` Kconfig option.
When enabled, NSIB programs an RRAMC region so later software stages including MCUboot itself cannot write to the upgradable bootloader partition (within the size limits in the Kconfig help text).

This option is only available when :kconfig:option:`CONFIG_FPROTECT` is disabled for the NSIB image, because both features compete for overlapping protection resources.

.. _ug_bootloader_nrf54l_memory_protection_nsib_handoff:

.. rst-class:: numbered-step

At |NSIB| hand-off
==================

Before jumping to MCUboot or directly to the application, NSIB can remove its own memory from the attack surface of later stages.
Before MCUboot boots the next image, it can prevent later stages from reading, writing, or executing the MCUboot partition
To enable this behavior, use the :kconfig:option:`CONFIG_SB_DISABLE_SELF_RWX` Kconfig option.
This option configures RRAMC so the NSIB image region is no longer readable, writable, or executable after hand-off, which mitigates attacks that attempt to execute or read back first-stage code once the application is running.

You cannot combine the :kconfig:option:`CONFIG_SB_DISABLE_SELF_RWX` Kconfig option with :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` on the NSIB image, because the combined FPROTECT path is intended for different sizing and locking trade-offs on RRAM.

.. _ug_bootloader_nrf54l_memory_protection_mcuboot:

.. rst-class:: numbered-step

During MCUboot execution
========================

If MCUboot is part of the chain, it manages slots, verifies signatures, then starts the application (or another loadable image).

You can use the :kconfig:option:`CONFIG_NCS_MCUBOOT_DISABLE_SELF_RWX` Kconfig option to prevent later boot stages from reading, writing, or executing the MCUboot partition.
How MCUboot applies this restriction depends on whether BOOTCONF protection is enabled:

* If :kconfig:option:`SB_CONFIG_MCUBOOT_BOOTCONF_LOCK_WRITES` is enabled, MCUboot adds read and execute restrictions to the immutable region (RRAMC region 3), which :file:`bootconf.hex` write-locks at reset.
* If :kconfig:option:`SB_CONFIG_MCUBOOT_BOOTCONF_LOCK_WRITES` is disabled, MCUboot configures RRAMC region 4.
  In this case, you cannot enable :kconfig:option:`CONFIG_FPROTECT` for the MCUboot image.

In firmware loader mode, MCUboot also uses RRAMC region 4 to write-protect the firmware loader partition before it boots either the firmware loader or the application.
The exception is an authenticated installer image, which is marked by a protected image TLV.
For such an image, MCUboot leaves the firmware loader partition writable so that the installer can replace the firmware loader.

If MCUboot is used without NSIB and fits within a single RRAMC region, the default layout can rely on FPROTECT alone for overwrite protection, as described in :ref:`ug_nrf54l_dfu_config`.

.. _ug_bootloader_nrf54l_memory_protection_application:
.. rst-class:: numbered-step

In the running application
==========================

After the boot chain completes, the application runs with the RRAMC configuration left by the last bootloader stage.

In a typical secure configuration that uses these features, the application cannot:

* Modify immutable bootloader memory
* Execute instructions from those addresses
* Read bootloader contents where self-RWX disabling is enabled

That is the intended mitigation for code-reuse and tampering attacks that target bootloader memory once application code is executing.

Operational changes to bootloader images still require a full image update flow through the supported DFU path (for example, upgrading MCUboot when a second stage is present), not a direct write to RRAM from the application.

.. _ug_bootloader_nrf54l_memory_protection_fprotect:

FPROTECT and combined regions (when they are still appropriate)
****************************************************************

The :ref:`fprotect_readme` library reflects an older nRF-family pattern based on BPROT-like hardware locking.
On the nRF54L15 SoC, it remains relevant when MCUboot alone must be protected as an immutable first stage and its image size exceeds one RRAMC region (above roughly 31 kB).
In that situation, enable the :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS` Kconfig option on the MCUboot image so that two RRAMC regions are merged for a larger locked span (up to the combined limit described in :ref:`ug_nrf54l_dfu_config`).

Choosing combined FPROTECT for large MCUboot images means you cannot also use the RRAMC "disable self RWX" options on NSIB or MCUboot, since those require FPROTECT to be disabled.
Choose one consistent strategy per stage: either the RRAMC boot-configuration and RWX-disable path (preferred when it fits your image sizes), or FPROTECT with optional combined regions when image size or dependencies require it.

Summary
*******

.. list-table::
   :header-rows: 1
   :widths: 22 28 50

   * - When it applies
     - |NCS| / Kconfig option
     - Role
   * - UICR programmed at flash time
     - :kconfig:option:`SB_CONFIG_SECURE_BOOT_BOOTCONF_LOCK_WRITES`, :kconfig:option:`SB_CONFIG_MCUBOOT_BOOTCONF_LOCK_WRITES`, and :file:`bootconf.hex`
     - Immutable boot region from reset; true write-once locking of the NSIB or MCUboot span in RRAMC.
   * - During NSIB, before chaining
     - :kconfig:option:`CONFIG_SB_DISABLE_NEXT_W`
     - Write-disables the next bootloader partition (typically MCUboot), within region size limits.
   * - End of NSIB
     - :kconfig:option:`CONFIG_SB_DISABLE_SELF_RWX`
     - Removes read, write, and execute access to NSIB program memory for later stages.
   * - End of MCUboot
     - :kconfig:option:`CONFIG_NCS_MCUBOOT_DISABLE_SELF_RWX`
     - Removes read, write, and execute access to MCUboot program memory for later stages.
   * - Firmware loader hand-off
     - :kconfig:option:`SB_CONFIG_MCUBOOT_BOOTCONF_LOCK_WRITES`
     - Write-protects the firmware loader partition except when booting an authenticated installer.
   * - Large standalone MCUboot on nRF54L15
     - :kconfig:option:`CONFIG_FPROTECT_ALLOW_COMBINED_REGIONS`
     - Extends hardware locking across two RRAMC regions when RWX-disable cannot cover the image alone.
