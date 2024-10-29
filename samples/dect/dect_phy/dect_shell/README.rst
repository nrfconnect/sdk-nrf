.. _dect_shell_application:

nRF91x1: DECT NR+ Shell
###########################

.. contents::
   :local:
   :depth: 2

The DECT NR+ physical layer (PHY) Shell (DeSh) sample application demonstrates how to set up a DECT NR+ application with the DECT PHY firmware and enables you to test various modem features.

.. important::

   The sample showcases the use of the :ref:`nrf_modem_dect_phy` interface of the :ref:`nrfxlib:nrf_modem`.

Requirements
************

The sample supports the following development kits and requires at least two kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

DeSh enables testing of :ref:`nrf_modem_dect_phy` interface and related modem features.
This sample is also a test application for aforementioned features.

The subsections list the DeSh features and show shell command examples for their usage.

.. note::
   To learn more about using a DeSh command, run the command without any parameters.

Main command structure:

  .. code-block:: console

     dect
       sett
       rssi_scan
       ping
       perf
       rf_tool
       status
       rx
      mac
        beacon_scan
        beacon_start
        beacon_stop
        rach_tx
        associate
        dissociate
        status

Application settings
====================

DeSh command: ``dect sett``

You can store some of the main DeSh command parameters into settings that are persistent between sessions.
The settings are stored in the persistent storage and loaded when the application starts.

Examples
--------

* See the usage and read the current settings:

  .. code-block:: console

     dect sett -?
     dect sett -r

* Reset the settings to their default values:

  .. code-block:: console

     dect sett --reset

* Change the default TX power:

  .. code-block:: console

     dect sett --tx_pwr -16

* Change the default band to ``2`` (has impact when automatic channel selection is used, in other words, the set channel is zero in ``dect rssi_scan`` or in ``dect mac beacon_start`` command):

  .. code-block:: console

     dect sett -b 2

RSSI measurement
================

DeSh command: ``dect rssi_scan``

Execute RSSI measurement/scan.

Examples
--------

* Execute RSSI measurement with default parameters:

  .. code-block:: console

     dect rssi_scan

* Execute longer (5000 ms on each channel) RSSI measurements on all channels on a set band:

  .. code-block:: console

     dect rssi_scan -c 0 -t 5000

* Stop RSSI measurement:

  .. code-block:: console

     dect rssi_scan stop

Ping command
============

DeSh command: ``dect ping``

Ping is a tool for testing the reachability of a neighbor host running with DeSh ping server.
You can also use it to measure the round-trip time, demonstrate HARQ and for range testing.

The ``ping`` command uses its own proprietary PDU format and protocol.
At end of a ping session, it prints out the statistics of the session.
On the client side, also server side statistics are printed out if the client side ``PING_RESULT_REQ`` is responded with ``PING_RESULT_RESP`` by the server.

Example 1: basic usage
----------------------

* On both client and server side - scan for a free channel:

  .. code-block:: console

     dect rssi_scan -c 0

* Server side: Set a unique transmission ID and start ping server on manually chosen free channel:

  .. code-block:: console

     dect sett -t 39
     dect ping -s --channel 1677

* Client side: Start basic pinging:

  .. code-block:: console

     dect ping -c --s_tx_id 39 --channel 1677

* Client side: Max amount of data in one request with custom parameters:

  .. code-block:: console

     dect ping -c --s_tx_id 39 --c_tx_pwr 7 -i 3 -t 2000 -l 4 --c_tx_mcs 4 --c_count 10 --channel 1677

* Server side: Stop ping server:

  .. code-block:: console

     dect ping stop

  .. note::
     You can use this command also on the client side to abort pinging.

Example 2: automatic TX power tuning
------------------------------------

You can use the ``dect ping`` command line option ``--tx_pwr_ctrl_auto`` to enable automatic TX power tuning.
Both sides indicate their expected RSSI level (``--tx_pwr_ctrl_pdu_rx_exp_rssi_level``) on their RX in the sent ping PDU.
Based on that and other information from RX for the received ping PDU, the actual TX power is tuned if the feature is enabled.

* On both client and server side - scan for a free channel (see previous example).
* Server side: Set a unique transmission ID and start the ping server on a manually chosen free channel:

  .. code-block:: console

     dect sett -t 39
     dect ping -s --tx_pwr_ctrl_auto --tx_pwr_ctrl_pdu_rx_exp_rssi_level -55 --channel 1677

