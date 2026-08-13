.. _ug_nrf93m1_testing:

Testing the cellular connection
###############################

.. contents::
   :local:
   :depth: 2

This page describes how to verify that an nRF93 Series design connects, transfers data, and reports location, and how to diagnose the common failure modes.

.. _ug_nrf93m1_testing_registration:

Verifying registration
**********************

The module boots at ``AT+CFUN=0`` and does not attach automatically, so the host owns the attach sequence.
Subscribe to events before you enable the radio, so you do not miss anything that happens during registration:

.. code-block:: console

   AT+CEREG=<notification level>
   OK
   AT+CGEREP=<packet domain event reporting>
   OK
   AT+CFUN=1
   OK
   AT+CEREG?
   +CEREG: 1,1,"1A2B","01234567",7
   OK

The second parameter carries the registration state.
``1`` means registered on the home network, and ``5`` means registered while roaming.
Any other value means the module is not registered.

.. list-table:: Registration failures
   :header-rows: 1

   * - Symptom
     - Likely cause
     - What to check
   * - No registration, no network found
     - Antenna or band mismatch
     - Antenna connected. ``AT%BAND=?`` against local operator bands. See :ref:`ug_nrf93m1_variants`.
   * - Network found, registration denied
     - SIM not activated, or no subscription
     - SIM activation status and data plan
   * - Registration succeeds, no data
     - APN not set
     - ``AT+CGDCONT`` and the operator's required APN
   * - Registration drops intermittently
     - Marginal signal, or power saving misconfiguration
     - ``AT+CESQ`` over time, and granted PSM values

.. note::
   First registration on a new SIM or in a new location can take several minutes, because the module scans for a network.
   Do not treat a slow first registration as a failure.

.. _ug_nrf93m1_testing_signal:

Checking signal quality
***********************

.. code-block:: console

   AT+CESQ
   +CESQ: 99,99,255,255,20,55
   OK

The last two values are RSRQ and RSRP, which are the meaningful indicators for LTE.

Record signal quality alongside any connectivity issue you report.
A problem at the edge of coverage behaves differently from a problem with strong signal, and the two need different investigation.

.. _ug_nrf93m1_testing_ping:

Confirming IP connectivity
**************************

Once registered, send an ICMP echo request from the module itself:

.. code-block:: console

   AT%PING=<target>

This tests the module's own IP stack and the operator data path, without involving the host IP stack.
Run it before you debug anything host side, because it isolates the module and network from your application.

.. _ug_nrf93m1_testing_throughput:

Testing throughput
******************

Use the :ref:`nrf93m1dk_ppp_shell` sample, which etablishes a PPP connection on the nRF54L15 host and includes zperf for throughput testing.

Bring the interface up, then check it:

.. code-block:: console

   uart:~$ net iface up 1
   EVENT: L4 [1] IPv4 connectivity available
   uart:~$ net iface
   uart:~$ zperf tcp upload <server> <port> <duration> <packet size>
   uart:~$ net iface down 1

The shell prints network management events as the connection comes up, which is the quickest way to see where a failing connection stops.
If you never see the L4 connectivity event, the problem is registration or APN, not throughput.

.. note::
   Cat 1 bis peaks at 10 Mbps downlink and 5 Mbps uplink.
   Measured throughput is typically well below the peak, because it depends on signal quality, network load, and how much of the channel the operator allocates.
   Test against your deployment network, not only on a bench.

The host also affects the result.
On the PPP path, the host IP stack, the UART baud rate, and flow control all bound throughput.
If you measure far below expectation, confirm the UART is running fast enough and that hardware flow control is enabled before investigating the network.

.. _ug_nrf93m1_testing_location:

Testing location
****************

Both location methods require the device to be provisioned and connected to nRF Cloud, because nRF Cloud resolves the measurements.
See :ref:`ug_93m1_cloud_connecting`.

Collect the raw measurements first, so you can tell a data collection problem from a cloud problem:

.. code-block:: console

   AT%BCINFO
   AT%WIFISCAN=12000,1,5,3,0

Then request a resolved position with ``AT%NRFCLOUDLOCATION``.
See :ref:`ug_93m1_cloud_connecting_location` for the method values and the response format.

.. note::
   Request the location while the module is idle.
   Multicell and Wi-Fi® positioning do not work in RRC Connected mode, so a location request issued immediately after a data transfer will fail or silently fall back to single-cell.

When you evaluate accuracy, test in the environment where your product will be deployed.
Wi-Fi-based location depends on access point density, so figures from a city center will not predict results at a rural site.
Cellular eCID is similarly sensitive to cell size.

.. _ug_nrf93m1_testing_rf:

RF testing
**********

``AT%RFTEST`` provides non-signalling RSSI measurement, supporting the same channel bandwidths as normal operation.
Use it for antenna validation and production test, where you need RF measurements without a network connection.

.. important::
   ``%RFTEST`` is a test command.
   Do not use it in production firmware.

.. _ug_nrf93m1_testing_observability:

Modem observability
*******************

Once a device is connected to nRF Cloud, modem observability reports connectivity and modem state for diagnostics without a debugger attached.
This is the practical way to diagnose intermittent field problems, where the failure does not reproduce on a bench.

.. _ug_nrf93m1_testing_checklist:

Troubleshooting
***************

Work through this order when a new design does not connect to the network.
Each check depends on the previous one.

1. Module powers on and responds to ``AT``.
#. ``AT+CMEE=2`` is set, so failures report a reason rather than a bare ``ERROR``.
#. ``AT+CGMR`` returns a firmware version.
#. ``AT%BAND=?`` returns a band set that covers your local operator.
#. SIM is detected and activated.
#. ``AT+CFUN=1`` succeeds.
#. ``AT+CEREG?`` reports registered.
#. ``AT+CESQ`` reports usable signal.
#. APN is set if the operator requires one.
#. ``AT%PING`` succeeds, confirming end-to-end IP connectivity.
#. Device appears connected in nRF Cloud.

Stopping at the first failure saves time, because a later step failing is usually a symptom of an earlier one.
