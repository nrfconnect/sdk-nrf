.. _nrf_cloud_coap_device_message:

Cellular: nRF Cloud CoAP device message
#######################################

.. contents::
   :local:
   :depth: 2

The nRF Cloud CoAP device message sample demonstrates how to use the `nRF Cloud CoAP API`_ to send `device messages <nRF Cloud Device Messages_>`_.
Every button press sends a message to nRF Cloud.

It also demonstrates the use of the :ref:`lib_nrf_cloud_alert` and :ref:`lib_nrf_cloud_log` libraries.
The sample sends an alert when the device first comes online.
It also sends a log message indicating the sample version when the button is pressed.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

The sample requires an `nRF Cloud`_ account.

Your device must be onboarded with `nRF Cloud`_.
If it is not, follow the instructions in `Device on-boarding <nrf_cloud_coap_device_message_onboarding_>`_.

.. note::
   This sample requires modem firmware v1.3.x or later for an nRF9160 DK, or modem firmware v2.x.x for the nRF9161 and nRF9151 DKs.

User interface
**************

Button 1:
   Press to send a device message to nRF Cloud.

LED 1:
   Indicates that the device is connected to LTE and is ready to send messages.

LED 2:
   Indicates that a message is being sent to nRF Cloud.

.. _nrf_cloud_coap_device_message_onboarding:

Configuration
*************

|config|

Setup
=====

You must onboard your device to nRF Cloud for this sample to function.
You only need to do this once for each device.

To onboard your device, install `nRF Cloud Utils`_ and follow the instructions in the README.

