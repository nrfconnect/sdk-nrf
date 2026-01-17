.. _throughput_many_to_one:

Bluetooth: Throughput many to one
##########################

.. contents::
   :local:
   :depth: 2

The Throughput Many-To-One sample demonstrates high-uplink many-to-one transfers over Bluetooth LE Periodic Advertising with Responses (PAwR).
It is organized as two cooperating applications: an advertiser/host and one or more synchronizer/responders.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf54l15dk_nrf54l15_cpuapp

You need at least two development kits to try the sample (one advertiser and one or more synchronizers). 

Overview
********
This sample is a starting point for implementing PAwR for higher-throughput many-to-one communication.
It consists of two roles:

- ``periodic_adv_rsp``: PAwR advertiser/host that creates a periodic advertising set, encodes control frames (open-slot bitmap, ACKs, and retransmit hints), receives responses from multiple devices, and prints measured and theoretical throughput.
- ``periodic_sync_rsp``: PAwR synchronizer/responder that discovers and synchronizes to the periodic advertiser, claims a free response slot, and transmits payloads in its assigned slot.

The synchronizer discovers the advertiser by device name (``"PAwR adv sample"``). The advertiser prints measured and theoretical throughput for the active set of responders.

Default parameter values
========================

.. list-table:: Default parameter values
   :header-rows: 1

   * - Parameter
     - Value
   * - Number of responders (slots)
     - 2
   * - Response payload size
     - 247 bytes
   * - PHY data rate
     - LE 2M
   * - Response slot delay
     - 3 units (3.75 ms)
   * - Response slot spacing
     - computed from payload and PHY
   * - Subevent interval
     - 6 units (7.50 ms)
   * - Retransmit bitmap bytes
     - up to 4

Limitations and trade-offs
===========================

This sample demonstrates a connectionless, token-based claim/ACK protocol over PAwR. Compared to GATT connections or using Periodic Advertising Sync Transfer (PAST) with a connected control plane, consider the following trade-offs:

- **Same-slot collisions**: If two unassigned responders choose the same open slot in the same subevent, their CLAIM responses collide at the PHY and the advertiser receives neither; no ACK is issued that event. Both remain unassigned and retry in later events with probabilistic backoff.
- **First-wins slot assignment**: When multiple CLAIMs target an already assigned slot, only the token that already owns the slot is ACKed; other CLAIMs are ignored. This can lead to temporary unfairness while new devices are joining.
- **Hidden-node effects**: Responders cannot sense each other; contention (and thus join latency) grows with device count and RF congestion. Backoff heuristics mitigate but do not eliminate collisions.
- **Limited per-response payload**: Each response is capped at 247 bytes by the controller, so larger data must be application-segmented.
- **Control bandwidth limits**: ACK list capacity per event is limited by control payload tailroom, and the retransmit bitmap is truncated to a few bytes to bound control size, delaying hints under heavy load.

These constraints are typical of random access, connectionless schemes. If your use case demands stronger fairness, reliability, or access control, a GATT-based design (optionally using PAST to distribute sync) is usually preferable.

Configuration
**************

|config|

.. _throughput_many_to_one_config_options:


Configuration options
======================

Check and configure the following sample-specific Kconfig options:

.. code-block::

   .. options-from-kconfig::
      :kconfig:option:`CONFIG_BT_NUM_RSP_DEVICES`

.. note::
   Before building the advertiser, set :kconfig:option:`CONFIG_BT_NUM_RSP_DEVICES` in ``periodic_adv_rsp/prj.conf`` to the number of synchronizer devices you plan to run.
   If this value does not match your setup, the advertiser may appear stalled while waiting for unassigned or missing responders.


Additional configuration
========================

Key Bluetooth and controller options used by this sample include:

- :kconfig:option:`CONFIG_BT_PER_ADV`
- :kconfig:option:`CONFIG_BT_PER_ADV_RSP`
- :kconfig:option:`CONFIG_BT_PER_ADV_SYNC`
- :kconfig:option:`CONFIG_BT_PER_ADV_SYNC_RSP`
- :kconfig:option:`CONFIG_BT_CTLR_PHY_2M`
- :kconfig:option:`CONFIG_BT_CTLR_DATA_LENGTH_MAX`
- :kconfig:option:`CONFIG_BT_L2CAP_TX_MTU`
- :kconfig:option:`CONFIG_BT_USER_PHY_UPDATE`
- :kconfig:option:`CONFIG_BT_DEVICE_NAME`

Configuration files
===================

This sample provides predefined configuration files for typical use:

- ``periodic_adv_rsp/prj.conf`` — advertiser/host settings (slot count, buffer sizes, timing).
- ``periodic_sync_rsp/prj.conf`` — synchronizer/responder settings (PAwR sync, payload size, buffers).


Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/throughput_many_to_one`

.. include:: /includes/build_and_run.txt

Build the two roles separately and program them to different development kits:

Advertiser ::
   cd periodic_adv_rsp
   west build -p always -b nrf54l15dk/nrf54l15/cpuapp 

Synchronizer ::
   cd periodic_sync_rsp
   west build -p always -b nrf54l15dk/nrf54l15/cpuapp 

After building, flash each role to its target kit.


Testing
=======

|test_sample|

#. Connect the development kits over USB.
#. Program the advertiser to one kit and the synchronizer to one or more other kits.
#. Open a terminal per device (115200 baud) and reset the kits.
#. Observe the advertiser logs for measured and theoretical throughput and the number of active responders.

Sample output
==============

The result should look similar to the following output.

For the advertiser::

    *** Booting nRF Connect SDK v3.1.99-28569c1ad6cb ***
    *** Using Zephyr OS v4.2.99-c539d475b659 ***
    [ADV] Buffer 0 initialized with len 4
    Bluetooth initialized
    PAwR config (1 subevent):
    Devices: 2, Resp size: 247 B, Slot: 1.24 ms
    Delay: 3 (3.75 ms), SubEvt: 6 (6.23 ms), AdvInt: 6 (7.50 ms)
    Est. uplink throughput (theoretical) ~526 kbps
    Starting PAwR Advertiser
    ===========================================
    Start Extended Advertising
    Start Periodic Advertising
    [CLAIM] accepted: slot 1 token 0xc7680719
    [CLAIM] accepted: slot 0 token 0x62f9b14e

    [ADV] received 63196 bytes (61 KB) in 1003 ms at 504 kbps
    [PAwR] theoretical ~526 kbps; efficiency ~95% (N=2, payload=247, AdvInt=7.50 ms)

    [ADV] received 65636 bytes (64 KB) in 1001 ms at 524 kbps
    [PAwR] theoretical ~526 kbps; efficiency ~99% (N=2, payload=247, AdvInt=7.50 ms)

    [ADV] received 65636 bytes (64 KB) in 1006 ms at 521 kbps
    [PAwR] theoretical ~526 kbps; efficiency ~99% (N=2, payload=247, AdvInt=7.50 ms)


For the synchronizers::

    *** Booting nRF Connect SDK v3.1.99-28569c1ad6cb ***
    *** Using Zephyr OS v4.2.99-c539d475b659 ***
    Starting PAwR Synchronizer
    [SCAN] Waiting for periodic advertiser...
    [SCAN] Creating periodic sync to SID 0
    [SYNC] Synced to 28:0E:0E:0E:D4:A9 (random) with 1 subevents
    [SYNC] Changed sync to subevent 0
    [JOIN] Sent claim for slot 0 with token 0xb4a6d332
    [JOIN] Acked: token 0xb4a6d332 assigned slot 0

.. note::
Retransmission log messages may occur. The advertiser reports missed slots and suggests retransmission, 
but the responder in this sample always sends fresh payloads and does not implement retransmission logic.

Troubleshooting
===============

- [LOSS] slot X missed response; should retransmit
  - The advertiser expected data from that slot this event but didn’t receive it.
  - After first real payload from a slot, retransmit hints may appear under RF loss.

- [CLAIM] Reclaimed idle slot X
  - The advertiser freed a slot after several missed events. Responders should re-join.

- Failed to set subevent data (err -13)
  - Controller rejected PAwR subevent parameters (e.g., subevent index out of range).
  - Ensure subevent index is modulo the configured number of subevents and data length fits.

Dependencies
*************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:bluetooth_api`:

  * :file:`include/bluetooth/bluetooth.h`
  * :file:`include/bluetooth/hci.h`

* :ref:`zephyr:kernel_api`:

  * :file:`include/kernel.h`

* :file:`include/zephyr/sys/byteorder.h`
* :file:`include/zephyr/random/random.h`
* :file:`include/zephyr/net/buf.h`
* :file:`include/zephyr/sys/printk.h`
* :file:`include/zephyr/sys/util.h`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:softdevice_controller`

References
*************

For more about Periodic Advertising with Responses (PAwR), see the Bluetooth Core Specification:
- LE Periodic Advertising
- LE Periodic Advertising with Responses (PAwR)
- LE Controller timing parameters (subevent interval, response slot delay/spacing)
