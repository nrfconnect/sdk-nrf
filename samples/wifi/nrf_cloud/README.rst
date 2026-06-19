.. _wifi_nrf_cloud:

Wi-Fi: nRF Cloud
################

.. contents::
   :local:
   :depth: 2

The nRF Cloud sample demonstrates an nRF Cloud application running on nRF70 Series and nRF71 Series hardware, using Wi-Fi® as the transport.
It integrates Wi-Fi-based location services, periodic sensor sampling, and more.
It also demonstrates how to build connected, error-tolerant applications using Zephyr's ``conn_mgr`` connectivity management framework.

.. _wifi_nrf_cloud_requirements:

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. note::
   The :zephyr:board:`nRF7120 DK <nrf7120dk>` has not been tested on target for this sample.

.. include:: /includes/tfm.txt

.. _wifi_nrf_cloud_features:

Features
********

This sample implements or demonstrates the following features:

* Error-tolerant use of the nRF Cloud CoAP API using the :ref:`lib_nrf_cloud_coap` library.
* Error-tolerant use of the `nRF Cloud MQTT API`_ using the :ref:`lib_nrf_cloud` library.
* Periodic Wi-Fi location tracking by scanning nearby access points and submitting them to nRF Cloud, using the :ref:`lib_location` library.
* Fake temperature measurements.
* Transmission of sensor samples to the nRF Cloud portal as `nRF Cloud Device Messages`_.
* Construction of valid `nRF Cloud Device Messages`_.
* Minimal LED status indication using the `Zephyr LED API`_.
* Definition of a user config Trace Event (``temperature_alert``).
* Transmission of an Alert Trace Event whenever a specified temperature limit is exceeded.

.. _wifi_nrf_cloud_structure_and_theory_of_operation:

Structure and theory of operation
*********************************

This sample is separated into a number of smaller functional units.
The top level functional unit and entry point for the sample is the :file:`src/main.c` file.
This file starts three primary threads, each with a distinct function:

* The cloud connection thread (``con_thread``, :file:`src/cloud_connection.c`) runs the :ref:`wifi_nrf_cloud_cloud_connection_loop`, which maintains a connection to `nRF Cloud`_.
* The application thread (``app_thread``, :file:`src/application.c`) runs the :ref:`wifi_nrf_cloud_application_thread_and_main_application_loop`, which controls demo features and submits `device messages <nRF Cloud Device Messages_>`_ to the :ref:`wifi_nrf_cloud_device_message_queue`.
* The message queue thread (``msg_thread``, :file:`src/message_queue.c`) then transmits these messages whenever there is an active connection.
  See :ref:`wifi_nrf_cloud_device_message_queue`.

:file:`src/main.c` also optionally starts a fourth thread, the ``led_thread``, which animates any onboard LEDs if :ref:`wifi_nrf_cloud_led_status_indication` is enabled.

When using CoAP, one additional thread starts:
The CoAP shadow delta checking thread (``coap_shadow``, :file:`src/shadow_support_coap.c`) runs the :ref:`wifi_nrf_cloud_coap_shadow_loop` which periodically asks nRF Cloud for any shadow changes.

.. _wifi_nrf_cloud_cloud_connection_loop:

Cloud connection loop
=====================

The cloud connection loop (implemented in :file:`src/cloud_connection.c`) monitors network availability.
It starts a connection with `nRF Cloud`_ whenever the Internet becomes reachable, and closes that connection whenever Internet access is lost.
It has error handling and timeout features to ensure that failed or lost connections are re-established after a waiting period (:option:`CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS`).

Whenever a connection to nRF Cloud is started, the cloud connection loop follows the :ref:`nRF Cloud connection process <lib_nrf_cloud_connect>`.
The :ref:`lib_nrf_cloud` library handles most of the connection process, with exception to the following behavior:

When an MQTT device is first being associated with an nRF Cloud account, the `nRF Cloud MQTT API`_ will send a user association request notification to the device.
Upon receiving this notification, the device must wait for a user association success notification, and then manually disconnect from and reconnect to nRF Cloud.
Notifications of user association success are sent for every subsequent connection after this as well, so the device must only disconnect and reconnect if it previously received a user association request notification.
This behavior is handled by the ``NRF_CLOUD_EVT_USER_ASSOCIATION_REQUEST`` and ``NRF_CLOUD_EVT_USER_ASSOCIATED`` cases inside the :c:func:`cloud_event_handler` function.

