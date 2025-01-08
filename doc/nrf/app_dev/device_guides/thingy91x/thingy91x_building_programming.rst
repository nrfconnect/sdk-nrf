.. _building_pgming_thingy91x:

Building and programming with Thingy:91 X
#########################################

.. contents::
   :local:
   :depth: 2

You can also program the Thingy:91 X with the images obtained by building the code in an |NCS| environment and the west tool.

To set up your system to be able to build a compatible firmware image, follow the processes described in the :ref:`building` section.
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

+--------------------------+----------------------------------------+----------------------------------------------------------------+
| File                     | File format                            | Programming scenario                                           |
+==========================+========================================+================================================================+
|:file:`merged.hex`        | Full image, HEX format                 | Using an external debug probe and nrfutil device.              |
+--------------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`zephyr.signed.hex` | MCUboot compatible image, HEX format   | Using the built-in bootloader and nrfutil device.              |
+--------------------------+----------------------------------------+----------------------------------------------------------------+
|:file:`app_update.bin`    | MCUboot compatible image, binary format|* Using the built-in bootloader and mcumgr command-line tool.   |
|                          |                                        |* For FOTA updates.                                             |
+--------------------------+----------------------------------------+----------------------------------------------------------------+

Programming onto Thingy:91 X
****************************

Complete the following steps to program firmware onto Thingy:91 X:

1. Set the Thingy:91 X SWD selection switch (**SW2**) to **nRF91** or **nRF53** depending on whether you want to program the nRF9151 SiP or the nRF5340 SoC component.
#. Connect the Thingy:91 X to the debug out port on a 10-pin external debug probe, for example, nRF9151 DK, using a 10-pin SWD cable.
#. Connect the external debug probe to the PC using a USB cable.
#. Make sure that the Thingy:91 X and the external debug probe are powered on.

.. include:: /app_dev/device_guides/thingy91/thingy91_building_programming.rst
   :start-after: thingy91_building_pgmin_start
   :end-before: thingy91_building_pgmin_end
