.. _memfault_sample:

Memfault
########

.. contents::
   :local:
   :depth: 2

The Memfault sample shows how to use the `Memfault SDK`_ in an |NCS| application to collect coredumps and metrics.
The sample connects to an LTE network and sends the collected data to Memfault's cloud using HTTPS.

To get started with Memfault integration in |NCS|, see :ref:`ug_memfault`.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

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

  * Enabled and disabled using :kconfig:option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`.
  * ``Ncs_LteTimeToConnect`` - Time from the point when the device starts to search for an LTE network until the time when it gets registered with the network.
  *  ``Ncs_LteConnectionLossCount`` - The number of times that the device has lost the LTE network connection after the initial network registration.

* Stack usage metrics:

  * Shows how many bytes of unused space is left in a stack.
  * Configurable using :kconfig:option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`.
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

The sample enables Memfault shell by default.
The shell offers multiple commands to test a wide range of functionality offered by the Memfault SDK.
Run the command ``mflt help`` in the terminal for more information on the available commands.

Coredumps can be triggered in the following ways:

*  Press **Button 1** to trigger stack overflow
*  Press **Button 2** to trigger division by zero
*  Use the Memfault shell to trigger a specific fault.
   Run the command ``mflt test help`` to see the possible options.

These faults cause crashes that are captured by Memfault.
After rebooting, the crash data can be sent to the Memfault cloud for further inspection and analysis.
See `Memfault documentation <Memfault Docs_>`_ for more information on the debugging possibilities offered by the Memfault platform.

Configuration
*************

The Memfault SDK allows the configuration of some of its options using Kconfig.
To configure the options in the SDK that are not available for configuration using Kconfig, use :file:`samples/nrf9160/memfault/config/memfault_platform_config.h`.
See `Memfault SDK`_ for more information.

|config|

Minimal setup
=============

To send data to the Memfault cloud, a project key must be configured using :kconfig:option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`.
If you are using the nRF7002 DK and want to automatically connect to a Wi-Fi network upon boot, configure the Wi-Fi credentials using the Kconfig options :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_SSID` and :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD`.

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

* :kconfig:option:`CONFIG_MEMFAULT`
* :kconfig:option:`CONFIG_MEMFAULT_LOGGING_ENABLE`
* :kconfig:option:`CONFIG_MEMFAULT_LOG_LEVEL_DBG`
* :kconfig:option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM`
* :kconfig:option:`CONFIG_MEMFAULT_SHELL`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_ENABLE`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE`
* :kconfig:option:`CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS`

If :kconfig:option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM` is enabled, TLS certificates used for HTTP uploads are provisioned to the nRF9160 modem when :c:func:`memfault_zephyr_port_install_root_certs` is called.

Check and configure the following options for Memfault that are specific to |NCS|:

* :kconfig:option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`

If :kconfig:option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP` is enabled, :kconfig:option:`CONFIG_PM_PARTITION_SIZE_MEMFAULT_STORAGE` can be used to set the flash partition size for the flash storage.


Configuration files
===================

.. include:: ../../../doc/nrf/libraries/others/memfault_ncs.rst
   :start-after: memfault_config_files_start
   :end-before: memfault_config_files_end

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/memfault`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

Before testing, ensure that your device is configured with the project key of your Memfault project.
|test_sample|

1. |connect_terminal_ANSI|
#. Observe that the sample starts.
   Following is a sample output on the terminal:

   .. code-block:: console

        *** Booting Zephyr OS build v3.0.99-ncs1-4913-gf7b06162027d  ***
        <inf> mflt: Reset Reason, RESETREAS=0x0
        <inf> mflt: Reset Causes:
        <inf> mflt: Power on Reset
        <inf> mflt: GNU Build ID: a09094cdf9da13f20719f87016663ab529b71267
        <inf> memfault_sample: Memfault sample has started

#. The sample connects to an available LTE or Wi-Fi network, which is indicated by the following message:

   .. code-block:: console

        <inf> memfault_sample: Connecting to network

#. When the connection is established, the following message is shown:

   .. code-block:: console

        <inf> memfault_sample: Connected to network.

#. If LTE is used, the sample displays the captured LTE time-to-connect metric (``Ncs_LteTimeToConnect``) in the terminal:

   .. code-block:: console

        <inf> memfault_sample: Time to connect: 3602 ms

#. Subsequently, all captured Memfault data will be sent to the Memfault cloud:

   .. code-block:: console

        <inf> memfault_sample: Sending already captured data to Memfault
        <dbg> mflt: memfault_platform_log: Response Complete: Parse Status 0 HTTP Status 202!
        <dbg> mflt: memfault_platform_log: Body: Accepted
        <dbg> mflt: memfault_platform_log: Response Complete: Parse Status 0 HTTP Status 202!
        <dbg> mflt: memfault_platform_log: Body: Accepted
        <dbg> mflt: memfault_platform_log: No more data to send

#. Upload the symbol file generated from your build to your Memfault account so that the information from your application can be parsed. The symbol file is located in the build folder: :file:`memfault/build/zephyr/zephyr.elf`:

   a. In a web browser, navigate to `Memfault`_.
   b. Login to your account and select the project you created earlier.
   c. Navigate to :guilabel:`Fleet` > :guilabel:`Devices` in the left menu.

      You can see your newly connected device and the software version in the list.

   d. Click on the software version number for your device and then the :guilabel:`Upload` button to upload the symbol file.

#. Back in the terminal, press <TAB> on your keyboard to confirm that the Memfault shell is working.
   The available shell commands are displayed:

   .. code-block:: console

        uart:~$
          clear              device             flash              help
          history            kernel             log                mcuboot
          mflt               mflt_nrf           nrf_clock_control  resize
          shell

#. Learn about the available Memfault shell commands by issuing the command ``mflt help``.
#. Press **Button 1** or **Button 2** to trigger a stack overflow or a division by zero, respectively.
#. Explore the Memfault user interface to look at the errors and metrics that has been sent from your device.


Dependencies
************

The sample requires the Memfault SDK, which is part of |NCS|'s West manifest, and will be downloaded automatically when ``west update`` is run.

This sample uses the following |NCS| libraries and drivers:

* :ref:`dk_buttons_and_leds_readme`
* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ libraries:

* :ref:`nrfxlib:nrf_modem`
* :ref:`nrfxlib:nrf_security`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`

For Wi-Fi, the sample also uses modules found in the following locations in the |NCS| folder structure:

* :file:`modules/lib/hostap`
* :file:`modules/mbedtls`