When a CoAP device attempts to connect to nRF Cloud, the connection will fail to be authenticated until the device is onboarded to an nRF Cloud account.
See :ref:`wifi_nrf_cloud_standard_onboarding` for details.

Upon startup, the cloud connection loop also updates the `device shadow <nRF Cloud Device Shadows_>`_.
This is performed in the :c:func:`update_shadow` function.

.. _wifi_nrf_cloud_device_message_queue:

Device message queue
====================

Any thread may submit `device messages <nRF Cloud Device Messages_>`_ to the device message queue (implemented in :file:`src/message_queue.c`), where they are stored until a working connection to `nRF Cloud`_ is established.
Once this happens, the message thread transmits all enqueued messages, one at a time, to nRF Cloud.
If an enqueued message fails to send, it is re-queued and retried later.
Transmission is paused whenever the connection to nRF Cloud is lost.
If more than :option:`CONFIG_MAX_CONSECUTIVE_SEND_FAILURES` messages in a row fail to send, the connection is reset and re-established after a delay (:option:`CONFIG_CLOUD_CONNECTION_RETRY_TIMEOUT_SECONDS`).

The following are sent directly rather than through the queue:

* `Device shadow <nRF Cloud Device Shadows_>`_ updates.
* Ground fix requests from the :ref:`lib_location` library.

.. _wifi_nrf_cloud_application_thread_and_main_application_loop:

Application thread and main application loop
============================================

The application thread is implemented in the :file:`src/application.c` file, and is responsible for the high-level behavior of this sample.

When it starts, it logs the reset reason code.

It performs the following major tasks:

* Establishes periodic position tracking (which the :ref:`lib_location` library performs).
* Periodically samples temperature data (using the :file:`src/temperature.c` file).
* Constructs timestamped sensor sample and location `device messages <nRF Cloud Device Messages_>`_.
* Sends sensor sample and location device messages to the :ref:`wifi_nrf_cloud_device_message_queue`.
* Prints temperature alerts.

.. note::
   Periodic location tracking is handled by the :ref:`lib_location` library once it has been requested, whereas temperature samples are individually requested by the Main Application Loop.

.. _wifi_nrf_cloud_coap_shadow_loop:

CoAP shadow delta checking loop
===============================

The CoAP shadow delta checking thread performs the following tasks:

* Checks for a change in the device shadow with the :c:func:`nrf_cloud_coap_shadow_get` function.
* Parse and process the JSON shadow delta document.
* If a change is received, the thread sends the change back with the :c:func:`nrf_cloud_coap_shadow_state_update` function.
  This is necessary to prevent the device from unnecessarily receiving the same shadow delta the next time through the loop.

.. _wifi_nrf_cloud_temperature_sensing:

Temperature sensing
===================

Temperature sensing is implemented in :file:`src/temperature.c`, including generation of simulated temperature readings.
For temperature readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_ (handled in :file:`src/cloud_connection.c`).

.. _wifi_nrf_cloud_location_tracking:

Location tracking
=================

Location tracking is handled in :file:`src/location_tracking.c`.
This sets up a periodic location request and passes results to a callback configured by :file:`src/application.c`.

For location readings to be visible in the nRF Cloud portal, they must be marked as enabled in the `Device Shadow <nRF Cloud Device Shadows_>`_ (handled in :file:`src/cloud_connection.c`).

The device scans MAC addresses of nearby Wi-Fi access points and submits them to nRF Cloud to obtain a location estimate.

.. _wifi_nrf_cloud_led_status_indication:

LED status indication
=====================

On boards that support LED status indication, this sample can indicate its current status with any on-board LEDs.

This is performed by a background thread implemented in the :file:`src/led_control.c` file.

Other threads may request either a temporary or indefinite LED pattern.
This wakes up the ``led_thread``, which begins animating the requested pattern, sleeping for 100 milliseconds at a time between animation frames, until the requested pattern has completed (if it is temporary), or until a new pattern is requested in its place.

