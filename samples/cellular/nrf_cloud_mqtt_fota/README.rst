.. _nrf_cloud_mqtt_fota:

Cellular: nRF Cloud MQTT FOTA
#############################

.. contents::
   :local:
   :depth: 2

The nRF Cloud MQTT FOTA sample demonstrates how to use the `nRF Cloud MQTT API`_ to perform :term:`Firmware Over-the-Air (FOTA) <Firmware Over-the-Air (FOTA) update>` updates over MQTT on your device.
This covers modem, application, and full modem FOTA updates (FMFU).
Also, with the nRF9160 DK, it supports SMP FOTA updates to the firmware on the nRF52840 SoC present on the DK board (not a separate device).

When using MQTT, the `FOTA update <nRF Cloud Getting Started FOTA Documentation_>`_ support is almost entirely implemented by enabling the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` option, which is implicitly enabled by :kconfig:option:`CONFIG_NRF_CLOUD_MQTT`.

However, even with the :kconfig:option:`CONFIG_NRF_CLOUD_FOTA` Kconfig option enabled, applications must still reboot themselves manually after FOTA download completion, and must still update their `Device Shadow <nRF Cloud Device Shadows_>`_ to reflect FOTA support.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device onboarding <nrf_cloud_mqtt_fota_sample_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 SiP and v2.0.0 or later for nRF9161 and nRF9151 SiPs.

.. include:: /includes/external_flash_nrf91.txt

.. note::
   Full modem FOTA requires development kit version 0.14.0 or higher if you are using an nRF9160 DK.

Overview
********

You can update your device firmware on the `nRF Cloud`_ portal or directly through the `nRF Cloud MQTT API`_.
See the `nRF Cloud Getting Started FOTA documentation`_ for details.

.. _nrf_cloud_mqtt_fota_sample_onboarding:

Setup
=====

You must onboard your device to nRF Cloud for this sample to function.
You only need to do this once for each device.

To onboard your device, install `nRF Cloud Utils`_ and follow the instructions in the README.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration options for the sample:

.. options-from-kconfig::
   :show-type:

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_mqtt_fota`

.. include:: /includes/build_and_run_ns.txt

The configuration files for this sample are located in the :file:`samples/cellular/nrf_cloud_mqtt_fota` folder.
See :ref:`configure_application` on how to configure the parameters.

To create a FOTA test version of this sample, change the ``PATCHLEVEL`` in the :file:`VERSION` file.

To enable full modem FOTA, add the following parameter to your build command:

``-DEXTRA_CONF_FILE="full_modem_fota.conf"``

Also, if you are using an nRF9160 DK, specify your development kit version by appending it to the board target.
For example, if you are using version 0.14.0, use the board target ``nrf9160dk@0.14.0/nrf9160/ns`` in your build command.

To enable SMP FOTA (nRF9160 DK only), add the following parameters to your build command:

* ``-DEXTRA_CONF_FILE="smp_fota.conf"``
* ``-DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_mcumgr_client_uart2.overlay"``

Once you have flashed your nRF9160 DK, change the switch **SW10** to the **nRF52** position to be able to flash the nRF52840 firmware on the DK.
The nRF52840 device on your DK must be running firmware compatible with SMP, such as the :ref:`smp_svr` sample.
Otherwise, the MQTT FOTA sample cannot connect to the nRF52840 and will keep trying to connect.
Build the :ref:`smp_svr` sample for the ``nrf9160dk/nrf52840`` board target with the following parameters:

* ``-DEXTRA_CONF_FILE="overlay-serial.conf"``
* ``-DEXTRA_DTC_OVERLAY_FILE="nrf9160dk_nrf52840_mcumgr_svr.overlay"``

To change :ref:`smp_svr` sample's application version, set the :kconfig:option:`CONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION` Kconfig option.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting My Application v1.0.0-f135319f826e ***
      *** Using nRF Connect SDK v3.1.99-f135319f826e ***
      *** Using Zephyr OS v4.2.99-be1a9fd0eeca ***
      [00:00:00.255,889] <inf> nrf_cloud_mqtt_fota: nRF Cloud MQTT FOTA Sample, version: 1.0.0
      [00:00:00.255,920] <inf> nrf_cloud_mqtt_fota: Reset reason: 0x1
      [00:00:00.828,735] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:00.828,765] <inf> nrf_cloud_credentials: CA Size: 1824, AWS: Likely, CoAP: Likely
      [00:00:00.828,796] <inf> nrf_cloud_mqtt_fota: nRF Cloud credentials detected!
      [00:00:00.829,589] <inf> nrf_cloud_mqtt_fota: Enabling connectivity...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"81A6","03229B10",7
      %MDMEV: PRACH CE-LEVEL 0
      +CSCON: 1
      +CGEV: ME PDN ACT 0
      %MDMEV: SEARCH STATUS 2
      +CEREG: 1,"81A6","03229B10",7,,,"11100000","11100000"
      [00:00:02.471,496] <inf> nrf_cloud_mqtt_fota: Connected to LTE
      +CGEV: IPV6 0
      [00:00:05.473,724] <inf> nrf_cloud_info: Device ID: 12345678-1234-5678-9abc-def012345678
      [00:00:05.474,395] <inf> nrf_cloud_info: IMEI:      359404230026479
      [00:00:05.474,575] <inf> nrf_cloud_info: UUID:      12345678-1234-5678-9abc-def012345678
      [00:00:05.474,822] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.3
      [00:00:05.474,853] <inf> nrf_cloud_info: Protocol:          MQTT
      [00:00:05.474,884] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:05.474,884] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:05.474,914] <inf> nrf_cloud_info: Host name:         mqtt.nrfcloud.com
      [00:00:05.474,945] <inf> nrf_cloud_mqtt_fota: Connecting to nRF Cloud...
      [00:00:08.681,701] <inf> nrf_cloud_mqtt_fota: Connection to nRF Cloud ready
      [00:00:08.681,793] <inf> nrf_cloud_info: Team ID:   12345678-1234-5678-9abc-def012345670

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