* Client side: Start pinging:

  .. code-block:: console

     dect ping -c --s_tx_id 39 --tx_pwr_ctrl_auto --tx_pwr_ctrl_pdu_rx_exp_rssi_level -55 --channel 1677

Example 3: HARQ
---------------

You can use the ``dect ping`` command hook ``-a`` to enable the client side to request a HARQ feedback for the sent ping requests.
Based on the received HARQ ACK/NACK, the ping request is resent within a timeout if possible .

* On both client and server side - scan for a free channel (see previous example).
* Server side: Set a unique transmission ID and start the ping server on a manually chosen free channel (see previous examples).
* Client side: start basic pinging and request HARQ feedback from the server:

  .. code-block:: console

     dect ping -c --s_tx_id 39 --channel 1677 -a

Throughput performance testing
==============================

DeSh command: ``dect perf``

Perf is a tool for testing the throughput performance.

The ``perf`` command uses its own proprietary PDU format and protocol.
At end of a session, it prints out the statistics.
On the client side, also server side statistics are printed out if the client side ``PERF_RESULT_REQ`` is responded with ``PERF_RESULT_RESP`` by the server.

Example 1: basic usage
----------------------

* On both client and server side - scan for a free channel:

  .. code-block:: console

     dect rssi_scan -c 0

* Server side: Set a unique transmission ID and start the RX server for 15 seconds on a free channel:

  .. code-block:: console

     dect sett -t 39
     dect perf -s -t 15 --channel 1671

* Server side: As an alternative to the previous command, start the RX server with continuous RX:

  .. code-block:: console

     dect sett -t 39
     dect perf -s -t -1 --channel 1671

* Client side: Send with MCS-4 and length of four slots for 10 seconds:

  .. code-block:: console

     dect perf -c --c_gap_subslots 3 --c_tx_mcs 4 --c_slots 4 --s_tx_id 39 -t 10 --channel 1671

* Server side: Stop perf server:

  .. code-block:: console

     dect perf stop

  .. note::
     You can use this command also on the client side to abort TX.

Example 2: HARQ
---------------

* On both client and server side - scan for a free channel (see previous example).
* Server side: Set unique transmission ID and start RX server with HARQ + some other tunings:

  .. code-block:: console

     dect sett -t 39
     dect perf -s -t -1 -a --s_harq_feedback_tx_delay_subslots 2 --s_harq_feedback_tx_rx_delay_subslots 3 --channel 1671

* Client side: Send with MCS-4 and length of 4 slots for 10 seconds:

  .. code-block:: console

     dect perf -c --c_gap_subslots 4 --c_tx_mcs 4 --c_slots 4 --s_tx_id 39 -t 12 --c_harq_feedback_rx_delay_subslots 2 --c_harq_feedback_rx_subslots 3 --c_harq_process_nbr_max 7 -a --channel 1671

* Client side: Decrease default scheduler delay and rerun the previous step:

  .. code-block:: console

     dect sett -d 5000

 .. note::
    Set the delay back to default to avoid scheduler problems on other use cases.

RX/TX testing with RF tool
==========================

DeSh command: ``dect rf_tool``

The ``rf_tool`` is meant for RX/TX testing.
You can use it as a util for running of ETSI EN 301 406-2: sections 4.3 (Conformance requirements for transmitter) and 4.4 (Conformance requirements for receiver).

A frame is sent ``--frame_repeat_count`` times and results are reported, no delay between frames.
Frames up to ``--frame_repeat_count`` define the interval.
Intervals are repeated until ``--frame_repeat_count_intervals``.
There is also a continuous mode that you can start with the ``--continuous`` option.
You can stop the continuous mode by using the ``dect rf_tool stop`` option.

To configure the frame structure, use the following command options:

.. code-block:: console

   rx_frame_start_offset : rx_subslot_count + rx_idle_subslot_count + tx_frame_start_offset + tx_subslot_count + tx_idle_subslot_count

Example 1: bi-directional testing
---------------------------------

* On both client and server side - scan for a free channel:

  .. code-block:: console

     dect rssi_scan -c 0

* Server side: Start RF mode RX_TX with default frame structure and put it waiting for RX synch:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx_tx --rx_find_sync --frame_repeat_count 50 --frame_repeat_count_intervals 10 -c 1677

* Client side: Trigger to start operation:

  .. code-block:: console

     dect rf_tool -m rx_tx --frame_repeat_count 50 --frame_repeat_count_intervals 10 -t 39 -c 1677

