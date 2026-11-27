CAN multi-DUT test
##################

The test verifies classic CAN communication between a development kit with the hardware M_CAN controller, a development kit with the software sCAN controller, and a host SocketCAN interface, all connected to a shared CAN bus.

Requirements
************

The test requires the following devices:

* nRF54L15 DK (``nrf54l15dk/nrf54l15/cpuapp``) with the sCAN controller running on the FLPR core, with the ``can`` and ``scan`` fixtures.
  The sCAN TX and RX pins are defined in the :file:`boards/nrf54l15dk_nrf54l15_cpuapp.overlay` file.
* nRF54H20 DK (``nrf54h20dk/nrf54h20/cpuapp``) with the M_CAN controller, with the ``can`` and ``mcan`` fixtures.
* CAN transceivers connecting both DKs to a common, properly terminated CAN bus.
* A Linux host with a SocketCAN interface ``can0`` connected to the same bus.

Before the test session, set up the host interface in listen-only mode at 1 Mbit/s, for example:

.. code-block:: console

   sudo ip link set can0 down
   sudo ip link set can0 type can bitrate 1000000 listen-only on
   sudo ip link set can0 up

The test does not change the host interface configuration and does not require root privileges.
If the host interface is not configured as expected, the tests that use it fail, and the other tests might be disturbed.

Overview
********

The test consists of two Twister scenarios defined in the :file:`testcase.yaml` file:

* ``drivers.can.multidut.peer`` - Build-only image for the nRF54H20 DK (M_CAN).
* ``drivers.can.multidut.classic`` - Pytest-based test running on the nRF54L15 DK (sCAN).
  It uses the ``drivers.can.multidut.peer`` image as a required device, so both DKs are programmed and used in one test session.

Both images run the same application, which consists of the Zephyr CAN shell and the following additional ``can_host`` shell commands:

* ``can_host chosen`` - Prints the ``zephyr,canbus`` chosen device and the controller type (``mcan`` or ``scan``).
  The test uses it to identify which DUT has which controller.
* ``can_host burst [-e] <CAN ID> <count>`` - Sends a burst of 8-byte frames with a 32-bit big-endian sequence number in ``data[0..3]`` and the ``0xA5`` pattern in ``data[4..7]``.
* ``can_host rx start [-e] <CAN ID>`` and ``can_host rx stop`` - Count the received frames with the given ID in firmware and report the number of frames, gaps in the sequence numbers, and duplicates.

The pytest scripts control both DKs through the :file:`pytest/can_shell.py` module, which implements a python-can bus on top of the Zephyr CAN shell.
It supports runtime configuration of the controller mode and bitrate, reading the controller state, error counters and statistics, RX filters, and the ``can_host`` commands.

Role of the host
================

The host interface never acknowledges, transmits, or sends error flags.
Therefore, every frame must be acknowledged by the other DK, or by the M_CAN controller itself when it runs in external loopback mode.
At 1 Mbit/s, the host acts as a passive sniffer that records all frames and bus errors.
At other bitrates, the host does not take part in the communication.

Test procedure
==============

Before each test case, both DKs are configured to a common bitrate and mode, and the controllers are started.
A test case is skipped if a controller does not support the requested bitrate, mode, or capability.

The M_CAN controller does not reset its error counters on stop and start.
To prevent errors provoked by one test case from affecting the following ones, the controllers in normal mode are brought back to the error-active state with both error counters at zero by sending bursts of frames with ID ``0x7F0``.

After each test case, both controllers are stopped and the accept-all CAN shell RX filters are restored.

Most test cases are run in both directions, from M_CAN to sCAN and from sCAN to M_CAN.

Test cases
==========

The test frame set includes standard and extended frames with boundary IDs, frames with every DLC from 0 to 8, data patterns ``0x00``, ``0x55``, ``0xAA`` and ``0xFF``, frames causing intensive bit stuffing, and remote transmission request (RTR) frames.

The test suite contains the following test cases:

* ``TestBidirectional::test_exchange`` - Sends all test frames from one DK to the other at every tested bitrate.
  The receiver must receive identical frames and no extra frames, and both controllers must stay error active.
  At 1 Mbit/s, the host additionally verifies that every frame appeared on the bus exactly once, in order, and without bus errors.
* ``TestHostSniffer::test_dk_to_all`` - Sends each test frame separately at 1 Mbit/s.
  The frame must be received by the other DK and seen by the host.
* ``TestArbitration::test_concurrent_bursts`` - Both DKs send bursts of 200 frames with adjacent IDs at the same time, so arbitration takes place on every collision.
  The test verifies that no frames are lost or duplicated, the per-sender order is preserved on the bus, and both controllers are error active afterwards.
* ``TestFilters::test_rx_filters`` - Configures a standard and an extended RX filter on the receiver at 20 kbit/s and 1 Mbit/s.
  Only the frames matching the filters must be delivered, including the distinction between standard and extended IDs.
* ``TestListenOnly::test_listen_only_receives`` - M_CAN in loopback mode sends frames that nobody acknowledges, and sCAN in listen-only mode must receive them.
  The listen-only sCAN must not transmit any frame.
  This test case is expected to fail, because sCAN in listen-only mode does not deliver unacknowledged frames.
* ``TestListenOnly::test_no_ack_from_listen_only`` - With the only other receiver in listen-only mode, the transmission must fail with ACK errors.
  Communication must recover after the receiver returns to normal mode.
  The sender must enter the error-warning or error-passive state, as required by ISO 11898-1.
  For sCAN, this check is expected to fail, because sCAN does not increment the transmit error counter on ACK errors.
* ``TestOneShot::test_one_shot_success`` - An acknowledged frame sent in one-shot mode must appear on the bus exactly once.
* ``TestOneShot::test_one_shot_no_ack`` - A frame sent in one-shot mode without any receiver must fail after a single attempt.
* ``TestBitrateMismatch::test_mismatch_and_recovery`` - With the DKs configured to different bitrates (500 kbit/s and 250 kbit/s), no frame must pass and the receiver must detect errors.
  After setting a common bitrate, communication in both directions must recover and both controllers must return to the error-active state.
* ``TestBitrateSequence::test_bitrate_sequence`` - Walks through all tested bitrates up and down in one session and exchanges a frame in both directions at each step.
  No stale frames from previous steps must be received.

Building and running
********************

Run the test with Twister on a hardware map containing both DKs with the required fixtures:

.. code-block:: console

   west twister -T nrf/tests/drivers/can/multidut -s drivers.can.multidut.classic \
     --device-testing --hardware-map <hardware_map.yaml>


The test supports the following pytest options, which you can pass with the ``--pytest-args`` Twister option:

* ``--can-bitrates=<list>`` - Comma-separated list of nominal bitrates in bit/s to test instead of the default list ``20000,50000,100000,125000,250000,500000,800000,1000000``.
  Specify the values as plain integers, without unit suffixes.
  For example, ``--can-bitrates=1000000`` runs the bitrate-dependent test cases only at 1 Mbit/s.
  This option affects only the ``TestBidirectional::test_exchange`` and ``TestBitrateSequence::test_bitrate_sequence`` test cases.
* ``--can-slow`` - Additionally test 10000 bit/s.
  This option is ignored when ``--can-bitrates`` is specified.

Pass each option to Twister in the ``--pytest-args=<option>`` form, for example:

.. code-block:: console

   west twister -T nrf/tests/drivers/can/multidut -s drivers.can.multidut.classic \
     --device-testing --hardware-map <hardware_map.yaml> \
     --pytest-args=--can-bitrates=1000000
