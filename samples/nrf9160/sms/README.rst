.. _sms_sample:

nRF9160: SMS
############

.. contents::
   :local:
   :depth: 2

The SMS sample demonstrates how you can send and receive SMS messages with your nRF9160-based device.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

The SMS sample registers the nRF9160-based device for SMS service within the nRF9160 modem using the :ref:`sms_readme` library.
The sample requires an LTE connection.

When the sample starts, it sends an SMS if a recipient phone number is set in the configuration.
The sample then receives all the SMS messages and displays the information about the messages including the text that is sent.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following configuration option for the sample:

.. _CONFIG_SMS_SEND_PHONE_NUMBER:

CONFIG_SMS_SEND_PHONE_NUMBER - Configuration for recipient phone number in international format
   The sample configuration is used to set the recipient phone number in international format if you need to send an SMS.

Additional configuration
========================

Check and configure the following mandatory library options that are used by the sample:

* :kconfig:option:`CONFIG_SMS`
* :kconfig:option:`CONFIG_NRF_MODEM_LIB`

Check and configure the following optional library options that are used by the sample:

* :kconfig:option:`CONFIG_SMS_SUBSCRIBERS_MAX_CNT`
* :kconfig:option:`CONFIG_LOG`
* :kconfig:option:`CONFIG_ASSERT`
* :kconfig:option:`CONFIG_ASSERT_VERBOSE`

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf9160/sms`

.. include:: /includes/build_and_run_ns.txt


Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

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

The sample also uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
