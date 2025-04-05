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
       activate
       deactivate
       sett
       radio_mode
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

* Change the default band to ``2`` (has impact when automatic channel selection is used, in other words, when the set channel is zero in ``dect rssi_scan`` or in ``dect mac beacon_start`` command):

  .. code-block:: console

     dect sett -b 2

.. caution::
   There might be region-specific limitations for radio channel usage.
   See Regulations and Channel frequency sections of the :ref:`nrfxlib:nrf_modem_dect_phy` page for using different DECT NR+ radio bands and channels in different regions.
   Make sure to always measure the channel with the ``dect rssi_scan`` command before accessing the band.

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

* Execute longer RSSI measurements on all permitted channels, and instead of default high/low RSSI value-based verdict, use subslot count-based verdict for BUSY/POSSIBLE/FREE and as an end result, see the verdict for the best channel:

  .. code-block:: console

     dect rssi_scan -c 0 --verdict_type_count -t 3000 -a

* Execute longer RSSI measurements on specific channel, and instead of default high/low RSSI value-based verdict, use subslot count-based verdict for BUSY/POSSIBLE/FREE.
  Additionally, print BUSY/POSSIBLE measurements:

  .. code-block:: console

     dect rssi_scan -c 1661 --verdict_type_count_details -t 6500

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
     dect rf_tool -m rx_tx --rx_find_sync --frame_repeat_count 15 --frame_repeat_count_intervals 10 -c 1677

* Client side: Trigger to start operation:

  .. code-block:: console

     dect rf_tool -m rx_tx --frame_repeat_count 15 --frame_repeat_count_intervals 10 -t 39 -c 1677

Example 2: unidirectional testing
---------------------------------

* On both TX and RX side - scan for a free channel (see previous example).
* RX device option 1: RX single shot mode:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx --rx_find_sync --frame_repeat_count 15 -c 1677

* RX device option 2: RX device on ``rx_cont`` mode:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx_cont -c 1677

* RX device option 3: RX device in ``rx_cont`` mode with the information of TX side to have interval reporting:

  .. code-block:: console

     dect sett -t 39
     dect rf_tool -m rx_cont --rf_mode_peer tx --frame_repeat_count 15 --rx_find_sync -c 1677

* TX device: Trigger to start operation:

  .. code-block:: console

     dect rf_tool -m tx --frame_repeat_count 15 -c 1677 -t 39

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
     dect rf_tool -m rx_tx --rx_find_sync --rx_subslot_count 9 --rx_idle_subslot_count 3 --tx_subslot_count 8 --tx_idle_subslot_count 3 --frame_repeat_count 15 -c 1677

     client:
     dect rf_tool -m rx_tx --rx_subslot_count 9 --rx_idle_subslot_count 3 --tx_subslot_count 8 --tx_idle_subslot_count 3 --frame_repeat_count 15 -c 1677 -t 39

  RX/TX duty cycle percentage 82.50%:

  .. code-block:: console

     server:
     dect rf_tool -m rx_tx --rx_find_sync --rx_subslot_count 17 --rx_idle_subslot_count 3 --tx_subslot_count 16 --tx_idle_subslot_count 4 --frame_repeat_count 15 -c 1677

     client:
     dect rf_tool -m rx_tx --rx_subslot_count 17 --rx_idle_subslot_count 3 --tx_subslot_count 16 --tx_idle_subslot_count 4 --frame_repeat_count 15 -c 1677 -t 39

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
     dect rf_tool -m rx_tx --rx_find_sync --rx_subslot_count 9 --rx_idle_subslot_count 4 --tx_subslot_count 8 --tx_idle_subslot_count 4 --tx_mcs 4 --frame_repeat_count 15 -c 1677

     client:
     dect rf_tool -m rx_tx --rx_subslot_count 9 --rx_idle_subslot_count 4 --tx_subslot_count 8 --tx_idle_subslot_count 4 --tx_mcs 4 --frame_repeat_count 15 -c 1677 --tx_pwr 15 -t 39

Dect NR+ PHY MAC
================

DeSh command: ``dect mac``

This command demonstrates a basic sample of the DECT NR+ MAC layer on top of PHY API. It is based on MAC specification `ETSI TS 103 636-4`_.
With this command, you can start a cluster beacon, scan for beacons, associate/dissociate, and send data to the beacon random access RX window.

