.. _ug_nrf54h20_custom_pcb:

Configuring your application for a custom PCB
#############################################

.. contents::
   :local:
   :depth: 2

This guide demonstrates how to create your custom board files for your PCB for the nRF54H20 SoC.

Prepare your PCB
****************

First, you need to create your PCB for the nRF54H20 SoC.

We highly recommend using the PCB layouts and component values provided by Nordic Semiconductor, especially for clock and power sources.
However, if you plan to select your own power sources, consider the following limitations:

* The DC/DC inductor must be present on the PCB.
  If the DC/DC power converter is to be used, the BICR value ``POWER.CONFIG.INDUCTOR`` must also be programmed to ``Present`` (``0``).

  .. note::
     Currently, there is a bug (LIL-9968) causing the nRF54H20 limited sampling SoC to not wake up from the System OFF if DC/DC is enabled.
     Temporarily, the workaround is to not use the DC/DC power converter.

* For the lowest sleep power consumption, use a 32 KHz crystal.
* The **P9** port (3 V) cannot be used with internal nor external pull-down resistors.
* For optimal performance, the output impedance of the **P6** and **P7** ports configured in ``IOPORT.DRIVECTRLX.PY`` should match the PCB and external device pin impedance.
  However, an existing limitation requires this configuration to be 50 ohms.


Prepare the configuration files for your custom board in the |NCS|
******************************************************************

The nRF54H20 DK uses multiple board files for its configuration:

* `ARM cores configuration files`_
* `Risc-V cores configuration files`_

You can use these files as a starting point for configuring your own custom board.
The easiest way to do so, when creating a :ref:`Zephyr repository application <zephyr:zephyr-repo-app>`, is to create a copy of these folders under :file:`sdk-nrf-next/boards/arm/your_custom_board_name` (for the ARM configuration files) and :file:`sdk-nrf-next/boards/riscv/your_custom_board_name` (for the Risc-V configuration files), respectively.

.. caution::
   Do not modify the configuration files related to the Secure Domain (:file:`*_cpusec` in the ARM folder) and the System Controller (:file:`*_cpusys` in the Risc-V folder).

You must edit the :file:`.dts` and :file:`.overlay` files for your project to match your board configuration, similarly to any new board added to the |NCS| or Zephyr.

See the following documentation pages for more information:

* The :ref:`zephyr:devicetree` documentation to familiarize yourself with the devicetree language and syntax.
* The :ref:`ug_nrf54h20_configuration` page for more information on how to configure your DTS files for the nRF54H20 SoC.
* The :ref:`zephyr:zephyr-repo-app` page for more information on Zephyr application types.
* The :ref:`dm_adding_code` documentation for details on the best user workflows to add your own code to the |NCS|.

.. note::
   The configuration of board files is based on the `nRF54H20 common SoC files`_.
   Each new |NCS| revision might change these files, breaking the compatibility with your custom board files created for previous revisions.
   Ensure the compatibility of your custom board files when migrating to a new |NCS| release.

Configure, generate, and flash BICR
***********************************

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including the information about the power and clock delivery to the SoC.
The power and clock control firmware uses this information to apply the proper regulator and oscillator configurations.

.. caution::
   You must ensure that the configuration is correct.
   An incorrect configuration can damage your device.

BICR allows for the configuration of various components on your custom board, like the following:

* Power rails
* Low-frequency oscillator
* High-frequency oscillator (HFXO)
* GPIO ports power and drive control
* Tamper switches
* Active shield channels

You can find the details of each register contained in BICR in the relevant `BICR register's PDF file`_.
When not set, the register's default value is ``0xFFFFFFFF``.

The ``LFOSC.LFXOCAL`` register is used by the device to store the calibration of the LFXO.

When ``LFOSC.LFXOCAL`` is ``0xFFFFFFFF`` at device boot, the firmware recalibrates the LFXO oscillator and writes the calibration data to the ``LFOSC.LFXOCAL`` register.
This is useful when making a change on the PCB (for example, when changing the crystal).
This initial calibration is only performed once.
Each subsequent start will use this initial calibration as the starting point.

BICR configuration
==================

The nRF54H20 PDK BICR configuration can be found in the board configuration directory as :file:`boards/arm/nrf54h20dk_nrf54h20/nrf54h20soc1_pdk_bicr.dtsi`.
This file is used by the |NCS| build system to generate a corresponding HEX file.
You can start from this file when editing the values of the devicetree properties inside your custom board folder (:file:`boards/arm/your_custom_board`), according to your board configuration.

Generating the BICR binary
==========================

To generate the BICR binary, you must first set the Kconfig option :kconfig:option:`CONFIG_INCLUDE_BICR` to ``y``.
When running ``west build``, the build system then creates the relevant HEX file (:file:`bicr.hex`) at build time.
Based on the peripheral definition extracted from the nRF54H20 SVD file, the modified registers from the configuration are mapped into their relevant position in memory.

.. note::
   If the build system cannot locate the ``bicr`` node inside your custom board's devicetree, or if you did not create a custom :file:`.dtsi` file for it, the BICR generation cannot progress, and the build system will skip it.

You can find the generated :file:`bicr.hex` file in the :file:`build_dir/zephyr/`.
The presence of a ``bicr`` node in the application devicetree will automatically trigger a build of the BICR binary, and will place this file alongside the other binary outputs such as ``zephyr.hex`` and ``uicr.hex``.

