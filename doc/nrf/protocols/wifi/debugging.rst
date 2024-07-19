.. _ug_nrf70_developing_debugging:

Debugging
#########

.. contents::
   :local:
   :depth: 2

This guide describes how to debug applications that use the nRF70 Series companion chips.

Software
********

The scope of this section is limited to the nRF Wi-Fi driver, WPA supplicant and networking stack.

Enable debug features
*********************

The nRF Wi-Fi driver, WPA supplicant, and networking stack have debug features that can be enabled to help debug issues.

You can enable debug features by using the :ref:`app_build_snippets` feature.

For example, to build the :ref:`wifi_shell_sample` sample for the nRF7002 DK with debugging enabled, run the following commands.

Driver debug logs
=================

To build with driver verbose, firmware interface, and BUS interface debug logs enabled, run the following commands:

Basic driver debug
------------------

With west

 .. code-block:: console

    west build -p -b nrf7002dk/nrf5340/cpuapp samples/wifi/shell -- -Dnrf_wifi_shell_SNIPPET="nrf70-driver-debug"

With CMake

 .. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf7002dk/nrf5340/cpuapp -Dnrf_wifi_shell_SNIPPET="nrf70-driver-debug" samples/wifi/shell
    ninja -C build

BUS interface level debug
-------------------------

With west

 .. code-block:: console

    west build -p -b nrf7002dk/nrf5340/cpuapp samples/wifi/shell -- -Dnrf_wifi_shell_SNIPPET="nrf70-driver-verbose-debug"

With CMake

 .. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf7002dk/nrf5340/cpuapp -Dnrf_wifi_shell_SNIPPET="nrf70-driver-verbose-debug" samples/wifi/shell
    ninja -C build

WPA supplicant debug logs
=========================

To build with WPA supplicant debug logs enabled.

With west

 .. code-block:: console

    west build -p -b nrf7002dk/nrf5340/cpuapp samples/wifi/shell -- -Dnrf_wifi_shell_SNIPPET="wpa-supplicant-debug"

With CMake

 .. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf7002dk/nrf5340/cpuapp -Dnrf_wifi_shell_SNIPPET="wpa-supplicant-debug" samples/wifi/shell
    ninja -C build

.. note::

   Enabling the :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` Kconfig option can help prevent log buffer overflows.
   However, it may impact system timing and performance.

Statistics
==========

The nRF Wi-Fi driver, firmware, and networking stack have statistics feature that can be enabled to help debug issues.

You can enable statistics by using either the ``wpa-supplicant-debug``, ``nrf70-driver-verbose-debug``, or ``nrf70-driver-debug`` snippets.
See `Enable debug features`_.

.. list-table:: Statistics table
    :header-rows: 1

    * - Command
      - Description
      - Functional area
    * - ``net stats``
      - Displays statistics for the networking stack, network interfaces, and network protocols.
      - Data path debugging (Networking stack)
    * - ``wifi statistics``
      - Displays frame statistics for the nRF Wi-Fi driver.
      - Data path debugging (nRF Wi-Fi driver)
    * - ``wifi_util tx_stats <vif_index>``
      - Displays transmit statistics for the nRF Wi-Fi driver.
      - Data path debugging (nRF Wi-Fi driver TX)
    * - ``wifi_util rpu_stats all`` [1]_
      - Displays statistics for the nRF70 firmware (all modules, support for specific modules is also available).
      - nRF70 firmware debugging (Data and control path)
.. [1] This command only works when the nRF70 control plane is functional, as it uses the control plane to retrieve the statistics.

.. note::
   All statistics, especially data path statistics, must be collected multiple times to see the incremental changes and understand the behavior.
