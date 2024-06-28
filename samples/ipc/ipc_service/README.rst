.. _ipc_service_sample:

IPC service
###########

.. contents::
   :local:
   :depth: 2

The IPC service sample demonstrates the functionality of the IPC service.

Overview
********

The sample application tests throughput of the IPC service with available backends.
Currently, the sample supports the following backends:

* :ref:`zephyr:nrf5340dk_nrf5340` board:

  * `OpenAMP`_ library
  * :ref:`zephyr:ipc_service_backend_icmsg`

* :ref:`zephyr:nrf54h20dk_nrf54h20` board:

  * :ref:`zephyr:ipc_service_backend_icbmsg`

Each core periodically prints out data throughput in bytes per second.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Configuration
*************

|config|

Both application and network core send data to each other in the time interval specified by the :ref:`CONFIG_APP_IPC_SERVICE_SEND_INTERVAL <CONFIG_APP_IPC_SERVICE_SEND_INTERVAL>` option.
You can change the value and observe how the throughput on each core changes.

.. note::
   Increasing the time interval to send data on one core, decreases the reading speed on the other core.

Configuration options
=====================

Check and configure the following Kconfig options:

.. _CONFIG_APP_IPC_SERVICE_SEND_INTERVAL:

CONFIG_APP_IPC_SERVICE_SEND_INTERVAL - Time interval to send data through the IPC service
   The sample configuration defines the time interval to send data packages through IPC service in µs.
   Since the kernel timeout has a 1 ms resolution, this value is rounded off.
   If the value is lesser than 1000 µs, use :c:func:`k_busy_wait` instead of :c:func:`k_msleep` function.

Building and running
********************

.. |sample path| replace:: :file:`samples/ipc/ipc_service`

.. include:: /includes/build_and_run.txt

**nRF5340 DK**

You can build the sample using either the RPMsg or ICMSG backend.
For the default RPMsg backend, use the following command:

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp

For the ICMSG backend, use the following command:

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp -T sample.ipc.ipc_service.nrf5340dk_icmsg_default .

A set of overlays is available for the sample to verify the throughput when only one core is sending the data.
Use these overlays when building the IPC sample to test the following scenarios:

* Either the network or application core is sending data through the IPC service using RPMsg:

  .. code-block:: console

     west build -p -b nrf5340dk/nrf5340/cpuapp -T sample.ipc.ipc_service.nrf5340dk_rpmsg_cpuapp_sending .
     west build -p -b nrf5340dk/nrf5340/cpuapp -T sample.ipc.ipc_service.nrf5340dk_rpmsg_cpunet_sending .

* Either the network or application core is sending data through the IPC service using the :ref:`zephyr:ipc_service_backend_icmsg` backend:

  .. code-block:: console

     west build -p -b nrf5340dk/nrf5340/cpuapp -T sample.ipc.ipc_service.nrf5340dk_icmsg_cpuapp_sending .
     west build -p -b nrf5340dk/nrf5340/cpuapp -T sample.ipc.ipc_service.nrf5340dk_icmsg_cpunet_sending .

**nRF54H20 DK**

To build the sample to test IPC between the application and radio domains using the default :ref:`zephyr:ipc_service_backend_icbmsg` backend, use the following command:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp

To build the sample to test IPC between the application and PPR core using the :ref:`zephyr:ipc_service_backend_icmsg` backend, use the following command:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -T sample.ipc.ipc_service.nrf54h20dk_cpuapp_cpuppr_icmsg .

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

In the default configuration, both application and network cores periodically print out the receiving speed of data that was sent by the other core.

After programming the sample to your development kit, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe the console output for both cores:

  * For the application core, the output is similar to the following one:

    .. code-block:: console

       *** Booting Zephyr OS build v3.0.99-ncs1  ***
       IPC-service nrf5340dk/nrf5340/cpuapp demo started
       Δpkt: 9391 (100 B/pkt) | throughput: 7512800 bit/s
       Δpkt: 9389 (100 B/pkt) | throughput: 7511200 bit/s
       Δpkt: 9388 (100 B/pkt) | throughput: 7510400 bit/s
       Δpkt: 9390 (100 B/pkt) | throughput: 7512000 bit/s
       Δpkt: 9396 (100 B/pkt) | throughput: 7516800 bit/s

  * For the network core, the output is similar to the following one:

    .. code-block:: console

       *** Booting Zephyr OS build v3.0.99-ncs1  ***
       IPC-service nrf5340dk/nrf5340/cpunet demo started
       Δpkt: 6665 (100 B/pkt) | throughput: 5332000 bit/s
       Δpkt: 6664 (100 B/pkt) | throughput: 5331200 bit/s
       Δpkt: 6658 (100 B/pkt) | throughput: 5326400 bit/s
       Δpkt: 6665 (100 B/pkt) | throughput: 5332000 bit/s
       Δpkt: 6671 (100 B/pkt) | throughput: 5336800 bit/s

Dependencies
************

The sample uses the following Zephyr subsystems:

* ``include/ipc/ipc_service.h``
* :ref:`zephyr:logging_api`
