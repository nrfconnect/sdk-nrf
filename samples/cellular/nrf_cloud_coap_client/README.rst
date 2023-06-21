.. _nrf_cloud_coap_client_sample:

nRF9160: nRF Cloud CoAP Client
##############################

.. contents::
   :local:
   :depth: 2

The nRF Cloud CoAP Client sample demonstrates the communication between the nRF Cloud CoAP server and an nRF9160 SiP that acts as the CoAP client.

The sample exercises all of the nRF Cloud services:

* FOTA
* Cellular location
* A-GPS
* P-GPS
* Device messages

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns

.. include:: /includes/tfm.txt

Modem firmware version 1.3.5 or above is recommended, in order to benefit from the power savings provided by DTLS Connection ID

Overview
********

The nRF Cloud CoAP Client sample performs the following actions:

#. Connect to the nRF Cloud CoAP server.
#. Gain authorization to use services by means of a JWT.
#. Request a URL for P-GPS data, then download and store it.
#. Periodically cycle through each service:

   #. Check for a FOTA job.
   #. Send a fake temperature reading.
   #. Get device location using cellular and/or Wi-Fi data.
   #. Send a GNSS PVT location.
   #. Request A-GPS data (if needed), and inject into the modem.
   #. Check for a shadow delta; if one received, acknowledge back to server.

Configuration
*************

.. include:: /libraries/modem/nrf_modem_lib/nrf_modem_lib_trace.rst
   :start-after: modem_lib_sending_traces_UART_start
   :end-before: modem_lib_sending_traces_UART_end

Building and running
********************

.. |sample path| replace:: :file:`samples/cellular/nrf_cloud_coap_client`

.. include:: /includes/build_and_run_ns.txt

Provisioning device on nRF Cloud
********************************

For more information, see `nRF Cloud Utilities documentation`_.

Only do the first step below once.

  .. code-block:: console

     python3 create_ca_cert.py -c US -st OR -l Portland -o "My Company" -ou "RD" -cn example.com -e admin@example.com -f CA
     Creating self-signed CA certificate...
     File created: CA0x3bc7f3b014a8ad492999c594f08bbc2fcffc5fd1_ca.pem
     File created: CA0x3bc7f3b014a8ad492999c594f08bbc2fcffc5fd1_prv.pem
     File created: CA0x3bc7f3b014a8ad492999c594f08bbc2fcffc5fd1_pub.pem

     python3 device_credentials_installer.py --ca CA0x3bc7f3b014a8ad492999c594f08bbc2fcffc5fd1_ca.pem \
     --ca_key CA0x3bc7f3b014a8ad492999c594f08bbc2fcffc5fd1_prv.pem --id_str nrf- --id_imei -s -d --coap
     ...
     <- OK
     Saving provisioning endpoint CSV file provision.csv...
     Provisioning CSV file saved

Once the script creates the provision.csv file, visit http://nrfcloud.com/#/provision-devices to provision the device in nRF Cloud.

Testing
=======

After programming the sample and all prerequisites to the development kit, test it by performing the following steps:

1. Connect your nRF9160 DK to the PC using a USB cable and power on or reset your nRF9160 DK.
#. Open a terminal emulator and observe that the following information is displayed:

       The nRF Cloud CoAP client sample started
#. Observe that the discovered IP address of the CoAP server is displayed on the terminal emulator.
#. Observe that the nRF9160 DK sends periodic CoAP GET requests to the configured server for a configured resource after it gets LTE connection.
#. Observe that the sample either displays the response data received from the server or indicates a timeout on the terminal.
   For more information on the response codes, see `COAP response codes`_.



Sample output
=============

The sample displays the data in the following format:

.. code-block:: console

   CoAP request sent: token 0x9772
   CoAP response: code: 0x45, token 0x9772, payload: 15:39:40

Instead of displaying every single CoAP frame content, the sample displays only the essential data.
For the above sample output, the information displayed on the terminal conveys the following:

* ``code:0x45`` -  CoAP response code (2.05 - Content), which is constant across responses
* ``token 0x9772`` - CoAP token, which is unique per request/response pair
* ``payload: 15:39:40`` - the actual message payload (current time in UTC format) from the resource that is queried in this sample

References
**********

`RFC 7252 - The Constrained Application Protocol`_

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`lte_lc_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`

It uses the following Zephyr library:

* :ref:`CoAP <zephyr:networking_api>`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