This feature is enabled by default for the *board_target* mentioned in the `Requirements`_ sections.

The patterns displayed, the states they describe, and the options required for them to appear are as follows:

+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Status                                               | Pattern                                                                                                | Conditions                                                                                                |
+======================================================+========================================================================================================+===========================================================================================================+
| Trying to connect to nRF Cloud (for the first time)  | Single LED lit, spinning around the square of LEDs                                                     | LED status indication is enabled                                                                          |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Connection to nRF Cloud lost, reconnecting           | Single LED lit, spinning around the square of LEDs                                                     | The :option:`CONFIG_LED_VERBOSE_INDICATION` option is enabled                                             |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Fatal error                                          | All four LEDs blinking, 75% duty cycle                                                                 | LED status indication is enabled                                                                          |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Device message sent successfully                     | Alternating checkerboard pattern (two LEDs are lit at a time, either LED1 and LED4, or LED2 and LED3)  | The :option:`CONFIG_LED_VERBOSE_INDICATION` option is enabled                                             |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+
| Idle                                                 | Single LED lit, bouncing between opposite corners (LED1 and LED4)                                      | The :option:`CONFIG_LED_CONTINUOUS_INDICATION` option is enabled                                          |
+------------------------------------------------------+--------------------------------------------------------------------------------------------------------+-----------------------------------------------------------------------------------------------------------+

Under all other circumstances, on-board LEDs are turned off.

.. note::
   The nRF7002 DK has only two on-board LEDs, so LED status indication works only partially on that target.
   Patterns that require four LEDs (such as the "Device message sent successfully" and "Idle" patterns) will not display as described.

.. note::
  The :option:`CONFIG_LED_VERBOSE_INDICATION` and :option:`CONFIG_LED_CONTINUOUS_INDICATION` options are enabled by default.

See :ref:`wifi_nrf_cloud_customizing_LED_status_indication` for details on customizing the LED behavior.

See :ref:`wifi_nrf_cloud_led_third_party` for details on configuring LED status indication on third-party boards.

.. _wifi_nrf_cloud_test_counter:

Test counter
============

Every time a sensor sample is sent, this counter is incremented and its current value is sent to `nRF Cloud`_.
A plot of the counter value over time appears automatically in the nRF Cloud portal, useful for visualizing connection loss, resets, and re-establishment.

You can enable or disable the counter using the device shadow's desired config section (``"counterEnable": true/false``), or force it always active with :option:`CONFIG_TEST_COUNTER`.

.. _wifi_nrf_cloud_device_message_formatting:

Device message formatting
=========================

This sample, when using MQTT to communicate with nRF Cloud, constructs JSON-based `device messages <nRF Cloud Device Messages_>`_.

While any valid JSON string can be sent as a device message, and accepted and stored by `nRF Cloud`_, there are some pre-designed message structures, known as schemas.
The nRF Cloud portal knows how to interpret these schemas.
These schemas are described in `nRF Cloud application protocols for long-range devices <nRF Cloud JSON protocol schemas_>`_.
The device messages constructed in the :file:`src/application.c` file all adhere to the general message schema.

Temperature data device messages conform to the ``temp`` ``deviceToCloud`` schemas.

This sample, when using CoAP to communicate with nRF Cloud, uses the :ref:`lib_nrf_cloud_coap` library to construct and transmit CBOR-based device messages.
Some CoAP traffic, specifically AWS shadow traffic, remains in JSON format.

.. _wifi_nrf_cloud_configuration:

Configuration
*************

|config|

Disabling key features
======================

You can independently disable the following key features of this sample by setting their respective Kconfig option disabled:

.. list-table::
   :widths: 50 50
   :header-rows: 1

   * - Feature
     - Kconfig option
   * - Location tracking
     - :option:`CONFIG_LOCATION_TRACKING`
   * - Temperature tracking
     - :option:`CONFIG_TEMP_TRACKING`
   * - Shadow handling when using CoAP
     - :option:`CONFIG_COAP_SHADOW`

For examples, see the :ref:`Configuration options <wifi_nrf_cloud_configuration>` section, or disable individual features using their respective Kconfig options.

.. _wifi_nrf_cloud_customizing_LED_status_indication:

Customizing LED status indication
=================================

