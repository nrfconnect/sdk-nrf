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

It is highly recommended to use the PCB layouts and component values provided by Nordic Semiconductor, especially for clock and power sources, considering the following limitations:

* The DC/DC inductor must be present on the PCB for any of the supported power schemes.
  Use one of the following power supply options:

  * VDDH higher than 2.05V.
  * VDDH shorted to VDD at 1.8V

* For the lowest sleep power consumption, use a 32 KHz crystal.
* The **P9** port cannot be used with internal or external pull-down resistors.
* For optimal performance, the output impedance of the **P6** and **P7** ports should match the PCB and external device pin impedance.

.. _ug_nrf54h20_custom_pcb_board:

Prepare the configuration files for your custom board in the |NCS|
******************************************************************

Use the `nRF Connect for VS Code Extension Pack`_ to generate a custom board skeleton.

Use the nRF54H20 DK board files found in :file:`sdk-zephyr/boards/nordic/nrf54h20dk` as a reference point for configuring your own custom board.

See the following documentation pages for more information:

* The :ref:`zephyr:devicetree` documentation to familiarize yourself with the devicetree language and syntax.
* The :ref:`ug_nrf54h20_configuration` page for more information on how to configure your DTS files for the nRF54H20 SoC.
* The :ref:`zephyr:zephyr-repo-app` page for more information on Zephyr application types.
* The :ref:`dm_adding_code` documentation for details on the best user workflows to add your own code to the |NCS|.

.. caution::
   The configuration of board files is based on the nRF54H20 common SoC files located in :file:`sdk-zephyr/dts/vendor/nordic/`.
   Each new |NCS| revision might change these files, breaking the compatibility with your custom board files created for previous revisions.
   Ensure the compatibility of your custom board files when migrating to a new |NCS| release.

   See :ref:`zephyr:board_porting_guide` for more information.

.. _ug_nrf54h20_custom_pcb_bicr:

Configure, generate, and program the BICR
*****************************************

The Board Information Configuration Registers (BICR) are non-volatile memory (NVM) registers that contain information on how the nRF54H20 SoC must interact with other board elements, including the information about the power and clock delivery to the SoC.
The power and clock control firmware uses this information to apply the proper regulator and oscillator configurations.

.. caution::
   You must ensure that the configuration is correct.
   An incorrect configuration can damage your device.

BICR allows for the configuration of various components on your custom board, like the following:

* Power scheme
* Low-frequency oscillator (LFXO or LFRC)
* High-frequency oscillator (HFXO)
* GPIO ports power and drive control
* Tamper switches
* Active shield channels

When the BICR has not been programmed, all the registers contain ``0xFFFFFFFF``.

The ``LFOSC.LFXOCAL`` register is used by the device to store the calibration of the LFXO.

When ``LFOSC.LFXOCAL`` is ``0xFFFFFFFF`` at device boot, the firmware recalibrates the LFXO oscillator and writes the calibration data to the ``LFOSC.LFXOCAL`` register.
This is useful when making a change on the PCB (for example, when changing the crystal).
This initial calibration is only performed once.
Each subsequent start will use this initial calibration as the starting point.

Configure the BICR
==================

You can find the nRF54H20 DK BICR configuration in the board configuration directory as :file:`sdk-zephyr/boards/nordic/nrf54h20dk/bicr.json`, and the details of the file scheme in the :file:`sdk-zephyr/soc/nordic/nrf54h/bicr/bicr-schema.json` file.
You can use these files as a reference for your own BICR configuration.

The |NCS| build system uses the BICR configuration JSON files to generate the corresponding HEX file.

.. caution::
   A mismatch between the board and the configuration values in BICR can damage the device or set it in an unrecoverable state.

Generate the BICR binary
========================

To generate the BICR binary, you must first set the Kconfig option :kconfig:option:`CONFIG_SOC_NRF54H20_GENERATE_BICR` to ``y``.
When running ``west build`` for the ``cpuapp`` core, the build system creates the relevant HEX file (:file:`bicr.hex`) at build time.

.. note::
   If the build system is unable to locate the :file:`bicr.json` file inside your custom board's directory, the build system will skip it.

You can find the generated :file:`bicr.hex` file in the :file:`build_dir/<sample>/zephyr/`.

Program the BICR binary
=======================

After the |NCS| build system generates the BICR binary, you must flash this binary manually.
The content of BICR should be loaded to the SoC only once and should not be erased nor modified unless the PCB layout changes.
To manually program the generated :file:`bicr.hex` file to the SoC, use nRF Util as follows::

    nrfutil device program --options chip_erase_mode=ERASE_NONE --firmware bicr.hex --core Application --serial-number <serial_number>

