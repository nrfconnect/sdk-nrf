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
=====================

The nRF Wi-Fi driver, WPA supplicant, and networking stack have debug features that can be enabled to help debug issues.

You can enable debug features by using the :ref:`app_build_snippets` feature.

For example, to build the :ref:`wifi_shell_sample` sample for the nRF7002 DK with debugging enabled, run the following commands:

With west
---------

.. code-block:: console

    west build -p -b nrf7002dk/nrf5340/cpuapp samples/wifi/shell -- -Dnrf_wifi_shell_SNIPPET="nrf70-debug"

With CMake
----------

.. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf7002dk/nrf5340/cpuapp -Dnrf_wifi_shell_SNIPPET="nrf70-debug" samples/wifi/shell
    ninja -C build

Statistics
==========

The nRF Wi-Fi driver, firmware, and networking stack have statistics feature that can be enabled to help debug issues.

You can enable statistics by using the ``nrf70-debug`` snippet.
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
