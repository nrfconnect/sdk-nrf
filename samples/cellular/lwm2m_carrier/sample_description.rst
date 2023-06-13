.. _lwm2m_carrier_sample_desc:

Sample description
##################

.. contents::
   :local:
   :depth: 2

The LwM2M carrier sample demonstrates how to run the :ref:`liblwm2m_carrier_readme` library in an application in order to connect to the operator LwM2M device management.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

LwM2M is an application layer protocol for IoT device management and service enablement.
It is designed to expose various resources for reading, writing, and executing through an LwM2M server in a lightweight environment.

The LwM2M carrier library is needed for certification in certain operator networks.
The LwM2M carrier sample shows how to integrate the LwM2M carrier library.
This sample is primarily meant to be run in the :ref:`applicable networks <lwm2m_certification>` where the certification applies.
It will automatically connect to the correct device management servers, depending on which network operator is detected.

Some of the configurations of the library must be changed according to your specific operator requirements.
For example, at some point during certification, you might have to connect to one or more of an operator's test (certification) servers, by overwriting the library's automatic URI and PSK selection.
When :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` is empty, the library connects to live (production) servers.

The sections below explain how you can configure the library in different ways to connect to Leshan and AVSystem's Coiote LwM2M servers.
To know more about the AVSystem integration with |NCS|, see :ref:`ug_avsystem`.
Configuring your application to connect to other servers (such as your operator's test servers) might look different, depending on the operator's device management framework.

Configuration
*************

|config|

Setup
=====

Before building and running the sample, complete the following steps:

1. Select the device that you plan to test.
#. Select the LwM2M server for testing.
#. Setup the LwM2M server by completing the steps listed in :ref:`server_setup_lwm2m_carrier`.
   This step retrieves the server address and the security tag that will be needed during the next steps.
#. :ref:`server_addr_PSK_carrier`.

.. |dtls_support| replace:: The same credentials must be provided in the :guilabel:`Leshan Demo Server Security configuration` page.

.. _server_setup_lwm2m_carrier:

.. include:: /includes/lwm2m_common_server_setup.txt

.. _server_addr_PSK_carrier:

Set the server address and PSK
------------------------------

a. Open :file:`prj.conf` (see :ref:`lwm2m_carrier_config_files` for more information).
#. Set :kconfig:option:`CONFIG_LWM2M_CARRIER_CUSTOM_URI` to the correct server URL:

   * For `Leshan Demo Server`_ - ``coaps://leshan.eclipseprojects.io:5684`` (`public Leshan Demo Server`_).
   * For `Coiote Device Management`_ - ``coaps://eu.iot.avsystem.cloud:5684`` (`Coiote Device Management server`_).
   * For `Leshan Bootstrap Server Demo web UI <public Leshan Bootstrap Server Demo_>`_ - ``coaps://leshan.eclipseprojects.io:5784``
   * For Coiote bootstrap server - ``coaps://eu.iot.avsystem.cloud:5694``
#. Set :kconfig:option:`CONFIG_LWM2M_CARRIER_IS_BOOTSTRAP_SERVER` if bootstrap is used. If bootstrap is not used, set :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_LIFETIME` to specify the lifetime of the LwM2M server.
#. Set :ref:`CONFIG_CARRIER_APP_PSK <CONFIG_CARRIER_APP_PSK>` to the hexadecimal representation of the PSK used when registering the device with the server.
#. Specify a :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG` to store the PSK.
   Alternatively, you could only specify a security tag if a PSK is previously stored to the security tag as shown in :ref:`LwM2M client provisioning documentation <lwm2m_client_provisioning>`.

Configuration options
=====================

Check and configure the following configuration options for the sample:

Server options
--------------

.. _CONFIG_CARRIER_APP_PSK:

CONFIG_CARRIER_APP_PSK - Configuration for Pre-Shared Key
   The sample configuration is used to set the hexadecimal representation of the PSK used when registering the device with the server.
   The PSK is stored in the security tag specified in :kconfig:option:`CONFIG_LWM2M_CARRIER_SERVER_SEC_TAG`.

.. _lwm2m_carrier_config_files:

Configuration files
===================

The sample provides predefined configuration files for typical use cases.

The following files are available:

* :file:`prj.conf` - Standard default configuration file.
* :file:`overlay-shell.conf` - Enables the :ref:`lwm2m_carrier_shell` and :ref:`lib_at_shell`.

