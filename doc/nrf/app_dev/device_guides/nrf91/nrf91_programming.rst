.. _build_pgm_nrf9160:

Programming onto nRF91 Series devices
#####################################

.. contents::
   :local:
   :depth: 2

You can program applications and samples onto the nRF91 Series devices after :ref:`building` the corresponding firmware images.

To program firmware onto nRF91 Series devices, follow the process described in the :ref:`programming` section.
Pay attention to the exceptions specific to the nRF91 Series, which are listed below.

.. |exceptions_step| replace:: Make sure to check the programming exceptions for the nRF91 Series in the sections below.

.. include:: /includes/vsc_build_and_run_series.txt

Compatible versions of modem and application
********************************************

When you update the application firmware on an nRF91 Series DK, make sure that the modem firmware and application firmware are compatible versions.

.. _build_pgm_nrf9160_board_controller:

Selecting board controller on nRF9160 DK
****************************************

When programming on the nRF9160 DK, make sure to set the **SW10** switch (marked PROG/DEBUG) in the **nRF91** position to program the nRF9160 application.
In nRF9160 DK board revision v0.9.0 and earlier, the switch is called **SW5**.

.. _building_pgming:

Programming onto Thingy:91
**************************

You can also program the Thingy:91 with the images obtained by building the code in an |NCS| environment.

To set up your system to be able to build a compatible firmware image, follow the :ref:`installation` guide for the |NCS| and read the :ref:`configuration_and_build` documentation.
The build targets of interest for Thingy:91 in |NCS| are as follows:

+---------------+---------------------------------------------------+
|Component      |  Build target                                     |
+===============+===================================================+
|nRF9160 SiP    |``thingy91/nrf9160/ns``                            |
+---------------+---------------------------------------------------+
|nRF52840 SoC   |``thingy91/nrf52840``                              |
+---------------+---------------------------------------------------+

You must use the build target ``thingy91/nrf9160/ns`` when building the application code for the nRF9160 SiP and the build target ``thingy91/nrf52840`` when building the application code for the onboard nRF52840 SoC.

.. note::

   * In |NCS| releases before v1.3.0, these build targets were named ``nrf9160_pca20035``, ``nrf9160_pca20035ns``, and ``nrf52840_pca20035``.
   * In |NCS| releases ranging from v1.3.0 to v1.6.1, the build target ``thingy91/nrf9160/ns`` was named ``thingy91_nrf9160ns``.

.. note::

   LTE/GNSS features can only be used with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (``_ns`` build target).

The following table shows the different types of build files that are generated and the different scenarios in which they are used:

+-----------------------+----------------------------------------+----------------------------------------------------------------+
| File                  | File format                            | Programming scenario                                           |
+=======================+========================================+================================================================+
|:file:`merged.hex`     | Full image, HEX format                 | Using an external debug probe and nRF Connect Programmer.      |
+-----------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_signed.hex` | MCUboot compatible image, HEX format   | Using the built-in bootloader and nRF Connect Programmer.      |
+-----------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_update.bin` | MCUboot compatible image, binary format|* Using the built-in bootloader and mcumgr command line tool.   |
|                       |                                        |* For FOTA updates.                                             |
+-----------------------+----------------------------------------+----------------------------------------------------------------+

For an overview of different types of build files in the |NCS|, see :ref:`app_build_output_files`.

There are multiple methods of programming a sample or application onto a Thingy:91.
It is recommended to use an external debug probe to program the Thingy:91.

.. note::

   If you do not have an external debug probe available to program the Thingy:91, you can directly program by :ref:`using the USB (MCUboot) method and nRF Connect Programmer <programming_thingy>`.
   In this scenario, use the :file:`app_signed.hex` firmware image file.

.. note::

   While building applications for Thingy:91, the build system changes the signing algorithm of MCUboot so that it uses the default RSA keys.
   This is to ensure backward compatibility with the MCUboot versions that precede the |NCS| v1.4.0.
   Use the default RSA keys only for development.
   In a final product, you must use your own, secret keys.
   See :ref:`ug_fw_update_development_keys` for more information.