To disable LED status indication (other than the selected idle behavior) after a connection to nRF Cloud has been established at least once, disable :option:`CONFIG_LED_VERBOSE_INDICATION`.

To turn the LED off while the sample is idle (rather than show an idle pattern), disable :option:`CONFIG_LED_CONTINUOUS_INDICATION`.

If you disable both of these options together, the status indicator LED remains off after a connection to nRF Cloud has been established at least once.

.. _wifi_nrf_cloud_led_third_party:

Configuring LED status indication for third-party boards
========================================================

This sample supports four discrete GPIO LEDs (:option:`CONFIG_LED_INDICATION_GPIO` and :option:`CONFIG_LED_INDICATOR_4LED`).
The board must have a compatible devicetree entry (``gpio-leds``).

To disable LED indication, enable :option:`CONFIG_LED_INDICATION_DISABLED`.

Useful debugging options
========================

To see all debug output for this sample, enable the ``CONFIG_WIFI_NRF_CLOUD_LOG_LEVEL_DBG`` option.

See also the :ref:`wifi_nrf_cloud_test_counter`.

Configuration options
=====================

Check and configure the following Kconfig options for the sample:

.. options-from-kconfig::
   :show-type:

.. _wifi_nrf_cloud_building_and_running:

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/nrf_cloud`

.. include:: /includes/build_and_run_ns.txt

.. _wifi_nrf_cloud_build_hardcoded:

Building with hard-coded CA and device credentials
==================================================

The :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option allows you to use hard-coded CA and device credentials stored in unprotected program memory for connecting to nRF Cloud.

.. important::
   The :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option is not secure and must not be used in production.
   It is only for testing and demonstration purposes.
   Because this setting stores your device's private key in unprotected program memory, using it makes your device vulnerable to impersonation.
   The private key is also generated on the host machine, so delete it from the host after provisioning.

.. note::
   This is only one of several mutually exclusive ways to install credentials to your device.
   See :ref:`wifi_nrf_cloud_onboarding` for other methods.

If you are certain you understand the inherent security risks, you can use this option as follows:

1. Follow the instructions under :ref:`wifi_nrf_cloud_onboard_hardcoded`.

#. Create a :file:`certs` folder directly in the :file:`wifi_nrf_cloud` folder, and copy :file:`client-cert.pem`, :file:`private-key.pem`, and :file:`ca-cert.pem` files into it.

   Make sure not to place the new folder in the :file:`wifi_nrf_cloud/src` folder by accident.

   Now, these certificates are automatically used if the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option is enabled.

#. Build the sample with the :kconfig:option:`CONFIG_NRF_CLOUD_PROVISION_CERTIFICATES` Kconfig option enabled and the device ID you created configured.

   This is required for onboarding to succeed.

   Enable the :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME` Kconfig option and set :kconfig:option:`CONFIG_NRF_CLOUD_CLIENT_ID` to the device ID.

   For example, if the device ID is ``698d4c11-0ccc-4f04-89cd-6882724e3f6f``:

   .. tabs::

      .. group-tab:: Bash

         .. code-block:: bash
            :class: highlight

            west build --board *board_target* -p always -- -DCONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y -DCONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y -DCONFIG_NRF_CLOUD_CLIENT_ID=\"698d4c11-0ccc-4f04-89cd-6882724e3f6f\"

      .. group-tab:: PowerShell

         .. code-block:: powershell
            :class: highlight

            west build --board *board_target* -p always -- -DCONFIG_NRF_CLOUD_PROVISION_CERTIFICATES=y -DCONFIG_NRF_CLOUD_CLIENT_ID_SRC_COMPILE_TIME=y  "-DCONFIG_NRF_CLOUD_CLIENT_ID='698d4c11-0ccc-4f04-89cd-6882724e3f6f'"


.. _wifi_nrf_cloud_setup_wifi_cred:

Setting up Wi-Fi access point credentials
=========================================

This sample uses the :ref:`Wi-Fi credentials <zephyr:lib_wifi_credentials>` library to manage Wi-Fi credentials.
Before the sample can connect to a Wi-Fi network, you must add at least one credential set.

Once your device has been flashed with this sample, you can add a credential by connecting to your device's UART interface and then entering the following command:

.. parsed-literal::
   :class: highlight

   wifi cred add *NetworkSSID* WPA2-PSK *NetworkPassword*

Where *NetworkSSID* is replaced with the SSID of the Wi-Fi access point you want your device to connect to, and *NetworkPassword* is its password.
Then, either reboot the device or use the ``wifi cred auto_connect`` command to manually trigger a connection attempt.

.. important::
   Do not use any special characters in the SSID or password, such as spaces or quotes.

From now on, these credentials will automatically be used when the configured network is reachable.

See the :ref:`Wi-Fi shell sample documentation <wifi_shell_sample>` for more details on the ``wifi`` commands.

.. _wifi_nrf_cloud_building_coap:

Building with CoAP
==================

By default, the sample uses MQTT to connect to nRF Cloud.
To use CoAP instead, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=coap.conf``

.. _wifi_nrf_cloud_onboarding:

Onboarding
**********

For your device to successfully connect to nRF Cloud, you must `onboard it <nRF Cloud Onboarding_>`_.
The following sections outline the onboarding methods that this sample supports.

  * Proceed to :ref:`wifi_nrf_cloud_standard_onboarding` (recommended).
  * If you are building with :ref:`hard-coded credentials <wifi_nrf_cloud_build_hardcoded>`, proceed to :ref:`wifi_nrf_cloud_onboard_hardcoded`.

.. note::
   You only need to perform one of the above methods.
   Select the one that best matches your needs, then build and run the sample with the required build configuration.

.. _wifi_nrf_cloud_install_nrf_utils:

Installing nRF Cloud Utils
==========================

To perform many of the actions described in the other sections, you need to install the `nRF Cloud Utils <nRF Cloud Utils_>`_.

To install the nRF Cloud Utils, run the following command to use this package as a dependency:

.. parsed-literal::
   :class: highlight

   pip3 install nrfcloud-utils

.. _wifi_nrf_cloud_standard_onboarding:

Onboarding a device with the device credentials generated on device
===================================================================

To onboard your device with the device credentials generated on the device, follow :ref:`lib_nrf_cloud_credentials_keygen_onboarding`.

Onboarding a device with the device credentials generated on the host machine
=============================================================================

With this method, the device private key and certificate are generated on the host machine and then installed onto the device over the :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>`.
Use it only when on-device key generation is not available.

.. caution::
   This method is less secure than :ref:`generating the key on the device <wifi_nrf_cloud_standard_onboarding>`, which is the recommended approach.
   Because the private key is created on the host machine, it exists outside the device and can be copied or leaked.
   Delete the private key from the host machine immediately after it has been installed on the device, and prefer on-device key generation for production.

First, :ref:`create a self-signed CA certificate <wifi_nrf_cloud_create_self_signed_ca>`.

Then, complete the following steps for each device you wish to onboard:

1. Make sure your device is plugged in and that this sample has been flashed to it.
#. Install the device and server credentials using the :file:`device_credentials_installer` script from :ref:`nRF Cloud Utils <wifi_nrf_cloud_install_nrf_utils>`:

   Select the protocol (MQTT or CoAP) you built the sample for:

   .. tabs::
      .. group-tab:: MQTT
         .. parsed-literal::
            :class: highlight

            device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str *device_id* -s -d --verify --local-cert --cmd-type tls_cred_shell

         Where:

           * *device_id* is the (globally unique) ID for your device.
             You must use the same device ID that you configured for the sample at build time.
             See :ref:`wifi_nrf_cloud_build_hardcoded` for details on setting a compile-time device ID.
           * The :file:`.pem` files are the self-signed CA certificate and private key files :ref:`you created <wifi_nrf_cloud_create_device_cred_locally>`.

         The ``--cmd-type tls_cred_shell`` option indicates that the device is using the :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>` for run-time credentials management.

         The ``--local-cert`` option indicates that the device private key and certificate should be generated on the host machine, not on-device.
         This is necessary because the TLS Credentials Shell does not support CSR generation currently.
         Delete the device private key from your machine after it is installed to the device by this script.

      .. group-tab:: CoAP
         .. parsed-literal::
            :class: highlight

            device_credentials_installer --ca self_*_ca.pem --ca-key self_*_prv.pem --id-str *device_id* -s -d --verify --coap --local-cert --cmd-type tls_cred_shell

         Where:

           * *device_id* is the (globally unique) ID for your device.
             You must use the same device ID that you configured for the sample at build time.
             See :ref:`wifi_nrf_cloud_build_hardcoded` for details on setting a compile-time device ID.
           * The :file:`.pem` files are the self-signed CA certificate and private key files :ref:`you created <wifi_nrf_cloud_create_device_cred_locally>`.

         The ``--cmd-type tls_cred_shell`` option indicates that the device is using the :ref:`TLS Credentials Shell <zephyr:tls_credentials_shell>` for run-time credentials management.

         The ``--local-cert`` option indicates that the device private key and certificate should be generated on the host machine, not on-device.
         This is necessary because the TLS Credentials Shell does not support CSR generation currently.
         Delete the device private key from your machine after it is installed to the device by this script.

   This script generates and signs a device credential on the host machine and installs it onto the device.
   Because the ``--local-cert`` option is used, the private key is generated on the host machine, not on the device.

   This script also installs any nRF Cloud root CA certificates required in a single chain to the :kconfig:option:`CONFIG_NRF_CLOUD_SEC_TAG` security tag (``sec_tag``).
   CoAP connections use one root CA certificate, whereas HTTPS and MQTT use another.

   If the script succeeds, you should see the following output:

   .. code-block:: console

      Saving nRF Cloud device onboarding CSV file onboard.csv...
      Onboarding CSV file saved, row count: 1

   And a new file, :file:`onboard.csv` should be created.
   This file will be used in the next step.