The sample can either be configured by editing the :file:`prj.conf` file and the relevant overlay files, or through menuconfig or guiconfig.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/lwm2m_carrier`

.. include:: /includes/build_and_run_ns.txt

.. _lwm2m_carrier_shell_overlay:

Building with overlay
=====================

To build with a Kconfig overlay, pass it to the build system using the ``OVERLAY_CONFIG`` CMake variable, as shown in the following example:

.. code-block:: console

   west build -b nrf9160dk_nrf9160_ns -- -DOVERLAY_CONFIG=overlay-shell.conf

This command builds for the nRF9160 DK using the configurations found in the :file:`overlay-shell.conf` file, in addition to the configurations found in the :file:`prj.conf` file.
If some options are defined in both files, the options set in the overlay take precedence.

Testing
=======

After programming the sample and all prerequisites to the development kit, test it by performing the following steps:

1. Connect the USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the kit prints the following information::

        LWM2M Carrier library sample.
#. Observe that the application receives events from the :ref:`liblwm2m_carrier_readme` library using the registered event handler. If the client and server configuration is correct, the initial output looks similar to the following output:

   .. code-block:: console

      LWM2M_CARRIER_EVENT_LTE_LINK_DOWN
      LWM2M_CARRIER_EVENT_LTE_LINK_UP
      LWM2M_CARRIER_EVENT_LTE_LINK_DOWN
      LWM2M_CARRIER_EVENT_BOOTSTRAPPED
      LWM2M_CARRIER_EVENT_LTE_LINK_UP
      LWM2M_CARRIER_EVENT_REGISTERED

Once bootstrap has been done, subsequent reconnects will not contain the bootstrap sequence. The output looks similar to the following output:

.. code-block:: console

   LWM2M Carrier library sample.
   LWM2M_CARRIER_EVENT_REGISTERED

The device is now registered to an LwM2M server, and the server can interact with it.
If you used your own custom server as described in :ref:`server_setup_lwm2m_carrier`, you can try reading and observing the available resources.

If you connected to the carrier (test) servers or live (production) servers, reach out to your mobile network operator to learn about how to proceed with certification tests or normal operation, respectively.

Testing with the LwM2M shell
----------------------------

See :ref:`lwm2m_carrier_shell` for more information about the LwM2M carrier shell and shell commands.
To test with the sample, complete the following steps:

1. Make sure the sample was built with the shell overlay as described in :ref:`lwm2m_carrier_shell_overlay`.
#. Open a terminal emulator and observe that the development kit produces an output similar to the above section.
#. Verify that the shell inputs are working correctly by running few commands. Example commands can be to set the Server URI and PSK to match the description in :ref:`server_setup_lwm2m_carrier`.

   a. Store a PSK:

      .. code-block:: console

         $ at AT+CFUN=4
         OK
         $ at AT%CMNG=0,450,3,\"000102030405060708090a0b0c0d0e0f\"
         OK
         $ at AT+CFUN=1
         OK

   #. Set the URI and security tag (containing the PSK that was stored):

      .. code-block:: console

         $ carrier_config server uri coaps://leshan.eclipseprojects.io:5784
         Set server URI: coaps://leshan.eclipseprojects.io:5784
         $ carrier_config server sec_tag 450
         Set security tag: 450

   #. Apply the server config.
      After rebooting, the sample loads these settings (instead of using the static Kconfigs).

      .. code-block:: console

         $ carrier_config server enable
         Enabled custom server config

   #. Finally, as described in ref:`lwm2m_carrier_shell`, set ``auto_startup`` (or else the sample will wait indefinitely for you to configure all the settings).

      .. code-block:: console

         $ carrier_config auto_startup y
         Set auto startup: Yes

   Once the device is registered to a device management server, you can experiment by setting some of the resources.
   For example, you can set the Location object (``/6``) resources for Latitude (``/6/0/0``) and Longitude (``/6/0/1``):

   .. code-block:: console

      $ carrier_api location position 63.43049 10.39506

Troubleshooting
===============

Bootstrapping can take several minutes.
This is expected and dependent on the availability of the LTE link.
During bootstrap, the application will receive the :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_UP` and :c:macro:`LWM2M_CARRIER_EVENT_LTE_LINK_DOWN` events.
This is expected and is part of the bootstrapping procedure.

You might encounter deferred events, as shown in the following output:

.. code-block:: console

   LWM2M Carrier library sample.
   LWM2M_CARRIER_EVENT_LTE_LINK_DOWN
   LWM2M_CARRIER_EVENT_LTE_LINK_UP
   LWM2M_CARRIER_EVENT_DEFERRED
   Reason: Failed to connect to bootstrap server, timeout: 60 seconds
   LWM2M_CARRIER_EVENT_DEFERRED
   ...

There are many scenarios which can lead to a deferred event. Following are some of the common scenarios:

* A mismatch in the provisioning of the device (wrong PSK).
* A DTLS session is already in use.
* Temporary networking issues when connecting to the bootstrap or device management servers.

For more information, see the :ref:`lwm2m_events` and :ref:`lwm2m_msc` sections in the LwM2M carrier library documentation.

To completely restart and trigger a new bootstrap, the device must be erased and re-programmed, as mentioned in :ref:`lwm2m_app_int`.

Dependencies
************

This sample uses the following |NCS| libraries:

* |NCS| modules abstracted by the LwM2M carrier OS abstraction layer (:file:`lwm2m_os.h`)

  .. include:: /libraries/bin/lwm2m_carrier/app_integration.rst
    :start-after: lwm2m_osal_mod_list_start
    :end-before: lwm2m_osal_mod_list_end

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
