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

* Change the default band to ``2`` (has impact when automatic channel selection is used, in other words, the set channel is zero in ``dect rssi_scan``):

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