Example 2: unidirectional testing
---------------------------------

* On both TX and RX side - scan for a free channel (see previous example).
* RX device option 1: RX single shot mode:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx --rx_find_sync --frame_repeat_count 50 -c 1677

* RX device option 2: RX device on ``rx_cont`` mode:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx_cont -c 1677

* RX device option 3: RX device in ``rx_cont`` mode with the information of TX side to have interval reporting:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx_cont --rf_mode_peer tx --frame_repeat_count 50 --rx_find_sync -c 1677

* TX device: Trigger to start operation:

  .. code-block:: console

     dect rf_tool -m tx --frame_repeat_count 50 -c 1677 -t 39

* RX device with option 2: Stop continuous RX to give a report:

  .. code-block:: console

     dect rf_tool stop
     cert command stopping.
     RF tool results at transmitter id 39:
     - RX/TX Duty Cycle percentage: ...

Example 3: duty cycle (RX+TX) testing
-------------------------------------

* On both TX and RX side - scan for a free channel (see previous examples).
* Bi-directional testing:

  RX/TX duty cycle percentage 73.91%:

  .. code-block:: console

     server:
     dect rf_tool -m rx_tx --rx_find_sync --rx_subslot_count 9 --rx_idle_subslot_count 3 --tx_subslot_count 8 --tx_idle_subslot_count 3 --frame_repeat_count 50 -c 1677

     client:
     dect rf_tool -m rx_tx --rx_subslot_count 9 --rx_idle_subslot_count 3 --tx_subslot_count 8 --tx_idle_subslot_count 3 --frame_repeat_count 50 -c 1677 -t 39

  RX/TX duty cycle percentage 82.50%:

  .. code-block:: console

     server:
     dect rf_tool -m rx_tx --rx_find_sync --rx_subslot_count 17 --rx_idle_subslot_count 3 --tx_subslot_count 16 --tx_idle_subslot_count 4 --frame_repeat_count 50 -c 1677

     client:
     dect rf_tool -m rx_tx --rx_subslot_count 17 --rx_idle_subslot_count 3 --tx_subslot_count 16 --tx_idle_subslot_count 4 --frame_repeat_count 50 -c 1677 -t 39

* TX/RX testing on separate devices:

  TX side: RX/TX duty cycle percentage 84.21%:

  .. code-block:: console

     server:
     dect rf_tool -m rx_cont -c 1677

     client:
     dect rf_tool -m tx --tx_subslot_count 16 --tx_idle_subslot_count 3 -c 1677 -t 39

  Alternatively, RX side with the information of TX side to have interval reporting:

  .. code-block:: console

     server:
     dect rf_tool -m rx_cont --rf_mode_peer tx --tx_subslot_count 16 --tx_idle_subslot_count 3 --rx_find_sync -c 1677

     client:
     dect rf_tool -m tx --tx_subslot_count 16 --tx_idle_subslot_count 3 -c 1677 -t 39

Example 4: Bi-directional testing with more data
------------------------------------------------

* On both TX and RX side - scan for a free channel (see previous example):

  .. code-block:: console

     server:
     dect rf_tool -m rx_tx --rx_find_sync --rx_subslot_count 9 --rx_idle_subslot_count 4 --tx_subslot_count 8 --tx_idle_subslot_count 4 --tx_mcs 4 --frame_repeat_count 50 -c 1677

     client:
     dect rf_tool -m rx_tx --rx_subslot_count 9 --rx_idle_subslot_count 4 --tx_subslot_count 8 --tx_idle_subslot_count 4 --tx_mcs 4 --frame_repeat_count 50 -c 1677 --tx_pwr 15 -t 39

Dect NR+ PHY MAC
================

DeSh command: ``dect mac``

This command demostrates basic sample of DECT NR+ MAC layer on top of PHY API based on ETSI TS 103 636-4 V1.4.8 (2024-01).
With this command you can start a cluster beacon, scan for beacons, associate/dissociate, and send data to beacon random access RX window.
Note: this is just a basic sample and not a full MAC implementation.

Example: starting of cluster beacon and sending RA data to it
-------------------------------------------------------------

