.. _sms_sample:

nRF9160: SMS
############

.. contents::
   :local:
   :depth: 2

The SMS sample demonstrates how you can send and receive SMS messages with your nRF9160-based device.


Overview
********

The SMS sample registers the nRF9160-based device for SMS service within the nRF9160 modem using the :ref:`sms_readme` library.
The sample requires an LTE connection.

When the sample starts, it sends SMS if a recipient phone number is set in the configuration.
The sample then receives all the SMS messages and displays the information about the messages including the text that is sent.

The maximum size of the AT command response defined by :option:`CONFIG_AT_CMD_RESPONSE_MAX_LEN` might limit the size of the SMS message that can be received.
This parameter is defined in the :ref:`at_cmd_readme` module.
Values over 512 bytes will not restrict the size of the received message as the maximum data length of the SMS is 140 bytes.

Requirements
************

The sample supports the following development kit:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf9160dk_nrf9160_ns


Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. option:: CONFIG_SMS_SEND_PHONE_NUMBER - Configuration for recipient phone number.

   The sample configuration is used to set the recipient phone number if you need to send SMS.

Additional Configuration
========================

Check and configure the following mandatory library options that are used by the sample:

* :option:`CONFIG_SMS`
* :option:`CONFIG_NRF_MODEM_LIB`

Check and configure the following optional library options that are used by the sample:

* :option:`CONFIG_SMS_SUBSCRIBERS_MAX_CNT`
* :option:`CONFIG_AT_CMD_RESPONSE_MAX_LEN`
* :option:`CONFIG_LTE_AUTO_INIT_AND_CONNECT`
* :option:`CONFIG_LOG`
* :option:`CONFIG_ASSERT`
* :option:`CONFIG_ASSERT_VERBOSE`

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/sms`

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample to your development kit, test the sample by performing the following steps:

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample shows the :ref:`UART output <sms_uart_output>` from the device.
   Note that this is an example, and the output need not be identical to your observed output.
#. Send an SMS message to the number associated with the SIM card that you have placed into your nRF9160-based device.

   .. note::

      Not all IoT SIM cards support SMS service. You must check with your operator if the SMS service does not work as expected.

.. _sms_uart_output:


Sample output
=============

The following output is logged in the terminal:

.. code-block:: console

   *** Booting Zephyr OS build v2.4.99-ncs1-1818-g54dea0b2b530  ***

   SMS sample starting
   SMS sample is ready for receiving messages
   Sending SMS: number=1234567890, text="SMS sample: testing"
   SMS status report received

   SMS received:
         Time:   21-04-12 15:42:52
         Text:   'Testing'
         Length: 7

Dependencies
************

This sample uses the following |NCS| library:

* :ref:`sms_readme`

It uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_modem`
