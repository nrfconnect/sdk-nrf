.. _wifi_wfa_qt_app_sample:

Wi-Fi: WFA QuickTrack control application
#########################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the WFA QuickTrack (WFA QT) library needed for Wi-Fi Alliance QuickTrack certification.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

This sample acts as a control application for the WFA QT library, to be used for Wi-Fi Alliance QuickTrack certification.
See `Wi-Fi Alliance Certification for nRF70 Series <Wi-Fi Alliance Certification for nRF70 Series_>`_ for more information.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/wfa_qt_app`

.. include:: /includes/build_and_run_ns.txt

Currently, the nRF7002 DK + QSPI configuration is supported.

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp

To build for the nRF7002 DK with the netusb support, use the ``nrf7002dk_nrf5340_cpuapp`` build target with the configuration overlay :file:`overlay-netusb.conf`.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-netusb.conf

To build for the nRF7002 DK with the Serial Line Internet Protocol (SLIP) support, use the ``nrf7002dk_nrf5340_cpuapp`` build target with the configuration overlay :file:`overlay-slip.conf` and DTC overlay :file:`nrf7002_uart_pipe.overlay`.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-slip.conf -DDTC_OVERLAY_FILE=nrf7002_uart_pipe.overlay

Also, run the following command to create a Serial Line Internet Protocol (SLIP) interface on the PC where the QT tool is running.

.. code-block:: console

   tunslip6 -s <serial_port> <IPv6_prefix>

See also :ref:`cmake_options` for instructions on how to provide CMake options.


Dependencies
************

This sample uses modules found in the following locations in the |NCS| folder structure:

* :file:`modules/lib/wfa-qt-control-app`
