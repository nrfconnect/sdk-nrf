.. _channel_sounding_ipt_reflector:

Bluetooth: Channel Sounding Reflector with Inline PCT Transfer
##############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the Bluetooth® Channel Sounding (CS) Inline Phase Correction Term Transfer (IPT) feature as a CS reflector, to achieve fast and efficient distance estimation.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a device running the :ref:`channel_sounding_ipt_initiator` sample.

Overview
********

The sample acts as a Bluetooth Low Energy Peripheral and Channel Sounding reflector.
After startup, it performs the following steps:

1. Starts connectable advertising using its name (``Nordic CS IPT Reflector``), which the :ref:`channel_sounding_ipt_initiator` sample scans for.
#. Waits for a central device to connect.
#. Configures the CS default settings to enable the reflector role.
#. The connected central device performs the rest of the CS setup.
   The central device is responsible for enabling CS IPT in the CS configuration.
#. Participates in the continuous CS procedures started by the central device.

The sample does not expose any ranging data to the initiator over GATT.
Instead, its contribution to the measurement is encoded directly in the phase of the tones it transmits as a CS reflector using the CS IPT feature during the CS procedures.

How CS IPT works
================

.. include:: ../ipt_initiator/README.rst
   :start-after: channel_sounding_ipt_initiator_cs_ipt_works_start
   :end-before: channel_sounding_ipt_initiator_cs_ipt_works_end

Benefits of CS IPT
==================

.. include:: ../ipt_initiator/README.rst
   :start-after: .. channel_sounding_ipt_initiator_cs_ipt_benefits_start
   :end-before: .. channel_sounding_ipt_initiator_cs_ipt_benefits_end

Drawbacks of CS IPT
===================

.. include:: ../ipt_initiator/README.rst
   :start-after: channel_sounding_ipt_initiator_cs_ipt_drawbacks_start
   :end-before: channel_sounding_ipt_initiator_cs_ipt_drawbacks_end

User interface
**************

The sample does not require user input.
The first LED on the development kit will be lit when a connection has been established.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/channel_sounding/ipt_reflector`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, test it together with a device programmed with the :ref:`channel_sounding_ipt_initiator` sample.

1. |connect_terminal_specific|
#. Reset both kits.
#. The reflector starts advertising and the initiator scans for ``Nordic CS IPT Reflector``.
   Once the initiator connects, check the reflector terminal for information similar to the following::

      *** Booting nRF Connect SDK v3.2.99-09f8a95ee5b4 ***
      *** Using Zephyr OS v4.3.99-2925b3840984 ***
      I: Starting Channel Sounding IPT Reflector Sample
      I: SoftDevice Controller build revision:
      I: 36 9c e5 98 e5 71 0a a0 |6....q..
      I: 2d b9 89 96 6b 15 65 4b |-...k.eK
      I: d6 e8 4a ac             |..J.
      I: HW Platform: Nordic Semiconductor (0x0002)
      I: HW Variant: nRF54Lx (0x0005)
      I: Firmware: Standard Bluetooth controller (0x00) Version 54.58780 Build 175236504
      I: HCI transport: SDC
      I: Identity: D2:CE:07:7E:40:79 (random)
      I: HCI: version 6.2 (0x10) revision 0x30a9, manufacturer 0x0059
      I: LMP: version 6.2 (0x10) subver 0x30a9
      I: Connected to E6:BA:C9:31:51:DB (random) (err 0x00)
      I: CS capability exchange completed.
      I: CS config creation complete.
      - id: 0
      - mode: 2 (PBR)
      - min_main_mode_steps: 0
      - max_main_mode_steps: 0
      - main_mode_repetition: 0
      - mode_0_steps: 3
      - role: Reflector
      - rtt_type: AA only
      - cs_sync_phy: LE 1M PHY
      - channel_map_repetition: 1
      - channel_selection_type: Algorithm #3b
      - ch3c_shape: Hat shape
      - ch3c_jump: 0
      - t_ip1_time_us: 30
      - t_ip2_time_us: 20
      - t_fcs_time_us: 60
      - t_pm_time_us: 10
      - channel_map: 0x1FFFFFFFFFFFFC7FFFFC

      I: CS security enabled.
      I: CS procedures enabled:
      - config ID: 0
      - antenna configuration index: 0
      - TX power: 0 dbm
      - subevent length: 7500 us
      - subevents per event: 1
      - subevent interval: 0
      - event interval: 3
      - procedure interval: 2
      - procedure count: 0
      - maximum procedure length: 12


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

This sample uses the following Zephyr libraries:

* :ref:`zephyr:logging_api`:

  * :file:`include/logging/log.h`

* :file:`include/zephyr/types.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/uuid.h`
* :file:`include/bluetooth/cs.h`