.. note::
   This is just an initial basic sample and not a full MAC implementation.
   It does not support all the features of the DECT NR+ MAC layer and is not fully compliant with the MAC specification.
   For example, cluster beaconing with RA allocation and LMS implementations overall are not what is required by the specification.

The following abbreviations from MAC specification are used in the examples:

* FT: Fixed Termination point
* PT: Portable Termination point

Example: starting of cluster beacon and sending RA data to it
-------------------------------------------------------------

* FT/Beacon device - Start periodic cluster beacon TX on default band 1 and on the first free channel:

  .. code-block:: console

      desh:~$ dect sett --reset
      desh:~$ dect sett -t 1234

      dect common settings saved
      desh:~$ dect mac beacon_start
      Beacon starting
      RSSI scan started.
      RSSI scan duration: scan_time_ms 2010 (subslots 9648)
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 2335226066):
      channel                               1657
      total scanning count                  201
      highest RSSI                          -94
      lowest RSSI                           -104
      Subslot count based results:
      total subslots: 9648
      free subslots: 9648, possible subslots: 0, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 100.00%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 2538716193):
      channel                               1659
      total scanning count                  201
      highest RSSI                          -97
      lowest RSSI                           -106
      Subslot count based results:
      total subslots: 9648
      free subslots: 9648, possible subslots: 0, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 100.00%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 2742115281):
      channel                               1661
      total scanning count                  201
      highest RSSI                          -97
      lowest RSSI                           -104
      Subslot count based results:
      total subslots: 9648
      free subslots: 9648, possible subslots: 0, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 100.00%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 2945512983):
      channel                               1663
      total scanning count                  201
      highest RSSI                          -96
      lowest RSSI                           -106
      Subslot count based results:
      total subslots: 9648
      free subslots: 9648, possible subslots: 0, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 100.00%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 3148976543):
      channel                               1665
      total scanning count                  201
      highest RSSI                          -68
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9641, possible subslots: 0, busy subslots: 7
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 99.93%, possible: 99.93%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 3352412279):
      channel                               1667
      total scanning count                  201
      highest RSSI                          -97
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9648, possible subslots: 0, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 100.00%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 3555822695):
      channel                               1669
      total scanning count                  201
      highest RSSI                          -78
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9645, possible subslots: 3, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 99.97%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 3759189351):
      channel                               1671
      total scanning count                  201
         saturations                         38
      highest RSSI                          -56
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9623, possible subslots: 5, busy subslots: 20
      not measured subslots: 0, saturated subslots: 8
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 99.74%, possible: 99.79%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 3963049409):
      channel                               1673
      total scanning count                  201
      highest RSSI                          -72
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9051, possible subslots: 597, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 93.81%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 4166505944):
      channel                               1675
      total scanning count                  201
      highest RSSI                          -57
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9627, possible subslots: 16, busy subslots: 5
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 99.78%, possible: 99.95%
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 4369912341):
      channel                               1677
      total scanning count                  201
      highest RSSI                          -95
      lowest RSSI                           -105
      Subslot count based results:
      total subslots: 9648
      free subslots: 9648, possible subslots: 0, busy subslots: 0
      not measured subslots: 0, saturated subslots: 0
      Final verdict FREE based on SCAN_SUITABLE 75%:
         free: 100.00%, possible: 100.00%
      -----------------------------------------------------------------------------
      RSSI scan done. Found 11 free, 0 possible and 0 busy channels.
         Best channel: 1659
         Final verdict: FREE
         Free subslots: 9648
         Possible subslots: 0
         Busy subslots: 0
      Scheduled beacon TX: interval 2000ms, tx pwr 0 dbm, channel 1659, payload PDU byte count: 50
      Channel 1659 was chosen for the beacon.
      Beacon TX started.

* FT/Beacon device - Check MAC status:

  .. code-block:: console

      desh:~$ dect mac status
      dect-phy-mac status:
      Cluster beacon status:
         Beacon running:                yes
         Beacon channel:                1659
         Beacon tx power:               0 dBm
         Beacon interval:               2000 ms
         Beacon payload PDU byte count: 50
      Client status:
      Neighbor list status:

* FT/Beacon device - Check generated long and short RD IDs from settings:

  .. code-block:: console

      desh:~$ dect sett -r
      Common settings:
      network id (32bit).............................305419896 (0x12345678)
      transmitter id (long RD ID)....................1234 (0x000004d2)
      short RD ID....................................27462 (0x6b46)
      band number....................................1

* PT/client side - Scan the beacon:

  .. code-block:: console

      desh:~$ dect sett --reset
      desh:~$ dect sett -t 1245
      desh:~$ dect mac beacon_scan -c 1659
      -----------------------------------------------------------------------------
      Beacon scan started.
      Starting RX: channel 1659, rssi_level 0, duration 4 secs.
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 16806798765):
      channel                               1659
      total scanning count                  102
      highest RSSI                          -105
      lowest RSSI                           -112
      PCC received (stf start time 16878096625): status: "valid - PDC can be received", snr 97, RSSI-2 -121 (RSSI -60)
      phy header: short nw id 120 (0x78), transmitter id 27462
      len 1, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 16878096625): snr 99, RSSI-2 -122 (RSSI -61), len 50
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
            System frame number:  108
            Max TX power:         19 dBm
            Power const:          The RD operating in FT mode does not have power constraints.
            Frame offset:         not included in the beacon
            Next cluster channel: current cluster channel.
            Time to next:         not included in the beacon.
                                    The next cluster beacon is
                                    transmitted based on Cluster beacon period.
            Network Beacon period 10 ms
            Cluster Beacon period 2000 ms
            Count to trigger:     0 (coded value)
            Relative quality:     0 (coded value)
            Min quality:          0 (coded value)
      SDU 2:
         MAC MUX header:
            IE type: Random Access Resource IE
            Payload length: 7
            Received RACH IE data:
            Repeat:               Repeated in the following frames as in repetition and validity fields
               Repetition:         2
               Validity:           100
            System frame number:  Not included - resource allocation immediately valid
            RA Channel:           Not included - resource allocation is valid for current channel
            RA Response Channel:  Not included - response is sent in same channel as this IE
            Start subslot:        12
            Length type:          in slots
            Length:               10
            Max RACH length type: in slots
            Max RACH length:      4
            CW min sig:           0
            DECT delay:           resp win starts 0.5 frames after the start of the RA TX
            Response win:         11 subslots
            CW max sig:           7
      SDU 3:
         MAC MUX header:
            IE type: Padding
            Payload length: 24
            Received padding data, len 24, payload is not printed
      Neighbor with long rd id 1234 (0x000004d2), short rd id 27462 (0x6b46) stored to nbr list.
      -----------------------------------------------------------------------------
      RSSI scanning results (meas #1 mdm time 16878683565):
      channel                               1659
         neighbor has been seen in this channel
      total scanning count                  102
      highest RSSI                          -104
      lowest RSSI                           -112
      -----------------------------------------------------------------------------
      ...
      -----------------------------------------------------------------------------
      RX DONE.

* PT/client side - As an alternative to the previous command, and if you do not know in which channel the beacon is running, you can scan all channels in a set band:

  .. code-block:: console

      desh:~$ dect mac beacon_scan -c 0

* PT/client side - Check that the scanned beacon is found from neighbor list:

  .. code-block:: console

      desh:~$ dect mac status
      dect-phy-mac status:
      Cluster beacon status:
         Beacon running: no
      Client status:
      Neighbor list status:
         Neighbor 1:
            network ID (24bit MSB): 1193046 (0x123456)
            network ID (8bit LSB):  120 (0x78)
            network ID (32bit):     305419896 (0x12345678)
            long RD ID:             1234
            short RD ID:            27462
            channel:                1659
            Last seen:              12964 msecs ago
            Background scan status:
               Running: false

* PT/client side - Send association request to the scanned beacon:

  .. code-block:: console

      desh:~$ dect mac associate -t 1234
      Sending association_req to FT 1234's random access resource
      Scheduled random access data TX/RX:
      target long rd id 1234 (0x000004d2), short rd id 27462 (0x6b46),
      target 32bit nw id 305419896 (0x12345678), tx pwr 0 dbm,
      channel 1659, payload PDU byte count: 50,
      beacon interval 2000, frame time 24205018225, beacon received 17016336625
      Association request TX started.
      TX for Association Request completed.
      PCC received (stf start time 24205392665): status: "valid - PDC can be received", snr 94, RSSI-2 -122 (RSSI -61)
      phy header: short nw id 120 (0x78), transmitter id 27462
      receiver id: 27761
      len 0, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 24205392665): snr 100, RSSI-2 -122 (RSSI -61), len 17
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
            Payload length: 2
            Received Association Response message:
            Acknowledgment:  ACK
            Flow count: 0b111: All flows accepted as configured in association request.
      SDU 2:
         MAC MUX header:
            IE type: Padding (1 byte)
            Payload length: 1
            Received padding data, len 1, payload is not printed
      RX for Association Response completed.

