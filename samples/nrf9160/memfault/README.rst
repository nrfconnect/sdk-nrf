.. _memfault_sample:

nRF9160: Memfault
#################

.. contents::
   :local:
   :depth: 2

The Memfault sample shows how to use the `Memfault SDK`_ in an |NCS| application to collect coredumps and metrics.
The sample connects to an LTE network and sends the collected data to Memfault's cloud using HTTPS.


Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns, nrf9160dk_nrf9160ns

Before using the Memfault platform, you must register an account in `Memfault`_ and a set up a project according to the instructions in `Memfault documentation <Memfault Docs_>`_.

.. include:: /includes/spm.txt

To get access to all the benefits, like up to 100 free devices connected, register at the `Memfault registration page`_.

Overview
********

In this sample, Memfault SDK is used as a module in |NCS| to collect coredumps, reboot reasons, metrics and trace events from devices and send to the Memfault cloud.
See `Memfault terminology`_ for more details on the various Memfault concepts.


Metrics
=======

The sample adds properties specific to the application, while the :ref:`Memfault SDK integration layer <mod_memfault>` in |NCS| adds the system property metrics.
See `Memfault: Collecting Device Metrics`_ for details on working and implementation of metrics.
Some metrics are collected by the Memfault SDK directly.
There are also some metrics, which are specific to |NCS| that are enabled by default:

* LTE metrics:

  * Enabled and disabled using :option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`.
  * ``Ncs_LteTimeToConnect`` - Time from the point when the device starts to search for an LTE network until the time when it gets registered with the network.
  *  ``Ncs_LteConnectionLossCount`` - The number of times that the device has lost the LTE network connection after the initial network registration.

* Stack usage metrics:

   * Shows how many bytes of unused space is left in a stack.
   * Configurable using :option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`.
   * ``Ncs_AtCmdUnusedStack`` - The :ref:`at_cmd_readme` library's stack.
   * ``Ncs_ConnectionPollUnusedStack``- Stack used by the cloud libraries for :ref:`lib_nrf_cloud`, :ref:`lib_aws_iot` and :ref:`lib_azure_iot_hub`.

In addition to showing the capturing of metrics provided by the Memfault SDK integration layer in |NCS|, the sample also shows how to capture an application-specific metric.
This metric is defined in :file:`samples/nrf9160/memfault/config/memfault_metrics_heartbeat_config.h`:

*  ``Switch1ToggleCount`` - The number of times **Switch 1** has been toggled on an nRF9160 DK.


Error Tracking with trace events
================================

The sample implements a user-defined trace reason for demonstration purposes.
The trace reason is called ``Switch2Toggled``, and is collected every time **Switch 2** is toggled on an nRF9160 DK.
In addition to detection of the event, the trace includes the current switch state.
See `Memfault: Error Tracking with Trace Events`_ for information on how to configure and use trace events.


Coredumps
=========

Coredumps can be triggered either by using the Memfault shell command ``mflt crash``, or by pressing a button:

*  **Button 1** triggers a stack overflow
*  **Button 2** triggers a NULL pointer dereference

These faults cause crashes that are captured by Memfault.
After rebooting, the crash data can be sent to the Memfault cloud for further inspection and analysis.
See `Memfault documentation <Memfault Docs_>`_ for more information on the debugging possibilities offered by the Memfault platform.

The sample enables Memfault shell by default.
The shell offers multiple test commands to test a wide range of functionality offered by Memfault SDK.
Run the command ``mflt help`` in the terminal for more information on the available commands.


Configuration
*************

The Memfault SDK allows the configuration of some of its options using Kconfig.
To configure the options in the SDK that are not available for configuration using Kconfig, use :file:`samples/nrf9160/memfault/config/memfault_platform_config.h`.
See `Memfault SDK`_ for more information.

|config|

Minimal setup
=============

