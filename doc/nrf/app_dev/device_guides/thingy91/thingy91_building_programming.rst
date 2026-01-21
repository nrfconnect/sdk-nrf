.. _building_pgming:

Building and programming with Thingy:91
#######################################

.. contents::
   :local:
   :depth: 2

You can also program the Thingy:91 with the images obtained by building the code in an |NCS| environment.

To set up your system to be able to build a compatible firmware image, follow the processes described in the :ref:`building` section.
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
   * In |NCS| releases ranging from v1.3.0 to v1.6.1, the build target ``thingy91/nrf9160/ns`` was named ``thingy91_nrf9160/ns``.

   LTE/GNSS features can only be used with :ref:`Cortex-M Security Extensions enabled <app_boards_spe_nspe_cpuapp_ns>` (``_ns`` build target).

The following table shows the different types of build files that are generated and the different scenarios in which they are used:

+---------------------------+-----------------------------------------+---------------------------------------------------------------+
|           File            |               File format               |                     Programming scenario                      |
+===========================+=========================================+===============================================================+
| :file:`merged.hex`        | Full image, HEX format                  | Using an external debug probe and the `Programmer app`_.      |
+---------------------------+-----------------------------------------+---------------------------------------------------------------+
| :file:`zephyr.signed.hex` | MCUboot compatible image, HEX format    | Using the built-in bootloader and the `Programmer app`_.      |
+---------------------------+-----------------------------------------+---------------------------------------------------------------+
| :file:`app_update.bin`    | MCUboot compatible image, binary format | * Using the built-in bootloader and mcumgr command line tool. |
|                           |                                         | * For FOTA updates.                                           |
+---------------------------+-----------------------------------------+---------------------------------------------------------------+

For an overview of different types of build files in the |NCS|, see :ref:`app_build_output_files`.

There are multiple methods of programming a sample or application onto a Thingy:91.
It is recommended to use an external debug probe to program the Thingy:91.

.. note::

   If you do not have an external debug probe available to program the Thingy:91, you can directly program by :ref:`using the USB (MCUboot) method and the Programmer app <programming_thingy>`.
   In this scenario, use the :file:`app_signed.hex` firmware image file.

.. note::

   While building applications for Thingy:91, the build system changes the signing algorithm of MCUboot so that it uses the default RSA keys.
   This is to ensure backward compatibility with the MCUboot versions that precede the |NCS| v1.4.0.
   Use the default RSA keys only for development.
   In a final product, you must use your own secret keys.
   See :ref:`ug_fw_update_development_keys` for more information.

Programming onto Thingy:91
**************************

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

      5. In |nRFVSC|, click the :guilabel:`Flash` option in the `Actions View`_.

         If you have multiple boards connected, you are prompted to pick a device at the top of the screen.

         A small notification banner appears in the bottom-right corner of |VSC| to display the progress and confirm when the flash is complete.

   .. group-tab:: Command line

      5. |open_terminal_window_with_environment|
      6. Program the sample or application to the device using the following command::

           west flash

         The device resets and runs the programmed sample or application.

.. thingy91_building_pgmin_end