#. Onboard the device to your account using the :file:`nrf_cloud_onboard` script as follows:

   .. parsed-literal::
      :class: highlight

      nrf_cloud_onboard --api-key *your_api_key* --csv onboard.csv

   Where *your_api_key* is your nRF Cloud API key from your `User Account`_.

   Alternatively, you can :ref:`Manually onboard the device to nRF Cloud <wifi_nrf_cloud_onboard_device_manually>`.

.. _wifi_nrf_cloud_onboard_hardcoded:

Onboarding with hard-coded device credentials
=============================================

It is possible to onboard your devices using hard-coded device credentials.

.. caution::
   Hard-coded credentials must not be used in production.
   The device private key is generated on the host machine and embedded in unprotected program memory, so it exists outside the device's secure storage and can be copied or leaked.
   Delete the private key from the host machine after provisioning, and prefer :ref:`on-device key generation <wifi_nrf_cloud_standard_onboarding>` for production.

If you are certain you understand the inherent security risks, you can do so as follows:

First, create a :ref:`self-signed CA certificate <wifi_nrf_cloud_create_self_signed_ca>`.

Next, complete the following steps for each device you wish to onboard:

1. :ref:`Locally generate a device credential <wifi_nrf_cloud_create_device_cred_locally>`

   In addition to the arguments prescribed in that section, also include the ``-embed_save`` argument when running the :file:`create_device_credentials.py` Python script:

   .. parsed-literal::
      :class: highlight

      create_device_credentials --ca self_*_ca.pem --ca-key self_*_prv.pem -c US --cn *device_id* -p dev_credentials --embed-save

   Where *device_id* is the device ID you created, and the :file:`.pem` files are the :ref:`self-signed CA certificate and private key files <wifi_nrf_cloud_create_self_signed_ca>`.

   This automatically generates the following three files inside the :file:`dev_credentials` folder:

     * :file:`client-cert.pem`
     * :file:`private-key.pem`
     * :file:`ca-cert.pem`

   The :file:`client-cert.pem` and :file:`private-key.pem` files are specially formatted versions of the :file:`cred_<device_id>_crt.pem` and :file:`cred_<device_id>_prv.pem` files respectively.
   The :file:`ca-cert.pem` is a copy of the nRF Cloud root CA in the same format.
   Do not confuse this CA certificate with your :ref:`self-signed CA certificate <wifi_nrf_cloud_create_self_signed_ca>`.
   See `CA certificates for server authentication in AWS IoT Core`_ for more details.

   Your device needs these three credentials to connect successfully with nRF Cloud.

