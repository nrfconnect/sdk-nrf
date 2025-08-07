.. _sdp_gpio:
.. _hpf_gpio_example:

High-Performance Framework GPIO
###############################

.. contents::
   :local:
   :depth: 2

.. caution::

   The High-Performance Framework (HPF) support in the |NCS| is :ref:`experimental <software_maturity>` and is limited to the nRF54L15 device.

This application demonstrates how to write a :ref:`High-Performance Framework (HPF) <hpf_index>` application implementing a simple peripheral.
The application implements a subset of the Zephyr GPIO API.

Application overview
********************

This application shows the implementation of a subset of the Zephyr GPIO API, intended for use with the :zephyr:code-sample:`blinky` sample from Zephyr.
It supports the mbox, icmsg, and icbmsg IPC backends.
Depending on your needs, you can enable them with the relevant sysbuild options.

The GPIO HPF application is structured into the following main components:

* The HPF application - Operates on the FLPR core and handles the GPIO state using VIO.
* The Hard Real Time (HRT) module - Runs on the FLPR core and implements low-level GPIO features.
* The GPIO driver - Operates on the application core and uses the Zephyr's scalable real-time operating system (RTOS) GPIO API for data and configuration transmission between the application and FLPR cores.

Requirements
************

The firmware supports the following development kit:

.. table-from-sample-yaml::

Configuration
*************

You can enable the following IPC backends:

* mbox (``SB_CONFIG_HPF_GPIO_BACKEND_MBOX``)
* icmsg (``SB_CONFIG_HPF_GPIO_BACKEND_ICMSG``)
* icbmsg (``SB_CONFIG_HPF_GPIO_BACKEND_ICBMSG``)

Building and running
********************

.. |application path| replace:: :file:`applications/hpf/gpio`

.. include:: /includes/application_build_and_run.txt

To build and run the application, you must include code for both the application core and FLPR core.
The process involves building the :zephyr:code-sample:`blinky` sample with the appropriate sysbuild configuration.

For example, to build with icmsg backend, run the following commands:

  .. code-block:: console

     west build -b nrf54l15dk/nrf54l15/cpuapp -- -DSB_CONFIG_PARTITION_MANAGER=n -DSB_CONFIG_HPF=y -DSB_CONFIG_HPF_GPIO=y -DSB_CONFIG_HPF_GPIO_BACKEND_ICMSG=y -DEXTRA_DTC_OVERLAY_FILE="./boards/nrf54l15dk_nrf54l15_cpuapp_hpf_gpio.overlay"
     west flash

Upon successful execution, **LED0** will start flashing.

Dependencies
************

* :file:`zephyr/doc/services/ipc/ipc_service` - Used for transferring data between application core and the FLPR core (if you are not using the mbox backend).
* :file:`zephyr/doc/hardware/peripherals/mbox.rst` - Used for transferring data between application core and the FLPR core (if you are not using the ipc backend).
* `nrf HAL <nrfx API documentation_>`_ - Enables access to the VPR CSR registers for direct hardware control.

API documentation
*****************

Application uses the following API elements:

Zephyr driver
=============

* Header file: :file:`drivers/gpio/gpio_hpf.h`
* Source file: :file:`drivers/gpio/gpio_hpf.c`

The following source files depend on the selected IPC backend:

* Source file: :file:`drivers/gpio/gpio_hpf_icmsg.c`
* Source file: :file:`drivers/gpio/gpio_hpf_mbox.c`

FLPR application
================

Source file: :file:`applications/hpf/gpio/src/main.c`

FLPR application HRT
====================

 * Header file: :file:`applications/hpf/gpio/src/hrt/hrt.h`
 * Source file: :file:`applications/hpf/gpio/src/hrt/hrt.c`
 * Assembly: :file:`applications/hpf/gpio/src/hrt/hrt-nrf54l15.s`
