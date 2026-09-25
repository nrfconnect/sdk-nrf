.. _wifi_radio_sample_desc_nrf71:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE Wi-Fi Radio test (Single domain) sample demonstrates how to configure the Wi-Fi® radio in a specific mode and then test its performance.
It provides a set of predefined commands that allow you to configure the radio in the following modes:

* Modulated carrier TX
* Modulated carrier RX
* Tone transmission
* IQ sample capture at ADC output

Requirements
************

The sample supports the following development kit:

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - Board target
   * - nRF7120 DK
     - ``nrf7120dk/nrf7120/cpuapp``, ``nrf7120dk/nrf7120/cpuapp/ns``

Overview
********

To run the tests, connect to the development kit through the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.

You can start running ``wifi_radio_test`` subcommands to set up and control the radio.
See :ref:`wifi_radio_subcommands_nrf71` for a list of available subcommands.

In the Modulated carrier RX mode, you can use the ``get_stats`` subcommand to display the statistics.
See :ref:`wifi_radio_subcommands_nrf71` for a list of available statistics.

.. _wifi_radio_sd_nrf71_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/radio_test/single_domain`

.. include:: /includes/build_and_run.txt

To build for the nRF7120 DK, use the ``nrf7120dk/nrf7120/cpuapp`` board target.
Because sysbuild defaults to the nRF70 Series driver, disable it with the ``SB_CONFIG_WIFI_NRF70`` sysbuild Kconfig option.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -- -DSB_CONFIG_WIFI_NRF70=n

See also :ref:`cmake_options` for instructions on how to provide CMake options.

.. include:: /includes/wifi_refer_sample_yaml_file.txt

Dependencies
************

This sample uses the following Zephyr library:

* :ref:`zephyr:shell_api`:

  * :file:`include/shell/shell.h`