Complete the following steps to program firmware onto Thingy:91:

1. Set the Thingy:91 SWD selection switch (**SW2**) to **nRF91** or **nRF52** depending on whether you want to program the nRF9160 SiP or the nRF52840 SoC component.
#. Connect the Thingy:91 to the debug out port on a 10-pin external debug probe, for example, nRF9160 DK, using a 10-pin JTAG cable.

   .. note::

      If you are using nRF9160 DK as the debug probe, make sure that **VDD_IO (SW11)** is set to 1.8 V.

#. Connect the external debug probe to the PC using a USB cable.
#. Make sure that the Thingy:91 and the external debug probe are powered on.

.. thingy91_building_pgmin_start

.. tabs::

   .. group-tab:: nRF Connect for VS Code

      5. In |nRFVSC|, click the :guilabel:`Flash` option in the **Actions View**.

         If you have multiple boards connected, you are prompted to pick a device at the top of the screen.

         A small notification banner appears in the bottom-right corner of |VSC| to display the progress and confirm when the flash is complete.

   .. group-tab:: Command line

      5. |open_terminal_window_with_environment|
      6. Program the sample or application to the device using the following command::

           west flash

         The device resets and runs the programmed sample or application.

.. thingy91_building_pgmin_end

.. _building_pgming_thingy91x:

Programming onto Thingy:91 X
****************************

You can also program the Thingy:91 X with the images obtained by building the code in an |NCS| environment and the west tool.

To set up your system to be able to build a compatible firmware image, follow the :ref:`installation` guide for the |NCS| and read the :ref:`configuration_and_build` documentation.
The build targets of interest for Thingy:91 X in the |NCS| are as follows:

+----------------------------------+---------------------------------+
| Component                        | Build target                    |
+==================================+=================================+
| nRF9151 SiP                      | ``thingy91x/nrf9151``           |
|                                  | ``thingy91x/nrf9151/ns``        |
+----------------------------------+---------------------------------+
| nRF5340 SoC - Application core   | ``thingy91x/nrf5340/cpuapp``    |
|                                  | ``thingy91x/nrf5340/cpuapp/ns`` |
+----------------------------------+---------------------------------+
| nRF5340 SoC - Network core       | ``thingy91x/nrf5340/cpunet``    |
+----------------------------------+---------------------------------+

.. note::
   LTE/GNSS features can only be used with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (nRF9151 ``ns`` build target).

The following table shows the different types of build files that are generated and the different scenarios in which they are used:

+-----------------------+----------------------------------------+----------------------------------------------------------------+
| File                  | File format                            | Programming scenario                                           |
+=======================+========================================+================================================================+
|:file:`merged.hex`     | Full image, HEX format                 | Using an external debug probe and nrfutil device.              |
+-----------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_signed.hex` | MCUboot compatible image, HEX format   | Using the built-in bootloader and nrfutil device.              |
+-----------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_update.bin` | MCUboot compatible image, binary format|* Using the built-in bootloader and mcumgr command-line tool.   |
|                       |                                        |* For FOTA updates.                                             |
+-----------------------+----------------------------------------+----------------------------------------------------------------+

Complete the following steps to program firmware onto Thingy:91 X:

1. Set the Thingy:91 X SWD selection switch (**SW2**) to **nRF91** or **nRF53** depending on whether you want to program the nRF9151 SiP or the nRF5340 SoC component.
#. Connect the Thingy:91 X to the debug out port on a 10-pin external debug probe, for example, nRF9151 DK, using a 10-pin SWD cable.
#. Connect the external debug probe to the PC using a USB cable.
#. Make sure that the Thingy:91 X and the external debug probe are powered on.

.. include:: nrf91_programming.rst
   :start-after: thingy91_building_pgmin_start
   :end-before: thingy91_building_pgmin_end
