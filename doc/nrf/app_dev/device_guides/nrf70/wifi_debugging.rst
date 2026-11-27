.. _ug_wifi_developing_debugging:
.. _ug_nrf70_developing_debugging:

Debugging
#########

.. contents::
   :local:
   :depth: 2

This guide describes how to debug applications that use the nRF70 Series companion chips.

Software
********

The scope of this section is limited to the WPA supplicant, networking stack, and nRF Wi-Fi driver.

Enable debug features
*********************

The nRF Wi-Fi driver, WPA supplicant, and networking stack have debug features that can be enabled to help debug issues.

You can enable debug features by using the :ref:`app_build_snippets` feature.
The |NCS| provides the following snippets for this purpose:

* :ref:`snippet-wpa-supplicant-debug` (``wpa-supplicant-debug``) - Enables the WPA supplicant debug logs.
  The WPA supplicant manages the Wi-Fi® connection and is responsible for the 802.11 protocol.
  Use this snippet to debug issues related to the Wi-Fi connection, such as failure to initiate, failed connections, or disconnections.
* :ref:`snippet-nrf70-driver-debug` (``nrf70-driver-verbose-debug``) - Enables the driver debug logs.
  The nRF Wi-Fi driver manages the data path and control path between the host and the nRF70 firmware.
  Use this snippet to debug issues related to the data path and control path, such as not receiving or sending packets, failures in sending commands, or not receiving events.
* :ref:`snippet-nrf70-driver-verbose-debug` (``nrf70-driver-debug``) - Enables the driver debug logs and, in addition, the BUS interface level debug logs.

see :ref:`ug_snippets` for more details where these snippets are documented.

Each snippet page lists the Kconfig options that the snippet sets and shows how to build the :ref:`wifi_shell_sample` sample with the snippet applied.

Statistics
**********

The nRF Wi-Fi driver, firmware, and networking stack have statistics feature that can be enabled to help debug issues.

You can enable statistics by using either the ``wpa-supplicant-debug``, ``nrf70-driver-verbose-debug``, or ``nrf70-driver-debug`` snippet.
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
    * - ``nrf70 util tx_stats <vif_index>``
      - Displays transmit statistics for the nRF Wi-Fi driver.
      - Data path debugging (nRF Wi-Fi driver TX)
    * - ``nrf70 util rpu_stats all`` [1]_
      - Displays statistics for the nRF70 firmware (all modules, support for specific modules is also available).
      - nRF70 firmware debugging (Data and control path)
    * - ``nrf70 util rpu_stats_mem all`` [2]_
      - Displays memory statistics for the nRF70 firmware (all modules, support for specific modules is also available).
      - nRF70 firmware debugging (Data and control path) even when the control plane is not functional
.. [1] This command only works when the nRF70 control plane is functional, as it uses the control plane to retrieve the statistics.
.. [2] This command always works independent of the processors state as it retrieves the statistics from the RPU memory directly.

.. note::
   All statistics, especially data path statistics, must be collected multiple times to see the incremental changes and understand the behavior.
