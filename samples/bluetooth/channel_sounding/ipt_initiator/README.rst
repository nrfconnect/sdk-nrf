.. _channel_sounding_ipt_initiator:

Bluetooth: Channel Sounding Initiator with Inline PCT Transfer
##############################################################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to use the Bluetooth® Channel Sounding (CS) Inline Phase Correction Term Transfer (IPT) feature as a CS initiator, to achieve fast and efficient distance estimation.


Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You can use any combination of supported development kits in your test setup.

The sample also requires a device running a Channel Sounding Reflector with CS IPT support to connect to, such as the :ref:`channel_sounding_ipt_reflector` sample.

Overview
********

The sample acts as a Bluetooth Low Energy Central and Channel Sounding initiator.
After startup, the sample performs the following steps:

1. Scans for a reflector advertising the name ``Nordic CS IPT Reflector``.
#. Connects to the reflector.
#. Encrypts the ACL link.
#. Reads the remote supported features.
#. Performs the required CS setup over the ACL:

   a. CS default settings
   #. CS capability exchange
   #. CS configuration creation with CS IPT enabled
   #. CS security enable
   #. CS procedures enable

#. Continuously receives CS subevent result events from the controller, computes a distance estimate for each completed procedure using :c:func:`cs_de_ifft`, and logs the estimate and sliding window median over the serial port together with the time delta since the previous estimate.

Unlike the :ref:`channel_sounding_ras_initiator` sample, this sample does not act as a GATT Ranging Requestor client and does not fetch peer ranging data over GATT.
The initiator computes distance estimates entirely from its own CS subevent result events, without ever receiving the reflector's raw measurements over the ACL.

.. channel_sounding_ipt_initiator_cs_ipt_start

How CS IPT works
================

.. channel_sounding_ipt_initiator_cs_ipt_works_start

CS IPT exploits the symmetry of CS Phase-Based Ranging (PBR) tones.
PBR tones are exchanged in mode 2 steps and in the PBR portion of mode 3 steps, and CS IPT is applicable to both.
During a PBR tone exchange the initiator and the reflector each transmit a continuous-wave tone on the same frequency, back-to-back.
With CS IPT enabled, the reflector adjusts the phase of the tone it transmits to the initiator to match the phase of the tone it just received from the initiator on the same frequency.

As a result, the phase that the initiator measures on the received tone already contains the sum of the following  components:

* The phase measured by the reflector on the initiator's tone.
* The phase accumulated on the return trip from the reflector to the initiator.

With CS IPT, the initiator therefore has everything it needs to compute a distance estimate from its local subevent data alone, and the reflector's phase measurements do not have to be relayed back over the ACL.

This sample only uses mode 2 steps, but the CS IPT feature also applies to the PBR tones within mode 3 steps.

.. note::
   The CS IPT feature only works for distance estimating using PBR.
   So if you want an RTT measurement from a mode 3 step you still need to use Ranging Service, or an equivalent, to relay the RTT measurement back from the peer device over the ACL.

.. channel_sounding_ipt_initiator_cs_ipt_works_end

Benefits of CS IPT
==================

.. channel_sounding_ipt_initiator_cs_ipt_benefits_start

Compared to using the Ranging Service (RAS) to transfer ranging data, CS IPT provides the following benefits:

* Reduced setup time for Channel Sounding, since no GATT Ranging Service discovery or subscription is required before distance estimates can be produced.
* Higher achievable update rate for distance estimates based on CS procedures, because the reflector's measurement data is already encoded in the tones the initiator receives and does not need a separate GATT transfer.
* Reduced latency between the measurements being taken during a CS procedure and the distance estimate being computed, because the reflector's measurement data is already encoded in the tones the initiator receives.
* Lower power consumption and reduced memory usage on both devices, since the reflector's measurement data does not have to be buffered on the reflector, sent over the ACL, or reassembled on the initiator.

.. channel_sounding_ipt_initiator_cs_ipt_benefits_end

Drawbacks of CS IPT
===================

.. channel_sounding_ipt_initiator_cs_ipt_drawbacks_start

Compared to using RAS, CS IPT has the following drawbacks:

* Reduced security level.
  This sample only runs CS mode 2 steps, so it cannot provide the same level of protection against ranging attacks as a RAS-based setup that combines mode 1 (RTT) and mode 2 (PBR) steps.

