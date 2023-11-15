.. _ug_nrf70_developing_debugging:

Debugging
#########

.. contents::
   :local:
   :depth: 2

This guide describes how to debug applications that use the nRF70 Series companion chips.

Software
********

The scope of this section is limited to the nRF70 driver, WPA supplicant and networking stack.

Enable debug features
=====================

The nRF70 driver, WPA supplicant, and networking stack have debug features that can be enabled to help debug issues.

You can enable debug features by using Zephyr's configuration :ref:`snippets` feature.

For example, to build the :ref:`wifi_shell_sample` sample for the nRF7002 DK with debugging enabled, run the following commands:

With west
---------

.. code-block:: console

    west build -p -b nrf7002dk_nrf5340_cpuapp -S nrf70-debug samples/wifi/shell

With CMake
----------

.. code-block:: console

    cmake -GNinja -Bbuild -DBOARD=nrf7002dk_nrf5340_cpuapp -DSNIPPET=nrf70-debug samples/wifi/shell
    ninja -C build
