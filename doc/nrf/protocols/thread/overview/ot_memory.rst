.. _thread_ot_memory:

OpenThread memory requirements
##############################

.. contents::
   :local:
   :depth: 2

This page provides information about the layout and the amount of flash memory and RAM that is required by samples that use the OpenThread stack.
Use these values to see whether your application has enough space for running on particular platforms for different application configurations.

.. _thread_ot_memory_introduction:

How to read the tables
**********************

The memory requirement tables list flash memory and RAM requirements for the :ref:`Thread CLI sample <ot_cli_sample>` that was built with OpenThread pre-built libraries.

The values change depending on device type and hardware platform.
Moreover, take into account the following considerations:

* The sample was compiled using:

  * The default :file:`prj.conf` configuration file and the corresponding Kconfig options :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY` and :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION` selected.
    See the :ref:`ug_thread_select_libraries` documentation for more information on the Kconfig choices.
  * The :kconfig:option:`CONFIG_ASSERT` Kconfig option set to ``n``.

* Values for the :ref:`Thread CLI sample <ot_cli_sample>`, which works with all OpenThread calls, are the highest possible for the OpenThread stack using the master image library configuration.

The tables provide memory requirements for the following device type variants:

* *FTD* - Full Thread Device.
* *MTD* - Minimal Thread Device.

Some tables also list a *master* variant, which is an FTD with additional features, such as being able to have the *commissioner* or *border router* commissioning roles.
See :ref:`thread_device_types` for more information on device types, and :ref:`thread_ot_commissioning` for more information on commissioning roles.

.. _thread_ot_memory_5340:

nRF5340 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF5340 DK <programming_board_names>` (:ref:`nrf5340dk <zephyr:nrf5340dk_nrf5340>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF5340 single protocol Thread 1.3 memory requirements

   +-----------------------------+-------+-------+
   |                             |   FTD |   MTD |
   +=============================+=======+=======+
   | ROM OT stack + App [kB]     |   330 |   275 |
   +-----------------------------+-------+-------+
   | ROM Bluetooth LE stack [kB] |     0 |     0 |
   +-----------------------------+-------+-------+
   | Persistent storage [kB]     |    32 |    32 |
   +-----------------------------+-------+-------+
   | Free ROM [kB]               |   662 |   717 |
   +-----------------------------+-------+-------+
   | RAM OT stack + App [kB]     |   100 |    90 |
   +-----------------------------+-------+-------+
   | RAM Bluetooth LE stack [kB] |     0 |     0 |
   +-----------------------------+-------+-------+
   | Free RAM [kB]               |   412 |   422 |
   +-----------------------------+-------+-------+
.. table:: nRF5340 multiprotocol Thread 1.3 memory requirements

   +-----------------------------+-------+-------+
   |                             |   FTD |   MTD |
   +=============================+=======+=======+
   | ROM OT stack + App [kB]     |   330 |   275 |
   +-----------------------------+-------+-------+
   | ROM Bluetooth LE stack [kB] |    30 |    30 |
   +-----------------------------+-------+-------+
   | Persistent storage [kB]     |    32 |    32 |
   +-----------------------------+-------+-------+
   | Free ROM [kB]               |   632 |   687 |
   +-----------------------------+-------+-------+
   | RAM OT stack + App [kB]     |   100 |    90 |
   +-----------------------------+-------+-------+
   | RAM Bluetooth LE stack [kB] |    15 |    15 |
   +-----------------------------+-------+-------+
   | Free RAM [kB]               |   397 |   407 |
   +-----------------------------+-------+-------+

.. _thread_ot_memory_52840:

nRF52840 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF52840 DK <programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF52840 single protocol Thread 1.3 memory requirements

   +-----------------------------+----------+-------+-------+
   |                             |   master |   FTD |   MTD |
   +=============================+==========+=======+=======+
   | ROM OT stack + App [kB]     |      454 |   360 |   305 |
   +-----------------------------+----------+-------+-------+
   | ROM Bluetooth LE stack [kB] |        0 |     0 |     0 |
   +-----------------------------+----------+-------+-------+
   | Persistent storage [kB]     |       32 |    32 |    32 |
   +-----------------------------+----------+-------+-------+
   | Free ROM [kB]               |      538 |   632 |   687 |
   +-----------------------------+----------+-------+-------+
   | RAM OT stack + App [kB]     |      101 |    93 |    83 |
   +-----------------------------+----------+-------+-------+
   | RAM Bluetooth LE stack [kB] |        0 |     0 |     0 |
   +-----------------------------+----------+-------+-------+
   | Free RAM [kB]               |      155 |   163 |   173 |
   +-----------------------------+----------+-------+-------+
.. table:: nRF52840 multiprotocol Thread 1.3 memory requirements

   +-----------------------------+----------+-------+-------+
   |                             |   master |   FTD |   MTD |
   +=============================+==========+=======+=======+
   | ROM OT stack + App [kB]     |      454 |   360 |   305 |
   +-----------------------------+----------+-------+-------+
   | ROM Bluetooth LE stack [kB] |       77 |    77 |    77 |
   +-----------------------------+----------+-------+-------+
   | Persistent storage [kB]     |       32 |    32 |    32 |
   +-----------------------------+----------+-------+-------+
   | Free ROM [kB]               |      461 |   555 |   610 |
   +-----------------------------+----------+-------+-------+
   | RAM OT stack + App [kB]     |      101 |    93 |    83 |
   +-----------------------------+----------+-------+-------+
   | RAM Bluetooth LE stack [kB] |       14 |    14 |    14 |
   +-----------------------------+----------+-------+-------+
   | Free RAM [kB]               |      141 |   149 |   159 |
   +-----------------------------+----------+-------+-------+
