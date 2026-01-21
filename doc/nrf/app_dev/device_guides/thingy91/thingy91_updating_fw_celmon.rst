.. _thingy91_update_firmware:
.. _programming_thingy:

Updating the Thingy:91 firmware using nRF Connect for Desktop apps
##################################################################

.. contents::
   :local:
   :depth: 2

Thingy:91 (v1.5.0 or earlier) comes preloaded with the nRF9160: Asset Tracker firmware and modem firmware on the nRF9160 :term:`System in Package (SiP)`, and the Connectivity bridge application firmware on the nRF52840 :term:`System on Chip (SoC)` that enable the device to use the environment sensors and track the device using :term:`Global Positioning System (GPS)`.
The data is transmitted to nRF Cloud.

.. tip::
   For a more compact nRF Cloud firmware application, you can build and install the :ref:`nrf_cloud_multi_service` sample.
   See the :ref:`building_pgming` section for more information.

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

When building firmware on Nordic Thingy:91, a static partition layout matching the factory layout is used.
This setup ensures that when you program the firmware through USB, it works correctly without updating the MCUboot bootloader.
You must keep the image partitions in their original place to avoid compatibility issues.
When you use an external debug probe to program the Thingy:91, you can update all the memory sections, including the MCUboot bootloader.
This allows you to use a newer version of the bootloader or define an application-specific partition layout.

Configure the partition layout using one of the following configuration options:

* :kconfig:option:`CONFIG_THINGY91_STATIC_PARTITIONS_FACTORY` - This option is the default Thingy:91 partition layout used in the factory firmware.
  This ensures firmware updates are compatible with Thingy:91 when programming firmware through USB.
* :kconfig:option:`CONFIG_THINGY91_STATIC_PARTITIONS_SECURE_BOOT` - This option is similar to the factory partition layout, but also has space for the immutable bootloader and two MCUboot slots.
  You need a debugger to program Thingy:91 for the first time.
  This is an :ref:`experimental <software_maturity>` feature.
* :kconfig:option:`CONFIG_THINGY91_STATIC_PARTITIONS_LWM2M_CARRIER` - This option uses a partition layout, including a storage partition needed for the :ref:`liblwm2m_carrier_readme` library.
* :kconfig:option:`CONFIG_THINGY91_NO_PREDEFINED_LAYOUT` - Enabling this option disables Thingy:91 pre-defined static partitions.
  This allows the application to use a dynamic layout or define a custom static partition layout for the application.
  You need a debugger to program Thingy:91 for the first time.
  This is an :ref:`experimental <software_maturity>` feature.
