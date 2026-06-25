.. _nrf_cloud_coap_fota_sample:

Cellular: nRF Cloud CoAP FOTA
#############################

.. contents::
   :local:
   :depth: 2

The nRF Cloud CoAP FOTA sample demonstrates how to perform Firmware Over-the-Air (FOTA) updates over CoAP on your device.
This covers application and delta modem FOTA updates.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be onboarded to nRF Cloud.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_coap_fota_sample_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 SiP and v2.0.0 or later for nRF9161 and nRF9151 SiPs.

Overview
********

You can update your device firmware on the `nRF Cloud`_ portal or using the `Memfault CLI tool`_.
See the `Memfault OTA Integration Guide`_ for details.

Limitations
***********

This sample requires the network carrier to provide date and time to the modem.
Without a valid date and time, the modem cannot generate JSON Web Tokens (JWT) with an expiration time.

User interface
**************

Button 1:
   Check for a FOTA update right away without waiting for the timeout.

.. _nrf_cloud_coap_fota_sample_onboarding:

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

.. _CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES:

CONFIG_COAP_FOTA_JOB_CHECK_RATE_MINUTES - Update check rate
   Defines how often the sample checks for FOTA updates.
   You can modify this value at runtime by adding or updating the ``"fotaInterval"`` item in the desired config section of the device's shadow.
   Use the `nRF Cloud`_ portal or the REST API to perform the config update.

.. _CONFIG_COAP_FOTA_LTE_LED_NUM:

CONFIG_COAP_FOTA_LTE_LED_NUM - LTE LED number
   Defines the LED used to indicate the connection to the LTE network.

.. _CONFIG_COAP_FOTA_BUTTON_EVT_NUM:

