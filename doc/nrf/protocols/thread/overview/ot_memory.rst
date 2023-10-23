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

The following tables present memory requirements for samples running on the :ref:`nRF5340 DK <programming_board_names>` (:ref:`nrf5340dk_nrf5340 <zephyr:nrf5340dk_nrf5340>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF5340 single protocol Thread 1.3 memory requirements

   +-----------------------------+-------+-------+
   |                             |   FTD |   MTD |
   +=============================+=======+=======+
   | ROM OT stack + App [kB]     |   387 |   334 |
   +-----------------------------+-------+-------+
   | ROM Bluetooth LE stack [kB] |     0 |     0 |
   +-----------------------------+-------+-------+
   | Persistent storage [kB]     |    24 |    24 |
   +-----------------------------+-------+-------+
   | Free ROM [kB]               |   613 |   666 |
   +-----------------------------+-------+-------+
   | RAM OT stack + App [kB]     |   106 |    96 |
   +-----------------------------+-------+-------+
   | RAM Bluetooth LE stack [kB] |     0 |     0 |
   +-----------------------------+-------+-------+
   | Free RAM [kB]               |   406 |   416 |
   +-----------------------------+-------+-------+
.. table:: nRF5340 multiprotocol Thread 1.3 memory requirements

   +-----------------------------+-------+-------+
   |                             |   FTD |   MTD |
   +=============================+=======+=======+
   | ROM OT stack + App [kB]     |   387 |   334 |
   +-----------------------------+-------+-------+
   | ROM Bluetooth LE stack [kB] |    30 |    29 |
   +-----------------------------+-------+-------+
   | Persistent storage [kB]     |    24 |    24 |
   +-----------------------------+-------+-------+
   | Free ROM [kB]               |   583 |   637 |
   +-----------------------------+-------+-------+
   | RAM OT stack + App [kB]     |   106 |    96 |
   +-----------------------------+-------+-------+
   | RAM Bluetooth LE stack [kB] |    11 |    11 |
   +-----------------------------+-------+-------+
   | Free RAM [kB]               |   395 |   405 |
   +-----------------------------+-------+-------+

.. _thread_ot_memory_52840:

nRF52840 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF52840 DK <programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF52840 single protocol Thread 1.3 memory requirements

   +-----------------------------+----------+-------+-------+
   |                             |   master |   FTD |   MTD |
   +=============================+==========+=======+=======+
   | ROM OT stack + App [kB]     |      468 |   435 |   382 |
   +-----------------------------+----------+-------+-------+
   | ROM Bluetooth LE stack [kB] |        0 |     0 |     0 |
   +-----------------------------+----------+-------+-------+
   | Persistent storage [kB]     |       32 |    32 |    32 |
   +-----------------------------+----------+-------+-------+
   | Free ROM [kB]               |      524 |   557 |   610 |
   +-----------------------------+----------+-------+-------+
   | RAM OT stack + App [kB]     |      111 |   107 |    97 |
   +-----------------------------+----------+-------+-------+
   | RAM Bluetooth LE stack [kB] |        0 |     0 |     0 |
   +-----------------------------+----------+-------+-------+
   | Free RAM [kB]               |      145 |   149 |   159 |
   +-----------------------------+----------+-------+-------+
.. table:: nRF52840 multiprotocol Thread 1.3 memory requirements

   +-----------------------------+----------+-------+-------+
   |                             |   master |   FTD |   MTD |
   +=============================+==========+=======+=======+
   | ROM OT stack + App [kB]     |      468 |   435 |   382 |
   +-----------------------------+----------+-------+-------+
   | ROM Bluetooth LE stack [kB] |       84 |    84 |    84 |
   +-----------------------------+----------+-------+-------+
   | Persistent storage [kB]     |       32 |    32 |    32 |
   +-----------------------------+----------+-------+-------+
   | Free ROM [kB]               |      440 |   473 |   526 |
   +-----------------------------+----------+-------+-------+
   | RAM OT stack + App [kB]     |      111 |   107 |    97 |
   +-----------------------------+----------+-------+-------+
   | RAM Bluetooth LE stack [kB] |       15 |    16 |    16 |
   +-----------------------------+----------+-------+-------+
   | Free RAM [kB]               |      130 |   133 |   143 |
   +-----------------------------+----------+-------+-------+