You only need to follow this programming process once, assuming the PCB configuration applied through the BICR is correct the first time.
However, it is also possible to reprogram the BICR while in the LCS ``RoT``.
This can be useful, for example, when adjusting the configuration as the PCB design gets refined or modified, requiring the process to be repeated.

Validate the BICR binary
------------------------

After programming the BICR binary onto the device, validate whether the BICR works with your device as follows:

1. Reset the device::

      nrfutil device reset --reset-kind RESET_PIN --serial-number <serial_number>

2. When in LCS ``EMPTY``, use ``nrfutil`` to validate the BICR status by reading the memory::

      nrfutil device x-read --address 0x2F88FF1C --serial-number 1051164514 --core Secure

   nrfutil returns the BICR loading status using one of the following values:

   * ``0x289CFB73``: BICR applied without error.
     This indicates that the power configuration of the BICR is valid and you can proceed to the next step.
   * ``0xD78213DF``: BICR application was skipped.
     This indicates that no BICR was programmed to the device.
     Revisit the previous step to ensure the programming command was executed, and that the BICR was correctly generated.
   * ``0xCE68C97C``: BICR application failed.
     This indicates that there is an issue with the BICR, but in most cases this can be recovered by programming the correct BICR for your board.
   * ``Error``: This indicates that the device is likely suffering from severe power issues after applying the BICR.
     This state is likely unrecoverable.

.. _54h_soc_binaries_provision:

Provision the nRF54H20 IronSide SE binaries
*******************************************

After programming the BICR, the nRF54H20 SoC requires the provisioning of the nRF54H20 IronSide SE binaries, a bundle containing the precompiled firmware for the Secure Domain and System Controller.
To provision the nRF54H20 IronSide SE binaries to the nRF54H20 SoC, do the following:

1. Download the right nRF54H20 IronSide SE binaries version for your |NCS| version.
   You can find the SoC binaries versions listed in the :ref:`abi_compatibility` page.
#. Move the :file:`ZIP` bundle to a folder of your choice.
#. |open_terminal_window_with_environment|
#. Run nRF Util to program the binaries using the following command::

      nrfutil device x-provision-nrf54h --firmware <path-to_bundle_zip_file> --serial-number <serial_number>

You can run the following command to confirm that the Secure Domain Firmware has loaded correctly:

   nrfutil device x-adac-lcs-change

If issues occur during bundle programming, the system will return an ``ADAC_FAILURE`` error.

.. _54h_soc_binaries_transition_rot:

Transition the nRF54H20 SoC to RoT
==================================

The nRF54H20 SoC comes with its lifecycle state (LCS) set to ``EMPTY``.
To operate correctly, you must transition its lifecycle state to Root of Trust (``RoT``).

.. note::
   The forward transition to LCS ``RoT`` is permanent.
   After the transition, it is impossible to transition backward to LCS ``EMPTY``.

To transition the LCS to ``RoT``, set the LCS of the nRF54H20 SoC to Root of Trust using the following command::

   nrfutil device x-adac-lcs-change --life-cycle rot --serial-number <serial_number>

Create or modify your application for your custom board
*******************************************************

You can now create or modify your application for your custom board.
When doing so, consider the following:

* When reusing the |NCS| applications and samples, you must provide board-specific overlay files when such files are needed.
  For more information on configuration overlays, see :ref:`configure_application`.

  However, on the nRF54H20 SoC, the application might require board overlays for multiple cores.
  In this case, ensure that these overlays are consistent with each other.

* When creating a new application specific to your new board, DTS board files can contain all necessary configurations, and no overlay file is needed.
  However, the same limitations regarding the consistency and UICR configuration apply, but should be kept on the board files level.

* You must manually program the BICR if it has been modified.

For more information on |NCS| application development, see :ref:`ug_app_dev`.
For more information on nRF54H20 SoC development, see :ref:`ug_nrf54h`.

.. _54h_soc_binaries_update:

Update the nRF54H20 IronSide SE binaries
****************************************

.. caution::
   It is not possible to update the nRF54H20 binaries from a SUIT-based (up to 0.9.6) to an IronSide-SE-based (2x.x.x) version.

To update the nRF54H20 IronSide SE binaries (versions 2x.x.x, based on IronSide SE) using the debugger on a nRF54H20 SoC, use the west ``ncs-ironside-se-update`` command.
This command takes the nRF54H20 SoC binary ZIP file and uses the IronSide SE update service to update both the IronSide SE and IronSide SE Recovery (or optionally just one of them).

For more information on how to use the ``ncs-ironside-se-update`` command, see :ref:`ug_nrf54h20_ironside_se_update`.
For more information on the nRF54H20 IronSide SE binaries, see :ref:`abi_compatibility`.