* FT/Beacon device - start periodic cluster beacon TX at default band #1 and at the 1st free channel:

  .. code-block:: console

      desh:~$ dect sett --reset
      desh:~$ dect sett -t 1234

      desh:~$ dect mac beacon_start
      Beacon starting
      RSSI scan started.
      RSSI scan duration: scan_time_ms 2010 (subslots 9648)
      RSSI scanning results:
      channel                               1657
      total scanning count                  201
         saturations                         30
      highest RSSI                          -59
      lowest RSSI                           -104
      RSSI scanning results:
      channel                               1659
      total scanning count                  201
         saturations                         30
      highest RSSI                          -73
      lowest RSSI                           -105
      RSSI scanning results:
      channel                               1661
      total scanning count                  201
         saturations                         30
      highest RSSI                          -95
      lowest RSSI                           -104
      RSSI scan done. Found 1 free, 1 possible and 1 busy channels.
      Scheduled beacon TX: interval 2000ms, tx pwr 0 dbm, channel 1661, payload PDU byte count: 50
      "Free" channel 1661 was chosen for the beacon.
      Beacon TX started.

* FT/Beacon device - Check MAC status:

  .. code-block:: console

      desh:~$ dect mac status
      dect-phy-mac status:
      Cluster beacon status:
      Beacon running: yes
      Beacon channel:                1661
      Beacon tx power:               0 dBm
      Beacon interval:               2000 ms
      Beacon payload PDU byte count: 50
      Neighbor list status:

* FT/Beacon device - Check generated long and short RD IDs from settings:

  .. code-block:: console

      desh:~$ dect sett -r
      Common settings:
      network id (32bit).............................305419896 (0x12345678)
      transmitter id (long RD ID)....................1234 (0x000004d2)
      short RD ID....................................10031 (0x272f)
      band number....................................1

* PT/client side: Scan beacon:

  .. code-block:: console

      desh:~$ dect mac beacon_scan -c 1661
      Starting RX: channel 1661, rssi_level 0, duration 4 secs.
      RX started.
      Beacon scan started.
      RSSI scanning results:
      channel                               1661
      total scanning count                  100
      highest RSSI                          -87
      lowest RSSI                           -106
      PCC received (stf start time 9246147106): status: "valid - PDC can be received", snr 96, RSSI-2 -101 (RSSI -50)
      phy header: short nw id 120 (0x78), transmitter id 10031
      receiver id: 0
      len 1, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 9246147106): snr 99, RSSI-2 -101 (RSSI -50), len 50
      DECT NR+ MAC PDU:
      MAC header:
         Version: 0
         Security: MAC security is not used
         Type: Beacon Header
            Network ID (24bit MSB):  1193046 (0x123456)
            Transmitter ID:          1234 (0x000004d2)
      SDU 1:
         MAC MUX header:
            IE type: Cluster Beacon message
            Payload length: 5
            Received cluster beacon:
            System frame number:  39
      ...
      ...
      Neighbor with long rd id 1234 (0x000004d2), short rd id 10031 (0x272f), nw (24bit MSB: 1193046 (0x123456), 8bit LSB: 120 (0x78))
      updated with time 9384387106 to nbr list.
      RX DONE

* PT/client side: See that scanned beacon can be found from neighbor list:

  .. code-block:: console

      desh:~$ dect mac status
      dect-phy-mac status:
      Cluster beacon status:
      Beacon running: no
      Neighbor list status:
      Neighbor 1:
         network ID (24bit MSB): 1193046 (0x123456)
         network ID (8bit LSB):  120 (0x78)
         network ID (32bit):     305419896 (0x12345678)
         long RD ID:             1234
         short RD ID:            10031
         channel:                1661
         last seen time:         9384387106

* PT/client side: Send association request to scanned beacon:

  .. code-block:: console

      desh:~$ dect mac client_associate -t 1234
      Sending association_req to beacon 1234 RACH window
      Scheduled random access data TX/RX:
      target long rd id 1234 (0x000004d2), short rd id 10031 (0x272f),
      target 32bit nw id 305419896 (0x12345678), tx pwr 0 dbm,
      channel 1661, payload PDU byte count: 50,
      beacon interval 2000, frame time 39244529506, beacon received 9384387106
      Association request TX started.
      TX for Association Request completed.
      PCC received (stf start time 39245148746): status: "valid - PDC can be received", snr 97, RSSI-2 -106 (RSSI -53)
      phy header: short nw id 120 (0x78), transmitter id 10031
      receiver id: 60629
      len 0, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 39245148746): snr 99, RSSI-2 -106 (RSSI -53), len 17
      DECT NR+ MAC PDU:
      MAC header:
         Version: 0
         Security: MAC security is not used
         Type: Unicast Header
            Reset: yes
            Seq Nbr: 0
            Receiver: 38 (0x00000026)
            Transmitter: 1234 (0x000004d2)
      SDU 1:
         MAC MUX header:
            IE type: Association Response message
            Payload length: 1
            Received Association Response message:
            Acknowledgment:  ACK
            Flow count: 0b111: All flows accepted as configured in association request.
      SDU 2:
         MAC MUX header:
            IE type: Padding
            Payload length: 0
            Received padding data, len 0, payload is not printed
      SDU 3:
         MAC MUX header:
            IE type: Padding
            Payload length: 1
            Received padding data, len 1, payload is not printed
      RX for Association Response completed.

