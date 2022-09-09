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

* The sample was compiled using the default :file:`prj.conf` plus :file:`harness\overlay-cert.conf`  and :file:`overlay-multiprotocol.conf` overlay files, and the corresponding :kconfig:option:`OPENTHREAD_NORDIC_LIBRARY`, :kconfig:option:`OPENTHREAD_THREAD_VERSION` choices.
* The single protocol sample was configured by setting :kconfig:option:`CONFIG_BT` to ``n``.
* Values for the :ref:`Thread CLI sample <ot_cli_sample>`, which works with all OpenThread calls, are the highest possible for the OpenThread stack using the master image library configuration.

The tables provide memory requirements for the following device type variants:

* *FTD* - Full Thread Device.
* *MTD* - Minimal Thread Device.

Some tables also list a *master* variant, which is an FTD with additional features, such as being able to have the *commissioner* or *border router* commissioning roles.
See :ref:`thread_ug_feature_sets` for more information.

.. _thread_ot_memory_5340:

nRF5340 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF5340 DK <gs_programming_board_names>` (:ref:`nrf5340dk_nrf5340 <zephyr:nrf5340dk_nrf5340>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF5340 single protocol Thread 1.3 memory requirements

   +-----------------------------+-------+-------+
   |                             |   FTD |   MTD |
   +=============================+=======+=======+
   | ROM OT stack + App [kB]     |   390 |   337 |
   +-----------------------------+-------+-------+
   | ROM Bluetooth LE stack [kB] |     0 |     0 |
   +-----------------------------+-------+-------+
   | Persistent storage [kB]     |    24 |    24 |
   +-----------------------------+-------+-------+
   | Free ROM [kB]               |   610 |   663 |
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
   | ROM OT stack + App [kB]     |   390 |   337 |
   +-----------------------------+-------+-------+
   | ROM Bluetooth LE stack [kB] |    33 |    33 |
   +-----------------------------+-------+-------+
   | Persistent storage [kB]     |    24 |    24 |
   +-----------------------------+-------+-------+
   | Free ROM [kB]               |   577 |   630 |
   +-----------------------------+-------+-------+
   | RAM OT stack + App [kB]     |   100 |    90 |
   +-----------------------------+-------+-------+
   | RAM Bluetooth LE stack [kB] |    11 |    11 |
   +-----------------------------+-------+-------+
   | Free RAM [kB]               |   401 |   411 |
   +-----------------------------+-------+-------+

.. _thread_ot_memory_52840:

nRF52840 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF52840 DK <gs_programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF52840 single protocol Thread 1.3 memory requirements

   +-----------------------------+----------+-------+-------+
   |                             |   master |   FTD |   MTD |
   +=============================+==========+=======+=======+
   | ROM OT stack + App [kB]     |      459 |   433 |   380 |
   +-----------------------------+----------+-------+-------+
   | ROM Bluetooth LE stack [kB] |        0 |     0 |     0 |
   +-----------------------------+----------+-------+-------+
   | Persistent storage [kB]     |       32 |    32 |    32 |
   +-----------------------------+----------+-------+-------+
   | Free ROM [kB]               |      533 |   559 |   612 |
   +-----------------------------+----------+-------+-------+
   | RAM OT stack + App [kB]     |       99 |    96 |    86 |
   +-----------------------------+----------+-------+-------+
   | RAM Bluetooth LE stack [kB] |        0 |     0 |     0 |
   +-----------------------------+----------+-------+-------+
   | Free RAM [kB]               |      157 |   160 |   170 |
   +-----------------------------+----------+-------+-------+

.. table:: nRF52840 multiprotocol Thread 1.3 memory requirements

   +-----------------------------+----------+-------+-------+
   |                             |   master |   FTD |   MTD |
   +=============================+==========+=======+=======+
   | ROM OT stack + App [kB]     |      459 |   433 |   380 |
   +-----------------------------+----------+-------+-------+
   | ROM Bluetooth LE stack [kB] |       86 |    87 |    86 |
   +-----------------------------+----------+-------+-------+
   | Persistent storage [kB]     |       32 |    32 |    32 |
   +-----------------------------+----------+-------+-------+
   | Free ROM [kB]               |      447 |   472 |   526 |
   +-----------------------------+----------+-------+-------+
   | RAM OT stack + App [kB]     |       99 |    96 |    86 |
   +-----------------------------+----------+-------+-------+
   | RAM Bluetooth LE stack [kB] |       17 |    16 |    16 |
   +-----------------------------+----------+-------+-------+
   | Free RAM [kB]               |      140 |   144 |   154 |
   +-----------------------------+----------+-------+-------+
