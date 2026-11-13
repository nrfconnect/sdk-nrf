.. _snippet-wpa-supplicant-debug:

WPA supplicant debug snippet (wpa-supplicant-debug)
###################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet enables debug logging and statistics for the WPA supplicant.
The WPA supplicant manages the Wi-Fi® connection and is responsible for the 802.11 protocol.
Use it to debug issues related to the Wi-Fi connection, such as failure to initiate, failed connections, or disconnections.

The snippet applies the following configuration:

* Enables the shell (:kconfig:option:`CONFIG_SHELL`) and the network shell (:kconfig:option:`CONFIG_NET_SHELL`), and increases the shell stack size (:kconfig:option:`CONFIG_SHELL_STACK_SIZE`) to accommodate the additional shell commands.
* Enables getopt-style argument parsing for shell commands (:kconfig:option:`CONFIG_SHELL_GETOPT`) and disables the shell resize command (:kconfig:option:`CONFIG_SHELL_CMDS_RESIZE`) to save space.
* Enables the Wi-Fi L2 shell commands (:kconfig:option:`CONFIG_NET_L2_WIFI_SHELL`).
* Enables network statistics (:kconfig:option:`CONFIG_NET_STATISTICS`), Wi-Fi statistics (:kconfig:option:`CONFIG_NET_STATISTICS_WIFI`), and the statistics user API (:kconfig:option:`CONFIG_NET_STATISTICS_USER_API`).
* Enables logging (:kconfig:option:`CONFIG_LOG`) in immediate mode (:kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE`) and enables :kconfig:option:`CONFIG_PRINTK`.
* Sets the WPA supplicant log level to debug (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_LOG_LEVEL_DBG`).

.. note::
   By default, this feature enables the :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` Kconfig option, which can help prevent log buffer overflows.
   However, it may impact system timing and performance.
   If you experience issues with the system timing or performance, you can disable this option by setting the :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` Kconfig option to ``n`` in your project configuration file.

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample on the nRF7002 DK:

With west

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp samples/wifi/shell -- -Dshell_SNIPPET="wpa-supplicant-debug"

With CMake

.. code-block:: console

   cmake -GNinja -Bbuild -DBOARD=nrf7002dk/nrf5340/cpuapp -Dshell_SNIPPET="wpa-supplicant-debug" samples/wifi/shell
   ninja -C build

See also
********

* :ref:`ug_nrf70_developing_debugging` for the statistics shell commands that this snippet enables.
