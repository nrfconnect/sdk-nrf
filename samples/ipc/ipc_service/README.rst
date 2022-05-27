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
Currently, the sample supports OpenAMP RPMSG and ICMSG backends.

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

A set of overlays are available for the sample to verify the throughput that only one core is sending the data.
You could use different overlay build commands for different testing scenarios, which are as follows:

* To test the application where only the application core is sending data through the IPC service, specify ``-DOVERLAY_CONFIG=overlay-cpuapp-sending.conf`` overlay parameter with the build command:

  .. code-block:: console

     west build -p -b nrf5340dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-cpuapp-sending.conf

  The :file:`CMakeLists.txt` of the application ensures adding a matching config overlay for the child image.

* To test the application for a scenario where only the network core is sending data through the IPC service, specify the ``-DOVERLAY_CONFIG=overlay-cpunet-sending.conf`` overlay parameter with the build command:

  .. code-block:: console

     west build -p -b nrf5340dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-cpunet-sending.conf

  The :file:`CMakeLists.txt` of the application ensures adding a matching config overlay for the child image.

* To test the application with the ICMSG backend, specify the ``-DCONF_FILE=prj_icmsg.conf`` parameter along with the build command:

  .. code-block:: console

     west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONF_FILE=prj_icmsg.conf

  The :file:`CMakeLists.txt` of the application ensures adding a matching ``config`` and ``DT`` overlay for the child image.

* Combine the above options and test maximal core to core throughput with the ICMSG backend.
  To do so, build the application with the following commands:

  .. code-block:: console

     west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONF_FILE=prj_icmsg.conf -DOVERLAY_CONFIG=overlay-cpuapp-sending.conf
     west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONF_FILE=prj_icmsg.conf -DOVERLAY_CONFIG=overlay-cpunet-sending.conf

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
       IPC-service nrf5340dk_nrf5340_cpuapp demo started
       Δpkt: 9391 (100 B/pkt) | throughput: 7512800 bit/s
       Δpkt: 9389 (100 B/pkt) | throughput: 7511200 bit/s
       Δpkt: 9388 (100 B/pkt) | throughput: 7510400 bit/s
       Δpkt: 9390 (100 B/pkt) | throughput: 7512000 bit/s
       Δpkt: 9396 (100 B/pkt) | throughput: 7516800 bit/s

  * For the network core, the output is similar to the following one:

    .. code-block:: console

       *** Booting Zephyr OS build v3.0.99-ncs1  ***
       IPC-service nrf5340dk_nrf5340_cpunet demo started
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
