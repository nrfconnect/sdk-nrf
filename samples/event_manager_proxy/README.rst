.. _event_manager_proxy_sample:

Event Manager Proxy
###################

.. contents::
   :local:
   :depth: 2

The Event Manager Proxy sample demonstrates how to use :ref:`event_manager_proxy` to transport events between remote and host cores.
The sample also demonstrates the proposed application structure where common events declarations and definitions are available for both cores.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The proxy sample copies the functionality from :ref:`app_event_manager_sample` sample, but splits it between the two cores.
The remote core runs the simulated sensor module, while the host core takes care of configuration and calculates statistics.

The sample uses modules from :ref:`app_event_manager_sample` sample, which communicate using events:

.. include:: ../app_event_manager/README.rst
   :start-after: event_manager_sample_modules_start
   :end-before: event_manager_sample_modules_end

The only change compared with the Application Event Manager sample modules is that the Statistic module now also counts control messages.
This change allows to transfer different range of messages between cores.

File and directory layout
=========================

The application is divided into host and remote.
The host configuration is placed directly in the :file:`samples/event_manager_proxy` folder.
The remote configuration is placed in the :file:`samples/event_manager_proxy/remote` folder.
The remote configuration is added to the host as a subproject, which means that it builds as part of the host application build.

Both the remote and the host use common event declarations and definitions that are located in the :file:`samples/event_manager_proxy/common_events` folder, and modules that are located in the :file:`samples/event_manager_proxy/modules` folder.

Configuration
*************

|config|

Selecting ICMSG backend
=======================

On the nRF5340 DK, this sample uses by default the OpenAMP backend provided by the IPC Service.
You can instead select the ICMSG backend configuration, which has smaller memory requirements.

The ICMSG backend configuration is provided in the :file:`prj_icmsg.conf` file.
To provide the ICMSG backend configuration, specify the ``-DFILE_SUFFIX=icmsg`` parameter along with the build command when building the sample:

   .. code-block:: console

      west build -p -b nrf5340dk/nrf5340/cpuapp -- -DFILE_SUFFIX=icmsg

The nRF54H20 target supports only ICMSG backend.

Building and running
********************
.. |sample path| replace:: :file:`samples/event_manager_proxy`

.. include:: /includes/build_and_run.txt

Complete the following steps to program the sample:

1. Go to the sample directory.
#. Open the command line terminal.
#. Run the following command to build the application code for the host and the remote:


   **nRF5340 DK**

   .. code-block:: console

      west build -b nrf5340dk/nrf5340/cpuapp

   **nRF54H20 DK**

   A snippet for running the PPR core is added automatically to build configuration.

   .. code-block:: console

      west build -b nrf54h20dk/nrf54h20/cpuapp

   or use twister test case:

   .. code-block:: console

      west build -b nrf54h20dk/nrf54h20/cpuapp -T sample.event_manager_proxy.nrf54h20dk_cpuapp .

#. Program both the cores:

   .. code-block:: console

      west flash

.. include:: /includes/nRF54H20_erase_UICR.txt

Testing
=======

After programming the sample to your development kit for both cores, test it by performing the following steps:

1. |connect_terminal|
#. Reset the kit.
#. Observe that output logged on two UART serial terminals.
   One for the host and the other for the remote core.

   * On the host core, we expect the following messages:

     .. code-block:: console

        *** Booting Zephyr OS build v2.7.99-ncs1-17-gc3208e7ff49d  ***
        Event Manager Proxy demo started
        [00:00:00.284,881] <inf> event_manager: e:config_event init_val_1=3
        [00:00:00.285,430] <inf> event_manager: e:measurement_event val1=3 val2=3 val3=3
        [00:00:00.785,675] <inf> event_manager: e:measurement_event val1=3 val2=6 val3=9
        [00:00:01.285,949] <inf> event_manager: e:measurement_event val1=3 val2=9 val3=18
        [00:00:01.786,254] <inf> event_manager: e:measurement_event val1=3 val2=12 val3=30
        [00:00:02.286,560] <inf> event_manager: e:measurement_event val1=3 val2=15 val3=45
        [00:00:02.286,682] <inf> event_manager: e: control_event
        [00:00:02.286,682] <inf> stats: Control event count: 1
        [00:00:02.787,017] <inf> event_manager: e:measurement_event val1=-3 val2=12 val3=57
        [00:00:03.287,322] <inf> event_manager: e:measurement_event val1=-3 val2=9 val3=66
        [00:00:03.787,597] <inf> event_manager: e:measurement_event val1=-3 val2=6 val3=72
        [00:00:04.287,872] <inf> event_manager: e:measurement_event val1=-3 val2=3 val3=75
        [00:00:04.788,177] <inf> event_manager: e:measurement_event val1=-3 val2=0 val3=75
        [00:00:04.788,208] <inf> stats: Average value3: 45
        [00:00:05.288,452] <inf> event_manager: e:measurement_event val1=-3 val2=-3 val3=72
        [00:00:05.788,726] <inf> event_manager: e:measurement_event val1=-3 val2=-6 val3=66
        [00:00:06.289,031] <inf> event_manager: e:measurement_event val1=-3 val2=-9 val3=57
        [00:00:06.789,306] <inf> event_manager: e:measurement_event val1=-3 val2=-12 val3=45
        [00:00:07.289,611] <inf> event_manager: e:measurement_event val1=-3 val2=-15 val3=30
        [00:00:07.289,733] <inf> event_manager: e: control_event
        [00:00:07.289,733] <inf> stats: Control event count: 2

     The host starts the communication by sending a ``config_event``, which is also received and processed by the remote core.
   * On the remote core following messages are expected:

     .. code-block:: console

        *** Booting Zephyr OS build v2.7.99-ncs1-17-gc3208e7ff49d  ***
        Event Manager Proxy remote_core started
        [00:00:00.010,864] <inf> event_manager: e:config_event init_val_1=3
        [00:00:00.011,047] <inf> event_manager: e:measurement_event val1=3 val2=3 val3=3
        [00:00:00.511,322] <inf> event_manager: e:measurement_event val1=3 val2=6 val3=9
        [00:00:01.011,566] <inf> event_manager: e:measurement_event val1=3 val2=9 val3=18
        [00:00:01.511,871] <inf> event_manager: e:measurement_event val1=3 val2=12 val3=30
        [00:00:02.012,176] <inf> event_manager: e:measurement_event val1=3 val2=15 val3=45
        [00:00:02.012,298] <inf> event_manager: e: control_event
        [00:00:02.012,451] <inf> event_manager: e: ack_event
        [00:00:02.512,634] <inf> event_manager: e:measurement_event val1=-3 val2=12 val3=57
        [00:00:03.012,939] <inf> event_manager: e:measurement_event val1=-3 val2=9 val3=66
        [00:00:03.513,244] <inf> event_manager: e:measurement_event val1=-3 val2=6 val3=72
        [00:00:04.013,488] <inf> event_manager: e:measurement_event val1=-3 val2=3 val3=75
        [00:00:04.513,793] <inf> event_manager: e:measurement_event val1=-3 val2=0 val3=75
        [00:00:05.014,099] <inf> event_manager: e:measurement_event val1=-3 val2=-3 val3=72
        [00:00:05.514,343] <inf> event_manager: e:measurement_event val1=-3 val2=-6 val3=66
        [00:00:06.014,648] <inf> event_manager: e:measurement_event val1=-3 val2=-9 val3=57
        [00:00:06.514,953] <inf> event_manager: e:measurement_event val1=-3 val2=-12 val3=45
        [00:00:07.015,197] <inf> event_manager: e:measurement_event val1=-3 val2=-15 val3=30
        [00:00:07.015,350] <inf> event_manager: e: control_event
        [00:00:07.015,502] <inf> event_manager: e: ack_event

Now all the measurement and control events are generated on the remote core and passed to the host core, where the statistics are generated.
On the remote core, we can also see an :c:type:`ack_event`, which is not passed to the host core.

Dependencies
************

This sample uses the following |NCS| subsystems:

* :ref:`app_event_manager`

In addition, it uses the following Zephyr subsystems:

* :file:`include/ipc/ipc_service.h`
* :ref:`zephyr:logging_api`