To send data to the Memfault cloud, a project key must be configured using :option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`.

.. note::
   The Memfault SDK requires certificates required for the HTTPS transport.
   The certificates are by default provisioned automatically by the |NCS| integration layer for Memfault SDK to sec tags 1001 - 1005.
   If other certificates exist at these sec tags, HTTPS uploads will fail.


Additional configuration
========================

There are two sources for Kconfig options when using Memfault SDK in |NCS|:

* Kconfig options defined within the Memfault SDK.
* Kconfig options defined in the |NCS| integration layer of the Memfault SDK. These configuration options are prefixed with ``CONFIG_MEMFAULT_NCS``.

Check and configure the following options in Memfault SDK that are used by the sample:

* :option:`CONFIG_MEMFAULT`
* :option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM`
* :option:`CONFIG_MEMFAULT_SHELL`
* :option:`CONFIG_MEMFAULT_HTTP_ENABLE`
* :option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD`
* :option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`
* :option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE`
* :option:`CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS`

If :option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM` is enabled, TLS certificates used for HTTP uploads are provisioned to the nRF9160 modem when :c:func:`memfault_zephyr_port_install_root_certs` is called.

Check and configure the following options for Memfault that are specific to |NCS|:

* :option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`
* :option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`
* :option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`
* :option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`

If :option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP` is enabled, :option:`CONFIG_PM_PARTITION_SIZE_MEMFAULT_STORAGE` can be used to set the flash partition size for the flash storage.


Configuration files
===================

.. include:: ../../../doc/nrf/libraries/others/memfault_ncs.rst
   :start-after: memfault_config_files_start
   :end-before: memfault_config_files_end

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/memfault`
.. include:: /includes/build_and_run.txt
.. include:: /includes/spm.txt

Testing
=======

Before testing, ensure that your device is configured with your Memfault project's project key.
|test_sample|

1. |connect_terminal|
#. Observe that the sample starts.
   Following is a sample output on the terminal:

   .. code-block:: console

        *** Booting Zephyr OS build v2.4.99-ncs1-4934-g6d2b8c7b17aa  ***
        <inf> <mflt>: Reset Reason, RESETREAS=0x0
        <inf> <mflt>: Reset Causes:
        <inf> <mflt>:  Power on Reset
        <inf> <mflt>: GNU Build ID: a09094cdf9da13f20719f87016663ab529b71267
        <inf> memfault_sample: Memfault sample has started

#. The sample connects to an available LTE network, which is indicated by the following message:

   .. code-block:: console

        <inf> memfault_sample: Connecting to LTE network, this may take several minutes...

#. When the connection is established, the sample displays the captured LTE time-to-connect metric (`Ncs_LteTimeToConnect``) on the terminal:

   .. code-block:: console

        <inf> memfault_sample: Connected to LTE network. Time to connect: 3602 ms

#. Subsequently, all captured Memfault data will be sent to the Memfault cloud:

   .. code-block:: console

        <inf> memfault_sample: Sending already captured data to Memfault
        <dbg> <mflt>: Response Complete: Parse Status 0 HTTP Status 202!
        <dbg> <mflt>: Body: Accepted
        <dbg> <mflt>: Response Complete: Parse Status 0 HTTP Status 202!
        <dbg> <mflt>: Body: Accepted
        <dbg> <mflt>: No more data to send

#. Press <TAB> on your keyboard to confirm that the Memfault shell is working.
   The available shell commands are displayed:

   .. code-block:: console

        uart:~$
          clear              device             flash              help
          history            kernel             log                mcuboot
          mflt               mflt_nrf           nrf_clock_control  resize
          shell

#. Explore the available Memfault shell commands by issuing the command ``mflt help``.

#. Press **Button 1** or **Button 2** to trigger a stack overflow or a NULL pointer dereference, respectively.

#. In a web browser, navigate to `Memfault`_, sign in and follow the instructions in `Memfault documentation <Memfault Docs_>`_ to view the uploaded data.
   Note that the symbol file for the sample must be uploaded for Memfault to parse the information.
   The symbol file is located at the following path: ``<sample folder>/build/zephyr/zephyr.elf``.


Dependencies
************

The sample requires the Memfault SDK, which is part of |NCS|'s West manifest, and will be downloaded automatically when ``west update`` is run.

This sample uses the following |NCS| libraries and drivers:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`
