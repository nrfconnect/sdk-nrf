.. _snippet-nrf71-driver-verbose-debug:

nRF71 driver verbose debug snippet (nrf71-driver-verbose-debug)
################################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet extends :ref:`snippet-nrf71-driver-debug` with a larger WPA supplicant thread stack, so that the additional debug logging emitted while the driver is at debug log level does not overflow that thread's stack.
Use it in the same situations as :ref:`snippet-nrf71-driver-debug`, when the extra stack margin is needed, for example when the driver debug logs are emitted from the WPA supplicant thread context.

The snippet applies the same configuration as :ref:`snippet-nrf71-driver-debug`, plus the following:

* Increases the WPA supplicant thread stack size (:kconfig:option:`CONFIG_WIFI_NM_WPA_SUPPLICANT_THREAD_STACK_SIZE`) to accommodate the additional logging.

.. note::
   Unlike the equivalent nRF70 Series snippet (``nrf70-driver-verbose-debug``), this snippet does not enable bus interface level logging.
.. note::
   Enabling :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` can help prevent log buffer overflows, but it may impact system timing and performance.
   If you experience issues with system timing or performance, disable this option by setting :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` to ``n`` in your project configuration file.

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample on the nRF7120 DK:

With west

.. code-block:: console

   west build -p -b nrf7120dk/nrf7120/cpuapp samples/wifi/shell -- -Dshell_SNIPPET="nrf71-driver-verbose-debug"

With CMake

.. code-block:: console

   cmake -GNinja -Bbuild -DBOARD=nrf7120dk/nrf7120/cpuapp -Dshell_SNIPPET="nrf71-driver-verbose-debug" samples/wifi/shell
   ninja -C build

See also
********

* :ref:`ug_nrf70_developing_debugging` for the equivalent guidance for the nRF70 Series.
* :ref:`snippet-nrf71-driver-debug` for the base set of driver debug logs and statistics.
