.. _wifi_radio_sample_desc_sd:

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

The sample also shows how to program the user region of FICR parameters on the development kit using a set of predefined commands.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

To run the tests, connect to the development kit through the serial port and send shell commands.
Zephyr's :ref:`zephyr:shell_api` module is used to handle the commands.

You can start running ``wifi_radio_test`` subcommands to set up and control the radio.
See :ref:`wifi_radio_subcommands` for a list of available subcommands.

In the Modulated carrier RX mode, you can use the ``get_stats`` subcommand to display the statistics.
See :ref:`wifi_radio_subcommands` for a list of available statistics.

You can use ``wifi_radio_ficr_prog`` subcommands to read or write OTP registers.
See :ref:`wifi_ficr_prog` for a list of available subcommands.

.. note::

   All the FICR registers are stored in the one-time programmable (OTP) memory.
   Consequently, the write commands are destructive.
   Once written, the contents of the OTP registers cannot be reprogrammed.

.. _wifi_radio_sd_sample_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/radio_test/single_domain`

.. include:: /includes/build_and_run.txt

To build for the nRF7002-EB II and nRF54L15 DK, use the ``nrf54l15dk/nrf54l15/cpuapp`` board target with the ``SHIELD`` CMake option set to ``nrf7002eb2``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf54l15dk/nrf54l15/cpuapp -- -DSHIELD=nrf7002eb2 -DSNIPPET=nrf70-wifi

See also :ref:`cmake_options` for instructions on how to provide CMake options.

.. include:: /includes/wifi_refer_sample_yaml_file.txt

See the :ref:`Peripheral radio test sample <radio_test>` for detailed documentation on the related features.

Dependencies
************

This sample uses the following Zephyr library:

* :ref:`zephyr:shell_api`:

  * :file:`include/shell/shell.h`