* PT/client side - Check that the client is associated with the device with long RD ID 1234 and a background scan is running:

  .. code-block:: console

      desh:~$ dect mac status
      dect-phy-mac status:
      Cluster beacon status:
         Beacon running: no
      Client status:
         Association #1: long RD ID 1234
      Neighbor list status:
         Neighbor 1:
            network ID (24bit MSB): 1193046 (0x123456)
            network ID (8bit LSB):  120 (0x78)
            network ID (32bit):     305419896 (0x12345678)
            long RD ID:             1234
            short RD ID:            64945
            channel:                1671
            Last seen:              1428 msecs ago
            Last timestamp: 16878096625 mdm ticks
            Metrics:
               Scan info updated count:            1
               Scan started ok count:              1
               Scan start fail count:              0
               Scan info time shift updated count: 0
               Scan info time shift last value:    0

* PT/client side - Send RA data to the scanned beacon:

  .. code-block:: console

      desh:~$ dect mac rach_tx -t 1234 -d "TAPPARA!"
      Sending data TAPPARA! to FT 1234's random access resource
      Scheduled random access data TX:
      target long rd id 1234 (0x000004d2), short rd id 27462 (0x6b46),
      target 32bit nw id 305419896 (0x12345678), tx pwr 0 dbm,
      channel 1659, payload PDU byte count: 17,
      beacon interval 2000, frame time 29319898225, beacon received 17016336625
      Client TX to RACH started.
      Client data TX completed.

* FT/Beacon device - Observe that data was received:

  .. code-block:: console

      PCC received (stf start time 32017011258): status: "valid - PDC can be received", snr 91, RSSI-2 -123 (RSSI -61)
      phy header: short nw id 120 (0x78), transmitter id 27761
      receiver id: 27462
      len 0, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 32017011258): snr 98, RSSI-2 -123 (RSSI -61), len 17
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
            IE type: Padding (0 byte)
            Payload length: 0
            Received padding data, len 0, payload is not printed

* PT/client side - Send JSON-formatted periodic RA data in 10-second intervals with the current modem temperature to the scanned beacon:

  .. code-block:: console

      desh:~$ dect mac rach_tx -t 1234 -d "Data from device 1245" -i 10 -j

* PT/client side - Stop periodic RA data sending:

  .. code-block:: console

      desh:~$ dect mac rach_tx stop

* PT/client side - Send association release to the scanned beacon:

  .. code-block:: console

      desh:~$ dect mac dissociate -t 1234
      Sending association release to FT 1234's random access resource
      Scheduled random access data TX/RX:
      target long rd id 1234 (0x000004d2), short rd id 27462 (0x6b46),
      target 32bit nw id 305419896 (0x12345678), tx pwr 0 dbm,
      channel 1659, payload PDU byte count: 17,
      beacon interval 2000, frame time 34434778225, beacon received 17016336625
      Association Release TX started.
      TX for Association Release completed.

* FT/Beacon device - Observe that the association release message was received:

  .. code-block:: console

      PCC received (stf start time 37131891398): status: "valid - PDC can be received", snr 93, RSSI-2 -123 (RSSI -61)
      phy header: short nw id 120 (0x78), transmitter id 27761
      receiver id: 27462
      len 0, MCS 0, TX pwr: 0 dBm
      PDC received (stf start time 37131891398): snr 94, RSSI-2 -123 (RSSI -61), len 17
      DECT NR+ MAC PDU:
      MAC header:
         Version: 0
         Security: MAC security is not used
         Type: Unicast Header
            Reset: yes
            Seq Nbr: 2
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