CONFIG_COAP_FOTA_BUTTON_EVT_NUM - Button number
   Defines the button to use for a manual FOTA update check.

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_coap_fota`

.. include:: /includes/build_and_run_ns.txt

The configuration files for this sample are located in the :file:`samples/cellular/nrf_cloud_coap_fota` folder.
See :ref:`configure_application` on how to configure the parameters.

To create a FOTA test version of this sample, change the ``PATCHLEVEL`` in the :file:`VERSION` file.

To enable delta modem FOTA, add ``-DEXTRA_CONF_FILE=delta_modem_fota.conf`` to your build command and configure the modem firmware project key in one of the following ways:

* At runtime, by adding the ``"memfaultModemKey"`` string item to the control section of the device's shadow
  Use the `nRF Cloud`_ portal or the REST API to perform the shadow update.
* At build time, by setting the :kconfig:option:`CONFIG_MEMFAULT_FOTA_MODEM_PROJECT_KEY` Kconfig option to your Memfault modem firmware project key.

See the `Memfault nRF Modem FOTA`_ documentation for more details on configuring a delta modem FOTA.

.. note::
   SMP and full modem FOTA update are not currently supported by this sample.
   However, the following legacy overlay and source files for enabling those features are left for reference:

   * :file:`overlays/legacy_smp_fota.conf`
   * :file:`overlays/legacy_smp_modem_fota.conf`
   * :file:`src/smp_reset.h`
   * :file:`src/smp_reset.c`

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting MCUboot v2.3.0-dev-69c7ba763a23 ***
      *** Using nRF Connect SDK v3.3.99-429be77c7cb0 ***
      *** Using Zephyr OS v4.4.0-43c5b94d1548 ***
      I: Starting bootloader
      I: LCS-awareness disabled, skipping LCS check
      I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Boot source: none
      I: Image index: 0, Swap type: none
      I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Secondary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Boot source: none
      I: Image index: 1, Swap type: none
      I: Bootloader chainload address offset: 0x28000
      I: Image version: v1.0.0
      �: Jumping to the first image slot
      [00:00:00.635,437] <inf> nrf_cloud_coap_fota_sample: Application Name: N/A
      [00:00:00.644,958] <inf> nrf_cloud_coap_fota_sample: nRF Connect SDK version: 3.3.99-429be77c7cb0
      [00:00:00.656,524] <inf> mflt: S/N: 50344654-3037-4f89-809b-1810f870fb69
      [00:00:00.665,893] <inf> mflt: SW type: app
      [00:00:00.672,729] <inf> mflt: SW version: 1.0.0+0
      [00:00:00.680,206] <inf> mflt: HW version: nrf9151dk
      [00:00:00.854,339] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:00.867,004] <inf> nrf_cloud_credentials: CA Size: 1792, AWS: Likely, CoAP: Likely
      [00:00:00.877,777] <inf> nrf_cloud_coap_fota_sample: nRF Cloud credentials detected!
      [00:00:00.888,183] <inf> nrf_cloud_coap_fota_sample: Enabling connectivity...
      [00:00:04.547,424] <inf> nrf_cloud_coap_fota_sample: Connected to LTE
      [00:00:04.620,513] <inf> nrf_cloud_coap_fota_sample: Waiting for modem to acquire network time...
      [00:00:07.641,357] <inf> nrf_cloud_coap_fota_sample: Network time obtained
      [00:00:07.650,909] <inf> nrf_cloud_info: Device ID: 50344654-3037-4f89-809b-1810f870fb69
      [00:00:07.668,579] <inf> nrf_cloud_info: IMEI:      359404230597438
      [00:00:07.770,263] <inf> nrf_cloud_info: UUID:      50344654-3037-4f89-809b-1810f870fb69
      [00:00:07.787,841] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.4
      [00:00:07.796,905] <inf> nrf_cloud_info: Protocol:          CoAP
      [00:00:07.805,572] <inf> nrf_cloud_info: Download protocol: CoAP
      [00:00:07.814,208] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:07.823,211] <inf> nrf_cloud_info: CoAP JWT Sec tag:  16842753
      [00:00:07.832,214] <inf> nrf_cloud_info: Host name:         coap.nrfcloud.com
      [00:00:10.353,149] <inf> nrf_cloud_coap_transport: Request authorization with JWT
      [00:00:10.614,532] <inf> nrf_cloud_coap_transport: Authorization result_code: 2.01
      [00:00:10.625,061] <inf> nrf_cloud_coap_transport: Authorized
      [00:00:10.633,666] <inf> nrf_cloud_coap_transport: DTLS CID is active
      [00:00:11.844,482] <inf> mflt: FOTA Update Available. Starting Download with URL: https://ota-cdn.memfault.com/<path>
      [00:00:11.862,823] <inf> mflt: FOTA In Progress
      [00:00:11.869,964] <inf> nrf_cloud_coap_fota_sample: FOTA download started
      [00:00:11.879,516] <inf> nrf_cloud_coap_fota_sample: Checking for FOTA job in 15 minute(s) or when button 1 is pressed
      [00:00:11.894,104] <inf> downloader: Setting up TLS credentials, sec tag count 1
      [00:00:11.904,449] <inf> downloader: Connecting to 2600:1f18:56ff:3d00::547c
      [00:00:11.914,611] <inf> downloader: Failed to connect on IPv6 (err -118), attempting IPv4
      [00:00:11.926,605] <inf> downloader: Setting up TLS credentials, sec tag count 1
      [00:00:11.937,133] <inf> downloader: Connecting to 23.22.110.24
      [00:00:14.521,545] <inf> downloader: Downloaded 1024/345316 bytes (0%)
      // ...
      [00:00:17.703,873] <inf> downloader: Downloaded 204800/345316 bytes (59%)
      [00:01:56.548,706] <inf> downloader: Downloaded 345316/345316 bytes (100%)
      [00:01:56.558,258] <inf> downloader: Download complete
      [00:01:56.570,281] <inf> dfu_target_mcuboot: MCUBoot image-0 upgrade scheduled. Reset device to apply
      [00:01:56.582,153] <inf> nrf_cloud_coap_fota_sample: FOTA download complete, rebooting to apply update...
      [00:01:57.264,526] <inf> nrf_cloud_coap_fota_sample: Network connectivity lost!
      *** Booting MCUboot v2.3.0-dev-69c7ba763a23 ***
      *** Booting nRF Connect SDK v3.3.99-429be77c7cb0 ***
      *** Using Zephyr OS v4.4.0-43c5b94d1548 ***
      I: Starting bootloader
      I: LCS-awareness disabled, skipping LCS check
      I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Secondary image: magic=good, swap_type=0x2, copy_done=0x3, image_ok=0x3
      I: Boot source: none
      I: Image index: 0, Swap type: test
      I: Primary image: magic=unset, swap_type=0x1, copy_done=0x3, image_ok=0x3
      I: Secondary image: magic=good, swap_type=0x2, copy_done=0x3, image_ok=0x3
      I: Boot source: none
      I: Starting swap using move algorithm.
      I: Bootloader chainload address offset: 0x28000
      I: Image version: v1.0.0
      �: Jumping to the first image slot
      // ...
      [00:00:00.635,742] <inf> nrf_cloud_coap_fota_sample: Confirming OTA image
      [00:00:00.645,263] <inf> nrf_cloud_coap_fota_sample: Application Name: N/A
      [00:00:00.654,785] <inf> nrf_cloud_coap_fota_sample: nRF Connect SDK version: 3.3.99-429be77c7cb0
      [00:00:00.666,351] <inf> mflt: S/N: 50344654-3037-4f89-809b-1810f870fb69
      [00:00:00.675,781] <inf> mflt: SW type: app
      [00:00:00.682,617] <inf> mflt: SW version: 1.4.2
      [00:00:00.689,880] <inf> mflt: HW version: nrf9151dk
      // ...
      [00:00:14.558,624] <inf> nrf_cloud_coap_fota_sample: Device is up to date

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_coap`
* :ref:`lib_modem_jwt`
* :ref:`lte_lc_readme`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
