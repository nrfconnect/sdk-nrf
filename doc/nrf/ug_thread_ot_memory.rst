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

The following tables present memory requirements for samples running on the :ref:`nRF52840 DK <gs_programming_board_names>` (:ref:`nrf52840dk_nrf52840 <zephyr:nrf52840dk_nrf52840>`) with the hardware cryptography support provided by the CC310.

.. table:: nRF52840 single protocol memory requirements

   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OpenThread stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OpenThread stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===================================+===============================+===========================+=================+===================================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                               398 |                             0 |                        32 |             594 |                                81 |                             0 |             175 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                               379 |                             0 |                        32 |             613 |                                80 |                             0 |             176 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                               322 |                             0 |                        32 |             670 |                                72 |                             0 |             184 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+


.. table:: nRF52840 multiprotocol memory requirements

   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OpenThread stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OpenThread stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===================================+===============================+===========================+=================+===================================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                               398 |                            73 |                        32 |             521 |                                81 |                            11 |             164 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                               379 |                            74 |                        32 |             539 |                                80 |                            11 |             165 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                               322 |                            73 |                        32 |             597 |                                72 |                            11 |             173 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+


.. _thread_ot_memory_52833:

nRF52833 DK RAM and flash memory requirements
*********************************************

The following tables present memory requirements for samples running on the :ref:`nRF52833 DK <gs_programming_board_names>` (:ref:`nrf52833dk_nrf52833 <zephyr:nrf52833dk_nrf52833>`) with the software cryptography support provided by the :ref:`nrfxlib:nrf_oberon_readme` module.

.. table:: nRF52833 single protocol memory requirements

   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OpenThread stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OpenThread stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===================================+===============================+===========================+=================+===================================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                               375 |                             0 |                        24 |             113 |                                88 |                             0 |              40 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                               356 |                             0 |                        24 |             132 |                                87 |                             0 |              41 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                               298 |                             0 |                        24 |             190 |                                79 |                             0 |              49 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+


.. table:: nRF52833 multiprotocol memory requirements

   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | Sample                             | Variant   |   ROM OpenThread stack + App [kB] |   ROM Bluetooth LE stack [kB] |   Persistent storage [kB] |   Free ROM [kB] |   RAM OpenThread stack + App [kB] |   RAM Bluetooth LE stack [kB] |   Free RAM [kB] |
   +====================================+===========+===================================+===============================+===========================+=================+===================================+===============================+=================+
   | :ref:`CLI <ot_cli_sample_minimal>` | master    |                               375 |                            74 |                        24 |              39 |                                88 |                            11 |              29 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | ftd       |                               356 |                            74 |                        24 |              58 |                                87 |                            11 |              30 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
   | :ref:`CLI <ot_cli_sample_minimal>` | mtd       |                               298 |                            74 |                        24 |             116 |                                79 |                            11 |              38 |
   +------------------------------------+-----------+-----------------------------------+-------------------------------+---------------------------+-----------------+-----------------------------------+-------------------------------+-----------------+