* PT/client side: Send RA data to scanned beacon:

  .. code-block:: console

      desh:~$ dect mac client_rach_tx -t 1234 -d "TAPPARA!"
      Sending data TAPPARA! to beacon 1234 RACH window
      Scheduled random access data TX:
      target long rd id 1234 (0x000004d2), short rd id 10031 (0x272f),
      target 32bit nw id 305419896 (0x12345678), tx pwr 0 dbm,
      channel 1661, payload PDU byte count: 17,
      beacon interval 2000, frame time 44497548706, beacon received 9384387106
      Client TX to RACH started.
      Client data TX completed.

* FT/Beacon device: Observe that data was received:

  .. code-block:: console

      PCC received (stf start time 47697522080): status: "valid - PDC can be received", snr 91, RSSI-2 -112 (RSSI -56)
      phy header: short nw id 120 (0x78), transmitter id 60629
      receiver id: 10031
      len 0, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 47697522080): snr 96, RSSI-2 -112 (RSSI -56), len 17
      DECT NR+ MAC PDU:
      MAC header:
         Version: 0
         Security: MAC security is not used
         Type: DATA MAC PDU header
            Reset: yes
            Seq Nbr: 1
      SDU 1:
         MAC MUX header:
            IE type: User plane data - flow 1
            Payload length: 10
            DLC IE type: Data: DLC Service type 0 without a routing header (0x01)
            Received data, len 9, payload as ascii string print:
               TAPPARA!
      SDU 2:
         MAC MUX header:
            IE type: Padding
            Payload length: 0
            Received padding data, len 0, payload is not printed

* PT/client side: Send association release to scanned beacon:

  .. code-block:: console

      desh:~$ dect mac client_dissociate -t 1234
      Sending association release to beacon 1234 RACH window
      Scheduled random access data TX/RX:
      target long rd id 1234 (0x000004d2), short rd id 10031 (0x272f),
      target 32bit nw id 305419896 (0x12345678), tx pwr 0 dbm,
      channel 1661, payload PDU byte count: 17,
      beacon interval 2000, frame time 61501068706, beacon received 9384387106
      Association Release TX started.
      TX for Association Release completed.

* FT/Beacon device: See that Association Release message was received:

  .. code-block:: console

      PCC received (stf start time 64701043040): status: "valid - PDC can be received", snr 99, RSSI-2 -104 (RSSI -52)
      phy header: short nw id 120 (0x78), transmitter id 60629
      receiver id: 10031
      len 0, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 64701043040): snr 102, RSSI-2 -104 (RSSI -52), len 17
      DECT NR+ MAC PDU:
      MAC header:
         Version: 0
         Security: MAC security is not used
         Type: Unicast Header
            Reset: yes
            Seq Nbr: 3
            Receiver: 1234 (0x000004d2)
            Transmitter: 38 (0x00000026)
      SDU 1:
         MAC MUX header:
            IE type: Association Release message
            Payload length: 1
            Received Association Release message:
            Release Cause:  Connection termination. (value: 0)
      SDU 2:
         MAC MUX header:
            IE type: Padding
            Payload length: 1
            Received padding data, len 1, payload is not printed

* FT/Beacon device: Stop beacon

+  .. code-block:: console

      desh:~$ dect mac beacon_stop
      Stopping beacon.
      Beacon TX stopped.

----


Building
********

.. |sample path| replace:: :file:`samples/dect/dect_shell`

.. include:: /includes/build_and_run_ns.txt

See :ref:`cmake_options` for instructions on how to provide CMake options, for example to use a configuration overlay.

Dependencies
************

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