Configuration options
=====================

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_coap_device_message`

.. include:: /includes/build_and_run_ns.txt

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Reset the development kit.
#. Observe in the terminal window that the application starts.
   This is indicated by output similar to the following (there is also a lot of additional information about the LTE connection):

   .. code-block:: console

      *** Booting nRF Connect SDK v3.1.99-c738e544af16 ***
      *** Using Zephyr OS v4.1.99-6872776607a9 ***
      Attempting to boot slot 0.
      Attempting to boot from address 0x8200.
      I: Trying to get Firmware version
      I: Verifying signature against key 0.
      I: Hash: 0x3e...f9
      I: Firmware signature verified.
      Firmware version 2
      �[00:00:00.252,197] <inf> spi_nor: GD25LE255E@0: 32 MiBy flash
      *** Booting My Application v1.0.0-c738e544af16 ***
      *** Using nRF Connect SDK v3.1.99-c738e544af16 ***
      *** Using Zephyr OS v4.1.99-6872776607a9 ***
      [00:00:00.309,814] <inf> nrf_cloud_device_message: nRF Cloud CoAP Device Message Sample, version: 1.0.0
      [00:00:00.319,671] <inf> nrf_cloud_device_message: Reset reason: 0x10000
      [00:00:00.995,178] <inf> nrf_cloud_credentials: Sec Tag: 16842753; CA: Yes, Client Cert: Yes, Private Key: Yes
      [00:00:01.005,645] <inf> nrf_cloud_credentials: CA Size: 1824, AWS: Likely, CoAP: Likely
      [00:00:01.014,221] <inf> nrf_cloud_device_message: nRF Cloud credentials detected!
      [00:00:01.113,800] <inf> nrf_cloud_device_message: Enabling connectivity...
      +CGEV: EXCE STATUS 0
      +CEREG: 2,"8169","03117F00",7
      %MDMEV: PRACH CE-LEVEL 0
      +CSCON: 1
      %XTIME: "80","52800241538280","01"
      +CGEV: ME PDN ACT 0
      %MDMCH STATUS 2
      +CEREG: 1,"8169","03117F00",7,0,2,"11100000","11100000"
      [00:00:05.384,887] <wrn> lte_lc: Registration rejected, EMM cause: 2, Cell ID: 51478272, Tracking area: 33129, LTE mode: 7
      [00:00:05.400,451] <inf> nrf_cloud_device_message: Connected to LTE
      [00:00:05.407,257] <inf> nrf_cloud_device_message: Waiting for modem to acquire network time...
      +CGEV: IPV6 0
      [00:00:08.417,083] <inf> nrf_cloud_device_message: Network time obtained
      [00:00:08.424,224] <inf> nrf_cloud_info: Device ID: 7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:08.433,288] <inf> nrf_cloud_info: IMEI:      359400123456789
      [00:00:08.558,837] <inf> nrf_cloud_info: UUID:      7e699894-79b6-11f0-a2b4-db93a314a2aa
      [00:00:08.567,932] <inf> nrf_cloud_info: Modem FW:  mfw_nrf91x1_2.0.2
      [00:00:08.574,829] <inf> nrf_cloud_info: Protocol:          CoAP
      [00:00:08.581,298] <inf> nrf_cloud_info: Download protocol: HTTPS
      [00:00:08.587,860] <inf> nrf_cloud_info: Sec tag:           16842753
      [00:00:08.594,696] <inf> nrf_cloud_info: CoAP JWT Sec tag:  16842753
      [00:00:08.601,531] <inf> nrf_cloud_info: Host name:         coap.nrfcloud.com
      [00:00:10.231,933] <inf> nrf_cloud_coap_transport: Request authorization with JWT
      [00:00:10.495,574] <inf> nrf_cloud_coap_transport: Authorization result_code: 2.01
      [00:00:10.503,692] <inf> nrf_cloud_coap_transport: Authorized
      [00:00:10.510,101] <inf> nrf_cloud_coap_transport: DTLS CID is active
      [00:00:10.970,703] <inf> nrf_cloud_device_message: Checking for shadow delta...
      [00:00:11.449,615] <inf> nrf_cloud_device_message: Initial config sent
      [00:00:11.456,573] <inf> nrf_cloud_device_message: Reset reason: 0x10000
      [00:00:11.684,783] <inf> nrf_cloud_device_message: Sending message:'{"sample_message":"Hello World, from the CoAP Device Message Sample! Message ID: 1755700534318"}'
      [00:00:11.928,649] <inf> nrf_cloud_device_message: Message sent
      [00:00:12.035,125] <inf> nrf_cloud_device_message: Sent Hello World message with ID: 1755700534318
      [00:00:16.847,137] <inf> nrf_cloud_device_message: Sending message:'{"appId":"BUTTON", "messageType":"DATA", "data":"1"}'
      [00:00:17.136,352] <inf> nrf_cloud_device_message: Message sent
      [00:00:17.242,858] <inf> nrf_cloud_device_message: Checking for shadow delta...
      [00:00:17.774,841] <inf> nrf_cloud_device_message: Initial config sent
      +CSCON: 0

Troubleshooting
===============

If you are not getting the output similar to the one in `Testing`_, check the following potential issue:

The network carrier does not provide date and time
   The sample requires the network carrier to provide date and time to the modem.
   Without a valid date and time, the modem cannot generate JWTs with an expiration time.

nRF Cloud logging
=================

To enable `Zephyr Logging`_ in nRF Cloud using the :ref:`lib_nrf_cloud_log` library, add the following parameter to your build command:

``-DEXTRA_CONF_FILE=nrfcloud_logging.conf``

This KConfig fragment allows the sample and various subsystems that have logging enabled to send their logs to nRF Cloud.
Set the :kconfig:option:`CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL` option to the log level of messages to be sent to nRF Cloud, such as ``4`` for debug log messages.

Querying device messages over REST API
**************************************

To query the device messages received by the `nRF Cloud`_ backend, send a GET request to the `ListMessages <nRF Cloud REST API ListMessages_>`_ endpoint.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_coap`
* :ref:`lib_nrf_cloud_alert`
* :ref:`lib_nrf_cloud_log`
* :ref:`lib_modem_jwt`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`modem_info_readme`
* :ref:`lib_at_host`