* PT/client side - Check that no associations and background scans are running:

  .. code-block:: console

      desh:~$ dect mac status

* FT/Beacon device - Stop the beacon:

  .. code-block:: console

      desh:~$ dect mac beacon_stop
      Stopping beacon.
      Beacon TX stopped, cause: User Initiated.

Example: two devices sending data to each other
-----------------------------------------------

* FT/Beacon device 1 - Start periodic cluster beacon TX on default band 1 and on the first free channel:

  .. code-block:: console

      desh:~$ dect sett --reset
      desh:~$ dect sett -t 1
      desh:~$ dect mac beacon_start
      ...
      Channel 1671 was chosen for the beacon.
      Beacon TX started.

* FT/Beacon device 2 - Scan the beacon from device 1:

  .. code-block:: console

      desh:~$ dect sett --reset
      desh:~$ dect sett -t 2
      desh:~$ dect mac beacon_scan -c 1671

* FT/Beacon device 2 - Start periodic cluster beacon TX on default band 1 and on the first free channel:

  .. code-block:: console

      desh:~$ dect mac beacon_start
      ...
      Channel 1675 was chosen for the beacon.
      Beacon TX started.

* FT/Beacon device 1 - Scan the beacon from device 2 by using special force:

  .. code-block:: console

      desh:~$ dect mac beacon_scan -c 1675 -f

* FT/Beacon device 1 - Send association request to device 2:

  .. code-block:: console

      desh:~$ dect mac associate -t 2


* FT/Beacon device 2 - Send association request to device 1:

  .. code-block:: console

      desh:~$ dect mac associate -t 1

* FT/Beacon device 1 - Send JSON-formatted periodic RA data in 10-second intervals with the current modem temperature to the device 2:

  .. code-block:: console

      desh:~$ dect mac rach_tx -t 2 -d "Data from device 1" -i 10 -j

* FT/Beacon device 2 - Send JSON-formatted periodic RA data in 10-second intervals with the current modem temperature to the device 1:

  .. code-block:: console

      desh:~$ dect mac rach_tx -t 1 -d "Data from device 2" -i 10 -j

* FT/Beacon devices 1 & 2 - Stop periodic RA data sending:

  .. code-block:: console

      desh:~$ dect mac rach_tx stop

* FT/Beacon device 1 - Send association release to the device 2:

  .. code-block:: console

      desh:~$ dect mac dissociate -t 2

* FT/Beacon device 2 - Send association release to the device 1:

  .. code-block:: console

      desh:~$ dect mac dissociate -t 1

* FT/Beacon devices 1 & 2 - Stop the beacon:

  .. code-block:: console

      desh:~$ dect mac beacon_stop

Running commands at bootup
==========================

DeSh command: ``startup_cmd``.

You can use the ``startup_cmd`` command to store shell commands to be run sequentially after bootup.

Example: starting of cluster beacon and sending RA data to it
-------------------------------------------------------------

* FT/Beacon device - Set a command to start a cluster beacon five seconds after bootup:

  .. code-block:: console

      desh:~$ startup_cmd -t 5 --mem_slot 1 --cmd_str "dect sett -t 1234"
      desh:~$ startup_cmd --mem_slot 2 --cmd_str "dect mac beacon_start"

* PT/Client device - Set a command to start scanning for the beacon 60 seconds after bootup, associate to it, and send RA data to it:

  .. code-block:: console

      desh:~$ startup_cmd -t 60 --mem_slot 1 --cmd_str "dect sett -t 1235"
      desh:~$ startup_cmd --mem_slot 2 --cmd_str "dect mac beacon_scan -c 0"
      desh:~$ startup_cmd --mem_slot 3 -d 60 --cmd_str "dect mac associate -t 1234"
      desh:~$ startup_cmd --mem_slot 4 -d 10 --cmd_str "dect mac rach_tx -t 1234 -d data"

Building
********

.. |sample path| replace:: :file:`samples/dect/dect_phy/dect_shell`

.. include:: /includes/build_and_run_ns.txt

See :ref:`cmake_options` for instructions on how to provide CMake options, for example to use a configuration overlay.

Dependencies
************

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`
* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
