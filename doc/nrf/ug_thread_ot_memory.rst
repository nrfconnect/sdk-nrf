.. _thread_ot_memory:

OpenThread memory requirements
##############################

.. contents::
   :local:
   :depth: 2

This page provides information about the layout and the amount of flash memory and RAM that is required by sample that are using the OpenThread stack.
Use these values to see whether your application has enough space for running on particular platforms for different application configurations.

.. _thread_ot_memory_introduction:

How to read the tables
**********************

The memory requirement tables list flash memory and RAM requirements for samples that were built using the available :ref:`nrfxlib:ot_libs`.

The values change depending on the sample, device type, and hardware platform.
Moreover, take into account the following considerations:

* All samples were compiled using the default :file:`prj.conf`, the overlay :file:`overlay-ot-defaults`, and images from :ref:`nrfxlib:ot_libs`.
* The single protocol samples were optimized with the overlay :file:`overlay-minimal_singleprotocol.conf`.
* The multiprotocol samples were optimized with the overlay :file:`overlay-minimal_multiprotocol.conf`.
* To enable the multiprotocol support, the following options were used:

  * :option:`CONFIG_MPSL` set to ``y`` (default setting for all samples)
  * :option:`CONFIG_BT_LL_SOFTDEVICE_DEFAULT` set to ``y``
  * :option:`CONFIG_BT` set to ``y``
  * :option:`CONFIG_BT_PERIPHERAL` set to ``y``
  * :option:`CONFIG_BT_DEVICE_NAME` set to ``"NCS DUT"``
  * :option:`CONFIG_BT_DEVICE_APPEARANCE` set to ``833``
  * :option:`CONFIG_BT_MAX_CONN` set to ``1``

* Values for the :ref:`Thread CLI sample <ot_cli_sample>`, which works with all OpenThread calls, are the highest possible for the OpenThread stack using the master image library configuration.

.. _thread_ot_memory_52840:

nRF52840 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF52840 DK <gs_programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`) with the hardware cryptography support provided by the :ref:`CC310 <nrfxlib:nrf_cc3xx_platform_readme>`.

.. table:: nRF52840 single protocol memory requirements

   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OT stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OT stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===========================+===============================+===========================+=================+===========================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                       315 |                             0 |                        32 |             677 |                        73 |                             0 |             183 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                       296 |                             0 |                        32 |             696 |                        72 |                             0 |             184 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                       262 |                             0 |                        32 |             730 |                        64 |                             0 |             192 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+

.. table:: nRF52840 multiprotocol memory requirements

   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OT stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OT stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===========================+===============================+===========================+=================+===========================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                       315 |                            64 |                        32 |             613 |                        73 |                             9 |             174 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                       296 |                            64 |                        32 |             632 |                        72 |                             9 |             175 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                       262 |                            64 |                        32 |             666 |                        64 |                             9 |             183 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+

.. _thread_ot_memory_52833:

nRF52833 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF52833 DK <gs_programming_board_names>` (:ref:`nrf52833dk_nrf52833 <zephyr:nrf52833dk_nrf52833>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF52833 single protocol memory requirements

   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OT stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OT stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===========================+===============================+===========================+=================+===========================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                       291 |                             0 |                        24 |             197 |                        80 |                             0 |              48 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                       273 |                             0 |                        24 |             215 |                        79 |                             0 |              49 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                       239 |                             0 |                        24 |             249 |                        71 |                             0 |              57 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+

.. table:: nRF52833 multiprotocol memory requirements

   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OT stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OT stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===========================+===============================+===========================+=================+===========================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                       291 |                            64 |                        24 |             133 |                        80 |                             9 |              39 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                       273 |                            63 |                        24 |             152 |                        79 |                             9 |              40 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                       239 |                            63 |                        24 |             186 |                        71 |                             9 |              48 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+

.. _thread_ot_memory_5340:

nRF5340 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF5340 DK <gs_programming_board_names>` (:ref:`nrf5340dk_nrf5340 <zephyr:nrf5340dk_nrf5340>`) with the hardware cryptography support provided by the :ref:`CC312 <nrfxlib:nrf_cc3xx_platform_readme>`.

.. table:: nRF5340 single protocol memory requirements.

   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OT stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OT stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===========================+===============================+===========================+=================+===========================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                       283 |                             0 |                        24 |             717 |                        82 |                             0 |             430 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                       265 |                             0 |                        24 |             735 |                        81 |                             0 |             431 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                       231 |                             0 |                        24 |             769 |                        73 |                             0 |             439 |
   +------------------------------------+-----------+---------------------------+-------------------------------+---------------------------+-----------------+---------------------------+-------------------------------+-----------------+
