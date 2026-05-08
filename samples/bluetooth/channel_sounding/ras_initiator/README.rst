.. _channel_sounding_ras_initiator:

Bluetooth: Channel Sounding Initiator with Ranging Requestor
############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the ranging service to request ranging data from a server.
Distance estimates are then computed from the ranging data and logged to the terminal.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires a device running a Channel Sounding Reflector with Ranging Responder to connect to, such as the :ref:`channel_sounding_ras_reflector` sample.

Overview
********

The sample demonstrates a basic Bluetooth® Low Energy Central role functionality that acts as a GATT Ranging Requestor client and configures the Channel Sounding initiator role.
Regular Channel Sounding procedures are set up, local subevent data is stored, and peer ranging data is fetched.

User interface
**************

The sample does not require user input and will scan for a device advertising with the GATT Ranging Service UUID.
The first LED on the development kit will be lit when a connection has been established.

Building and running
********************
.. |sample path| replace:: :file:`samples/bluetooth/channel_sounding/ras_initiator`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by connecting to another device programmed with a Channel Sounding Reflector role with Ranging Responder, such as the :ref:`channel_sounding_ras_reflector` sample.

1. |connect_terminal_specific|
#. Reset both kits.
#. Wait until the scanner detects the Peripheral.
   In the terminal window, check for information similar to the following::

      *** Booting nRF Connect SDK v3.2.99-4dc52e0d80a6 ***
      *** Using Zephyr OS v4.4.0-2b3d25313ec3 ***
      I: Starting Channel Sounding Initiator Sample
      I: SoftDevice Controller build revision:
      I: 10 7f 81 88 e0 eb fc fc |........
      I: 53 3c 0e d3 0f e7 d0 4f |S<.....O
      I: 0c 64 24 fc             |.d$.
      I: HW Platform: Nordic Semiconductor (0x0002)
      I: HW Variant: nRF54Lx (0x0005)
      I: Firmware: Standard Bluetooth controller (0x00) Version 16.33151 Build 4243316872
      I: HCI transport: SDC
      I: Identity: E6:BA:C9:31:51:DB (random)
      I: HCI: version 6.2 (0x10) revision 0x30b8, manufacturer 0x0059
      I: LMP: version 6.2 (0x10) subver 0x30b8
      I: Filters matched. Address: D2:CE:07:7E:40:79 (random) connectable: 1
      I: Connecting
      I: Connected to D2:CE:07:7E:40:79 (random) (err 0x00)
      I: Security changed: D2:CE:07:7E:40:79 (random) level 2
      I: MTU exchange success (498)
      I: The discovery procedure succeeded
      I: Read RAS feature bits: 0x1
      I: CS capability exchange completed.
      Remote CS capabilities:
      - num_config_supported: 1
      - max_consecutive_procedures_supported: 0
      - num_antennas_supported: 1
      - max_antenna_paths_supported: 1
      - initiator_supported: no
      - reflector_supported: yes
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
      - t_ip2_ipt_times_supported: 0x0000
      - t_sw_ipt_time_supported: 0 us

      I: CS config creation complete.
      - id: 0
      - mode: Invalid
      - min_main_mode_steps: 2
      - max_main_mode_steps: 5
      - main_mode_repetition: 0
      - mode_0_steps: 3
      - role: Initiator
      - rtt_type: AA only
      - cs_sync_phy: LE 1M PHY
      - channel_map_repetition: 1
      - channel_selection_type: Algorithm #3b
      - ch3c_shape: Hat shape
      - ch3c_jump: 2
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
      - subevent length: 16000 us
      - subevents per event: 1
      - subevent interval: 0
      - event interval: 2
      - procedure interval: 5
      - procedure count: 0
      - maximum procedure length: 128
      I: Latest distance estimates on antenna path 0: ifft: 1.35, phase_slope: 1.32, rtt: 2.86 meters
      I: Latest distance estimates on antenna path 0: ifft: 1.35, phase_slope: 1.32, rtt: 2.42 meters
      I: Latest distance estimates on antenna path 0: ifft: 1.35, phase_slope: 1.32, rtt: 1.98 meter

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`
* :file:`include/bluetooth/gatt_dm.h`
* :file:`include/bluetooth/services/ras.h`

This sample uses the following Zephyr libraries:

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/cs.h`