Flashing the BICR binary
========================

After the |NCS| build system generates the BICR binary, you must flash this binary manually.
The content of BICR should be loaded to the SoC only once and should not be erased nor modified unless the PCB layout changes.
To manually flash the generated :file:`bicr.hex` file to the SoC, use nRF Util as follows::

    nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr.hex` --core Secure --serial-number <serial_number>

You need to follow this flashing process only one time, as the PCB configuration will not change.

Programming the SDFW and SCFW
=============================

After programming the BICR, the nRF54H20 SoC requires the provisioning of a bundle ( :file:`nrf54h20_soc_binaries_v0.3.3.zip`) containing the precompiled firmware for the Secure Domain and System Controller.
To program the Secure Domain Firmware (SDFW, also known as ``urot``) and the System Controller Firmware (SCFW) from the firmware bundle to the nRF54H20 DK, do the following:

1. Download the `nRF54H20 firmware bundle`_.
#. Move the :file:`ZIP` bundle to a folder of your choice, then run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

Updating the FICR
=================

After programming the SDFW and SCFW from the firmware bundle, you must update the Factory Information Configuration Registers (FICR) to correctly configure some trims of the nRF54H20 SoC.
To update the FICR, you must run a J-Link script:

1. Get the Jlink script that updates the FICR::

      curl -LO https://files.nordicsemi.com/artifactory/swtools/external/scripts/nrf54h20es_trim_adjust.jlink

#. Run the script::

      JLinkExe -CommanderScript nrf54h20es_trim_adjust.jlink

Verify the LCS and transition to RoT
************************************

To successfully run your custom application on your custom board, the SoC must have its Lifecycle State (LCS) set to ``RoT`` (meaning Root of Trust).
If the LCS is set to ``EMPTY``, you must transition it to ``RoT``.

.. note::
   The forward transition to LCS ``RoT`` is permanent.
   After the transition, it is not possible to transition backward to LCS ``EMPTY``.

To transition the LCS to ``RoT``, do the following:

1. Verify the current lifecycle state of the nRF54H20::

      nrfutil device x-adac-discovery --serial-number <serial_number>

   The output will look similar to the following::

      *serial_number*
      adac_auth_version     1.0
      vendor_id             Nordic VLSI ASA
      soc_class             0x00005420
      soc_id                [e6, 6f, 21, b6, dc, be, 11, ee, e5, 03, 6f, fe, 4d, 7b, 2e, 07]
      hw_permissions_fixed  [00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      hw_permissions_mask   [01, 00, 00, 00, 87, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      psa_lifecycle         LIFECYCLE_EMPTY (0x1000)
      sda_id                0x01
      secrom_revision       0xad3b3cd0
      sysrom_revision       0xebc8f190
      token_formats         [TokenAdac]
      cert_formats          [CertAdac]
      cryptosystems         [Ed25519Sha512]
      Additional TLVs:
      TargetIdentity: [ff, ff, ff, ff, ff, ff, ff, ff]

#. If the lifecycle state (``psa_lifecycle``) shown is ``RoT`` (``LIFECYCLE_ROT (0x2000)``), no LCS transition is required.
   If the lifecycle state (``psa_lifecycle``) shown is not ``RoT`` (``LIFECYCLE_EMPTY (0x1000)`` means the LCS is set to ``EMPTY``), set it to Root of Trust using the following command::

      nrfutil device x-adac-lcs-change --life-cycle rot --serial-number <serial_number>

#. Verify again the current lifecycle state of the nRF54H20::

      nrfutil device x-adac-discovery --serial-number <serial_number>

   The output will look similar to the following::

      *serial_number*
      adac_auth_version     1.0
      vendor_id             Nordic VLSI ASA
      soc_class             0x00005420
      soc_id                [e6, 6f, 21, b6, dc, be, 11, ee, e5, 03, 6f, fe, 4d, 7b, 2e, 07]
      hw_permissions_fixed  [00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      hw_permissions_mask   [01, 00, 00, 00, 87, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00, 00]
      psa_lifecycle         LIFECYCLE_ROT (0x2000)
      sda_id                0x01
      secrom_revision       0xad3b3cd0
      sysrom_revision       0xebc8f190
      token_formats         [TokenAdac]
      cert_formats          [CertAdac]
      cryptosystems         [Ed25519Sha512]
      Additional TLVs:
      TargetIdentity: [ff, ff, ff, ff, ff, ff, ff, ff]

   The lifecycle state (``psa_lifecycle``) is now correctly set to *Root of Trust* (``LIFECYCLE_ROT (0x2000)``)

#. After the LCS transition, reset the device::

      nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

Create or modify your application for your custom board
*******************************************************

You can now create or modify your application for your custom board.
When doing so, consider the following:

* When reusing the |NCS| applications and samples, you must provide board-specific overlay files when such files are needed.
  For general information on configuration overlays, see :ref:`configure_application`.

  However, you must consider the following nRF54H20-specific difference:

  * The application might require board overlays for multiple cores.
    In this case, ensure that these overlays are consistent with each other.

* When creating a new application specific to your new board, DTS board files can contain all necessary configurations, and no overlay file is needed.
  However, the same limitations regarding the consistency and UICR configuration apply, but should be kept on the board files level.