#. Onboard the device to your account with the :file:`nrf_cloud_onboard` script:

   .. parsed-literal::
      :class: highlight

      nrf_cloud_onboard --api-key *your_api_key* --csv onboard.csv

   Where *your_api_key* is your nRF Cloud API key from your `User Account`_.

   Alternatively, you can :ref:`Manually onboard the device to nRF Cloud <wifi_nrf_cloud_onboard_device_manually>`.

#. Follow the instructions under :ref:`wifi_nrf_cloud_build_hardcoded`.

.. _wifi_nrf_cloud_create_self_signed_ca:

Creating a self-signed CA certificate for device certificate signing
====================================================================

Before a device can connect to nRF Cloud, it must have device credentials.
You need to sign these credentials yourself using a CA certificate you create.
This is referred to as your self-signed CA certificate.

To create your self-signed CA certificate:

1. :ref:`Install the nRF Cloud Utils <wifi_nrf_cloud_install_nrf_utils>`.
#. Use the nRF Cloud Utils :file:`create_ca_cert` script to generate the certificate:

   .. code-block:: console

      create_ca_cert -c US -f self_

   Remember to set ``-c`` to your two-letter country code.
   See the `Create CA Cert <nRF Cloud Utils Create CA Cert_>`_ section in the nRF Cloud Utils documentation for more details.

   You should now have the following three files:

     * :file:`self_<self_cert_serial>_ca.pem`
     * :file:`self_<self_cert_serial>_prv.pem`
     * :file:`self_<self_cert_serial>_pub.pem`

   Where ``<self_cert_serial>`` is your self-signed CA certificate's serial number in hex.
   These three files are your self-signed CA certificate and private/public keypair.
   You will use them to sign device credentials.

   If you were directed here as part of other instructions, proceed to the next step of those instructions.

   .. note::
      You only need to generate these three files once.
      You can use them to sign as many device credentials as you need.

.. _wifi_nrf_cloud_create_device_cred_locally:

Generating device credentials locally
=====================================

To generate and sign a device credential locally using your :ref:`self-signed CA certificate <wifi_nrf_cloud_create_self_signed_ca>`, perform the following steps:

1. Create a globally unique `device ID <nRF Cloud Device ID_>`_ for the device.

   The ID can be anything you want, as long as it is not prefixed with ``nrf-`` and is globally unique.
   See `nRF Cloud Device ID`_ for details.

   Each device should have its own device ID, and you must use it exactly for all other actions involving that device, otherwise onboarding will fail.
   This ID is referred to in other steps using the term *device_id*.

#. Navigate to the folder where you :ref:`created your self-signed CA certificate <wifi_nrf_cloud_create_self_signed_ca>`, and use the :file:`create_device_credentials.py` Python script :ref:`you installed <wifi_nrf_cloud_install_nrf_utils>` to generate a self-signed certificate for the device:

   .. parsed-literal::
      :class: highlight

      create_device_credentials -ca self_*_ca.pem -ca_key self_*_prv.pem -c US -cn *device_id* -f cred\_

   Where *device_id* is the device ID you created, and the :file:`.pem` files are the self-signed CA certificate and private key files :ref:`you created <wifi_nrf_cloud_create_self_signed_ca>`.

   Remember to set ``-c`` to your two-letter country code.
   See the `Create Device Credentials <nRF Cloud Utils Create Device Credentials_>`_ section in the nRF Cloud Utils documentation for more details.

   You should now have the following three files:

     * :file:`cred_<device_id>_crt.pem`
     * :file:`cred_<device_id>_pub.pem`
     * :file:`cred_<device_id>_prv.pem`

   These three files are your device credentials, and can only be used by a single device.

   If you were directed here as part of other instructions, proceed to the next step of those instructions.

   .. important::

      If an attacker obtains the private key contained in :file:`cred_<device_id>_prv.pem`, they will be able to impersonate your device.

.. _wifi_nrf_cloud_onboard_device_manually:

Onboarding a device manually
============================

Once you have obtained device credentials for your device, it can be `onboarded <nRF Cloud Onboarding_>`_.

To onboard devices manually, you can use the `Bulk Onboard Devices <nRF Cloud Portal Bulk Onboard Devices_>`_ page of the nRF Cloud portal as follows:

1. Create an onboarding CSV file named :file:`<device_id>_onboard.csv` and give it the following contents:

   .. parsed-literal::
      :class: highlight

      *device_id*\ ,,,,"\ *device_cert*\ "

   Where *device_id* is replaced by the device ID :ref:`you created <wifi_nrf_cloud_create_device_cred_locally>`, and *device_cert* is replaced by the exact contents of :file:`<device_id>_crt.pem`.

   For example, if the device ID you created is ``698d4c11-0ccc-4f04-89cd-6882724e3f6f``:

   .. code-block:: none

      698d4c11-0ccc-4f04-89cd-6882724e3f6f,,,,"-----BEGIN CERTIFICATE-----
      sCC8AtbNQhzbp4y01FEzXaf5Fko3Qdq0o5LbuNpVA7S6AKAkjt17QzKJAiGWHakh
      RnwzoA2dF4wR0rMP5vR6dqBblaGAA5hN7GE2vPBHTDNZGJ6tZ9dnO6446dg9gGds
      eeadE1HdVnUj8nb+7CGm39vJ4fuNk9vogH0nMdxjCnXAinoOMRx8EklQsR747+Gz
      sxcdVYuNEb/E2vWBHTDNZGJ6tZC1JC9d6/RC3Vb1JC4tWnK9mk/Jw984ZuYugpMc
      1t9umoGFYCz0nMdxjCnXAbnoOMC5A0RxcWPzxfC5A0RH+j+mwoMxwhgfFY4EhVxp
      oCC8labNQhzRC3Vc1JC4tWnK9mpVA7k/o5LbuNpVA7S6AKAkjt17QzKJAiGWHakh
      RXwcoAndF4wPzxfC5A0RHponmwBHTDoM7GE2vPBHTDNZGJ6tZ9dnO6446dg9gGds
      eefdE1HcVnULbuNpVA7S6AKAkjxjCnt1gH0nMdxjCnXAinoOMRx8EklQsR747+Fz
      srm/VYaNEb/E2vPBHTDNZGJ6tZc1JC9d6/RC3Vc1JC4tWnK9mk/Jr984ZuYugpMc
      nt9uZTGFYCzZD0FFAA5NAC4i1PARStFycWPzxfC5A0RqodhswoMxwhgfFY4EhVx=
      -----END CERTIFICATE-----
      "

   Do not attempt to use this example directly, it has been filled with dummy data.
   See `nRF Cloud REST API ProvisionDevices`_ for more details.

   If you are onboarding more than a single device at once, you can repeat this format (separated by a newline) for each device you are onboarding.
   Alternatively, you can create a separate CSV file for each device you wish to onboard, and upload them individually.

#. Navigate to the `Bulk Onboard Devices`_ page of the nRF Cloud portal and upload the :file:`<device_id>_onboard.csv` file you created.

   To get there from the :guilabel:`Dashboard`, click :guilabel:`Devices` under :guilabel:`Device Management` in the navigation pane on the left, then click :guilabel:`Add Devices` and select **Bulk Onboard**.

   Once the `Bulk Onboard Devices`_ page is open, drag in the :file:`<device_id>_onboard.csv` file and click **Onboard**.

   You should see a message stating that the file was uploaded successfully, and a device with the device IDs you created should appear in the `Devices <nRF Cloud Portal Devices_>`_ page.

If you were directed here as part of other instructions, proceed to the next step of those instructions.

.. _wifi_nrf_cloud_dependencies:

References
**********

* `RFC 7252 - The Constrained Application Protocol`_
* `RFC 7959 - Block-Wise Transfer in CoAP`_
* `RFC 7049 - Concise Binary Object Representation`_
* `RFC 8610 - Concise Data Definition Language (CDDL)`_
* `RFC 8132 - PATCH and FETCH Methods for CoAP`_
* `RFC 9146 - Connection Identifier for DTLS 1.2`_

Dependencies
************

This sample uses the following |NCS| libraries and drivers:

* :ref:`lib_nrf_cloud`
* :ref:`lib_location`

It uses the following Zephyr libraries:

* :ref:`CoAP <zephyr:networking_api>`
* :ref:`CoAP Client <zephyr:coap_client_interface>`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
