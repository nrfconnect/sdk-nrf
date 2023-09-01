.. _ncs_introduction:

Introduction
############

.. contents::
   :local:
   :depth: 2


The |NCS| is a scalable and unified software development kit for building low-power wireless applications based on the Nordic Semiconductor wireless devices.
It offers an extensible framework for building size-optimized software for memory-constrained devices as well as powerful and complex software for more advanced devices and applications.

It integrates the Zephyr™ real-time operating system (RTOS) and a wide range of complete applications, samples, and protocol stacks such as Bluetooth® Low Energy, Bluetooth mesh, Matter, Thread/Zigbee, Wi-Fi®, and LTE-M/NB-IoT/GPS, TCP/IP.
It also includes middleware such as CoAP, MQTT, LwM2M, various libraries, hardware drivers, Trusted Firmware-M for security, and a secure bootloader (MCUboot).

The |NCS| supports :ref:`Microsoft Windows, Linux, and macOS <requirements>` for development.

The following table provides an overview of the supported devices for each nRF Series family.

.. list-table::
   :header-rows: 1
   :align: center
   :widths: auto

   * - nRF Series family
     - Devices supported
     - Getting started guide
     - :ref:`Protocols supported <software_maturity_protocol>`
   * - nRF91 Series
     - :ref:`nRF9160 and nRF9161 <app_boards>`
     - :ref:`ug_nrf91`
     - LTE and :ref:`ug_wifi`
   * - nRF70 Series
     - :ref:`nRF7002 <app_boards>`
     - :ref:`ug_nrf70`
     - :ref:`ug_wifi`
   * - nRF53 Series
     - :ref:`nRF5340 <app_boards>`
     - :ref:`ug_nrf53`
     - | * :ref:`ug_ble_controller`
       | * :ref:`ug_bt_mesh`
       | * :ref:`ug_esb`
       | * :ref:`ug_matter`
       | * :ref:`ug_nfc`
       | * :ref:`ug_thread`
       | * :ref:`ug_zigbee`
       | * :ref:`ug_wifi`
   * - nRF52 Series
     - | * :ref:`nRF52833 and nRF52840 <app_boards>` for :ref:`most protocols <software_maturity_protocol>`
       | * :ref:`nRF52810, nRF52811, nRF52820, nRF52832 <app_boards>` for :ref:`Bluetooth <software_maturity_protocol>`
     - :ref:`ug_nrf52`
     - | * :ref:`ug_ble_controller`
       | * :ref:`ug_bt_mesh`
       | * :ref:`ug_esb`
       | * :ref:`ug_gz`
       | * :ref:`ug_matter`
       | * :ref:`ug_nfc`
       | * :ref:`ug_thread`
       | * :ref:`ug_zigbee`
       | * :ref:`ug_wifi`
