.. _modem_trace_backend_sample:

Cellular: Modem trace backend
#############################

.. contents::
   :local:
   :depth: 2

The Modem trace backend sample demonstrates how to add a user-defined modem trace backend to an application.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

.. include:: /includes/external_flash_nrf91.txt

Overview
********

The Modem trace backend sample implements and selects a custom trace backend to receive traces from the nRF9160 modem.
For demonstration purposes, the custom trace backend counts the number of bytes received and calculates the data rate of modem traces received.
The CPU utilization is also measured.
The byte count, data rate, and CPU load are periodically printed to terminal using a delayable work item and the system workqueue.
The custom trace backend is implemented in :file:`modem_trace_backend/src/trace_print_stats.c`.
However, it is possible to add a custom modem trace backend as a library and use it in more than one application.
See :ref:`adding_custom_modem_trace_backends` for details.

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/modem_trace_backend`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Power on or reset your nRF9160 DK.
#. Observe that the sample starts and connects to the network.
#. Observe that the sample displays the amount of received trace data bytes, throughput, and percentage of CPU load.
#. Observe that the sample completes with a message on the terminal.

Sample Output
=============

The sample shows the following output:

.. code-block:: console

   Custom trace backend initialized
   *** Booting Zephyr OS build v3.0.99-ncs1  ***
   Modem trace backend sample started
   Connecting to network
   LTE mode changed to 1
   Traces received:   9.2kB, 18.4kB/s, CPU-load:  6.48%
   Traces received:  14.4kB, 10.3kB/s, CPU-load:  3.65%
   Traces received:  36.7kB, 44.3kB/s, CPU-load:  6.36%
   Traces received:  51.6kB, 29.4kB/s, CPU-load:  5.72%
   Traces received:  65.9kB, 28.3kB/s, CPU-load:  5.00%
   Traces received:  67.5kB,  3.3kB/s, CPU-load:  2.34%
   Traces received:  68.6kB,  2.1kB/s, CPU-load:  2.10%
   Traces received:  70.0kB,  2.7kB/s, CPU-load:  2.12%
   Traces received:  71.5kB,  3.0kB/s, CPU-load:  2.23%
   Traces received:  74.2kB,  5.4kB/s, CPU-load:  2.60%
   Traces received:  81.8kB, 15.1kB/s, CPU-load:  3.38%
   Traces received:  82.2kB,  0.7kB/s, CPU-load:  1.64%
   Traces received:  83.9kB,  3.4kB/s, CPU-load:  2.37%
   LTE mode changed to 0
   Custom trace backend deinitialized
   Bye

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nrf_modem_lib_readme`
* :ref:`cpu_load`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`workqueues_v2`
* :ref:`float_v2`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
