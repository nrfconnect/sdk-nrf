.. _snippet-nrf71-driver-debug:

nRF71 driver debug snippet (nrf71-driver-debug)
################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet enables debug logging and statistics for the nRF71 Wi-Fi® driver.
Use it to debug issues in the data path and control path, such as not receiving or sending packets, failures in sending commands, or not receiving events.

The snippet applies the following configuration:

* Enables the shell (:kconfig:option:`CONFIG_SHELL`) and the network shell (:kconfig:option:`CONFIG_NET_SHELL`), and increases the shell stack size (:kconfig:option:`CONFIG_SHELL_STACK_SIZE`) to accommodate the additional shell commands.
* Enables getopt-style argument parsing for shell commands (:kconfig:option:`CONFIG_SHELL_GETOPT`) and disables the shell resize command (:kconfig:option:`CONFIG_SHELL_CMDS_RESIZE`) to save space.
* Enables the nRF71 utility shell commands (:kconfig:option:`CONFIG_NRF71_UTIL`) and the Wi-Fi L2 shell commands (:kconfig:option:`CONFIG_NET_L2_WIFI_SHELL`).
* Enables network statistics (:kconfig:option:`CONFIG_NET_STATISTICS`), Wi-Fi statistics (:kconfig:option:`CONFIG_NET_STATISTICS_WIFI`), and the statistics user API (:kconfig:option:`CONFIG_NET_STATISTICS_USER_API`).
* Enables heap runtime statistics (:kconfig:option:`CONFIG_SYS_HEAP_RUNTIME_STATS`).
* Enables logging (:kconfig:option:`CONFIG_LOG`) in immediate mode (:kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE`) and enables :kconfig:option:`CONFIG_PRINTK`.
* Sets the nRF71 driver log level to debug (:kconfig:option:`CONFIG_WIFI_NRF71_LOG_LEVEL_DBG`).

.. note::
   Enabling :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` can help prevent log buffer overflows, but it may impact system timing and performance.
   If you experience issues with system timing or performance, disable this option by setting :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` to ``n`` in your project configuration file.

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample on the nRF7120 DK:

With west

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- -Dshell_SNIPPET="nrf71-driver-debug"

With CMake

.. code-block:: console

   cmake -GNinja -Bbuild -DBOARD=nrf7120dk/nrf7120/cpuapp -Dshell_SNIPPET="nrf71-driver-debug" samples/wifi/shell
   ninja -C build

See also
********

* :ref:`ug_nrf70_developing_debugging` for the equivalent guidance for the nRF70 Series.
* :ref:`snippet-nrf71-driver-verbose-debug` for additional IPC-level debug logging.
