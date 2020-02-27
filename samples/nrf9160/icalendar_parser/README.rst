.. _icalendar_parser:

nRF9160: iCalendar Parser
#########################

The iCalendar parser sample demonstrates how to utilize **iCalendar Parser** library to parse a shared `iCalendar`_ via HTTPS.

Overview
********

`iCalendar`_ is data format for representing and exchanging calendaring and scheduling information. This sample application utilize
**iCalendar Parser** library to parse an iCalendar from a shared Google calendar.

The sample first provisions a root CA certificate to the modem using the :ref:`modem_key_mgmt` library.
Provisioning must be done before connecting to the LTE network, because the certificates can only be provisioned when the device is not connected.

The sample then establishes a connection to the LTE network, sets up the necessary TLS socket options, and connects to an calendar server.
It then sends HTTP GET request and parse the returned iCalendar content.

The out-of-date calendar events are filtered according to the current date-time. If there is event happening or will happen in the future,
the sample will lit the corresponding LED to inform users.

Obtaining a certificate
=======================

The sample connects to ``calendar.google.com``, which requires an X.509 certificate.
This certificate is provided in the :file:`samples/nrf9160/icalendar_parser/cert` folder.

To connect to other servers, you might need to provision a different certificate.
You can download a certificate for a given server using your web browser.
Alternatively, you can obtain it from a dedicated website like `SSL Labs`_.

Certificates come in different formats.
To provision the certificate to the nRF9160 DK/Thingy:91, it must be in PEM format.
The PEM format looks like this::

   "-----BEGIN CERTIFICATE-----\n"
   "MIIDujCCAqKgAwIBAgILBAAAAAABD4Ym5g0wDQYJKoZIhvcNAQEFBQAwTDEgMB4G\n"
   "A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjIxEzARBgNVBAoTCkdsb2JhbFNp\n"
   "Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDYxMjE1MDgwMDAwWhcNMjExMjE1\n"
   "MDgwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMjETMBEG\n"
   "A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI\n"
   "hvcNAQEBBQADggEPADCCAQoCggEBAKbPJA6+Lm8omUVCxKs+IVSbC9N/hHD6ErPL\n"
   "v4dfxn+G07IwXNb9rfF73OX4YJYJkhD10FPe+3t+c4isUoh7SqbKSaZeqKeMWhG8\n"
   "eoLrvozps6yWJQeXSpkqBy+0Hne/ig+1AnwblrjFuTosvNYSuetZfeLQBoZfXklq\n"
   "tTleiDTsvHgMCJiEbKjNS7SgfQx5TfC4LcshytVsW33hoCmEofnTlEnLJGKRILzd\n"
   "C9XZzPnqJworc5HGnRusyMvo4KD0L5CLTfuwNhv2GXqF4G3yYROIXJ/gkwpRl4pa\n"
   "zq+r1feqCapgvdzZX99yqWATXgAByUr6P6TqBwMhAo6CygPCm48CAwEAAaOBnDCB\n"
   "mTAOBgNVHQ8BAf8EBAMCAQYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUm+IH\n"
   "V2ccHsBqBt5ZtJot39wZhi4wNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDovL2NybC5n\n"
   "bG9iYWxzaWduLm5ldC9yb290LXIyLmNybDAfBgNVHSMEGDAWgBSb4gdXZxwewGoG\n"
   "3lm0mi3f3BmGLjANBgkqhkiG9w0BAQUFAAOCAQEAmYFThxxol4aR7OBKuEQLq4Gs\n"
   "J0/WwbgcQ3izDJr86iw8bmEbTUsp9Z8FHSbBuOmDAGJFtqkIk7mpM0sYmsL4h4hO\n"
   "291xNBrBVNpGP+DTKqttVCL1OmLNIG+6KYnX3ZHu01yiPqFbQfXf5WRDLenVOavS\n"
   "ot+3i9DAgBkcRcAtjOj4LaR0VknFBbVPFd5uRHg5h6h+u/N5GJG79G+dwfCMNYxd\n"
   "AfvDbbnvRG15RjF+Cv6pgsH/76tuIMRQyV+dTZsXjAzlAcmgQWpzU/qlULRuJQ/7\n"
   "TBj0/VLZjmmx6BEP3ojY+x1J96relc8geMJgEtslQIxq/H5COEBkEveegeGTLg==\n"
   "-----END CERTIFICATE-----\n"

Note the ``\n`` at the end of each line.

See the comprehensive `tutorial on SSL.com`_ for instructions on how to convert between different certificate formats and encodings.


Requirements
************

* One of the following development boards:

    * |nRF9160DK|
    * |Thingy91|

* .. include:: /includes/spm.txt

User interface
**************

The application state is indicated by the LEDs.

nRF9160 DK:
    * LED 3 blinking: Connecting and downloading - The device is fetching new calendar content from server.
    * LED 3 ON: The calendar was downloaded and parsed. No calendar event.
    * LED 2 ON: The calendar was downloaded and parsed. There is event has not happens yet.
    * LED 1 ON: The calendar was downloaded and parsed. There is event now happening.

Thingy:91:
    * Blue LED blinking: Connecting and downloading - The device is fetching new calendar content from server.
    * Blue LED ON: The calendar was downloaded and parsed. No calendar event.
    * Green LED ON: The calendar was downloaded and parsed. There is event has not happens yet.
    * Red LED ON: The calendar was downloaded and parsed. There is event now happening.

The button has the following functions when the connection is established:

Button 1 (SW3 on Thingy:91):
    * Start downloading new calendar content.

Building and running
********************

Before building the sample, please get the shared Google calendar link:

1. Open Google Calendar from a web browser.
#. In the top right, click Settings icon.
#. Click the name of the calendar you want to share.
#. In the "Secret Address" section, click ICAL.
#. Copy the ICAL link that appears in the window.

.. |sample path| replace:: :file:`samples/nrf9160/icalendar_parser`

Edit calendar host and calendar file of prj.conf according to the shared ICAL link.
For example, if the ICAL link is https://calendar.google.com/calendar/ical/foo.ics, fill the host and file as::

      CONFIG_DOWNLOAD_HOST="calendar.google.com"
      CONFIG_DOWNLOAD_FILE="/calendar/ical/foo.ics"

.. include:: /includes/build_and_run_nrf9160.txt


Testing
=======

After programming the sample to the board, the application will:

1. Automatically connect to cellurar network.
#. Observe that LED 3 starts blinking when the sample downloads new content from calendar server.
#. If there is no event in the future or is happening, LED3 will be ON.
#. If there is event in the future but has not happened, LED2 will be ON.
#. If there is event happening, LED1 will be ON.
#. The sample will download new content from calendar server every minute. The interval can be configured at CONFIG_DOWNLOAD_INTERVAL.

Dependencies
************

This sample uses the following libraries:

From |NCS|
  * :ref:`modem_info_readme`
  * :ref:`modem_key_mgmt`
  * :ref:`dk_buttons_and_leds_readme`
  * ``lib/lte_link_control``

In addition, it uses the following samples:

From |NCS|
  * :ref:`secure_partition_manager`

References
**********

.. _iCalendar:
   https://tools.ietf.org/html/rfc5545
