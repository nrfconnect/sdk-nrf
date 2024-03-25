.. _smp_svr:

Cellular: SMP Server
####################

.. contents::
   :local:
   :depth: 2

This sample is a reference implementation to use MCUboot recovery mode or serial SMP server mode for external image update.

Requirements
************

.. _supported boards:

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample supports the ``mcuboot`` shell commands for checking image information and a new activated image will be confirmed automatically.

Connecting nRF9160 SiP to nRF52840 SoC
======================================

.. include:: /includes/connecting_soc.txt

Configuration
*************

|config|

Configuration files
===================

The sample supports UART1 connection for the nRF9160 DK with or without MCUboot recovery mode.
The nRF52840 SoC needs to enable UART1 on the devicetree using the following configuration files and recovery mode overlay files:

* :file:`overlay-serial.conf` - Defines the MCUmgr server configuration with SMP serial transport for UART1.
  This requires an additional devicetree overlay file :file:`nrf9160dk_nrf52840_mcumgr_srv.overlay`.
* :file:`nrf9160dk_nrf52840_recovery.overlay` - Devicetree overlay file that enables resetting of the MCUboot recovery mode.

MCUboot configuration
---------------------

The sample has defined configuration and device tree overlay files for MCUboot that are available in the sample folder:

* :file:`child_image/mcuboot.conf` - Defines the MCUboot recovery mode.
* :file:`child_image/mcuboot/boards/nrf9160dk_nrf52840_0_14_0.overlay` - Define UART1 for MCUmgr.

The MCUboot configuration is enabled automatically at build.

Partition management
--------------------

The sample has a static partition management file :file:`pm_static_nrf9160dk_nrf52480.yml` to enable MCUboot secondary slot usage, which is a requirement.

Building
********

.. |sample path| replace:: :file:`samples/cellular/smp_svr/`

.. include:: /includes/build_and_run_ns.txt

.. note::
   Before building the sample, make sure the **PROG/DEBUG** switch is set to **nRF52**.

MCUboot recovery mode
=====================

.. code-block:: console

   west build --pristine -b nrf9160dk/nrf52840 -- -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_nrf52840_recovery.overlay"

MCUmgr server image management
==============================

.. code-block:: console

   west build --pristine -b nrf9160dk/nrf52840 -- -DEXTRA_CONF_FILE="overlay-serial.conf" -DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_nrf52840_mcumgr_srv.overlay"

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Observe that the sample starts.
#. Open a terminal connection for the nRF52840 SoC (VCOM1), in Linux ``/dev/ttyACM1``, and call the ``mcuboot`` command on the shell.
   You can see the following output on the terminal:

   .. code-block:: console

      uart:~$ mcuboot
      swap type: none
      confirmed: 1

      primary area (2):
         version: 1.0.0+0
         image size: 70136
         magic: unset
         swap type: none
         copy done: unset
         image ok: unset

      failed to read secondary area (5) header: -5
      uart:~$

Dependencies
************

The sample uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
