:orphan:

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

.. note::
   For a list of major differences between the final and the prototype silicon provided in the initial limited sampling, consult the `nRF54H20 prototype difference`_ document.

Consult the `nRF54H20 Objective Product Specification 0.3.1`_ (OPS) for more information on the mechanical specification of the SoC package and the reference circuitry used in the PDK.

Prepare the configuration files for your custom board in the |NCS|
******************************************************************

The nRF54H20 PDK uses multiple board files for its configuration:

* `ARM cores configuration files`_
* `Risc-V cores configuration files`_

You can use these files as a starting point for configuring your own custom board.
The easiest way to do so, when creating a :ref:`Zephyr repository application <zephyr:zephyr-repo-app>`, is to create a copy of these folders under :file:`sdk-nrf-next/boards/arm/your_custom_board_name` (for the ARM configuration files) and :file:`sdk-nrf-next/boards/riscv/your_custom_board_name` (for the Risc-V configuration files), respectively.

.. caution::
   Do not modify the configuration files related to the Secure Domain (:file:`*_cpusec` in the ARM folder) and the System Controller (:file:`*_cpusys` in the Risc-V folder).

You must edit the :file:`.dts` and :file:`.overlay` files for your project to match your board configuration, similarly to any new board added to the |NCS| or Zephyr.

See the following documentation pages for more information:

* The :ref:`zephyr:devicetree` documentation to familiarize yourself with the devicetree language and syntax
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
When running ``west build``, the build system then runs the BICR devicetree node through `nrf-regtool`_ to create the relevant HEX file (:file:`bicr.hex`) at build time.
Based on the peripheral definition extracted from the nRF54H20 SVD file, the modified registers from the configuration are mapped into their relevant position in memory.

.. note::
   If the build system cannot locate the ``bicr`` node inside your custom board's devicetree, or if you did not create a custom :file:`.dtsi` file for it, the BICR generation cannot progress, and the build system will skip it.

You can find the generated :file:`bicr.hex` file in the :file:`build_dir/zephyr/`.
The presence of a ``bicr`` node in the application devicetree will automatically trigger a build of the BICR binary, and will place this file alongside the other binary outputs such as ``zephyr.hex`` and ``uicr.hex``.

Flashing the BICR binary
========================

After the nRF Connect SDK build system generates the BICR binary, you must flash this binary manually.
The content of BICR should be loaded to the SoC only once and should not be erased nor modified unless the PCB layout changes.
To manually flash the generated :file:`bicr.hex` file to the SoC, use ``nrfjprog`` as follows::

    nrfjprog --snr ${FPGA_SEGGER_ID} --coprocessor CP_SECURE -f nrf54h --program bicr.hex --verify
    nrfjprog --coprocessor CP_SECURE --erasepage 0xfff8000
    nrfjprog --coprocessor CP_SECURE --erasepage 0xfffa000

You need to follow this flashing process only one time, as the PCB configuration will not change.

Verify the Life Cycle State (LCS) of the SoC
********************************************

To successfully run your custom application on your custom board, the SoC must have its Lifecycle State (LCS) set to ``RoT`` (meaning Root of Trust).
To verify that, run nrfjprog from the nRF Command Line Tools version 10.23.3_ec as follows::

   nrfjprog -s <serial_number> --memrd 0x0E000084 --w 32 --n 8

If nrfjprog returns ``0x2000`` twice, the LCS of the SoC is set to ``RoT``.
If nrfjprog returns ``0x1000`` twice, the LCS of the SoC is set to ``EMPTY``, meaning no LCS is set, and it needs to be switched to ``RoT``.

If you get the following error, the SoC is in ROM boot mode::

   [error] [ Client] - Encountered error -90: Command read_memory_descriptors executed for 1 milliseconds with result -90
   [error] [ Worker] - Ap-protect is enabled, can't read memory descriptors.
   [error] [ Client] - Encountered error -90: Command read executed for 80 milliseconds with result -90
   [error] [haltium] - Device responded to command with error status in ADAC packet: INVALID_COMMAND (0x7FFF).
   [error] [ Worker] - Access protection is enabled, can't access memory.
   ERROR: The operation attempted is unavailable due to readback protection in
   ERROR: your device. Please use --recover to unlock the device.
   NOTE: For additional output, try running again with logging enabled (--log).
   NOTE: Any generated log error messages will be displayed.

Switch to ``NORMAL`` mode first, then run again the previous command::

   nrfjprog -s <serial_number> --bootmode NORMAL
   nrfjprog -s <serial_number> --memrd 0x0E000084 --w 32 --n 8

Switch LCS to RoT
=================

To transition the LCS to ``RoT``, do the following:

1. Program BICR and secdom::

      nrfjprog -f nrf54h --coprocessor CP_SECURE --program /*insert_path_to_your_build_directory_here*/secdom/src/secdom-build/zephyr/zephyr.hex --verify
      nrfjprog -f nrf54h --coprocessor CP_SECURE --program bicr.hex --verify

#. Run these commands::

      nrfjprog --coprocessor CP_SECURE --memwr 0x0E000108 --val 0x40000
      nrfjprog --coprocessor CP_SECURE --memwr 0x0E00010C --val 0x0E003000

      nrfjprog --family nrf54h --bootmode ROM
      nrfjprog --family nrf54h --adac lcs_change PSA_ROT_PROVISIONING --single-step
      nrfjprog --family nrf54h --bootmode NORMAL

#. Flash your application using west::

      west flash

#. Verify if the LCS is set to ``RoT``::

      nrfjprog --memrd 0x0E000084 --w 32 --n 8

   If nrfjprog returns ``0x2000`` twice, the LCS of the SoC is correctly set to ``RoT``.

Create or modify your application for your custom board
*******************************************************

You can now create or modify your application for your custom board.
When doing so, consider the following:

* When reusing the |NCS| applications and samples, you must provide board-specific overlay files when such files are needed.
  For general information on configuration overlays, see :ref:`configure_application`.

  However, you must consider the following nRF54H20-specific differences:

  * The application might require board overlays for multiple cores.
    In this case, ensure that these overlays are consistent with each other.

* When creating a new application specific to your new board, DTS board files can contain all necessary configurations, and no overlay file is needed.
  However, the same limitations regarding the consistency and UICR configuration apply, but should be kept on the board files level.
