.. _snippet-nrf70-wifi:

nRF70 Wi-Fi snippet (nrf70-wifi)
################################

.. contents::
   :local:
   :depth: 2

The ``nrf70-wifi`` snippet provides the optimal build configuration for using an nRF70 Series companion chip with a host SoC that needs board-specific memory and heap tuning.

Overview
********

For each supported board target, the snippet adjusts the memory map and the heap and stack sizes that the nRF Wi-Fi® driver requires.
It applies the following configuration:

* For ``nrf54h20dk/nrf54h20/cpuapp``, enables the test random generator (:kconfig:option:`CONFIG_TEST_RANDOM_GENERATOR`) and the socketpair heap (:kconfig:option:`CONFIG_NET_SOCKETPAIR_HEAP`), and applies a Wi-Fi memory map overlay.
* For ``nrf54h20dk/nrf54h20/cpurad``, disables the UART console (:kconfig:option:`CONFIG_UART_CONSOLE`) and applies a radio core memory map overlay.
* For ``nrf54l15dk/nrf54l15/cpuapp``, applies the same options as for the nRF54H20 application core and additionally tunes the shell stack size, the system workqueue stack size, the ISR stack size, and the driver control and data heap sizes.

Supported board targets
***********************

The snippet supports the following board targets:

* ``nrf54h20dk/nrf54h20/cpuapp``
* ``nrf54h20dk/nrf54h20/cpurad``
* ``nrf54l15dk/nrf54l15/cpuapp``

Usage
*****

Apply the snippet when building, for example for the :ref:`wifi_shell_sample` sample on the nRF54L15 DK with the nRF7002 EB II shield:

.. code-block:: console

   west build -p -b nrf54l15dk/nrf54l15/cpuapp -- -Dshell_SHIELD="nrf7002eb2" -Dshell_SNIPPET=nrf70-wifi

See also
********

* :ref:`ug_nrf7002eb2_gs` for the shield that this snippet is typically used with.