.. channel_sounding_ipt_initiator_cs_ipt_drawbacks_end
.. channel_sounding_ipt_initiator_cs_ipt_end

Limitations
===========

To keep the sample minimal, it has the following limitations:

* Multi-antenna configurations are not supported.
  The sample is configured for a single antenna path (``CONFIG_BT_CS_DE_MAX_NUM_ANTENNA_PATHS=1``).
* Distance estimation only uses the IFFT-based estimator (:c:func:`cs_de_ifft`).
* Only basic post-processing is applied to the distance estimates in the form of a sliding-window median filter.
* Tones are not filtered out based on the Tone Quality Indicator (TQI).
  All reported tones are fed into the distance estimator.

User interface
**************

The sample does not require user input and scans for a device advertising with the name ``Nordic CS IPT Reflector``.
The first LED on the development kit will be lit when a connection has been established.

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/channel_sounding/ipt_initiator`

.. include:: /includes/build_and_run.txt

Testing
=======

After programming the sample to your development kit, you can test it by connecting to another device programmed with a Channel Sounding Reflector role with CS IPT support, such as the :ref:`channel_sounding_ipt_reflector` sample:

1. |connect_terminal_specific|
#. Reset both kits.
#. Wait until the scanner detects the Peripheral.
   In the terminal window, check for information similar to the following::

      *** Booting nRF Connect SDK v3.2.99-d0824283f866 ***
      *** Using Zephyr OS v4.3.99-2925b3840984 ***
      I: Starting Channel Sounding IPT Initiator Sample
      I: SoftDevice Controller build revision:
      I: 36 9c e5 98 e5 71 0a a0 |6....q..
      I: 2d b9 89 96 6b 15 65 4b |-...k.eK
      I: d6 e8 4a ac             |..J.
      I: HW Platform: Nordic Semiconductor (0x0002)
      I: HW Variant: nRF54Lx (0x0005)
      I: Firmware: Standard Bluetooth controller (0x00) Version 54.58780 Build 175236504
      I: HCI transport: SDC
      I: Identity: E6:BA:C9:31:51:DB (random)
      I: HCI: version 6.2 (0x10) revision 0x30a9, manufacturer 0x0059
      I: LMP: version 6.2 (0x10) subver 0x30a9
      I: Filters matched. Address: D2:CE:07:7E:40:79 (random) connectable: 1
      I: Connecting
      I: Connected to D2:CE:07:7E:40:79 (random) (err 0x00)
      I: Security changed: D2:CE:07:7E:40:79 (random) level 2
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
      - t_ip2_ipt_times_supported: 0x007e
      - t_sw_ipt_time_supported: 10 us

      I: CS config creation complete.
      - id: 0
      - mode: 2 (PBR)
      - min_main_mode_steps: 0
      - max_main_mode_steps: 0
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
      - subevent length: 7500 us
      - subevents per event: 1
      - subevent interval: 0
      - event interval: 3
      - procedure interval: 2
      - procedure count: 0
      - maximum procedure length: 12
      I: Distance estimates: median: 0.60m, update: 0.60m, time_delta: 0ms
      I: Distance estimates: median: 0.60m, update: 0.59m, time_delta: 30ms
      I: Distance estimates: median: 0.59m, update: 0.59m, time_delta: 30ms
      I: Distance estimates: median: 0.60m, update: 0.60m, time_delta: 30ms
      I: Distance estimates: median: 0.60m, update: 0.60m, time_delta: 30ms
      I: Distance estimates: median: 0.60m, update: 0.58m, time_delta: 30ms
      I: Distance estimates: median: 0.59m, update: 0.59m, time_delta: 30ms
      I: Distance estimates: median: 0.59m, update: 0.59m, time_delta: 30ms
      I: Distance estimates: median: 0.59m, update: 0.59m, time_delta: 30ms
      I: Distance estimates: median: 0.59m, update: 0.59m, time_delta: 30ms
      I: Distance estimates: median: 0.59m, update: 0.59m, time_delta: 30ms


Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`
* :file:`include/bluetooth/cs_de.h`
* :file:`include/bluetooth/scan.h`

This sample uses the following Zephyr libraries:

* :file:`include/sys/printk.h`
* :file:`include/zephyr/types.h`
* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :ref:`zephyr:bluetooth_api`:

* :file:`include/bluetooth/bluetooth.h`
* :file:`include/bluetooth/conn.h`
* :file:`include/bluetooth/cs.h`
