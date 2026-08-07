.. _thingy91_update_firmware:
.. _programming_thingy:

Updating the Thingy:91 firmware using nRF Connect for Desktop apps
##################################################################

.. contents::
   :local:
   :depth: 2

Thingy:91 (v1.5.0 or earlier) comes preloaded with the nRF9160: Asset Tracker firmware and modem firmware on the nRF9160 :term:`System in Package (SiP)`, and the Connectivity bridge application firmware on the nRF52840 :term:`System on Chip (SoC)` that enable the device to use the environment sensors and track the device using :term:`Global Positioning System (GPS)`.
The data is transmitted to nRF Cloud.

You can update the application and modem firmware on a Thingy:91 through a :term:`Universal Serial Bus (USB)` cable using MCUboot.
MCUboot is a secure bootloader that is used to update applications if you do not have an external debugger.
The board enters MCUboot mode if you press one of the following buttons while the Thingy:91 is being powered on (using **SW1**):

* **SW3** - The main button used to flash the nRF9160 SiP.
  You use this button when getting started with the Thingy:91.
* **SW4** - The button used to update the nRF52840 SoC.

Before you start, make sure the Thingy:91 is connected to the computer with a micro-USB cable and powered on.

.. note::

   Do not unplug the Nordic Thingy:91 during this process.

You can update the firmware on the Thingy:91 using the following nRF Connect for Desktop apps:

* Programmer app
* Cellular Monitor app

Updating the Thingy:91 firmware using the Programmer app
========================================================

To update the firmware on the Thingy:91 using the `Programmer app`_ in nRF Connect for Desktop, complete the steps listed on the `Programming Nordic Thingy prototyping platforms`_ page in the tool documentation.

Updating the Thingy:91 firmware using the Cellular Monitor app
==============================================================

To update the firmware on the Thingy:91 using the `Cellular Monitor app`_ in nRF Connect for Desktop, complete the steps listed on the `Programming Nordic Thingy:91 firmware`_ page in the tool documentation.

.. _thingy91_partition_layout:

Partition layout
================

When building firmware for Nordic Thingy:91, the partition layout is defined in devicetree.
The board provides a default layout in the :file:`nrf/boards/nordic/thingy91/thingy91_nrf9160_partition.dtsi` file in the |NCS| installation, which is included by the board devicetree and matches the factory layout.
This setup ensures that when you program the firmware through USB, it works correctly without updating the MCUboot bootloader.
You must keep the image partitions in their original place to avoid compatibility issues.

The default layout reserves the following partitions:

.. list-table::
   :header-rows: 1

   * - Partition
     - Address
     - Size
   * - MCUboot (``boot_partition``)
     - ``0x0000_0000``
     - 48 kB
   * - Primary image (``slot0_partition``, split into a secure and a non-secure partition)
     - ``0x0000_c000``
     - 420 kB
   * - Secondary image (``slot1_partition``, split into a secure and a non-secure partition)
     - ``0x0007_5000``
     - 420 kB
   * - DFU scratch area (``scratch_partition``)
     - ``0x000d_e000``
     - 120 kB
   * - Settings storage (``storage_partition``)
     - ``0x000f_e000``
     - 8 kB

When you use an external debug probe to program the Thingy:91, you can update all the memory sections, including the MCUboot bootloader.
This allows you to use a newer version of the bootloader or define an application-specific partition layout.

Using a custom partition layout
-------------------------------

To use a layout that differs from the default one, add a devicetree overlay for the ``thingy91/nrf9160/ns`` board target to your application, as described in :ref:`configure_application_hw`.
Because devicetree overlays of the main application are not applied to the other images built by sysbuild, you must also add a matching overlay for MCUboot in the :file:`sysbuild/mcuboot/boards/thingy91_nrf9160.overlay` file of your application.
MCUboot runs in secure mode, so sysbuild builds it for the ``thingy91/nrf9160`` board target, without the ``/ns`` variant.

Consider the following when replacing the default layout:

* Adding a partition for the :ref:`liblwm2m_carrier_readme` library - Define an ``lwm2m_carrier`` partition and select it with the ``nordic,lwm2m-carrier-partition`` chosen node.
  For an example, see the overlay files in the :file:`nrf/samples/cellular/lwm2m_carrier/boards/` directory.
* Adding the |NSIB| - Enable the :kconfig:option:`SB_CONFIG_SECURE_BOOT_APPCORE` Kconfig option and reserve space for the immutable bootloader and the two MCUboot slots in the overlay, as described in :ref:`ug_bootloader_adding_sysbuild`.
  You need a debugger to program the Thingy:91 for the first time.

.. note::

   A Thingy:91 programmed with a layout that does not match the factory layout can no longer be updated through USB using MCUboot.
   Reprogramming such a device requires an external debug probe.
