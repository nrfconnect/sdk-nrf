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

      *** Booting nRF Connect SDK v3.2.99-4dc52e0d80a6 ***
      *** Using Zephyr OS v4.4.0-2b3d25313ec3 ***
      I: Starting Channel Sounding IPT Reflector Sample
      I: SoftDevice Controller build revision:
      I: 10 7f 81 88 e0 eb fc fc |........
      I: 53 3c 0e d3 0f e7 d0 4f |S<.....O
      I: 0c 64 24 fc             |.d$.
      I: HW Platform: Nordic Semiconductor (0x0002)
      I: HW Variant: nRF54Lx (0x0005)
      I: Firmware: Standard Bluetooth controller (0x00) Version 16.33151 Build 4243316872
      I: HCI transport: SDC
      I: Identity: D2:CE:07:7E:40:79 (random)
      I: HCI: version 6.2 (0x10) revision 0x30b8, manufacturer 0x0059
      I: LMP: version 6.2 (0x10) subver 0x30b8
      I: Connected to E6:BA:C9:31:51:DB (random) (err 0x00)
      I: CS capability exchange completed.
      Remote CS capabilities:
      - num_config_supported: 1
      - max_consecutive_procedures_supported: 0
      - num_antennas_supported: 1
      - max_antenna_paths_supported: 1
      - initiator_supported: yes
      - reflector_supported: no
      - mode_3_supported: no
      - rtt_aa_only_precision: 150 ns (2)
      - rtt_sounding_precision: not supported (0)
      - rtt_random_payload_precision: 150 ns (2)
      - rtt_aa_only_n: 30
      - rtt_sounding_n: 0
      - rtt_random_payload_n: 30
      - phase_based_nadm_sounding_supported: no
      - phase_based_nadm_random_supported: no
      - cs_sync_2m_phy_supported: yes
      - cs_sync_2m_2bt_phy_supported: no
      - cs_without_fae_supported: yes
      - chsel_alg_3c_supported: no
      - pbr_from_rtt_sounding_seq_supported: no
      - t_ip1_times_supported: 0x007c
      - t_ip2_times_supported: 0x007e
      - t_fcs_times_supported: 0x01e0
      - t_pm_times_supported: 0x0003
      - t_sw_time: 0 us
      - tx_snr_capability: 0x00
      - t_ip2_ipt_times_supported: 0x007e
      - t_sw_ipt_time_supported: 10 us

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
