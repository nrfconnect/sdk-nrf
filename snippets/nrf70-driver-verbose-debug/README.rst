.. _snippet-nrf70-driver-verbose-debug:

nRF70 driver verbose debug snippet (nrf70-driver-verbose-debug)
###############################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet enables the same debug logging and statistics as the :ref:`snippet-nrf70-driver-debug` snippet, and additionally enables BUS interface level debug logging for the nRF Wi-Fi® driver.
Use it when the basic driver debug output is not sufficient to identify a problem in the communication between the host and the nRF70 Series device.

In addition to the configuration of the ``nrf70-driver-debug`` snippet, this snippet applies the following configuration:

* Sets the nRF70 bus library log level to debug (:kconfig:option:`CONFIG_WIFI_NRF70_BUSLIB_LOG_LEVEL_DBG`).
* Increases the WPA supplicant thread stack size (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_THREAD_STACK_SIZE`) to accommodate the additional logging.

.. note::
   Enabling :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` can help prevent log buffer overflows, but it may impact system timing and performance.
   If you experience issues with system timing or performance, disable this option by setting :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` to ``n`` in your project configuration file.

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample on the nRF7002 DK:

With west

.. code-block:: console

   west build -p -b nrf7002dk/nrf5340/cpuapp samples/wifi/shell -- -Dshell_SNIPPET="nrf70-driver-verbose-debug"

With CMake

.. code-block:: console

   cmake -GNinja -Bbuild -DBOARD=nrf7002dk/nrf5340/cpuapp -Dshell_SNIPPET="nrf70-driver-verbose-debug" samples/wifi/shell
   ninja -C build

See also
********

* :ref:`ug_nrf70_developing_debugging` for the statistics shell commands that this snippet enables.
* :ref:`snippet-nrf70-driver-debug` for the basic driver debug logging.
